#include <iostream>
#include "helper.h"
#include <string>

using namespace std;

int main() {
    char* name = "ud01";
    int id = 123;
    string total = get_host_id(name, id);
    cout << total << endl;
}