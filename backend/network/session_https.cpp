#include "session_https.h"
#include <iostream>


//
// THTTPSSession
//

THTTPSSession::THTTPSSession(
    boost::asio::io_service &ioService,
    boost::asio::ssl::context &context,
    THTTPRequestHandlerPtr requestHandler,
    TOutgoingRequestsPtr outgoing
)
    : TSession(requestHandler, outgoing)
    , Socket(ioService, context)
{
}

THTTPSSession::~THTTPSSession() {
}

boost::asio::ip::tcp::socket::lowest_layer_type &THTTPSSession::GetSocket() {
    return Socket.lowest_layer();
}

void THTTPSSession::Accept(TSessionPtr This) {
    Socket.async_handshake(boost::asio::ssl::stream_base::server,
        [this, This] (const boost::system::error_code &error) {
            HandleHandshake(This, error);
        }
    );
}

void THTTPSSession::Connect(TSessionPtr This, TEndPointIterator endpoint_iterator) {
    Socket.set_verify_mode(boost::asio::ssl::verify_peer);
    Socket.set_verify_callback(
        [](bool preverified, boost::asio::ssl::verify_context &ctx) {
            // TODO: verify peer
            return preverified;
        }
    );
    boost::asio::async_connect(Socket.lowest_layer(), endpoint_iterator,
        [this, This](const boost::system::error_code &error, boost::asio::ip::tcp::resolver::iterator endpoint_iterator) {
            HandleConnect(This, error);
        }
    );
}

void THTTPSSession::StartReading(TSessionPtr This) {
    Socket.async_read_some(boost::asio::buffer(GetData(), GetDataSize()),
        [this, This] (const boost::system::error_code &error, size_t bytesTransferred) {
            HandleRead(This, error, bytesTransferred);
        }
    );
}

void THTTPSSession::HandleConnect(TSessionPtr This, const boost::system::error_code &error) {
    if (!error) {
        Socket.async_handshake(boost::asio::ssl::stream_base::client,
            [this, This] (const boost::system::error_code &error) {
                HandleHandshake(This, error);
            }
        );
    } else {
        std::cout << "Error in connect: " << error.message() << std::endl;
    }
}

void THTTPSSession::HandleHandshake(TSessionPtr This, const boost::system::error_code &error) {
    if (!error) {
        StartIO(This);
    } else {
        std::cout << "Error in handshake: " << error.message() << std::endl;
    }
}

void THTTPSSession::StartWriting(TSessionPtr This) {
    boost::asio::async_write(Socket, boost::asio::buffer(GetOutgoing()->GetFirstToSend()),
        [this, This] (const boost::system::error_code &error, size_t bytesTransferred) {
            HandleWrite(This, error, bytesTransferred);
        }
    );
}

TSessionPtr Connect(const std::string &host, int port, boost::asio::io_service &iOService, boost::asio::ssl::context &sSLContext, THTTPRequestHandlerPtr handler, TOutgoingRequestsPtr outgoingRequests) {
    boost::asio::ip::tcp::resolver resolver(iOService);
    boost::asio::ip::tcp::resolver::query query(host, boost::lexical_cast<std::string>(port));
    boost::asio::ip::tcp::resolver::iterator iterator = resolver.resolve(query);
    TSessionPtr sess(new THTTPSSession(iOService, sSLContext, handler, outgoingRequests));
    sess->Connect(sess, iterator);
    return sess;
}

