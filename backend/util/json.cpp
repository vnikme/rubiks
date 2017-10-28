
#include "../dist/json/json.h"
#include "json.h"
#include <iostream>


Json::Value ParseJson(const std::string &json) {
    Json::Value root;
    Json::Reader reader;
    std::istringstream str(json);
    if (!reader.parse(str, root, false)) {
        std::cerr << json << std::endl;
        throw std::runtime_error("Could not parse json");
    }
    return root;
}

std::string SaveJson(const Json::Value &root) {
    Json::StyledWriter writer;
    return writer.write(root);
}

