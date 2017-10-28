#include "http_request.h"


//
// THTTPRequest
//

THTTPRequest::THTTPRequest(std::string &&startingLine, THTTPRequest::THTTPHeaders &&headers, std::vector<char> &&body)
    : StartingLine(startingLine)
    , Headers(std::move(headers))
    , Body(std::move(body))
{}

THTTPRequest::THTTPRequest(std::string &&startingLine, THTTPRequest::THTTPHeaders &&headers, const std::string &body)
    : StartingLine(startingLine)
    , Headers(std::move(headers))
{
    Body.insert(Body.end(), body.begin(), body.end());
}

const std::string &THTTPRequest::GetStartingLine() const {
    return StartingLine;
}

const THTTPRequest::THTTPHeaders &THTTPRequest::GetHeaders() const {
    return Headers;
}

const std::vector<char> &THTTPRequest::GetBody() const {
    return Body;
}

std::string THTTPRequest::GetBodyStr() const {
    return std::string(Body.begin(), Body.end());
}


//
// THTTPRequestHandler
//
void THTTPRequestHandler::ProcessRequest(TSessionPtr session, THTTPRequestPtr req) {
}

void THTTPRequestHandler::OnSessionDestroyed() {
}


//
// THTTPRequestBuilder::TReadingStartingLine
//

void THTTPRequestBuilder::TReadingStartingLine::Init() {
    Line.clear();
}

THTTPRequestBuilder::TState *THTTPRequestBuilder::TReadingStartingLine::OnSymbol(
    char ch,
    THTTPRequestBuilder &bldr
) {
    if (ch == '\r')
        return this;
    if (ch == '\n' || ch == 0) {
        if (Line.empty())
            throw std::runtime_error("Empty starting line");
        TReadingHeaderName *hn = bldr.GetReadingHeaderNameState();
        bldr.OnStartingLine(std::move(Line));
        hn->Init();
        return hn;
    }
    Line += ch;
    return this;
}

//
// THTTPRequestBuilder::TReadingHeaderName
//

void THTTPRequestBuilder::TReadingHeaderName::Init() {
    Name.clear();
}

THTTPRequestBuilder::TState *THTTPRequestBuilder::TReadingHeaderName::OnSymbol(
    char ch,
    THTTPRequestBuilder &bldr
) {
    if (ch == '\r')
        return this;
    if (ch == '\n') {
        if (!Name.empty())
            throw std::runtime_error("Corrupted http header: header without value");
        int contentLength = bldr.GetContentLength();
        if (contentLength != -1) {
            TReadingBodyContentLength *rb = bldr.GetReadingBodyContentLengthState();
            rb->Init(static_cast<size_t>(contentLength));
            return rb;
        }
        std::string transferEncoding = bldr.GetTransferEncoding();
        if (!transferEncoding.empty()) {
            if (transferEncoding != "chunked")
                throw std::runtime_error(std::string("Unsupported transfer encoding: ") + transferEncoding);
            TReadingBodyChunked *rb = bldr.GetReadingBodyChunkedState();
            rb->Init();
            return rb;
        }
        if (bldr.GetStartingLine().substr(0, 3) == "POST") {
            TReadingBodyEmptyLine *rb = bldr.GetReadingBodyEmptyLineState();
            rb->Init();
            return rb;
        } else {
            bldr.YieldRequest(std::vector<char>());
            TReadingStartingLine *sl = bldr.GetReadingStartingLineState();
            sl->Init();
            return sl;
        }
    }
    if (ch == ':') {
        if (Name.empty())
            throw std::runtime_error("Corrupted http header: empty name");
        TReadingSpacesBeforeHeaderValue *sp = bldr.GetReadingSpacesBeforeHeaderValueState();
        sp->Init(std::move(Name));
        return sp;
    }
    if (ch == 0)
        throw std::runtime_error("Corrupted http header: unexpected end of stream");
    Name += ch;
    return this;
}

//
// THTTPRequestBuilder::TReadingSpacesBeforeHeaderValue
//

void THTTPRequestBuilder::TReadingSpacesBeforeHeaderValue::Init(std::string &&name) {
    Name.swap(name);
}

THTTPRequestBuilder::TState *THTTPRequestBuilder::TReadingSpacesBeforeHeaderValue::OnSymbol(
    char ch,
    THTTPRequestBuilder &bldr
) {
    if (ch == '\r' || ch == '\n')
        throw std::runtime_error("Corrupted http header: unexpected end of line");
    if (ch == ' ' || ch == '\t')
        return this;
    if (ch == 0)
        throw std::runtime_error("Corrupted http header: unexpected end of stream");
    TReadingHeaderValue *rv = bldr.GetReadingHeaderValueState();
    rv->Init(std::move(Name));
    return rv->OnSymbol(ch, bldr);
}

//
// THTTPRequestBuilder::TReadingHeaderValue
//

void THTTPRequestBuilder::TReadingHeaderValue::Init(std::string &&name) {
    Name.swap(name);
    Value.clear();
}

THTTPRequestBuilder::TState *THTTPRequestBuilder::TReadingHeaderValue::OnSymbol(
    char ch,
    THTTPRequestBuilder &bldr
) {
    if (ch == '\r')
        return this;
    if (ch == '\n') {
        bldr.AddHeader(std::move(Name), std::move(Value));
        TReadingHeaderName *hn = bldr.GetReadingHeaderNameState();
        hn->Init();
        return hn;
    }
    Value += ch;
    return this;
}

//
// THTTPRequestBuilder::TReadingBodyContentLength
//

void THTTPRequestBuilder::TReadingBodyContentLength::Init(size_t contentLength) {
    Data.clear();
    ContentLength = contentLength;
    Data.reserve(ContentLength);
}

THTTPRequestBuilder::TState *THTTPRequestBuilder::TReadingBodyContentLength::OnSymbol(
    char ch,
    THTTPRequestBuilder &bldr
) {
    Data.push_back(ch);
    if (Data.size() >= ContentLength) {
        bldr.YieldRequest(std::move(Data));
        TReadingStartingLine *sl = bldr.GetReadingStartingLineState();
        sl->Init();
        return sl;
    }
    return this;
}

//
// THTTPRequestBuilder::TReadingBodyChunked
//
void THTTPRequestBuilder::TReadingBodyChunked::Init() {
    SubState = ESS_READING_LENGTH;
    ChunkLength = ChunkRead = 0;
    Data.clear();
}

THTTPRequestBuilder::TState *THTTPRequestBuilder::TReadingBodyChunked::OnSymbol(
    char ch,
    THTTPRequestBuilder &bldr
) {
    if (SubState == ESS_READING_LENGTH) {
        if (ch == '\r') {
            SubState = ESS_READING_SLASH_N_AFTER_LENGTH;
        } else {
            ChunkLength *= 16;
            if ('0' <= ch && ch <= '9')
                ChunkLength += (ch - '0');
            else if ('a' <= ch && ch <= 'f')
                ChunkLength += (ch - 'a' + 10);
            else if ('A' <= ch && ch <= 'F')
                ChunkLength += (ch - 'A' + 10);
            else
                throw std::runtime_error(std::string("Invalid symbol in chunk length: ") + ch);
        }
    } else if (SubState == ESS_READING_SLASH_N_AFTER_LENGTH) {
        if (ch != '\n')
            throw std::runtime_error(std::string("Invalid symbol, expected \\n, got: ") + ch);
        if (ChunkLength != 0) {
            SubState = ESS_READING_DATA;
            Data.reserve(Data.size() + ChunkLength);
            ChunkRead = 0;
        } else {
            SubState = ESS_READING_SLASH_R_AFTER_BODY;
        }
    } else if (SubState == ESS_READING_DATA) {
        Data.push_back(ch);
        if (++ChunkRead == ChunkLength) {
            SubState = ESS_READING_SLASH_R_AFTER_DATA;
        }
    } else if (SubState == ESS_READING_SLASH_R_AFTER_DATA) {
        if (ch != '\r')
            throw std::runtime_error(std::string("Invalid symbol, expected \\r, got: ") + ch);
        SubState = ESS_READING_SLASH_N_AFTER_DATA;
    } else if (SubState == ESS_READING_SLASH_N_AFTER_DATA) {
        if (ch != '\n')
            throw std::runtime_error(std::string("Invalid symbol, expected \\n, got: ") + ch);
        SubState = ESS_READING_LENGTH;
        ChunkLength = ChunkRead = 0;
    } else if (SubState == ESS_READING_SLASH_R_AFTER_BODY) {
        if (ch != '\r')
            throw std::runtime_error(std::string("Invalid symbol, expected \\r, got: ") + ch);
        SubState = ESS_READING_SLASH_N_AFTER_BODY;
    } else if (SubState == ESS_READING_SLASH_N_AFTER_BODY) {
        if (ch != '\n')
            throw std::runtime_error(std::string("Invalid symbol, expected \\n, got: ") + ch);
        bldr.YieldRequest(std::move(Data));
        TReadingStartingLine *sl = bldr.GetReadingStartingLineState();
        sl->Init();
        return sl;
    }
    return this;
}

//
// THTTPRequestBuilder::TReadingBodyEmptyLine
//
void THTTPRequestBuilder::TReadingBodyEmptyLine::Init() {
    Data.clear();
}

THTTPRequestBuilder::TReadingBodyEmptyLine::TState *THTTPRequestBuilder::TReadingBodyEmptyLine::OnSymbol(
    char ch,
    THTTPRequestBuilder &bldr
) {
    Data.push_back(ch);
    if (IsEmptyLine()) {
        bldr.YieldRequest(std::move(Data));
        TReadingStartingLine *sl = bldr.GetReadingStartingLineState();
        sl->Init();
        return sl;
    }
    return this;
}

bool THTTPRequestBuilder::TReadingBodyEmptyLine::IsEmptyLine() const {
    if (Data.size() == 2 && Data[0] == '\r' && Data[1] == '\n')
        return true;
    if (Data.size() >= 4) {
        auto it = Data.end() - 4;
        if (it[0] == it[2] && it[0] == '\r' && it[1] == it[3] && it[1] == '\n')
            return true;
    }
    return false;
}

//
// THTTPRequestBuilder
//

THTTPRequestBuilder::THTTPRequestBuilder(const THandler &handler)
    : Handler(handler)
    , CurrentState(&ReadingStartingLineState)
{}

void THTTPRequestBuilder::OnSymbol(char ch) {
    CurrentState = CurrentState->OnSymbol(ch, *this);
}

void THTTPRequestBuilder::OnStartingLine(std::string &&line) {
    StartingLine.swap(line);
}

const std::string &THTTPRequestBuilder::GetStartingLine() const {
    return StartingLine;
}

void THTTPRequestBuilder::AddHeader(std::string &&name, std::string &&value) {
    Headers[name] = value;
}

void THTTPRequestBuilder::YieldRequest(std::vector<char> &&data) {
    THTTPRequestPtr req(new THTTPRequest(std::move(StartingLine), std::move(Headers), std::move(data)));
    Handler(req);
}

int THTTPRequestBuilder::GetContentLength() const {
    auto cl = Headers.find("Content-Length");
    return cl != Headers.end() ? boost::lexical_cast<int>(cl->second) : -1;
}

std::string THTTPRequestBuilder::GetTransferEncoding() const {
    auto te = Headers.find("Transfer-Encoding");
    return te != Headers.end() ? te->second : "";
}

THTTPRequestBuilder::TReadingStartingLine *THTTPRequestBuilder::GetReadingStartingLineState() {
    return &ReadingStartingLineState;
}

THTTPRequestBuilder::TReadingHeaderName *THTTPRequestBuilder::GetReadingHeaderNameState() {
    return &ReadingHeaderNameState;
}

THTTPRequestBuilder::TReadingBodyContentLength *THTTPRequestBuilder::GetReadingBodyContentLengthState() {
    return &ReadingBodyContentLengthState;
}

THTTPRequestBuilder::TReadingBodyChunked *THTTPRequestBuilder::GetReadingBodyChunkedState() {
    return &ReadingBodyChunkedState;
}

THTTPRequestBuilder::TReadingBodyEmptyLine *THTTPRequestBuilder::GetReadingBodyEmptyLineState() {
    return &ReadingBodyEmptyLineState;
}

THTTPRequestBuilder::TReadingSpacesBeforeHeaderValue *THTTPRequestBuilder::GetReadingSpacesBeforeHeaderValueState() {
    return &ReadingSpacesBeforeHeaderValueState;
}

THTTPRequestBuilder::TReadingHeaderValue *THTTPRequestBuilder::GetReadingHeaderValueState() {
    return &ReadingHeaderValueState;
}


void SplitStartingLine(const std::string &line, std::string &method, std::string &url, std::string &protocol) {
    std::istringstream str(line);
    str >> method >> url >> protocol;
}

