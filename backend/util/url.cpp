#include "url.h"
#include <cctype>
#include <utility>


std::string UrlEncode(const std::string &str) {
    std::vector<char> res;
    res.reserve(str.size() * 3);
    for (unsigned char c : str) {
        if (c == ' ') {
            res.push_back('+');
        } else if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            res.push_back(static_cast<char>(c));
        } else {
            res.push_back('%');
            int v = c >> 4;
            if (v < 10)
                res.push_back(v + '0');
            else
                res.push_back(v - 10 + 'A');
            v = c & 0xF;
            if (v < 10)
                res.push_back(v + '0');
            else
                res.push_back(v - 10 + 'A');
        }
    }
    return std::string(res.begin(), res.end());
 }

std::string UrlDecode(const std::string &str) {
    std::vector<char> res;
    res.reserve(str.size());
    for (std::string::const_iterator it = str.begin(), end = str.end(); it != end; ) {
        char ch = *it;
        if (ch != '%') {
            res.push_back(ch != '+' ? ch : ' ');
            ++it;
        } else {
            int v = 0;
            for (int i = 0; i < 2; ++i) {
                v *= 16;
                if (++it == end)
                    return "";
                ch = *it;
                if ('0' <= ch && ch <= '9')
                    v += (ch - '0');
                else if ('A' <= ch && ch <= 'F')
                    v += (ch - 'A' + 10);
                else
                    return "";
            }
            ++it;
            res.push_back(static_cast<char>(v));
        }
    }
    return std::string(res.begin(), res.end());
}

static void SplitCgiParam(const std::string &param, std::string &name, std::string &value) {
    size_t p = param.find('=');
    if (p == std::string::npos) {
        name = param;
    } else {
        name = param.substr(0, p);
        value = param.substr(p + 1, std::string::npos);
    }
}

void ParseUrlResource(const std::string &url, std::string &resource, TUrlCgiParams &params) {
    size_t p = url.find('?');
    resource = url.substr(0, p);
    for (; p != std::string::npos; ) {
        size_t n = url.find('&', p + 1);
        std::string name, value;
        SplitCgiParam(url.substr(p + 1, n - p - 1), name, value);
        params.push_back(std::make_pair(UrlDecode(name), UrlDecode(value)));
        p = n;
    }
}

std::string ConstructUrl(const std::string &resource, const TUrlCgiParams &params) {
    std::string url = resource;
    bool first = true;
    for (const auto &item : params) {
        if (first) {
            url += '?';
            first = false;
        } else {
            url += '&';
        }
        url += UrlEncode(item.first);
        if (!item.second.empty()) {
            url += '=';
            url += UrlEncode(item.second);
        }
    }
    return url;
}

