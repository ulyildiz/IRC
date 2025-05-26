#include "../include/IRC_Protocol.hpp"
#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <map>

IRCMessage IRC_Protocol::parser(const std::string& rawData) {
    IRCMessage msg;
    std::string data = rawData;

    // C++98'de std::string::back() olmadığından, doğrudan erişim yapıyoruz
    while (!data.empty() && (data[data.size() - 1] == '\n' || data[data.size() - 1] == '\r')) {
        data.erase(data.size() - 1);
    }

    std::istringstream iss(data);
    std::string token;

    if (!data.empty() && data[0] == ':') {
        iss >> msg.prefix;
        msg.prefix.erase(0, 1);
    }

    if (iss >> msg.command) {
        std::transform(msg.command.begin(), msg.command.end(), msg.command.begin(), ::toupper);
    }

    std::string param;
    while (iss >> param) {
        if (param[0] == ':') {
            std::string rest;
            std::getline(iss, rest);
            msg.params.push_back(param.substr(1) + rest);
            break;
        }
        std::istringstream paramStream(param);
        std::string splitParam;
        while (std::getline(paramStream, splitParam, ',')) {
            msg.params.push_back(splitParam);
        }
    }

    return msg;
}


std::vector<IRCMessage> IRC_Protocol::parseMultiple(const std::string& rawData) {
    std::vector<IRCMessage> messages;
    std::istringstream stream(rawData);
    std::string line;
    while (std::getline(stream, line)) {
        if (!line.empty()) {
            messages.push_back(parser(line));
        }
    }
    return messages;
}
