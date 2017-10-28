#include "utf.h"
#include <boost/locale.hpp>
#include <boost/algorithm/string.hpp>


//
// TLocale
//
TLocale::TLocale(const std::locale &loc)
    : Locale(loc)
{
}

std::wstring TLocale::ToWide(const std::string &text) {
    return boost::locale::conv::utf_to_utf<wchar_t>(text);
}

std::string TLocale::FromWide(const std::wstring &text) {
    return boost::locale::conv::utf_to_utf<char>(text);
}

std::wstring TLocale::ToLower(const std::wstring &text) {
    std::wstring res(text);
    boost::algorithm::to_lower(res, Locale);
    return res;
}

std::string TLocale::ToLower(const std::string &text) {
    return FromWide(ToLower(ToWide(text)));
}

std::vector<std::wstring> TLocale::ExtractWords(const std::wstring &text) {
    std::vector<std::wstring> res;
    const auto &ct = std::use_facet<std::ctype<wchar_t>>(Locale);
    std::wstring word;
    for (wchar_t ch : text) {
        if (ct.is(std::ctype<wchar_t>::alnum, ch)) {
            word += ch;
        } else {
            if (!word.empty()) {
                res.push_back(word);
                word.clear();
            }
        }
    }
    if (!word.empty())
        res.push_back(word);
    return res;
}

std::vector<std::string> TLocale::ExtractWords(const std::string &text) {
    std::vector<std::string> res;
    for (const std::wstring &word : ExtractWords(ToWide(text)))
        res.push_back(FromWide(word));
    return res;
}


//
// TLocales
//
TLocales &TLocales::Instance() {
    static TLocales Object;
    return Object;
}

TLocale &TLocales::GetLocale(const std::string &name) {
    auto it = Locales.find(name);
    if (it == Locales.end()) {
        it = Locales.insert(std::make_pair(name, TLocale(std::locale(name.c_str())))).first;
    }
    return it->second;
}

