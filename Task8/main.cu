#include <iostream>
#include <boost/program_options.hpp>
#include <cmath>
#include <memory>
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <chrono>
#include <nvtx3/nvToolsExt.h>

namespace opt = boost::program_options;

#include <cuda_runtime.h>
#include <cub/cub.cuh>

template<typename T>
using cuda_unique_ptr = std::unique_ptr<T,std::function<void(T*)>>;

template<typename T>
T* cuda_new(size_t size) {
    T *d_ptr;
    cudaMalloc((void **)&d_ptr, sizeof(T) * size);
    return d_ptr;
}

template<typename T>
void cuda_delete(T *dev_ptr) {
    cudaFree(dev_ptr);
}

cudaStream_t* cuda_new_stream() {
    cudaStream_t* stream = new cudaStream_t;
    cudaStreamCreate(stream);
    return stream;
}

void cuda_delete_stream(cudaStream_t* stream) {
    cudaStreamDestroy(*stream);
    delete stream;
}

cudaGraph_t* cuda_new_graph() {
    cudaGraph_t* graph = new cudaGraph_t;
    return graph;
}

void cuda_delete_graph(cudaGraph_t* graph) {
    cudaGraphDestroy(*graph);
    delete graph;
}

cudaGraphExec_t* cuda_new_graph_exec() {
    cudaGraphExec_t* graphExec = new cudaGraphExec_t;
    return graphExec;
}

void cuda_delete_graph_exec(cudaGraphExec_t* graphExec) {
    cudaGraphExecDestroy(*graphExec);
    delete graphExec;
}


#define CHECK(call)                                                             \
    {                                                                           \
        const cudaError_t error = call;                                         \
        if (error != cudaSuccess)                                               \
        {                                                                       \
            printf("Error: %s:%d, ", __FILE__, __LINE__);                       \
            printf("code: %d, reason: %s\n", error, cudaGetErrorString(error)); \
            exit(1);                                                            \
        }                                                                       \
    }

double linearInterpolation(double x, double x1, double y1, double x2, double y2) {
    return y1 + ((x - x1) * (y2 - y1) / (x2 - x1));
}

void initMatrix(std::unique_ptr<double[]> &arr, int N){
        for (size_t i = 0; i < N * N - 1; i++) {
            arr[i] = 0;
        }
        
        arr[0] = 10.0;
        arr[N - 1] = 20.0;
        arr[(N - 1) * N + (N - 1)] = 30.0;
        arr[(N - 1) * N] = 20.0;

        for (size_t i = 1; i < N - 1; i++) {
            arr[0 * N + i] = linearInterpolation(i, 0.0, arr[0], N-1, arr[N - 1]);
            arr[i * N + 0] = linearInterpolation(i, 0.0, arr[0], N-1, arr[(N - 1) * N]);
            arr[i * N + (N - 1)] = linearInterpolation(i, 0.0, arr[N - 1], N - 1, arr[(N - 1) * N + (N - 1)]);
            arr[(N -1 ) * N + i] = linearInterpolation(i, 0.0, arr[(N - 1) * N], N - 1, arr[(N - 1) * N + (N - 1)]);
        }
}

void saveMatrixToFile(const double* matrix, int N, const std::string& filename) {
    std::ofstream outputFile(filename);
    if (!outputFile.is_open()) {
        std::cerr << "Unable to open file " << filename << " for writing." << std::endl;
        return;
    }
    int fieldWidth = 10;
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            outputFile << std::setw(fieldWidth) << std::fixed << std::setprecision(4) << matrix[i * N + j];
        }
        outputFile << std::endl;
    }
    outputFile.close();
}

void swapMatrices(double* &prevmatrix, double* &curmatrix) {
    double* temp = prevmatrix;
    prevmatrix = curmatrix;
    curmatrix = temp;
}

__global__ void computeOneIteration(double *prevmatrix, double *curmatrix, int size) {
    int i = blockIdx.y * blockDim.y + threadIdx.y;
    int j = blockIdx.x * blockDim.x + threadIdx.x;
    if (!(j == 0 || i == 0 || i >= size - 1 || j >= size - 1))
        curmatrix[i * size + j]  = 0.25 * (prevmatrix[i * size + j + 1] + prevmatrix[i * size + j - 1] + prevmatrix[(i - 1) * size + j] + prevmatrix[(i + 1) * size + j]);
}

__global__ void matrixSub(double *prevmatrix, double *curmatrix,double *error,int size) {
    int i = blockIdx.y * blockDim.y + threadIdx.y;
    int j = blockIdx.x * blockDim.x + threadIdx.x;
    if (!(j == 0 || i == 0 || i >= size - 1 || j >= size - 1))
        error[i * size + j] = fabs(curmatrix[i * size + j] - prevmatrix[i * size + j]);
}


int main(int argc, char const *argv[]) {
    opt::options_description desc("опции");
    desc.add_options()
        ("accuracy", opt::value<double>()->default_value(1e-6), "точность")
        ("matrixSize", opt::value<int>()->default_value(256), "размер матрицы")
        ("maxIterations", opt::value<int>()->default_value(1000000), "количество операций")
        ("help", "помощь");

    opt::variables_map vm;
    opt::store(opt::parse_command_line(argc, argv, desc), vm);
    opt::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << "\n";
        return 1;
    }
   
    int N = vm["matrixSize"].as<int>();
    double accuracy = vm["accuracy"].as<double>();
    int countIter = vm["maxIterations"].as<int>();
   
    cuda_unique_ptr<cudaStream_t> stream(cuda_new_stream(), cuda_delete_stream);
    cuda_unique_ptr<cudaGraph_t>graph(cuda_new_graph(), cuda_delete_graph);
    cuda_unique_ptr<cudaGraphExec_t>g_exec(cuda_new_graph_exec(), cuda_delete_graph_exec);
    
    size_t tmp_size = 0;
    double* tmp = NULL;

    double error =1.0;
    int iter = 0;

    std::unique_ptr<double[]> A(std::make_unique<double[]>(N * N));
    std::unique_ptr<double[]> Anew(std::make_unique<double[]>(N * N));
    std::unique_ptr<double[]> B(std::make_unique<double[]>(N * N));

    initMatrix(std::ref(A),N);
    initMatrix(std::ref(Anew),N);
    
    double* curmatrix = A.get();
    double* prevmatrix = Anew.get();
    double* error_matrix = B.get();

    cuda_unique_ptr<double> curmatrix_GPU_ptr(cuda_new<double>(N*N), cuda_delete<double>);
    cuda_unique_ptr<double> prevmatrix_GPU_ptr(cuda_new<double>(N*N), cuda_delete<double>);
    cuda_unique_ptr<double> error_gpu_ptr(cuda_new<double>(N*N), cuda_delete<double>);
    cuda_unique_ptr<double>error_GPU_ptr(cuda_new<double>(1), cuda_delete<double>);
    
    double* curmatrix_GPU = curmatrix_GPU_ptr.get();
    double* prevmatrix_GPU = prevmatrix_GPU_ptr.get();
    double* error_gpu = error_gpu_ptr.get();
    double* error_GPU = error_GPU_ptr.get();

    CHECK(cudaMemcpy(curmatrix_GPU,curmatrix,N*N*sizeof(double), cudaMemcpyHostToDevice));
    CHECK(cudaMemcpy(prevmatrix_GPU,prevmatrix,N*N*sizeof(double), cudaMemcpyHostToDevice));
    CHECK(cudaMemcpy(error_gpu,error_matrix,N*N*sizeof(double), cudaMemcpyHostToDevice));
    
    cub::DeviceReduce::Max(tmp, tmp_size, prevmatrix_GPU, error_GPU, N*N);

    cuda_unique_ptr<double>tmp_ptr(cuda_new<double>(tmp_size), cuda_delete<double>);
    tmp = tmp_ptr.get();

    dim3 threads_in_block = dim3(32, 32);
    dim3 blocks_in_grid((N + threads_in_block.x - 1) / threads_in_block.x, (N + threads_in_block.y - 1) / threads_in_block.y);

    nvtxRangePushA("build-cuda-graph");
    cudaStreamBeginCapture(*stream, cudaStreamCaptureModeGlobal);
    
    for(size_t i =0 ; i<999;i++) {
        computeOneIteration<<<blocks_in_grid, threads_in_block, 0, *stream>>>(prevmatrix_GPU, curmatrix_GPU, N);
        swapMatrices(prevmatrix_GPU, curmatrix_GPU);
    }

    computeOneIteration<<<blocks_in_grid, threads_in_block, 0, *stream>>>(prevmatrix_GPU, curmatrix_GPU, N);
    matrixSub<<<blocks_in_grid, threads_in_block, 0, *stream>>>(prevmatrix_GPU, curmatrix_GPU, error_gpu, N);
    
    cub::DeviceReduce::Max(tmp, tmp_size, error_gpu, error_GPU, N*N, *stream);
    cudaStreamEndCapture(*stream, graph.get());
    nvtxRangePop();

    cudaGraphInstantiate(g_exec.get(), *graph, NULL, NULL, 0);

    auto start = std::chrono::high_resolution_clock::now();
    
    nvtxRangePushA("main-compute-loop");
    while(error > accuracy && iter < countIter){
        nvtxRangePushA("error");
        cudaGraphLaunch(*g_exec, *stream);
        cudaMemcpy(&error, error_GPU, 1*sizeof(double), cudaMemcpyDeviceToHost);
        iter+=1000;
        //std::cout << "iteration: "<<iter << ' ' <<"error: "<<error << std::endl;

        nvtxRangePop();
    }
    nvtxRangePop();


    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end - start;

    std::cout << "Время: " << duration.count() << " сек, Ошибка: " << error << ", Итерации: " << iter << std::endl;

    nvtxRangePushA("cuda-memcpy-device2host");
    CHECK(cudaMemcpy(prevmatrix, prevmatrix_GPU, sizeof(double)*N*N, cudaMemcpyDeviceToHost));
    CHECK(cudaMemcpy(error_matrix, error_gpu, sizeof(double)*N*N, cudaMemcpyDeviceToHost));
    CHECK(cudaMemcpy(curmatrix, curmatrix_GPU, sizeof(double)*N*N, cudaMemcpyDeviceToHost));
    nvtxRangePop();

    if (N <= 13) {        
        for (size_t i = 0; i < N; i++) {
            for (size_t j = 0; j < N; j++) {
                std::cout << A[i * N + j] << ' ';                
            }
            std::cout << std::endl;
        }

        for (size_t i = 0; i < N; i++) {
            for (size_t j = 0; j < N; j++) {
                std::cout << Anew[i * N + j] << ' ';
            }
            std::cout << std::endl;
        }
    }
    saveMatrixToFile(curmatrix, N, "matrix.txt");
    return 0;
}