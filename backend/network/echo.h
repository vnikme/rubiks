#pragma once

#include "http_request.h"

class TServer;

class TEchoHTTPRequestHandler : public THTTPRequestHandler {
    public:
        TEchoHTTPRequestHandler(TServer &server);

    private:
        TServer &Server;
        virtual void ProcessRequest(TSessionPtr session, THTTPRequestPtr req);
};

