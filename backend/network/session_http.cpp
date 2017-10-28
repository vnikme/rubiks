#include "session_http.h"
#include <iostream>


//
// THTTPSession
//

THTTPSession::THTTPSession(
    boost::asio::io_service &ioService,
    THTTPRequestHandlerPtr requestHandler,
    TOutgoingRequestsPtr outgoing
)
    : TSession(requestHandler, outgoing)
    , Socket(ioService)
{
}

THTTPSession::~THTTPSession() {
}

boost::asio::ip::tcp::socket::lowest_layer_type &THTTPSession::GetSocket() {
    return Socket.lowest_layer();
}

void THTTPSession::Accept(TSessionPtr This) {
    StartIO(This);
}

void THTTPSession::Connect(TSessionPtr This, TEndPointIterator endpoint_iterator) {
    boost::asio::async_connect(GetSocket(), endpoint_iterator,
        [this, This](const boost::system::error_code &error, boost::asio::ip::tcp::resolver::iterator endpoint_iterator) {
            HandleConnect(This, error);
        }
    );
}

void THTTPSession::HandleConnect(TSessionPtr This, const boost::system::error_code &error) {
    if (!error) {
        StartIO(This);
    } else {
        std::cout << "Error in connect: " << error.message() << std::endl;
    }
}

void THTTPSession::StartReading(TSessionPtr This) {
    Socket.async_read_some(boost::asio::buffer(GetData(), GetDataSize()),
        [this, This] (const boost::system::error_code &error, size_t bytesTransferred) {
            HandleRead(This, error, bytesTransferred);
        }
    );
}

void THTTPSession::StartWriting(TSessionPtr This) {
    boost::asio::async_write(Socket, boost::asio::buffer(GetOutgoing()->GetFirstToSend()),
        [this, This] (const boost::system::error_code &error, size_t bytesTransferred) {
            HandleWrite(This, error, bytesTransferred);
        }
    );
}

TSessionPtr Connect(const std::string &host, int port, boost::asio::io_service &iOService, THTTPRequestHandlerPtr handler, TOutgoingRequestsPtr outgoingRequests) {
    boost::asio::ip::tcp::resolver resolver(iOService);
    boost::asio::ip::tcp::resolver::query query(host, boost::lexical_cast<std::string>(port));
    boost::asio::ip::tcp::resolver::iterator iterator = resolver.resolve(query);
    TSessionPtr sess(new THTTPSession(iOService, handler, outgoingRequests));
    sess->Connect(sess, iterator);
    return sess;
}

