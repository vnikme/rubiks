#pragma once

#include <map>
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <boost/lexical_cast.hpp>
#include <boost/noncopyable.hpp>

using THTTPHeaders = std::map<std::string, std::string>;

class THTTPRequest {
public:
    using THTTPHeaders = std::map<std::string, std::string>;

    THTTPRequest(std::string &&startingLine, THTTPHeaders &&headers, std::vector<char> &&body);
    THTTPRequest(std::string &&startingLine, THTTPHeaders &&headers, const std::string &body);

    const std::string &GetStartingLine() const;
    const THTTPHeaders &GetHeaders() const;
    const std::vector<char> &GetBody() const;
    std::string GetBodyStr() const;

private:
    std::string StartingLine;
    THTTPHeaders Headers;
    std::vector<char> Body;
};

using THTTPRequestPtr = std::shared_ptr<THTTPRequest>;


class TSession;
using TSessionPtr = std::shared_ptr<TSession>;
using TSessionWeakPtr = std::weak_ptr<TSession>;


class THTTPRequestHandler : private boost::noncopyable {
public:
    THTTPRequestHandler() = default;
    virtual ~THTTPRequestHandler() = default;
    virtual void ProcessRequest(TSessionPtr session, THTTPRequestPtr req);
    virtual void OnSessionDestroyed();
};

using THTTPRequestHandlerPtr = std::shared_ptr<THTTPRequestHandler>;

class THTTPRequestBuilder {
public:
    class TState {
    public:
        virtual TState *OnSymbol(char ch, THTTPRequestBuilder &bldr) = 0;
    protected:
        ~TState() = default;
    };

    using THandler = std::function<void (THTTPRequestPtr)>;

    explicit THTTPRequestBuilder(const THandler &handler);

    void OnSymbol(char ch);
    void OnStartingLine(std::string &&line);
    const std::string &GetStartingLine() const;
    void AddHeader(std::string &&name, std::string &&value);
    void YieldRequest(std::vector<char> &&data);
    int GetContentLength() const;
    std::string GetTransferEncoding() const;

private:
    class TReadingStartingLine : public TState {
    public:
        void Init();
        TState *OnSymbol(char ch, THTTPRequestBuilder &bldr) override;
    private:
        std::string Line;
    };

    class TReadingHeaderName : public TState {
    public:
        void Init();
        TState *OnSymbol(char ch, THTTPRequestBuilder &bldr) override;
    private:
        std::string Name;
    };

    class TReadingSpacesBeforeHeaderValue : public TState {
    public:
        void Init(std::string &&name);
        TState *OnSymbol(char ch, THTTPRequestBuilder &bldr) override;
    private:
        std::string Name;
    };

    class TReadingHeaderValue : public TState {
    public:
        void Init(std::string &&name);
        TState *OnSymbol(char ch, THTTPRequestBuilder &bldr) override;
    private:
        std::string Name;
        std::string Value;
    };

    class TReadingBodyContentLength : public TState {
    public:
        void Init(size_t contentLength);
        TState *OnSymbol(char ch, THTTPRequestBuilder &bldr) override;
    private:
        std::vector<char> Data;
        size_t ContentLength = 0;
    };

    class TReadingBodyChunked : public TState {
    public:
        void Init();
        TState *OnSymbol(char ch, THTTPRequestBuilder &bldr) override;
    private:
        enum ESubState {
            ESS_READING_LENGTH,
            ESS_READING_SLASH_N_AFTER_LENGTH,
            ESS_READING_DATA,
            ESS_READING_SLASH_R_AFTER_DATA,
            ESS_READING_SLASH_N_AFTER_DATA,
            ESS_READING_SLASH_R_AFTER_BODY,
            ESS_READING_SLASH_N_AFTER_BODY
        };
        std::vector<char> Data;
        ESubState SubState;
        size_t ChunkLength = 0;
        size_t ChunkRead = 0;
    };

    class TReadingBodyEmptyLine : public TState {
    public:
        void Init();
        TState *OnSymbol(char ch, THTTPRequestBuilder &bldr) override;
    private:
        std::vector<char> Data;
        bool IsEmptyLine() const;
    };

    std::string StartingLine;
    THTTPHeaders Headers;
    THandler Handler;
    TReadingStartingLine ReadingStartingLineState;
    TReadingHeaderName ReadingHeaderNameState;
    TReadingBodyContentLength ReadingBodyContentLengthState;
    TReadingBodyChunked ReadingBodyChunkedState;
    TReadingBodyEmptyLine ReadingBodyEmptyLineState;
    TReadingSpacesBeforeHeaderValue ReadingSpacesBeforeHeaderValueState;
    TReadingHeaderValue ReadingHeaderValueState;
    TState *CurrentState;

private:
    TReadingStartingLine *GetReadingStartingLineState();
    TReadingHeaderName *GetReadingHeaderNameState();
    TReadingBodyContentLength *GetReadingBodyContentLengthState();
    TReadingBodyChunked *GetReadingBodyChunkedState();
    TReadingBodyEmptyLine *GetReadingBodyEmptyLineState();
    TReadingSpacesBeforeHeaderValue *GetReadingSpacesBeforeHeaderValueState();
    TReadingHeaderValue *GetReadingHeaderValueState();
};

void SplitStartingLine(const std::string &line, std::string &method, std::string &url, std::string &protocol);

