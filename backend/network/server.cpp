#include "server.h"
#include "session_http.h"
#include "session_https.h"
#include <chrono>


//
// TServer
//

TServer::TListenerInfo::TListenerInfo(boost::asio::io_service &ioService, bool isPublic, unsigned short port, THTTPRequestHandlerPtr handler)
    : Acceptor(ioService, boost::asio::ip::tcp::endpoint(isPublic ? boost::asio::ip::address_v4::any() : boost::asio::ip::address_v4::loopback(), port))
    , MessageHandler(handler)
{
}

TServer::TServer()
    : ClientSSLContext(boost::asio::ssl::context::sslv23)
{
    ClientSSLContext.set_default_verify_paths();
}

TServer::~TServer() {
}

boost::asio::io_service &TServer::GetIOService() {
    return IOService;
}

boost::asio::ssl::context &TServer::GetClientSSLContext() {
    return ClientSSLContext;
}

void TServer::AddHTTPService(bool isPublic, unsigned short port, THTTPRequestHandlerPtr handler) {
    auto listener = std::make_shared<TListenerInfo>(GetIOService(), isPublic, port, handler);
    Services.push_back(listener);
}

void TServer::AddHTTPSService(bool isPublic, unsigned short port,
                              THTTPRequestHandlerPtr handler,
                              const std::string &fullChainPath,
                              const std::string &privKeyPath,
                              const std::string &dhPath)
{
    auto listener = std::make_shared<TListenerInfo>(GetIOService(), isPublic, port, handler);
    listener->SSLContext.reset(new boost::asio::ssl::context(boost::asio::ssl::context::sslv23));
    listener->SSLContext->set_options(boost::asio::ssl::context::default_workarounds | boost::asio::ssl::context::no_sslv2 | boost::asio::ssl::context::single_dh_use);
    listener->SSLContext->set_password_callback([] (size_t maxLength, boost::asio::ssl::context::password_purpose purpose) -> std::string { return ""; });
    listener->SSLContext->use_certificate_chain_file(fullChainPath);
    listener->SSLContext->use_private_key_file(privKeyPath, boost::asio::ssl::context::pem);
    listener->SSLContext->use_tmp_dh_file(dhPath);
    Services.push_back(listener);
}

void TServer::Run() {
    for (; ;) {
        try {
            {
                std::unique_lock<std::mutex> lk(Mutex);
                if (Exit) {
                    std::cout << "Server is about to stop." << std::endl;
                    break;
                }
            }
            for (; !Services.empty(); ) {
                auto listener = Services.front();
                Services.pop_front();
                StartAccept(listener);
            }
            GetIOService().run();
        } catch (const std::exception &ex) {
            std::cerr << "Exception in TServer::Run: " << ex.what() << std::endl;
        } catch (...) {
            std::cerr << "Exception in TServer::Run" << std::endl;
        }
    }
}

void TServer::Stop() {
    std::unique_lock<std::mutex> lk(Mutex);
    Exit = true;
    GetIOService().stop();
}

void TServer::StartAccept(TListenerInfoPtr listener) {
    TSessionPtr newSession;
    if (listener->SSLContext.get() == nullptr)
        newSession.reset(new THTTPSession(GetIOService(), listener->MessageHandler, TOutgoingRequestsPtr()));
    else
        newSession.reset(new THTTPSSession(GetIOService(), *listener->SSLContext, listener->MessageHandler, TOutgoingRequestsPtr()));
    listener->Acceptor.async_accept(newSession->GetSocket(), [listener, newSession, this] (const boost::system::error_code &error) {
        HandleAccept(listener, newSession, error);
    });
}

void TServer::HandleAccept(TListenerInfoPtr listener, TSessionPtr newSession, const boost::system::error_code &error) {
    if (!error) {
        newSession->Accept(newSession);
    } else {
    }
    StartAccept(listener);
}

