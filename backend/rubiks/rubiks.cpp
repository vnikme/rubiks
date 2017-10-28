#include "rubiks.h"
#include "../dist/json/json.h"
#include "../network/session_http.h"
#include "../util/json.h"
#include "../util/url.h"
#include "../util/random_util.h"
#include <boost/filesystem.hpp>
#include <cube.h>
#include <kociemba.h>
#include <sstream>


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
    WorkerPool = std::make_shared<TWorkerPool>(data.get("worker_count", 10).asInt(), data.get("seconds_for_shutdown", 5).asInt());
    SolverPool = std::make_shared<TWorkerPool>(data.get("solver_count", 1).asInt(), data.get("seconds_for_shutdown", 30).asInt());
    auto httpHandler = std::make_shared<TServiceDispatcher>(*this, &TRubiks::ProcessHTTP);
    auto httpForwarder = std::make_shared<TWorkerHTTPRequestHandler>(WorkerPool, httpHandler);
    HttpPort = data.get("http_port", 17071).asInt();
    Server->AddHTTPService(false, HttpPort, httpForwarder);
}

void TRubiks::Run() {
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

void TRubiks::Stop() {
    Server->Stop();
    WorkerPool->Stop();
    std::unique_lock<std::mutex> lk(Mutex);
    Exit = true;
    Condition.notify_all();
}

void TRubiks::Join() {
    ServiceThread.join();
}

void TRubiks::ProcessHTTP(TSessionPtr session, THTTPRequestPtr req) {
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
    } else {
        result = false;
    }
    std::map<std::string, std::string> headers;
    std::string json = SaveJson(data);
    headers["Content-Length"] = boost::lexical_cast<std::string>(json.size());
    THTTPRequest response(std::string(result ? "HTTP/1.1 200 OK" : "HTTP/1.1 400 Bad Request"), std::move(headers), json);
    session->AddOutgoingRequest(session, response, THTTPReplyHandlerPtr());
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
        while (!(Exit)) {
            if (Condition.wait_for(lk, std::chrono::milliseconds(100)) == std::cv_status::timeout) {
            }
        }
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
                TCube zero = MakeSolvedCube();
                TCube puzzle = MakePuzzle(cube);
                std::vector<ETurnExt> solution;
                if (!KociembaSolution(puzzle, zero, solution))
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

