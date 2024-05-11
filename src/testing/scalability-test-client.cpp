#include "random-generate2.hpp"
#include "../Server/BaseSocket.hpp"
#include "../Utils/utils.hpp"
#include <iostream>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <cstring>
#include <boost/thread.hpp>
#include <tbb/task.h>
#include <tbb/task_group.h>
#include <tbb/task_scheduler_init.h>

using namespace std;
using namespace boost;
using namespace tbb;

boost::mutex outputLock;

void requestService(boost::barrier &bar, struct addrinfo &host_info, int reqperThread) {
    bool flip = true;
    try {
        // wait until there're all requests prepared
        bar.wait();

        for (int i = 0; i < reqperThread; ++i) {
            MySocket sock(host_info, "vcm-25032.vm.duke.edu", "12345");
            sock.connectSocket();

            string req;
            if (flip) {
                req = generateCreate();
                flip = false;
            } else {
                req = generateTransaction();
                flip = true;
            }

            std::cout << "=============Sending=============\n";
            sock.sendData(req);
            std::cout << "=============Receiving=============\n";
            sock.receiveData();
            // cout << sock.receiveData() << endl;
        }
        
    } catch (const std::exception &e) {
        cerr << e.what() << endl;
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char const *argv[]) {
    if (argc != 2) {
        cerr << "The input should be ./scalability-test-client [number of requests]\n";
        exit(EXIT_FAILURE);
    }

    int reqNum;

    if (!isValidInteger(argv[1], true, &reqNum)) {
        cerr << "The number of requests should be a positive integer\n";
        exit(EXIT_FAILURE);
    }

    struct addrinfo host_info;

    memset(&host_info, 0, sizeof(host_info));

    host_info.ai_family   = AF_UNSPEC;
    host_info.ai_socktype = SOCK_STREAM;

    // create 64 threads sending requests to server at the same time
    int threadNum = 64;
    boost::barrier bar(threadNum);
    task_group group;
    task_scheduler_init init(threadNum);
	vector<boost::thread> threads;
	for (int i = 0; i < threadNum; ++i) {
        int reqperThread = reqNum / threadNum;
        if (i == threadNum - 1) {
            reqperThread += reqNum % threadNum;
        }
        cout << "Creating thread " << i << endl;
		boost::thread thr(boost::bind(&requestService, boost::ref(bar), boost::ref(host_info), reqperThread));
		threads.push_back(move(thr));
	}

	for (boost::thread &thr : threads) {
		thr.join();
	}

    return 0;
}