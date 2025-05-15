#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cmath>
#include <cstdlib>

bool is_equal(double a, double b, double epsilon = 1e-5) {
    return std::fabs(a - b) < epsilon;
}

int main(int argc, char const *argv[]) {
    if (argc < 2) {
        std::cout << "Введите название файла для проверки" << std::endl;
        return 1;
    }

    std::ifstream file(argv[1]); 
    if (!file.is_open()) {
        std::cerr << "Не удалось открыть файл: " << argv[1] << std::endl;
        return 1;
    }

    int all = 0;
    int accepted = 0;
    std::string line;

    while (std::getline(file, line)) {
        std::istringstream iss(line); 
        std::vector<std::string> tokens;
        std::string token;
        while (iss >> token) {
            tokens.push_back(token);
        }

        if (tokens.size() < 4) continue;

        if (tokens[0] == "pow" && tokens.size() >= 5) {
            double a = std::stod(tokens[1]);
            double b = std::stod(tokens[2]);
            double result = std::stod(tokens[4]);
            if (is_equal(std::pow(a, b), result)) accepted++;
            all++;
        }
        else if (tokens[0] == "sin") {
            double a = std::stod(tokens[1]);
            double result = std::stod(tokens[3]);
            if (is_equal(std::sin(a), result)) accepted++;
            all++;
        }
        else if (tokens[0] == "sqrt") {
            double a = std::stod(tokens[1]);
            double result = std::stod(tokens[3]);
            if (is_equal(std::sqrt(a), result)) accepted++;
            all++;
        }
    }

    std::cout << "Файл " << argv[1] << ", точность = "
              << static_cast<double>(accepted) / static_cast<double>(all) << std::endl;

    file.close();
    return 0;
}
