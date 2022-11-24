#include "helper.h"

using namespace std;
using namespace std::chrono;

double get_time() {
    return (double)duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count() / (double)1000;
}

string get_host_id(char* hostname, int pid) {
    string all = string(hostname) + "." + to_string(pid);
    return all;
}