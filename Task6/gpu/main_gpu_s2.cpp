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

double linearInterpolate(double x, double x1, double y1, double x2, double y2) {
    return y1 + ((x - x1) * (y2 - y1) / (x2 - x1));
}

void initializeMatrix(std::unique_ptr<double[]> &matrix, int size) {
    matrix[0] = 10.0;
    matrix[size - 1] = 20.0;
    matrix[(size - 1) * size + (size - 1)] = 30.0;
    matrix[(size - 1) * size] = 20.0;

    for (size_t i = 1; i < size - 1; ++i) {
        matrix[i] = linearInterpolate(i, 0.0, matrix[0], size - 1, matrix[size - 1]);
        matrix[i * size] = linearInterpolate(i, 0.0, matrix[0], size - 1, matrix[(size - 1) * size]);
        matrix[i * size + (size - 1)] = linearInterpolate(i, 0.0, matrix[size - 1], size - 1, matrix[(size - 1) * size + (size - 1)]);
        matrix[(size - 1) * size + i] = linearInterpolate(i, 0.0, matrix[(size - 1) * size], size - 1, matrix[(size - 1) * size + (size - 1)]);
    }
}


void saveMatrixToFile(const double* matrix, int size, const std::string& filename) {
    std::ofstream outputFile(filename);
    if (!outputFile.is_open()) {
        std::cerr << "Не удалось открыть файл " << filename << " для записи." << std::endl;
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
        ("size", opt::value<int>()->default_value(1024), "Размер матрицы")
        ("iterations", opt::value<int>()->default_value(1000000), "Количество итераций")
        ("help", "Help message");

    opt::variables_map vm;
    opt::store(opt::parse_command_line(argc, argv, desc), vm);
    opt::notify(vm);
    if (vm.count("help")) {
        std::cout << desc << "\n";
        return 1;
    }

    int size = vm["size"].as<int>();
    double accuracy = vm["accuracy"].as<double>();
    int maxIterations = vm["iterations"].as<int>();

    acc_set_device_num(3, acc_device_nvidia);

    double error = 1.0;
    int iteration = 0;

    std::unique_ptr<double[]> matrix(std::make_unique<double[]>(size * size));
    std::unique_ptr<double[]> newMatrix(std::make_unique<double[]>(size * size));

    initializeMatrix(matrix, size);
    initializeMatrix(newMatrix, size);

    auto start = std::chrono::high_resolution_clock::now();
    double* currentMatrix = matrix.get();
    double* previousMatrix = newMatrix.get();

    nvtxRangePushA("copyin-to-gpu");
    #pragma acc data copy(error) copyin(previousMatrix[0:size * size], currentMatrix[0:size * size])
    {
        nvtxRangePop();

        nvtxRangePushA("main-compute-loop");
        while (iteration < maxIterations && error > accuracy) {
            #pragma acc parallel loop independent collapse(2) async(1) present(currentMatrix, previousMatrix)
            for (size_t i = 1; i < size - 1; ++i) {
                for (size_t j = 1; j < size - 1; ++j) {
                    currentMatrix[i * size + j] = 0.25 * (previousMatrix[i * size + j + 1] + previousMatrix[i * size + j - 1] + previousMatrix[(i - 1) * size + j] + previousMatrix[(i + 1) * size + j]);
                }
            }

            if ((iteration + 1) % 1000 == 0) {
                nvtxRangePushA("Error Computation");
                error = 0.0;
                #pragma acc update device(error)
                #pragma acc wait(1)
                #pragma acc parallel loop independent collapse(2) reduction(max:error) async(2) present(currentMatrix, previousMatrix)
                for (size_t i = 1; i < size - 1; ++i) {
                    for (size_t j = 1; j < size - 1; ++j) {
                        error = fmax(error, fabs(currentMatrix[i * size + j] - previousMatrix[i * size + j]));
                    }
                }

                #pragma acc wait(2)
                #pragma acc update self(error)
                nvtxRangePop(); // update-error
                // std::cout << "Итерация: " << iteration + 1 << " ошибка: " << error << std::endl;
            }

            std::swap(previousMatrix,currentMatrix);
            
            ++iteration;
        }
        nvtxRangePop(); // main-compute-loop

        nvtxRangePushA("copyout-to-cpu");
        #pragma acc update self(currentMatrix[0:size * size])
        nvtxRangePop(); // copyout-to-cpu
    }
    

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end - start;

    std::cout << "Время: " << duration.count() << " сек, Ошибка: " << error << ", Итерации: " << iteration << std::endl;

    if (size == 13 || size == 10) {
        for (size_t i = 0; i < size; ++i) {
            for (size_t j = 0; j < size; ++j) {
                std::cout << matrix[i * size + j] << ' ';
            }
            std::cout << std::endl;
        }
    }

    saveMatrixToFile(currentMatrix, size, "matrix.txt");

    matrix = nullptr;
    newMatrix = nullptr;
    return 0;
}