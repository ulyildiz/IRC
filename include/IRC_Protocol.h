#ifndef IRC_PROTOCOL_H
#define IRC_PROTOCOL_H

#include "../lib/SimpleTCPServerListener/SimpleTcpServerListener.h"
#include <string>
#include <vector>
#include <map>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <ctime>

struct IRCMessage {
    std::string prefix;
    std::string command;
    std::vector<std::string> params;
};

struct IRCUser {
    std::string nickname;
    std::string username;
    std::string realname;
    int isBot;
    int clientSocket;
    std::vector<std::string> channels; 
    int registrationState;
    // (isteğe bağlı) server-wide operatörlük flag’i
    bool globalOperator;
};

// Kayıt durumu sabitleri
#define REG_STATE_INIT 0
#define REG_STATE_PASS 1
#define REG_STATE_NICK 2
#define REG_STATE_USER 3

struct IRCChannel {
    std::string name;        // Kanal adı, örn: "#chat"
    std::string topic;       // Kanal konusu
    // MODE komutuyla değişecek alt-modlar:
    bool inviteOnly;         // +i
    bool topicOnlyOps;       // +t
    std::string key;         // +k (şifre)
    int limit;               // +l (üye limiti)
    // Üyeler ve operatörler:
    std::vector<IRCUser*> users;
    std::vector<IRCUser*> operators;     // +o ile atanmış operatörler
    std::vector<IRCUser*> invitedUsers;  // +i modunda davet edilenler
};

class IRC_Protocol {
private:
    std::string serverName;
    int _port;
    SimpleTcpServer Listener;
    const std::string &_password;
    std::map<int, IRCUser*> connectedUsers;
    std::map<std::string, IRCChannel*> channels;
    std::map<int, std::string> recvBuffers;

    // Parser
    IRCMessage parser(const std::string &rawData);
    std::vector<IRCMessage> parseMultiple(const std::string &rawData);
    static void lifeloop(int clientSocket, const char *data, int length, void *userData);

    // Checker
    void addUserIfNotExists(int clientSocket);
    void updateUserIfIncomplete(int clientSocket,
        const std::string &nickname,
        const std::string &username,
        const std::string &realname);

    // Handler
    std::map<std::string, void(IRC_Protocol::*)(const IRCMessage&, IRCUser*)> commandHandlers;
    void handler(const IRCMessage &msg, int clientSocket);

    // IRC komutları
    void handlePASS(const IRCMessage &msg, IRCUser *user);
    void handleNICK(const IRCMessage &msg, IRCUser *user);
    void handleUSER(const IRCMessage &msg, IRCUser *user);
    void handlePRIVMSG(const IRCMessage &msg, IRCUser *user);
    void handleJOIN(const IRCMessage &msg, IRCUser *user);
    void handlePART(const IRCMessage &msg, IRCUser *user);
    void handleQUIT(const IRCMessage &msg, IRCUser *user);
    void handleCAP(const IRCMessage &msg, IRCUser *user);
    void handlePING(const IRCMessage &msg, IRCUser *user);
    void handleLIST(const IRCMessage &msg, IRCUser *user);
    void handleWHO(const IRCMessage &msg, IRCUser *user);
    void handleMODE(const IRCMessage &msg, IRCUser *user);
    void handleKICK(const IRCMessage &msg, IRCUser *user);
    void handleINVITE(const IRCMessage &msg, IRCUser *user);
    void handleTOPIC(const IRCMessage &msg, IRCUser *user);
    void handleBOT(const IRCMessage &msg, IRCUser *user);

public:
    IRC_Protocol(int port, const std::string &password);
    ~IRC_Protocol();
    int start();
};

#endif // IRC_PROTOCOL_H
