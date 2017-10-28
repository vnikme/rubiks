#pragma once

#include <map>
#include <string>
#include <locale>
#include <vector>


class TLocale {
    private:
        std::locale Locale;

    public:
        explicit TLocale(const std::locale &loc);
        std::wstring ToWide(const std::string &text);
        std::string FromWide(const std::wstring &text);
        std::wstring ToLower(const std::wstring &text);
        std::string ToLower(const std::string &text);
        std::vector<std::wstring> ExtractWords(const std::wstring &text);
        std::vector<std::string> ExtractWords(const std::string &text);
};

class TLocales {
    private:
        std::map<std::string, TLocale> Locales;

    public:
        static TLocales &Instance();
        TLocale &GetLocale(const std::string &name);
};

