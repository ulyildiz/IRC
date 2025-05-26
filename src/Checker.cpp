#include "../include/IRC_Protocol.hpp"

void IRC_Protocol::updateUserIfIncomplete(int clientSocket, const std::string& nickname,
                                           const std::string& username, const std::string& realname) {
    std::map<int, IRCUser*>::iterator it = connectedUsers.find(clientSocket);
    if (it != connectedUsers.end()) {
        IRCUser* user = it->second;
        
        bool updated = false;
        
        if (user->nickname.empty() && !nickname.empty()) {
            user->nickname = nickname;
            updated = true;
        }
        if (user->username.empty() && !username.empty()) {
            user->username = username;
            updated = true;
        }
        if (user->realname.empty() && !realname.empty()) {
            user->realname = realname;
            updated = true;
        }

        if (user->nickname != "" && user->username != "" && user->realname != "") {
            if (updated) {
                std::string welcomeMessage = ":" + serverName + " :Welcome to the IRC server!";
                send(clientSocket, welcomeMessage.c_str());
            }
        }
    }
}
