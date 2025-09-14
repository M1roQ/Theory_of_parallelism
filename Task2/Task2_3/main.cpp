#include <iostream>
#include <vector>
#include <cmath>
#include <omp.h>
#include <chrono>
#include <fstream>
#include <memory>

#define N        1900
#define TAU      0.001
#define EPSILON  0.00001
// #define CHUNK    200

using vec = std::unique_ptr<double[]>;

double run(void (*generate)(vec&, vec&, vec&, vec&, int, int),
           void (*step)(const vec&, const vec&, const vec&, vec&, double, int, int),
           double tau, int n, int NumThreads);

void generate_serial(vec& A, vec& b, vec& x, vec& x0, int n, int _);
void generate_parallel_threads(vec& A, vec& b, vec& x, vec& x0, int n, int NumThreads);

void step_serial               (const vec& A, const vec& b, const vec& x0, vec& x,  double tau, int n, int _);
void step_parallel_threads     (const vec& A, const vec& b, const vec& x0, vec& x,  double tau, int n, int NumThreads);
void step_parallel_for_auto    (const vec& A, const vec& b, const vec& x0, vec& x,  double tau, int n, int NumThreads);
void step_parallel_for_static  (const vec& A, const vec& b, const vec& x0, vec& x,  double tau, int n, int NumThreads);
void step_parallel_for_dynamic (const vec& A, const vec& b, const vec& x0, vec& x,  double tau, int n, int NumThreads);
void step_parallel_for_guided  (const vec& A, const vec& b, const vec& x0, vec& x,  double tau, int n, int NumThreads);

double length      (const vec& v, int n);
double mult_length (const vec& A, const vec& b, const vec& x, int n);

int main()
{
    std::ofstream output("../results/sole_result.csv");
    std::vector<int> threads_list{1,2,4,7,8,16,20,40};

    // 1) Последовательный базовый прогон
    std::cout << "Running serial...\n";
    double t_serial = 0;
    for(int i=0; i<10; ++i)
        t_serial += run(generate_serial, step_serial, TAU, N, 0);
    t_serial /= 10;
    std::cout << "Serial time = " << t_serial << " s" << std::endl;


    // Заголовок CSV
    output << "N=" << N;
    for (int t : threads_list) {
        output << ",T" << t;
        if (t != 1) output << ",S" << t;
    }
    output << "\n";

    auto write_variant = [&](const std::string &name,
                             const std::vector<double> &times)
    {
        output << N;
        for (size_t i = 0; i < threads_list.size(); ++i) {
            int t = threads_list[i];
            double ti = times[i];
            double speedup = (t == 1 ? 1.0 : t_serial / ti);
            output << "," << ti;
            if (t != 1) output << "," << speedup;
        }
        output << "\n";
    };

    // 2) Сбор времен для каждого варианта
    std::vector<double> times_manual, times_auto, times_static, times_dynamic, times_guided;
    
    std::cout << "Running parallel...\n";

    for (int T : threads_list) {
        double sum;

        // Ручная параллельная версия
        sum = 0;
        for(int i=0;i<10;++i)
            sum += run(generate_parallel_threads, step_parallel_threads, TAU, N, T);
        double avg_manual = sum / 10;
        times_manual.push_back(avg_manual);
        std::cout << T << " threads [manual]: time = " << avg_manual << " s, speedup = " << (t_serial / avg_manual) << std::endl;


        // schedule(auto)
        sum = 0;
        for(int i=0;i<10;++i)
            sum += run(generate_serial, step_parallel_for_auto, TAU, N, T);
        double avg_auto = sum / 10;
        times_auto.push_back(avg_auto);
        std::cout << T << " threads [auto]:   time = " << avg_auto << " s, speedup = " << (t_serial / avg_auto) << std::endl;

        // schedule(static)
        sum = 0;
        for(int i=0;i<10;++i)
            sum += run(generate_serial, step_parallel_for_static, TAU, N, T);
        double avg_static = sum / 10;
        times_static.push_back(avg_static);
        std::cout << T << " threads [static]: time = " << avg_static << " s, speedup = " << (t_serial / avg_static) << std::endl;


        // schedule(dynamic)
        sum = 0;
        for(int i=0;i<10;++i)
            sum += run(generate_serial, step_parallel_for_dynamic, TAU, N, T);
        double avg_dynamic = sum / 10;
        times_dynamic.push_back(avg_dynamic);
        std::cout << T << " threads [dynamic]: time = " << avg_dynamic << " s, speedup = " << (t_serial / avg_dynamic) << std::endl;


        // schedule(guided)
        sum = 0;
        for(int i=0;i<10;++i)
            sum += run(generate_serial, step_parallel_for_guided, TAU, N, T);
        double avg_guided = sum / 10;
        times_guided.push_back(avg_guided);
        std::cout << T << " threads [guided]: time = " << avg_guided << " s, speedup = " << (t_serial / avg_guided) << std::endl;
    }

    // 3) Запись в CSV
    write_variant("manual",  times_manual);
    write_variant("auto",    times_auto);
    write_variant("static",  times_static);
    write_variant("dynamic", times_dynamic);
    write_variant("guided",  times_guided);

    output.close();
    std::cout << "Results written to result/sole_result.csv\n";
    return 0;
}

double run(void (*gen)(vec&, vec&, vec&, vec&, int, int),
           void (*step)(const vec&, const vec&, const vec&, vec&, double, int, int),
           double tau, int n, int T)
{
    vec A (new double[n*n]);
    vec b (new double[n]);
    vec x0(new double[n]);
    vec x (new double[n]);

    gen(A,b,x0,x0,n,T);  

    double bnorm = length(b,n);
    double rnorm = mult_length(A,b,x0,n);
    double g     = rnorm/bnorm;
    double time  = 0;
    int iter     = 0;

    while (g >= EPSILON) {
        auto t0 = std::chrono::steady_clock::now();
        step(A,b, (iter%2?x:x0), (iter%2?x0:x), tau, n, T);
        auto t1 = std::chrono::steady_clock::now();
        time += std::chrono::duration<double>(t1-t0).count();

        // новая невязка
        vec &cur = (iter%2? x0 : x);
        rnorm = mult_length(A,b,cur,n);
        g = rnorm/bnorm;
        ++iter;
    }
    return time;
}

// Генерация данных последовательно
void generate_serial(vec& A, vec& b, vec& x, vec& x0, int n, int _)
{
    for(int i=0;i<n;++i){
        b[i] = n+1;
        x[i] = x0[i] = 0.0;
        for(int j=0;j<n;++j) A[i*n+j] = (i==j?2.0:1.0);
    }
}

// Ручное распараллеливание генерации
void generate_parallel_threads(vec& A, vec& b, vec& x, vec& x0, int n, int T)
{
    #pragma omp parallel num_threads(T)
    {
        int tid = omp_get_thread_num();
        int nt = omp_get_num_threads();

        int chunk = n / nt;
        int remainder = n % nt;
        int lo = tid * chunk + (tid < remainder ? tid : remainder);
        int hi = lo + chunk + (tid < remainder ? 1 : 0);

        for (int i = lo; i < hi; ++i) {
            b[i] = static_cast<double>(n + 1);
            for (int j = 0; j < n; ++j) {
                A[i * n + j] = static_cast<double>((i == j) ? 2.0 : 1.0);
            }
            x[i] = x0[i] = 0.0;
        }
    }
}


// Последовательный шаг итерации
void step_serial(const vec& A, const vec& b, const vec& x0, vec& x,
                 double tau, int n, int _)
{
    for(int i=0;i<n;++i){
        double tmp=0;
        for(int j=0;j<n;++j) tmp += A[i*n+j]*x0[j];
        x[i] = x0[i] - tau*(tmp - b[i]);
    }
}

// Ручной параллелизм
void step_parallel_threads(const vec& A, const vec& b, const vec& x0, vec& x,
                           double tau, int n, int T)
{
    #pragma omp parallel num_threads(T)
    {
        int tid = omp_get_thread_num(), nt = omp_get_num_threads();
        int chunk = n/nt, lo = tid*chunk, hi = (tid==nt-1? n : lo+chunk);
        for(int i=lo;i<hi;++i){
            double tmp=0;
            for(int j=0;j<n;++j) tmp += A[i*n+j]*x0[j];
            x[i] = x0[i] - tau*(tmp - b[i]);
        }
    }
}

// OpenMP-for schedule(auto)
void step_parallel_for_auto(const vec& A, const vec& b, const vec& x0, vec& x,
                            double tau, int n, int T)
{
    omp_set_num_threads(T);
    #pragma omp parallel for schedule(auto)
    for(int i=0;i<n;++i){
        double tmp=0;
        for(int j=0;j<n;++j) tmp += A[i*n+j]*x0[j];
        x[i] = x0[i] - tau*(tmp - b[i]);
    }
}

// schedule(static, CHUNK)
void step_parallel_for_static(const vec& A, const vec& b, const vec& x0, vec& x,
                              double tau, int n, int T)
{
    omp_set_num_threads(T);
    #pragma omp parallel for schedule(static)
    for(int i=0;i<n;++i){
        double tmp=0;
        for(int j=0;j<n;++j) tmp += A[i*n+j]*x0[j];
        x[i] = x0[i] - tau*(tmp - b[i]);
    }
}

// schedule(dynamic, CHUNK)
void step_parallel_for_dynamic(const vec& A, const vec& b, const vec& x0, vec& x,
                               double tau, int n, int T)
{
    omp_set_num_threads(T);
    #pragma omp parallel for schedule(dynamic)
    for(int i=0;i<n;++i){
        double tmp=0;
        for(int j=0;j<n;++j) tmp += A[i*n+j]*x0[j];
        x[i] = x0[i] - tau*(tmp - b[i]);
    }
}

// schedule(guided, CHUNK)
void step_parallel_for_guided(const vec& A, const vec& b, const vec& x0, vec& x,
                              double tau, int n, int T)
{
    omp_set_num_threads(T);
    #pragma omp parallel for schedule(guided)
    for(int i=0;i<n;++i){
        double tmp=0;
        for(int j=0;j<n;++j) tmp += A[i*n+j]*x0[j];
        x[i] = x0[i] - tau*(tmp - b[i]);
    }
}

// Норма вектора
double length(const vec& v, int n)
{
    double s=0;
    for(int i=0;i<n;++i) s += v[i]*v[i];
    return std::sqrt(s);
}

// Норма невязки ||Ax-b||
double mult_length(const vec& A, const vec& b, const vec& x, int n)
{
    double s=0;
    for(int i=0;i<n;++i){
        double tmp=0;
        for(int j=0;j<n;++j) tmp += A[i*n+j]*x[j];
        double r = tmp - b[i];
        s += r*r;
    }
    return std::sqrt(s);
}
