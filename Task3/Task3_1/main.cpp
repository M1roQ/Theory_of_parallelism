#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <fstream>
#include <memory>

void initialize_range(double* a, double* b, double* c, int start_row, int end_row, int n) {
    for (int i = start_row; i < end_row; ++i) {
        c[i] = 0.0;
        for (int j = 0; j < n; ++j) {
            a[i * n + j] = static_cast<double>(i + j);
        }
    }
}

void multiply_range(const double* a, const double* b, double* c, int start_row, int end_row, int n) {
    for (int i = start_row; i < end_row; ++i) {
        double sum = 0.0;
        for (int j = 0; j < n; ++j) {
            sum += a[i * n + j] * b[j];
        }
        c[i] = sum;
    }
}

void parallel_initialize(double* a, double* b, double* c, int m, int n, int num_threads) {
    std::vector<std::thread> threads;

    int chunk = m / num_threads;
    for (int t = 0; t < num_threads; ++t) {
        int start = t * chunk;
        int end = (t == num_threads - 1) ? m : start + chunk;
        threads.emplace_back(initialize_range, a, b, c, start, end, n);
    }

    for (auto& th : threads) {
        th.join();
    }

    for (int j = 0; j < n; ++j) {
        b[j] = static_cast<double>(j);
    }
}

void parallel_multiply(double* a, double* b, double* c, int m, int n, int num_threads) {
    std::vector<std::thread> threads;

    int chunk = m / num_threads;
    for (int t = 0; t < num_threads; ++t) {
        int start = t * chunk;
        int end = (t == num_threads - 1) ? m : start + chunk;
        threads.emplace_back(multiply_range, a, b, c, start, end, n);
    }

    for (auto& th : threads) {
        th.join();
    }
}

void serial_initialize(double* a, double* b, double* c, int m, int n) {
    for (int i = 0; i < m; ++i) {
        c[i] = 0.0;
        for (int j = 0; j < n; ++j) {
            a[i * n + j] = static_cast<double>(i + j);
        }
    }

    for (int j = 0; j < n; ++j) {
        b[j] = static_cast<double>(j);
    }
}

void serial_multiply(double* a, double* b, double* c, int m, int n) {
    for (int i = 0; i < m; ++i) {
        double sum = 0.0;
        for (int j = 0; j < n; ++j) {
            sum += a[i * n + j] * b[j];
        }
        c[i] = sum;
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

        std::cout << "Running for size: " << m << "x" << n << std::endl;

        double serial_time = 0.0;

        std::cout << "_______START SERIAL_______" << std::endl;
        for (int i = 0; i < 3; ++i) {
            std::unique_ptr<double[]> a(new double[m * n]);
            std::unique_ptr<double[]> b(new double[n]);
            std::unique_ptr<double[]> c(new double[m]);


            serial_initialize(a.get(), b.get(), c.get(), m, n);

            auto start = std::chrono::steady_clock::now();
            serial_multiply(a.get(), b.get(), c.get(), m, n);
            auto end = std::chrono::steady_clock::now();

            serial_time += std::chrono::duration<double, std::milli>(end - start).count();
        }
        serial_time /= 3.0;
        std::cout << "serial time = " << serial_time << " ms\n";
        file << size << "," << serial_time;

        std::cout << "_______START PARALLEL_______" << std::endl;
        for (int threads : thread_counts) {
            double total_parallel_time = 0.0;

            for (int i = 0; i < 3; ++i) {
                std::unique_ptr<double[]> a(new double[m * n]);
                std::unique_ptr<double[]> b(new double[n]);
                std::unique_ptr<double[]> c(new double[m]);


                parallel_initialize(a.get(), b.get(), c.get(), m, n, threads);

                auto start = std::chrono::steady_clock::now();
                parallel_multiply(a.get(), b.get(), c.get(), m, n, threads);
                auto end = std::chrono::steady_clock::now();

                total_parallel_time += std::chrono::duration<double, std::milli>(end - start).count();
            }

            total_parallel_time /= 3.0;
            double speedup = serial_time / total_parallel_time;
            std::cout << threads << " threads: " << total_parallel_time << " ms, speedup = " << speedup << std::endl;
            file << "," << total_parallel_time << "," << speedup;
        }

        file << "\n";
    }

    file.close();
    std::cout << "Results written to matrix_benchmark.csv\n";

    return 0;
}
