#include "../include/IRC_Protocol.hpp"
#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <map>

std::vector<std::string> IRC_Protocol::split(const std::string &s, char delimiter)
{
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter))
        tokens.push_back(token);
    return tokens;
}


IRCMessage IRC_Protocol::parser(const std::string &rawData) {
	IRCMessage msg;
	std::string line = rawData;
	std::string token;

	std::istringstream iss(line);

	if (!line.empty() && line[0] == ':') {
		iss >> token;
		msg.prefix = token.substr(1); 
	}


	if (!(iss >> msg.command)) {
		return msg;
	}
		std::transform(msg.command.begin(), msg.command.end(), msg.command.begin(), ::toupper);

	while (iss >> token) {
		if (token[0] == ':') {
	
			std::string trailing;
			std::getline(iss, trailing);
			if (!trailing.empty() && trailing[0] == ' ')
				trailing.erase(0, 1); 
			msg.params.push_back(token.substr(1) + (trailing.empty() ? "" : " " + trailing));
			break;
		} else {
			msg.params.push_back(token);
		}
	}

	return msg;
}
