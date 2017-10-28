#include "worker_pool.h"
#include <chrono>
#include <iostream>


//
// TWorkerPool
//

TWorkerPool::TWorkerPool(size_t threadCount, time_t secondsForShutdown)
    : WorkingThreads(threadCount)
    , SecondsForShutdown(secondsForShutdown)
{
}

TWorkerPool::~TWorkerPool() {
}

void TWorkerPool::AddEvent(const TEvent &event) {
    {
        std::unique_lock<std::mutex> lk(Mutex);
        Events.push_back(event);
    }
    Condition.notify_one();                     // Notify working thread about new request
}

void TWorkerPool::Stop() {
    std::unique_lock<std::mutex> lk(Mutex);
    Exit = true;
    Condition.notify_all();
}

void TWorkerPool::Run() {
    for (auto &thread : WorkingThreads)
        thread = std::thread([this]() { WorkingThreadMethod(); });
}

void TWorkerPool::Join() {
    Stop();
    for (auto &thread : WorkingThreads)
        thread.join();
}

void TWorkerPool::WorkingThreadMethod() {
    for (; ;) {
        std::unique_lock<std::mutex> lk(Mutex);
        while (!Exit && Events.empty()) {
            if (Condition.wait_for(lk, std::chrono::milliseconds(100)) == std::cv_status::timeout) {
            }
        }
        time_t shutdownTime = 0;
        while (!Events.empty()) {
            if (Exit) {
                time_t currentTime = time(nullptr);
                if (shutdownTime == 0)
                    shutdownTime = currentTime + SecondsForShutdown;
                if (shutdownTime >= currentTime)
                    break;
            }
            auto event = Events.front();
            Events.pop_front();
            lk.unlock();
            try {
                event();
            } catch (...) {
                std::cerr << "Exception in TWorkerPool::WorkingThreadMethod, event()" << std::endl;
            }
            lk.lock();
        }
        if (Exit)
            return;
    }
}


//
// TWorkerHTTPRequestHandler
//
TWorkerHTTPRequestHandler::TWorkerHTTPRequestHandler(TWorkerPoolPtr pool, THTTPRequestHandlerPtr handler)
    : Pool(pool)
    , Handler(handler)
{
}

TWorkerHTTPRequestHandler::~TWorkerHTTPRequestHandler() {
}

// Just put received request into the queue, do all usefull work in working threads
void TWorkerHTTPRequestHandler::ProcessRequest(TSessionPtr session, THTTPRequestPtr request) {
    TWorkerPoolPtr pool(Pool.lock());
    if (pool.get() == nullptr)
        return;
    auto handler = Handler;
    pool->AddEvent([session, request, handler]() { handler->ProcessRequest(session, request); });
}

