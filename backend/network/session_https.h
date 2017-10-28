#pragma once

#include "session.h"
#include <boost/asio/ssl.hpp>


class THTTPSSession : public TSession {
public:
    using TSSLSocket = boost::asio::ssl::stream<boost::asio::ip::tcp::socket>;

public:
    THTTPSSession(
        boost::asio::io_service &ioService,
        boost::asio::ssl::context &context,
        THTTPRequestHandlerPtr requestHandler,
        TOutgoingRequestsPtr outgoing
    );

    ~THTTPSSession();

    boost::asio::ip::tcp::socket::lowest_layer_type &GetSocket() override;
    void Accept(TSessionPtr This) override;
    void Connect(TSessionPtr This, TEndPointIterator endpoint_iterator) override;

private:
    TSSLSocket Socket;

private:
    void StartReading(TSessionPtr This) override;
    void StartWriting(TSessionPtr This) override;           // Mutex should be locked

    void HandleConnect(TSessionPtr This, const boost::system::error_code &error);
    void HandleHandshake(TSessionPtr This, const boost::system::error_code &error);
};

TSessionPtr Connect(const std::string &host, int port, boost::asio::io_service &iOService, boost::asio::ssl::context &sSLContext, THTTPRequestHandlerPtr handler, TOutgoingRequestsPtr outgoingRequests);

