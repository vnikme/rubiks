#pragma once

#include "../dist/json/json-forwards.h"
#include <string>

Json::Value ParseJson(const std::string &json);
std::string SaveJson(const Json::Value &root);

