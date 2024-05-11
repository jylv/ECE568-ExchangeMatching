#include <iostream>
// #include <list>
#include <tbb/task.h>
#include <tbb/task_group.h>
#include <tbb/task_scheduler_init.h>
#include <boost/thread.hpp>
#include <mutex>
#include <chrono>
#include <atomic>

using namespace std::chrono;
using namespace tbb;

std::mutex output;
int count = 0;

long fib(long a) {
    if (a < 2) return 1;
    
    return fib(a - 1) + fib(a - 2);
}

class PrintTask {
public:
    void operator()() const {
        // output.lock();
        // std::cout << "In thread " <<  boost::this_thread::get_id() << std::endl;
        // ++count;
        // output.unlock();
        fib(40);
    }

    
};

void myTask(long x) {
<<<<<<< HEAD
=======
    std::cout << boost::this_thread::get_id() << std::endl;
>>>>>>> add scalability testing logic
    fib(x);
}

int main() {
    // std::cout << boost::thread::hardware_concurrency() << std::endl;
<<<<<<< HEAD
    tbb::task_scheduler_init init(64); //creates 8 threads
    task_group group;
    
    auto start = high_resolution_clock::now();
    for (int i = 0; i < 100; ++i) {
=======
    tbb::task_scheduler_init init(4); //creates 8 threads
    task_group group;
    
    auto start = high_resolution_clock::now();
    for (int i = 0; i < 10; ++i) {
>>>>>>> add scalability testing logic
        // group.run(PrintTask());
        group.run([=]{myTask(40);});
        //std::cout << i << std::endl;
    }
    
    std::cout << "done" << std::endl;
    group.wait();
    auto stop = high_resolution_clock::now();
    
    auto duration = duration_cast<seconds>(stop - start);
    std::cout << duration.count() << std::endl;

    return 0;
}