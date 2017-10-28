#pragma once

#include <string>
#include <vector>

std::string UrlEncode(const std::string &s);
std::string UrlDecode(const std::string &s);
using TUrlCgiParams = std::vector<std::pair<std::string, std::string>>;
void ParseUrlResource(const std::string &url, std::string &resource, TUrlCgiParams &params);
std::string ConstructUrl(const std::string &resource, const TUrlCgiParams &params);

