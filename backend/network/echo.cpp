#include "echo.h"
#include "../network/server.h"
#include "../network/session.h"
#include "../util/json.h"
#include "../dist/json/json.h"
#include <iostream>
#include <thread>


TEchoHTTPRequestHandler::TEchoHTTPRequestHandler(TServer &server)
    : Server(server)
{
}

void TEchoHTTPRequestHandler::ProcessRequest(TSessionPtr session, THTTPRequestPtr req) {
    try {
        std::cout << std::this_thread::get_id() << std::endl;

        std::cout << req->GetStartingLine() << std::endl;
        for (const auto item : req->GetHeaders())
            std::cout << item.first << ": " << item.second << std::endl;
        std::cout << req->GetBodyStr() << std::endl;

        Json::Value data = ParseJson(req->GetBodyStr());
        std::cout << data["request"].asString() << std::endl;
        if (data["request"].asString() == "9999")
            Server.Stop();

        std::ostringstream res;
        res << "{\"ok\": true, \"data\": \"" << data["request"].asString() << "\"}";
        std::string json = res.str();
        std::map<std::string, std::string> headers;
        headers["Content-Type"] = "application/json; charset=utf-8";
        headers["Content-Length"] = boost::lexical_cast<std::string>(json.size());
        THTTPRequest response(std::string("HTTP/1.1 200 OK"), std::move(headers), json);
        session->AddOutgoingRequest(session, response, THTTPReplyHandlerPtr());
        std::cout << std::endl;
    } catch (const std::exception &ex) {
        std::cerr << "TEchoHTTPRequestHandler::DoProcessRequest, exception: " << ex.what() << std::endl;
    } catch(...) {
        std::cerr << "TEchoHTTPRequestHandler::DoProcessRequest, unknown exception" << std::endl;
    }
}

