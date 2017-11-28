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
        TSolutions Solutions;                               // Cached solutions
        std::vector<std::string> LogRecords;                // Strings to write into log
        std::string LogPath;                                // Path to log file
        std::mutex LogFlushMutex;                           // Mutex protecting log file
        size_t LogFlushInterval = 0;                        // How often flush the log
        size_t LogFillingThreshold = 0;                     // Number of log records after which flushing is needed anyway
        time_t LogLastFlushingTime = 0;                     // Last time the log has been flushed

        void ProcessHTTP(TSessionPtr session, THTTPRequestPtr request);
        void MainThreadMethod();
        bool Stop();
        bool Solve(const TUrlCgiParams &params, Json::Value &data);
        bool LogEvent(const TUrlCgiParams &params, Json::Value &data);
        void FlushLog();
};

