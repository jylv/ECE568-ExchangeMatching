#include <iostream>
#include <cstdlib>
#include "Utils/utils.hpp"
#include "Server/BaseServer.hpp"
using namespace std;

int main(int argc, char const *argv[]) {
    if (argc != 3) {
        cerr << "The input should be ./scalability-test-server [number of threads] [number of requests]\n";
        exit(EXIT_FAILURE);
    }

    int threadNum;
    int reqNum;

    if (!isValidInteger(argv[1], true, &threadNum) || threadNum <= 1) {
        cerr << "The number of threads should be a greater-than-one integer\n";
        exit(EXIT_FAILURE);
    }
    if (!isValidInteger(argv[2], true, &reqNum)) {
        cerr << "The number of requests should be a positive integer\n";
        exit(EXIT_FAILURE);
    }

    try {
        BaseServer server(nullptr, "12345", 500, threadNum);
        server.scalabilityTest(reqNum);
    } catch (const exception &e) {
        cerr << e.what() << endl;
        exit(EXIT_FAILURE);
    }
    return 0;
}