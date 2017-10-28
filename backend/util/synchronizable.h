#pragma once

#include <mutex>

class TSynchronizable {
    private:
        mutable std::recursive_mutex Mutex;

    public:
        void lock() const {
            Mutex.lock();
        }

        void unlock() const {
            Mutex.unlock();
        }
};

