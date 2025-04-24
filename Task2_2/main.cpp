#include <iostream>
#include <omp.h>
#include <chrono>
#include <vector>
#include <fstream>
#include <cmath>

typedef double (*func)(double);

double integrate(func f, double a, double b, int n) {
    double h = (b - a) / n;
    double sum = 0.0;
    for (int i = 0; i < n; i++) {
        sum += f(a + h * (i + 0.5))*h;
    }
    return sum;
}

double integrate_omp(func f, double a, double b, int n, int num_threads) {
    double h = (b - a) / n;
    double sum = 0.0;

    #pragma omp parallel num_threads(num_threads)
    {
        int nthreads = omp_get_num_threads();
        int threadid = omp_get_thread_num();
        int items_per_thread = n / nthreads;
        int lb = threadid * items_per_thread;
        int ub = (threadid == nthreads - 1) ? (n - 1) : (lb + items_per_thread - 1);
        
        double sumloc = 0.0;
        for (int i = lb; i <= ub; i++) {
            sumloc += f(a + h * (i + 0.5))*h;
        }

        #pragma omp atomic
        sum += sumloc;
    }

    return sum;
}

int main() {
    const int nsteps = 40000000;
    const double a = 0.0;
    const double b = 1.0;

    std::vector<int> thread_counts = {1, 2, 4, 7, 8, 16, 20, 40};

    std::ofstream file("../results/integration_speedup.csv");
    file << "N=M,T1,T2,S2,T4,S4,T7,S7,T8,S8,T16,S16,T20,S20,T40,S40\n";

    file << nsteps;

    std::cout << "Running serial version..." << std::endl;
    double serial_time = 0.0;
    for (int i = 0; i < 10; ++i) {
        auto start = std::chrono::steady_clock::now();
        double reference = integrate(std::exp, a, b, nsteps);
        auto end = std::chrono::steady_clock::now();
        
        auto elapsed_time = std::chrono::duration<double>(end - start);
        serial_time += elapsed_time.count();
    }
    serial_time /= 10.0;
    std::cout << "Serial time = " << serial_time << " s" << std::endl;

    file << "," << serial_time;

    std::cout << "Running parallel version..." << std::endl;
    for (int threads : thread_counts) {
        double parallel_time = 0.0;
        for (int i = 0; i < 10; ++i) {
            auto start = std::chrono::steady_clock::now();
            double result = integrate_omp(std::exp, a, b, nsteps, threads);
            auto end = std::chrono::steady_clock::now();
            
            auto elapsed_time = std::chrono::duration<double>(end - start);
            parallel_time += elapsed_time.count();
        }
        parallel_time /= 10.0;

        double speedup = serial_time / parallel_time;

        std::cout << threads << " threads: time = " << parallel_time << " s, speedup = " << speedup << std::endl;

        file << "," << parallel_time << "," << speedup;
    }

    file << "\n";
    file.close();
    std::cout << "Results written to integration_speedup.csv" << std::endl;
    return 0;
}
