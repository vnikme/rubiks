#pragma once

#include <iostream>
#include <fstream>
#include <list>
#include <condition_variable>
#include <thread>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include "http_request.h"
#include "session.h"


class TServer : private boost::noncopyable {
    public:
        TServer();
        ~TServer();

        boost::asio::io_service &GetIOService();
        boost::asio::ssl::context &GetClientSSLContext();
        void AddHTTPService(bool isPublic, unsigned short port, THTTPRequestHandlerPtr handler);
        void AddHTTPSService(bool isPublic, unsigned short port,
                             THTTPRequestHandlerPtr handler,
                             const std::string &fullChainPath,
                             const std::string &privKeyPath,
                             const std::string &dhPath);
        void Run();
        void Stop();

    private:
        struct TListenerInfo {
            boost::asio::ip::tcp::acceptor Acceptor;
            THTTPRequestHandlerPtr MessageHandler;
            std::unique_ptr<boost::asio::ssl::context> SSLContext;
            TListenerInfo(boost::asio::io_service &ioService, bool isPublic, unsigned short port, THTTPRequestHandlerPtr handler);
        };
        using TListenerInfoPtr = std::shared_ptr<TListenerInfo>;

        boost::asio::io_service IOService;
        boost::asio::ssl::context ClientSSLContext;
        std::list<TListenerInfoPtr> Services;
        std::mutex Mutex;
        bool Exit = false;

    private:
        void StartAccept(TListenerInfoPtr listener);
        void HandleAccept(TListenerInfoPtr listener, TSessionPtr newSession, const boost::system::error_code &error);
};

using TServerPtr = std::shared_ptr<TServer>;

