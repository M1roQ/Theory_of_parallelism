#include <memory>
#include <iostream>
#include <numeric>
#include <cmath>

template<typename T>
class SinArray{
public:
    SinArray(size_t size) : size_(size), data_(std::make_unique<T[]>(size)) {}

    void fillArray(){
        const double PI = 3.141592653589793;
        for (size_t i = 0; i < size_; ++i){
            double x = 2 * PI * i / size_;
            data_[i] = std::sin(x);

        }
    }

    T sum() const{
        return std::accumulate(data_.get(), data_.get() + size_, T(0));
    }

    size_t size() const{
        return size_;
    }
private:
    size_t size_;
    std::unique_ptr<T[]> data_;
};

int main(){
    const size_t N = 10000000;

    #ifdef USE_FLOAT
        using T = float;
        std::cout << "Type: float\n";
    #else
        using T = double;
        std::cout << "Type: double\n";
    #endif

    SinArray<T> array(N);

    array.fillArray();

    T result = array.sum();

    std::cout << "SUM = " << result << std::endl;

    return 0;
}
