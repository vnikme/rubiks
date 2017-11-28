#include "rubiks.h"
#include "../dist/json/json.h"
#include "../network/session_http.h"
#include "../util/json.h"
#include "../util/url.h"
#include "../util/random_util.h"
#include <fstream>
#include <cube.h>
#include <kociemba.h>
#include <sstream>
#include <ctime>


//
// TRubiks
//

void TRubiks::Init(const Json::Value &data)
{
    class TServiceDispatcher : public THTTPRequestHandler {
        private:
            typedef void (TRubiks::*THandler) (TSessionPtr, THTTPRequestPtr);

        public:
            TServiceDispatcher(TRubiks &host, THandler handler)
                : Host(host)
                , Handler(handler)
            {}

        private:
            TRubiks &Host;
            THandler Handler;
            void ProcessRequest(TSessionPtr session, THTTPRequestPtr request) override {
                (Host.*Handler)(session, request);
            }
    };

    Server = std::make_shared<TServer>();
    WorkerPool = std::make_shared<TWorkerPool>(data.get("worker_count", 10).asInt(), data.get("seconds_for_shutdown", 30).asInt());
    SolverPool = std::make_shared<TWorkerPool>(data.get("solver_count", 1).asInt(), data.get("seconds_for_shutdown", 30).asInt());
    LogPath = data.get("log_path", "data/rubiks.log").asString();
    LogFlushInterval = data.get("log_flush_interval", 10).asInt();
    LogFillingThreshold = data.get("log_filling_threshold", 100).asInt();
    LogLastFlushingTime = 0;
    auto httpHandler = std::make_shared<TServiceDispatcher>(*this, &TRubiks::ProcessHTTP);
    auto httpForwarder = std::make_shared<TWorkerHTTPRequestHandler>(WorkerPool, httpHandler);
    HttpPort = data.get("http_port", 17071).asInt();
    Server->AddHTTPService(false, HttpPort, httpForwarder);
}

void TRubiks::Run() {
    InitKociemba();
    MainThread = std::thread([this]() {
        MainThreadMethod();
    });
    ServiceThread = std::thread([this]() {
        WorkerPool->Run();
        SolverPool->Run();
        Server->Run();
        WorkerPool->Join();
        SolverPool->Join();
        MainThread.join();
    });
}

void TRubiks::Join() {
    ServiceThread.join();
}

void TRubiks::ProcessHTTP(TSessionPtr session, THTTPRequestPtr req) {
    {
        std::unique_lock<std::mutex> lk(Mutex);
        if (Exit)
            return;
    }
    std::string method, url, protocol, resource;
    SplitStartingLine(req->GetStartingLine(), method, url, protocol);
    TUrlCgiParams params;
    ParseUrlResource(url, resource, params);
    bool result = true;
    Json::Value data;
    if (url == "/exit") {
        Stop();
    } else if (resource == "/solve") {
        result = Solve(params, data);
    } else if (resource == "/log") {
        result = LogEvent(params, data);
    } else {
        result = false;
    }
    std::map<std::string, std::string> headers;
    std::string json = SaveJson(data);
    headers["Content-Length"] = boost::lexical_cast<std::string>(json.size());
    THTTPRequest response(std::string(result ? "HTTP/1.1 200 OK" : "HTTP/1.1 400 Bad Request"), std::move(headers), json);
    session->AddOutgoingRequest(session, response, THTTPReplyHandlerPtr());
}

bool TRubiks::Stop() {
    Server->Stop();
    WorkerPool->Stop();
    std::unique_lock<std::mutex> lk(Mutex);
    Exit = true;
    Condition.notify_all();
    return true;
}

class TPrintHandler : public THTTPReplyHandler {
    public:
        virtual void ProcessReply(TSessionPtr sess, THTTPRequestPtr req) override {
            std::cout << req->GetStartingLine() << std::endl << req->GetBodyStr() << std::endl;
        }
};

void TRubiks::MainThreadMethod() {
    for (; ;) {
        std::unique_lock<std::mutex> lk(Mutex);
        if (!Exit && LogRecords.empty())
            Condition.wait_for(lk, std::chrono::milliseconds(100));
        if (LogRecords.size() >= LogFillingThreshold || LogLastFlushingTime + LogFlushInterval < time(nullptr))
            FlushLog();
        if (Exit)
            return;
    }
}

bool TRubiks::Solve(const TUrlCgiParams &params, Json::Value &data) {
    if (params.empty())
        return false;
    std::string cube;
    for (const auto &param : params) {
        if (param.first == "cube")
            cube = param.second;
    }
    if (cube.empty())
        return false;
    std::cout << "solving " << cube << std::endl;
    std::unique_lock<std::mutex> lk(Mutex);
    auto it = Solutions.find(cube);
    if (it == Solutions.end()) {
        Solutions[cube] = "pending";
        auto *mtx = &Mutex;
        auto *solutions = &Solutions;
        SolverPool->AddEvent([cube, mtx, solutions]() {
            try {
                TCube puzzle = MakePuzzle(cube);
                std::vector<ETurnExt> solution;
                if (!KociembaSolution(puzzle, solution))
                    throw std::logic_error("no solution");
                std::stringstream str;
                str << "[";
                for (size_t i = 0; i < solution.size(); ++i) {
                    if (i > 0)
                        str << ",";
                    str << "\"" << TurnExt2String(solution[i]) << "\"";
                }
                str << "]";
                std::unique_lock<std::mutex> lk(*mtx);
                (*solutions)[cube] = str.str();
            } catch (...) {
                std::unique_lock<std::mutex> lk(*mtx);
                (*solutions)[cube] = "no solution";
            }
        });
        it = Solutions.find(cube);
    }
    std::cout << "result is " << it->second << std::endl;
    if (it->second == "pending") {
        data["state"] = "pending";
    } else if (it->second == "no solution") {
        data["state"] = "fail";
    } else {
        data["state"] = "ok";
        data["result"] = it->second;
    }
    return true;
}

bool TRubiks::LogEvent(const TUrlCgiParams &params, Json::Value &data) {
    if (params.empty())
        return false;
    std::string event;
    for (const auto &param : params) {
        if (param.first == "data")
            event = param.second;
    }
    if (event.empty())
        return false;
    std::unique_lock<std::mutex> lk(Mutex);
    LogRecords.push_back(event);
    data["state"] = "ok";
    return true;
}

void TRubiks::FlushLog() {
    LogLastFlushingTime = time(nullptr);
    if (LogRecords.empty())
        return;
    auto records = std::make_shared<std::vector<std::string>>(std::move(LogRecords));
    std::mutex *mtx = &LogFlushMutex;
    std::string path = LogPath;
    WorkerPool->AddEvent([records, mtx, path]() {
        std::unique_lock<std::mutex> lk(*mtx);
        std::ofstream fout(path, std::ios_base::app | std::ios_base::out);
        for (const auto &event : *records)
            fout << event << '\n';
    });
}

