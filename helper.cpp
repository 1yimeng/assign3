#include <chrono>
#include <iostream>

using namespace std;
using namespace std::chrono;

double get_time() {
    return (double)duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count() / (double)1000;
}
