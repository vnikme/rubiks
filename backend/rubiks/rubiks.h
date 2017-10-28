#pragma once

#include "../dist/json/json-forwards.h"
#include "../network/http_request.h"
#include "../network/session.h"
#include "../network/server.h"
#include "../network/worker_pool.h"
#include "../util/url.h"
#include <boost/noncopyable.hpp>
#include <string>
#include <thread>
#include <condition_variable>
#include <map>


class TRubiks : private boost::noncopyable {
    public:
        void Init(const Json::Value &data);
        void Run();
        void Stop();
        void Join();

    private:
        using TSolutions = std::map<std::string, std::string>;

        // Threads and network processors
        TServerPtr Server;
        TWorkerPoolPtr WorkerPool;
        TWorkerPoolPtr SolverPool;
        std::thread MainThread;
        std::thread ServiceThread;
        // Constants initialized before work
        int HttpPort = 0;
        // Runtime objects
        mutable std::mutex Mutex;                           // Guards all runtime objects
        std::condition_variable Condition;                  // Condition for wake up MainThread
        bool Exit = false;                                  // Flag to stop all processes
        TSolutions Solutions;

        void ProcessHTTP(TSessionPtr session, THTTPRequestPtr request);
        void MainThreadMethod();
        bool Solve(const TUrlCgiParams &params, Json::Value &data);
};

