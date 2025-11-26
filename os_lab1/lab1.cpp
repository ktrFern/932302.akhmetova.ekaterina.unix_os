#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>

using namespace std;

class Monitor {
private:
    mutex mtx;
    condition_variable cv;

    int ready = 0;
    bool finish = false;

    class Data {
    public:
        int num;
    };

    Data* data = nullptr;
    int limit;

public:
    Monitor(int count) : limit(count) {}

    void produce() {
        for (int i = 1; i <= limit; i++)
        {
            this_thread::sleep_for(chrono::seconds(1));
            unique_lock<mutex> lock(mtx);

            while (ready == 1)
                cv.wait(lock);

            data = new Data{ i };
            ready = 1;
            cout << "Producer: sent message " << i << endl;
            cv.notify_one();
            lock.unlock();
        }

        unique_lock<mutex> lock(mtx);
        finish = true;
        cv.notify_one();
        lock.unlock();
    }

    void consume() {
        while (true)
        {
            unique_lock<mutex> lock(mtx);
            while (!(ready == 1 || finish)) {
                cv.wait(lock);
            }

            if (finish && !ready)
                break;

            cout << "Consumer: received message " << data->num << endl;
            delete data;
            data = nullptr;
            ready = 0;
            cv.notify_one();
            lock.unlock();
        }
    }
};

int main() {
    int n = 15;
    Monitor monit(n);
    thread producer(&Monitor::produce, &monit);
    thread consumer(&Monitor::consume, &monit);
    producer.join();
    consumer.join();
    cout << "Program finished." << endl;
    return 0;
}

