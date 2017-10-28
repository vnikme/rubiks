#pragma once

#include <sstream>
#include <list>
#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>
#include <mutex>
#include "http_request.h"
#include "../dist/json/json.h"
#include "../util/synchronizable.h"


class THTTPReplyHandler : private boost::noncopyable {
public:
    THTTPReplyHandler() = default;
    virtual ~THTTPReplyHandler() = default;
    virtual void ProcessReply(TSessionPtr session, THTTPRequestPtr req) = 0;
};

using THTTPReplyHandlerPtr = std::shared_ptr<THTTPReplyHandler>;


struct TOutgoingRequest {
    std::string Request;
    THTTPReplyHandlerPtr Handler;
    TOutgoingRequest(std::string request, THTTPReplyHandlerPtr handler)
        : Request(std::move(request))
        , Handler(handler)
    {}
};


class TOutgoingRequests : public TSynchronizable, private boost::noncopyable {
    private:
        std::list<TOutgoingRequest> Outgoing;
        std::unique_ptr<std::list<TOutgoingRequest>> Sent;

    public:
        TOutgoingRequests(bool initSent);
        bool IsEmpty() const;
        bool IsEmptyIncludingSent() const;
        bool HasSent() const;                       // true if Sent.get() != nullptr // all sent requests move to Sent
        void AddRequest(std::string request, THTTPReplyHandlerPtr handler);
        std::string &GetFirstToSend();
        void OnSendingFinished();
        THTTPReplyHandlerPtr PopReplyHandler();
        void ResetAllSentRequests();
};

using TOutgoingRequestsPtr = std::shared_ptr<TOutgoingRequests>;


class TSession : private boost::noncopyable {
public:
    using TEndPointIterator = boost::asio::ip::tcp::resolver::iterator;

public:
    TSession(
        THTTPRequestHandlerPtr requestHandler,
        TOutgoingRequestsPtr outgoing
    );

    virtual ~TSession();

    virtual boost::asio::ip::tcp::socket::lowest_layer_type &GetSocket() = 0;

    virtual void Accept(TSessionPtr This) = 0;
    virtual void Connect(TSessionPtr This, TEndPointIterator endpoint_iterator) = 0;
    void AddOutgoingRequest(
        TSessionPtr This,
        const THTTPRequest &response,
        THTTPReplyHandlerPtr replyHandler
    );

protected:
    TOutgoingRequestsPtr GetOutgoing();
    char *GetData();
    size_t GetDataSize() const;

    void StartIO(TSessionPtr This);
    void HandleRead(TSessionPtr This, const boost::system::error_code &error, size_t bytesTransferred);
    void HandleReadUnsafe(TSessionPtr This, const boost::system::error_code &error, size_t bytesTransferred);
    void HandleWrite(TSessionPtr This, const boost::system::error_code& error, size_t bytesTransferred);
    void HandleHTTP(THTTPRequestPtr req);
    void ProcessHTTPRequest(TSessionPtr This, THTTPRequestPtr req);

private:
    bool Connected = false;
    static constexpr size_t MAX_LENGTH = 65536;
    char Data[MAX_LENGTH];
    THTTPRequestBuilder ReadHandler;
    std::list<THTTPRequestPtr> IncomingRequests;
    THTTPRequestHandlerPtr RequestHandler;
    TOutgoingRequestsPtr Outgoing;

private:
    virtual void StartReading(TSessionPtr This) = 0;
    virtual void StartWriting(TSessionPtr This) = 0;            // Mutex should be locked
};

