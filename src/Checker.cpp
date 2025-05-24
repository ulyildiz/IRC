#include "../include/IRC_Protocol.h"

void IRC_Protocol::addUserIfNotExists(int clientSocket) {
    if (connectedUsers.find(clientSocket) == connectedUsers.end()) {
        IRCUser* newUser = new IRCUser();
        newUser->clientSocket = clientSocket;
        newUser->nickname = "";
        newUser->username = "";
        newUser->realname = "";
        newUser->registrationState = REG_STATE_INIT;
        connectedUsers[clientSocket] = newUser;
    }
}

void IRC_Protocol::updateUserIfIncomplete(int clientSocket, const std::string& nickname,
                                           const std::string& username, const std::string& realname) {
    std::map<int, IRCUser*>::iterator it = connectedUsers.find(clientSocket);
    if (it != connectedUsers.end()) {
        IRCUser* user = it->second;
        
        // Nickname, username ve realname boş değilse, bunları güncelle
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

        // Eğer kullanıcı tam anlamıyla tanımlandıysa, hoş geldiniz mesajı gönder
        if (user->nickname != "" && user->username != "" && user->realname != "") {
            if (updated) {
                std::string welcomeMessage = ":" + serverName + " :Welcome to the IRC server!";
                Listener.send(clientSocket, welcomeMessage.c_str());
            }
        }
    }
}
