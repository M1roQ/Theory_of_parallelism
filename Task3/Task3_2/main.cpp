#include <iostream>
#include <queue>
#include <mutex>
#include <future>
#include <thread>
#include <chrono>
#include <cmath>
#include <functional>
#include <unordered_map>
#include <fstream>
#include <random>
#include <iomanip>

template<typename T>
T f_pow(T x, T y)
{
    return std::pow(x, y);
}

template<typename T>
T f_sin(T x)
{
    return std::sin(x);
}

template<typename T>
T f_sqrt(T x)
{
    return std::sqrt(x);
}

template<typename T>
class Server{
public:
    Server(){}

    void start(){
        working = true;
        potok = std::thread(&Server::work,this);
    }

    void stop(){
        working = false;
        potok.join();
    }

    size_t add_task(std::packaged_task<T()> task){
        std::unique_lock<std::mutex> lock(mtx);
        std::future<T> result = task.get_future();
        size_t idx = ++task_idx;
        results[idx] = result.share();
        tasks.push(std::move(task));
        idxs.push(idx);
        return idx;
    }

    T request_result(size_t idx){
        std::shared_future<T> future;
        {
            std::unique_lock<std::mutex> lock(mtx);
            auto it = results.find(idx);
            if (it == results.end()) {
                throw std::runtime_error("Invalid task ID");
            }
            future = it->second;
        }
        return future.get();
    }

private:
    void work() {
        while (working) {
            std::unique_lock<std::mutex> lock(mtx);
            if (!tasks.empty()) {
                size_t idx = idxs.front();
                auto task = std::move(tasks.front());
                tasks.pop();
                idxs.pop();
                lock.unlock();
                task();
            } else {
                lock.unlock();
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
        std::cout << "Server stop!\n";
    }

    std::mutex mtx;
    std::thread potok;
    bool working = false;
    size_t task_idx = 0;
    std::queue<size_t> idxs;
    std::queue<std::packaged_task<T()>> tasks;
    std::unordered_map<size_t, std::shared_future<T>> results;
};

void client(Server<double>& server, int N, int func_type, const std::string& filename)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> dist(1.0, 5.0);

    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Не удалось открыть файл: " << filename << std::endl;
        return;
    }

    for (size_t i = 0; i < N; ++i) {
        if (func_type == 0) {
            double arg1 = dist(gen);
            double arg2 = dist(gen);
            std::packaged_task<double()> task(std::bind(f_pow<double>, arg1, arg2));
            size_t idx = server.add_task(std::move(task));
            double res = server.request_result(idx);
            file << std::fixed << std::setprecision(8)
                 << "pow " << arg1 << " " << arg2 << " = " << res << "\n";
        }
        else if (func_type == 1) {
            double arg = dist(gen);
            std::packaged_task<double()> task(std::bind(f_sin<double>, arg));
            size_t idx = server.add_task(std::move(task));
            double res = server.request_result(idx);
            file << std::fixed << std::setprecision(8)
                 << "sin " << arg << " = " << res << "\n";
        }
        else if (func_type == 2) {
            double arg = dist(gen);
            std::packaged_task<double()> task(std::bind(f_sqrt<double>, arg));
            size_t idx = server.add_task(std::move(task));
            double res = server.request_result(idx);
            file << std::fixed << std::setprecision(8)
                 << "sqrt " << arg << " = " << res << "\n";
        }
    }

    file.close();
}

int main()
{
    std::cout << "Start\n";
    Server<double> servak;
    servak.start();

    std::thread client_pow(client, std::ref(servak), 10000, 0, "results/pow.txt");
    std::thread client_sin(client, std::ref(servak), 10000, 1, "results/sin.txt");
    std::thread client_sqrt(client, std::ref(servak), 10000, 2, "results/sqrt.txt");

    client_pow.join();
    client_sin.join();
    client_sqrt.join();

    servak.stop();

    std::cout << "End\n";
    return 0;
}