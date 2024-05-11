#include <iostream>
#include <cstdlib>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include "Server/BaseServer.hpp"
#include "Utils/utils.hpp"
using namespace std;

int main(int argc, char const *argv[]) {
    if (argc != 2) {
        cerr << "The input should be ./functionality-test [thread pool size]\n";
        exit(EXIT_FAILURE);
    }

    int threadNum;
    if (!isValidInteger(argv[1], true, &threadNum) || threadNum <= 1) {
        cerr << "The size of thread pool should be a >=1 integer\n";
        exit(EXIT_FAILURE);
    }
    
    try {
        BaseServer server(nullptr, "12345", 500, threadNum);
        server.launch();
    } catch (const exception &e) {
        cerr << e.what() << endl;
        exit(EXIT_FAILURE);
    }
    return 0;
}
