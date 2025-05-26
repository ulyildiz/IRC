#ifndef IRC_PROTOCOL_HPP
# define IRC_PROTOCOL_HPP

# include "../lib/SimpleTCPServerListener/SimpleTcpServerListener.hpp"
# include <vector>
# include <map>
# include <iostream>
# include <sstream>
# include <algorithm>

// Registration Statement Flags
# define REG_STATE_INIT 0
# define REG_STATE_PASS 1
# define REG_STATE_NICK 2
# define REG_STATE_USER 3

// Numeric replies constant
# define ERR_NEEDMOREPARAMS 461
# define ERR_PASSWDMISMATCH 464
# define ERR_NOTREGISTERED 451
# define RPL_WELCOME 001

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
    bool globalOperator;
    IRCUser(int ClientSocket);
    ~IRCUser();
};

struct IRCChannel {
    std::string name;        // Channel Name, for ex: "#chat"
    std::string topic;       // Channel Topic
    
    bool inviteOnly;         // +i
    bool topicOnlyOps;       // +t
    std::string key;         // +k (password)
    int limit;               // +l (user limit)
    std::vector<IRCUser*> users;
    std::vector<IRCUser*> operators;     // Operator users
    std::vector<IRCUser*> invitedUsers;  // Invited users
    IRCChannel(std::string chName, IRCUser* user);
};

class IRC_Protocol : public SimpleTcpServer {
private:
    std::string serverName;
    int _port;
    const std::string &_password;
    std::map<int, IRCUser*> connectedUsers;
    std::map<std::string, IRCChannel*> channels;
    std::map<int, std::string> recvBuffers;

    // Parser
    IRCMessage parser(const std::string &rawData);
    std::vector<IRCMessage> parseMultiple(const std::string &rawData);
    static void lifeloop(int clientSocket, const char *data, int length, void *userData);

    // Checker
        void updateUserIfIncomplete(int clientSocket,
        const std::string &nickname,
        const std::string &username,
        const std::string &realname);

    // Handler
    std::map<std::string, void(IRC_Protocol::*)(const IRCMessage&, IRCUser*)> commandHandlers;
    void handler(const IRCMessage &msg, int clientSocket);

    // IRC Commands
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
};

#endif
