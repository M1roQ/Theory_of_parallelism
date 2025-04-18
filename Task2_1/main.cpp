#include <iostream>
#include <vector>
#include <omp.h>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <chrono>
#include <memory>

void initialization_serial(std::unique_ptr<double[]>& a, std::unique_ptr<double[]>& b, std::unique_ptr<double[]>& c, int m, int n) {
    for (int i = 0; i < m; i++) {
        c[i] = 0.0;
        for (int j = 0; j < n; j++) {
            a[i * n + j] = static_cast<double>(i + j);
        }
    }
    for (int j = 0; j < n; j++) {
        b[j] = static_cast<double>(j);
    }
}

void initialization_parallel(std::unique_ptr<double[]>& a, std::unique_ptr<double[]>& b, std::unique_ptr<double[]>& c, int m, int n, int num_threads) {
    #pragma omp parallel num_threads(num_threads)
    {
        int thread_id = omp_get_thread_num();
        int total_threads = omp_get_num_threads();

        int chunk_size = m / total_threads;
        int start_row = thread_id * chunk_size;
        int end_row = (thread_id == total_threads - 1) ? (m-1) : (start_row + chunk_size - 1);

        for (int i = start_row; i < end_row; i++) {
            c[i] = 0.0;
            for (int j = 0; j < n; j++) {
                a[i * n + j] = static_cast<double>(i + j);
            }
        }
    }

    for (int j = 0; j < n; j++) {
        b[j] = static_cast<double>(j);
    }
}


void matrix_vector_product_serial(const std::unique_ptr<double[]>& a, const std::unique_ptr<double[]>& b, std::unique_ptr<double[]>& c, int m, int n) {
    for (int i = 0; i < m; i++) {
        c[i] = 0.0;
        for (int j = 0; j < n; j++) {
            c[i] += a[i * n + j] * b[j];
        }
    }
}

void matrix_vector_product_parallel(const std::unique_ptr<double[]>& a, const std::unique_ptr<double[]>& b, std::unique_ptr<double[]>& c, int m, int n, int num_threads) {
    #pragma omp parallel num_threads(num_threads)
    {
        int current_thread_number = omp_get_thread_num();
        int threads_count = omp_get_num_threads();

        int items_per_thread = m / threads_count;
        int lower_bound = current_thread_number * items_per_thread;
        int upper_bound = (current_thread_number == threads_count - 1) ? (m - 1) : (lower_bound + items_per_thread - 1);

        for (int i = lower_bound; i <= upper_bound; i++) {
            double temp = 0.0;
            for (int j = 0; j < n; j++) {
                temp += a[i * n + j] * b[j];
            }
            c[i] = temp;
        }
    }
}

int main() {
    std::cout << "_______START_______" << std::endl;

    std::vector<int> sizes = {20000, 40000};
    std::vector<int> thread_counts = {1, 2, 4, 7, 8, 16, 20, 40};

    std::ofstream file("../results/matrix_benchmark.csv");
    file << "N=M,T1,T2,S2,T4,S4,T7,S7,T8,S8,T16,S16,T20,S20,T40,S40\n";

    for (int size : sizes) {
        int m = size, n = size;
        
        std::cout << "Running for size: " << size << "x" << size << std::endl;

        std::cout << "_______START SERIAL_______" << std::endl;
        double serial_time = 0.0;
        for (int i = 0; i < 10; ++i) {
            std::unique_ptr<double[]> a(new double[m * n]);
            std::unique_ptr<double[]> b(new double[n]);
            std::unique_ptr<double[]> c(new double[m]);
            
            initialization_serial(a, b, c, m, n);
            
            auto start = std::chrono::steady_clock::now();
            matrix_vector_product_serial(a, b, c, m, n);
            auto end = std::chrono::steady_clock::now();
            
            auto elapsed_time =std::chrono::duration_cast<std::chrono::milliseconds>( end - start);
            serial_time += elapsed_time.count();
        }
        serial_time /= 10.0;
        file << size << "," << serial_time;
        std::cout << "serial time = " << serial_time << " s" << std::endl;

        std::cout << "_______START PARALLEL_______" << std::endl;
        for (int threads : thread_counts) {
            double parallel_time = 0.0;

            for (int i = 0; i < 10; ++i) {
                std::unique_ptr<double[]> a(new double[m * n]);
                std::unique_ptr<double[]> b(new double[n]);
                std::unique_ptr<double[]> c(new double[m]);
                
                initialization_parallel(a, b, c, m, n, threads);
                
                auto start = std::chrono::steady_clock::now();
                matrix_vector_product_parallel(a, b, c, m, n, threads);
                auto end = std::chrono::steady_clock::now();
                
                auto elapsed_time =std::chrono::duration_cast<std::chrono::milliseconds>( end - start);
                parallel_time += elapsed_time.count();
            }

            parallel_time /= 10.0;
            double speedup = serial_time / parallel_time;
            std::cout << threads << " threads: " << parallel_time << " s, speedup = " << speedup << std::endl;
            file << "," << parallel_time << "," << speedup;
        }

        file << "\n";
    }

    file.close();
    std::cout << "\nResults written to matrix_benchmark.csv" << std::endl;

    return 0;
}
