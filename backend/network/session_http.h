#pragma once

#include "session.h"


class THTTPSession : public TSession {
public:
    using TSocket = boost::asio::basic_stream_socket<boost::asio::ip::tcp>;
    using TEndPointIterator = boost::asio::ip::tcp::resolver::iterator;

public:
    THTTPSession(
        boost::asio::io_service &ioService,
        THTTPRequestHandlerPtr requestHandler,
        TOutgoingRequestsPtr outgoing
    );

    ~THTTPSession();

    boost::asio::ip::tcp::socket::lowest_layer_type &GetSocket() override;
    void Accept(TSessionPtr This) override;
    void Connect(TSessionPtr This, TEndPointIterator endpoint_iterator) override;

private:
    TSocket Socket;

private:
    void StartReading(TSessionPtr This) override;
    void StartWriting(TSessionPtr This) override;           // Mutex should be locked

    void HandleConnect(TSessionPtr This, const boost::system::error_code &error);
};

TSessionPtr Connect(const std::string &host, int port, boost::asio::io_service &iOService, THTTPRequestHandlerPtr handler, TOutgoingRequestsPtr outgoingRequests);

