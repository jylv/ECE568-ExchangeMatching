#include <iostream>
#include <thread>
#include <vector>
#include <mutex>

using namespace std;

mutex xMutex;
static int x = 0;

void func(string &a, string &b) {
    xMutex.lock();
    ++x;

    cout << "From Thread ID " << this_thread::get_id() << ":\n";
    cout << a << ' ' << b << ' ' << x << endl;
    xMutex.unlock();

    a += " after func";
    b += " after func";
}

int main(int argc, char const *argv[]) {
    // Create a vector of threads
    std::vector<std::thread> vecOfThreads;
    
    // Add a Thread object to vector
    // vecOfThreads.push_back(std::thread(func));
    // Create 3 differet thread objects
    string s1 = "first thread";
    string s2 = "second thread";
    string s3 = "third thread";
    std::thread th1(func, ref(s1), ref(s1));
    std::thread th2(func, ref(s2), ref(s2));
    std::thread th3(func, ref(s3), ref(s3));
    // Move all three thread objects to vector
    vecOfThreads.push_back(std::move(th1));
    vecOfThreads.push_back(std::move(th2));
    vecOfThreads.push_back(std::move(th3));
    // Do some important work in main thread.
    /** Wait for all the threads in vector to join **/
    // Iterate over the thread vector
    for (std::thread & th : vecOfThreads)
    {
        // If thread Object is Joinable then Join that thread.
        if (th.joinable())
            th.join();
    }

    cout << s1 << endl;
    cout << s2 << endl;
    cout << s3 << endl;

    return 0;
}
