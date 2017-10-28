#include "session.h"
#include <iostream>


//
// TOutgoingRequests
//

TOutgoingRequests::TOutgoingRequests(bool initSent) {
    if (initSent)
        Sent.reset(new std::list<TOutgoingRequest>);
}

bool TOutgoingRequests::IsEmpty() const {
    std::unique_lock<const TOutgoingRequests> lk(*this);
    return Outgoing.empty();
}

bool TOutgoingRequests::IsEmptyIncludingSent() const {
    std::unique_lock<const TOutgoingRequests> lk(*this);
    return Outgoing.empty() && (Sent.get() == nullptr || Sent->empty());
}

bool TOutgoingRequests::HasSent() const {
    std::unique_lock<const TOutgoingRequests> lk(*this);
    return (Sent.get() != nullptr);
}

void TOutgoingRequests::AddRequest(std::string request, THTTPReplyHandlerPtr handler) {
    std::unique_lock<const TOutgoingRequests> lk(*this);
    Outgoing.emplace_back(std::move(request), handler);
}

std::string &TOutgoingRequests::GetFirstToSend() {
    std::unique_lock<const TOutgoingRequests> lk(*this);
    return Outgoing.front().Request;
}

void TOutgoingRequests::OnSendingFinished() {
    std::unique_lock<const TOutgoingRequests> lk(*this);
    if (Sent.get() != nullptr)
        Sent->push_back(Outgoing.front());
    Outgoing.pop_front();
}

THTTPReplyHandlerPtr TOutgoingRequests::PopReplyHandler() {
    std::unique_lock<const TOutgoingRequests> lk(*this);
    THTTPReplyHandlerPtr handler(Sent->front().Handler);
    Sent->pop_front();
    return handler;
}

void TOutgoingRequests::ResetAllSentRequests() {
    std::unique_lock<const TOutgoingRequests> lk(*this);
    if (Sent.get() == nullptr)
        return;
    std::cout << "Moving " << Sent->size() << " messages to Outgoing" << std::endl;
    while (!Sent->empty()) {
        Outgoing.push_front(Sent->back());
        Sent->pop_back();
    }
}


//
// TSession
//

TSession::TSession(
    THTTPRequestHandlerPtr requestHandler,
    TOutgoingRequestsPtr outgoing
)
    : ReadHandler([this](THTTPRequestPtr req) { HandleHTTP(req); })
    , RequestHandler(requestHandler)
    , Outgoing(outgoing)
{
    if (!Outgoing)
        Outgoing.reset(new TOutgoingRequests(false));
    else
        Outgoing->ResetAllSentRequests();
}

TSession::~TSession() {
    if (RequestHandler.get())
        RequestHandler->OnSessionDestroyed();
}

void TSession::StartIO(TSessionPtr This) {
    {
        std::unique_lock<const TOutgoingRequests> lk(*Outgoing);
        Connected = true;
        if (!Outgoing->IsEmpty())
            StartWriting(This);
    }
    StartReading(This);
}

void TSession::AddOutgoingRequest(TSessionPtr This, const THTTPRequest &response, THTTPReplyHandlerPtr replyHandler) {
    std::ostringstream answer;
    answer << response.GetStartingLine() << "\r\n";
    for (const auto &it : response.GetHeaders()) {
        answer << it.first << ": " << it.second << "\r\n";
    }
    answer << "\r\n" << response.GetBodyStr();
    std::unique_lock<const TOutgoingRequests> lk(*Outgoing);
    bool writingInProgress = !Outgoing->IsEmpty();
    Outgoing->AddRequest(std::move(answer.str()), replyHandler);
    if (!writingInProgress && Connected)
        StartWriting(This);
}

TOutgoingRequestsPtr TSession::GetOutgoing() {
    return Outgoing;
}

char *TSession::GetData() {
    return Data;
}

size_t TSession::GetDataSize() const {
    return MAX_LENGTH;
}

void TSession::HandleRead(TSessionPtr This, const boost::system::error_code &error, size_t bytesTransferred) {
    try {
        HandleReadUnsafe(This, error, bytesTransferred);
        while (!IncomingRequests.empty()) {
            THTTPRequestPtr req = IncomingRequests.front();
            IncomingRequests.pop_front();
            ProcessHTTPRequest(This, req);
        }
    } catch (...) {
    }
}

void TSession::HandleReadUnsafe(TSessionPtr This, const boost::system::error_code &error, size_t bytesTransferred) {
    if (!error) {
        for (int i = 0; i < bytesTransferred; ++i) {
            ReadHandler.OnSymbol(Data[i]);
        }
        StartReading(This);
    } else {
        ReadHandler.OnSymbol(0);
    }
}

void TSession::HandleHTTP(THTTPRequestPtr req) {
    IncomingRequests.push_back(req);
}

void TSession::ProcessHTTPRequest(TSessionPtr This, THTTPRequestPtr req) {
    if (Outgoing->HasSent()) {
        std::unique_lock<const TOutgoingRequests> lk(*Outgoing);
        THTTPReplyHandlerPtr replyHandler = Outgoing->PopReplyHandler();
        lk.unlock();
        if (replyHandler.get()) {
            replyHandler->ProcessReply(This, req);
        }
    } else {
        if (RequestHandler.get()) {
            RequestHandler->ProcessRequest(This, req);
        }
    }
}

void TSession::HandleWrite(TSessionPtr This, const boost::system::error_code& error, size_t bytesTransferred) {
    std::unique_lock<const TOutgoingRequests> lk(*Outgoing);
    Outgoing->OnSendingFinished();
    if (!error) {
        if (!Outgoing->IsEmpty())
            StartWriting(This);
    } else {
    }
}

