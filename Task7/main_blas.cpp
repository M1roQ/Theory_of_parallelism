#include <iostream>
#include <boost/program_options.hpp>
#include <cmath>
#include <memory>
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <chrono>
#include <openacc.h>
#include <nvtx3/nvToolsExt.h>

namespace opt = boost::program_options;

#include "cublas_v2.h"

double linearInterpolation(double x, double x1, double y1, double x2, double y2) {
    return y1 + ((x - x1) * (y2 - y1) / (x2 - x1));
}

void initializeMatrix(std::unique_ptr<double[]> &matrix, int size) {
    matrix[0] = 10.0;
    matrix[size-1] = 20.0;
    matrix[(size-1)*size + (size-1)] = 30.0;
    matrix[(size-1)*size] = 20.0;
    
    for (size_t i = 1; i < size-1; i++) {
        matrix[0*size+i] = linearInterpolation(i, 0.0, matrix[0], size-1, matrix[size-1]);
        matrix[i*size+0] = linearInterpolation(i, 0.0, matrix[0], size-1, matrix[(size-1)*size]);
        matrix[i*size+(size-1)] = linearInterpolation(i, 0.0, matrix[size-1], size-1, matrix[(size-1)*size + (size-1)]);
        matrix[(size-1)*size+i] = linearInterpolation(i, 0.0, matrix[(size-1)*size], size-1, matrix[(size-1)*size + (size-1)]);
    }
}

void saveMatrixToFile(const double* matrix, int size, const std::string& filename) {
    std::ofstream outputFile(filename);
    if (!outputFile.is_open()) {
        std::cerr << "Не удалось открыть файл " << filename << " для записи" << std::endl;
        return;
    }

    int fieldWidth = 10; 

    for (int i = 0; i < size; ++i) {
        for (int j = 0; j < size; ++j) {
            outputFile << std::setw(fieldWidth) << std::fixed << std::setprecision(4) << matrix[i * size + j];
        }
        outputFile << std::endl;
    }
    outputFile.close();
}

int main(int argc, char const *argv[]) {
    opt::options_description desc("Options");
    desc.add_options()
        ("accuracy", opt::value<double>()->default_value(1e-6), "Точность")
        ("matrixSize", opt::value<int>()->default_value(1024), "Размер матрицы")
        ("maxIterations", opt::value<int>()->default_value(1000000), "Количество итераций")
        ("help", "Help");

    opt::variables_map vm;
    opt::store(opt::parse_command_line(argc, argv, desc), vm);
    opt::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << "\n";
        return 1;
    }

    int matrixSize = vm["matrixSize"].as<int>();
    double accuracy = vm["accuracy"].as<double>();
    int maxIterations = vm["maxIterations"].as<int>();

    acc_set_device_num(3, acc_device_nvidia);

    cublasStatus_t status;
    cublasHandle_t cublasHandle;
    status = cublasCreate(&cublasHandle);
    if (status != CUBLAS_STATUS_SUCCESS) {
        std::cerr << "CUBLAS initialization failed\n";
        std::cerr << "Name: " << cublasGetStatusName(status) << '\n';
        std::cerr << "Description: " << cublasGetStatusString(status) << '\n';
        return 3;
    }

    double error = 1.0;
    int iteration = 0;

    std::unique_ptr<double[]> currentMatrix(std::make_unique<double[]>(matrixSize * matrixSize));
    std::unique_ptr<double[]> previousMatrix(std::make_unique<double[]>(matrixSize * matrixSize));

    double alpha = -1.0;
    int maxIndex = 0;

    initializeMatrix(currentMatrix, matrixSize);
    initializeMatrix(previousMatrix, matrixSize);

    auto start = std::chrono::high_resolution_clock::now();
    double* currentMatrixPtr = currentMatrix.get();
    double* previousMatrixPtr = previousMatrix.get();
    
    nvtxRangePushA("copyin-to-gpu");
    #pragma acc data copyin(maxIndex, previousMatrixPtr[0:matrixSize*matrixSize], currentMatrixPtr[0:matrixSize*matrixSize], matrixSize, alpha)
    {
        nvtxRangePop();

        nvtxRangePushA("main-compute-loop");
        while (iteration < maxIterations && error > accuracy) {
            #pragma acc parallel loop independent collapse(2) vector vector_length(1024) gang num_gangs(1024) present(currentMatrixPtr, previousMatrixPtr)
            for (size_t i = 1; i < matrixSize - 1; i++) {
                for (size_t j = 1; j < matrixSize - 1; j++) {
                    currentMatrixPtr[i * matrixSize + j] = 0.25 * (previousMatrixPtr[i * matrixSize + j + 1] + previousMatrixPtr[i * matrixSize + j - 1] + previousMatrixPtr[(i - 1) * matrixSize + j] + previousMatrixPtr[(i + 1) * matrixSize + j]);
                }
            }
            
            if ((iteration + 1) % 1000 == 0) {
                #pragma acc data present(previousMatrixPtr, currentMatrixPtr) wait
                #pragma acc host_data use_device(currentMatrixPtr, previousMatrixPtr)
                {
                    nvtxRangePushA("DAXPY + IDAMAX");
                    status = cublasDaxpy(cublasHandle, matrixSize * matrixSize, &alpha, currentMatrixPtr, 1, previousMatrixPtr, 1);
                    if (status != CUBLAS_STATUS_SUCCESS) {
                        std::cerr << "cublasDaxpy failed\n";
                        std::cerr << "Name: " << cublasGetStatusName(status) << '\n';
                        std::cerr << "Description: " << cublasGetStatusString(status) << '\n';
                    }

                   status = cublasIdamax(cublasHandle, matrixSize * matrixSize, previousMatrixPtr, 1, &maxIndex);
                    if (status != CUBLAS_STATUS_SUCCESS) {
                        std::cerr << "cublasIdamax failed\n";
                        std::cerr << "Name: " << cublasGetStatusName(status) << '\n';
                        std::cerr << "Description: " << cublasGetStatusString(status) << '\n';
                    }
                    
                    nvtxRangePop();
                }

                #pragma acc update self(previousMatrixPtr[maxIndex - 1])
                nvtxRangePushA("Error Calculation");
                error = fabs(previousMatrixPtr[maxIndex - 1]);
                nvtxRangePop();
                // std::cout << "Итерация: " << iteration + 1 << " ошибка: " << error << std::endl;

                #pragma acc host_data use_device(currentMatrixPtr, previousMatrixPtr)
                {
                    nvtxRangePushA("DCOPY");
                    status = cublasDcopy(cublasHandle, matrixSize * matrixSize, currentMatrixPtr, 1, previousMatrixPtr, 1);
                    nvtxRangePop();
                    if (status != CUBLAS_STATUS_SUCCESS) {
                        std::cerr << "cublasDcopy failed\n";
                        std::cerr << "Name: " << cublasGetStatusName(status) << '\n';
                        std::cerr << "Description: " << cublasGetStatusString(status) << '\n';
                    }
                }
            }

            std::swap(previousMatrix,currentMatrix);
            iteration++;
        }
        nvtxRangePop();

        cublasDestroy(cublasHandle);

        nvtxRangePushA("Final GPU->CPU Copy");
        #pragma acc update self(currentMatrixPtr[0:matrixSize*matrixSize])
        nvtxRangePop();
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end - start;

    std::cout << "Время: " << duration.count() << " сек, Ошибка: " << error << ", Итерации: " << iteration << std::endl;

    if (matrixSize == 13 || matrixSize == 10) {
        for (size_t i = 0; i < matrixSize; i++) {
            for (size_t j = 0; j < matrixSize; j++) {
                std::cout << currentMatrix[i * matrixSize + j] << ' ';
            }
            std::cout << std::endl;
        }
    }

    saveMatrixToFile(currentMatrixPtr, matrixSize, "matrix.txt");

    return 0;
}