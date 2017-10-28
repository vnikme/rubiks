#pragma once

#include <iostream>
#include <fstream>
#include <list>
#include <condition_variable>
#include <thread>
#include <functional>
#include "http_request.h"
#include "session.h"


/*
    Object with N working threads.
    Gets request from processing queue and executes it when there is a free thread.
*/
class TWorkerPool : private boost::noncopyable {
    public:
        using TEvent = std::function<void()>;

        TWorkerPool(size_t threadCount, time_t secondsForShutdown);
        ~TWorkerPool();

        void AddEvent(const TEvent &event);
        void Stop();
        void Run();
        void Join();

    private:
        std::mutex Mutex;
        std::condition_variable Condition;
        std::list<TEvent> Events;
        bool Exit = false;
        std::vector<std::thread> WorkingThreads;
        time_t SecondsForShutdown = 0;

    private:
        void WorkingThreadMethod();
};
using TWorkerPoolPtr = std::shared_ptr<TWorkerPool>;
using TWorkerPoolWeakPtr = std::weak_ptr<TWorkerPool>;


/*
    Handler for forwarding requests to workers pool.
*/
class TWorkerHTTPRequestHandler : public THTTPRequestHandler {
    public:
        TWorkerHTTPRequestHandler(TWorkerPoolPtr pool, THTTPRequestHandlerPtr handler);
        ~TWorkerHTTPRequestHandler();

        // Just put received request into the queue, do all usefull work in working threads
        void ProcessRequest(TSessionPtr session, THTTPRequestPtr req) override;

    private:
        TWorkerPoolWeakPtr Pool;
        THTTPRequestHandlerPtr Handler;
};

