#include "../include/IRC_Protocol.hpp"
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

std::string intToString(int n)
{
    std::ostringstream oss;
    oss << n;
    return oss.str();
}

std::string getLocalIP()
{
    char hostbuffer[256];
    if (gethostname(hostbuffer, sizeof(hostbuffer)) == -1)
        return std::string("127.0.0.1");
    
    struct hostent *host_entry = gethostbyname(hostbuffer);
    if (!host_entry)
        return std::string("127.0.0.1");

    char *ip = inet_ntoa(*((struct in_addr*)host_entry->h_addr_list[0]));

	return std::string(ip);
}

IRCUser::IRCUser(int ClientSocket)
{
    clientSocket = ClientSocket;
    nickname = "";
    username = "";
    realname = "";
    isBot    = 0 ;
    registrationState = REG_STATE_INIT;
}

IRCUser::~IRCUser()
{
    close(clientSocket);
}

IRCChannel::IRCChannel(std::string chName, IRCUser* user)
{
    name                = chName    ;
    inviteOnly          = false     ;
    topicOnlyOps        = false     ;
    limit               = 0         ;
    key.clear()                     ;
    users.clear()                   ;
    operators.clear()               ;
    invitedUsers.clear()            ;
    operators.push_back(user)       ;
}

IRC_Protocol::IRC_Protocol(int port, const std::string& password)
    : _password(password)
{
    _port = port;
    serverName = getLocalIP();
    start(serverName.c_str(), intToString(port).c_str(), this, this->lifeloop);    

	commandHandlers["PASS"] = &IRC_Protocol::handlePASS;
    commandHandlers["NICK"] = &IRC_Protocol::handleNICK;
    commandHandlers["USER"] = &IRC_Protocol::handleUSER;
    commandHandlers["WHO"] = &IRC_Protocol::handleWHO;
    commandHandlers["PRIVMSG"] = &IRC_Protocol::handlePRIVMSG;
    commandHandlers["JOIN"] = &IRC_Protocol::handleJOIN;
    commandHandlers["PART"] = &IRC_Protocol::handlePART;
    commandHandlers["QUIT"] = &IRC_Protocol::handleQUIT;
    commandHandlers["PING"] = &IRC_Protocol::handlePING;
    commandHandlers["MODE"] = &IRC_Protocol::handleMODE;
    commandHandlers["KICK"] = &IRC_Protocol::handleKICK;
    commandHandlers["INVITE"] = &IRC_Protocol::handleINVITE;
    commandHandlers["TOPIC"] = &IRC_Protocol::handleTOPIC;
    commandHandlers["BOT"] = &IRC_Protocol::handleBOT;
}

IRC_Protocol::~IRC_Protocol()
{
    for (std::map<std::string, IRCChannel*>::iterator it = channels.begin();
         it != channels.end(); ++it)
        delete it->second;

    channels.clear();

    for (std::map<int, IRCUser*>::iterator it = connectedUsers.begin();
         it != connectedUsers.end(); ++it)
    {
        close(it->first);
        delete it->second;
    }

	connectedUsers.clear();
}

void IRC_Protocol::lifeloop(int clientSocket, const char* data, int length, void* userData)
{
    IRC_Protocol* server = static_cast<IRC_Protocol*>(userData);

    if (length == 0)
	{
        std::map<int, IRCUser*>::iterator uit = server->connectedUsers.find(clientSocket);
        if (uit != server->connectedUsers.end())
		{
            IRCMessage quitMsg;
            quitMsg.command = "QUIT";
            quitMsg.params.push_back("Client disconnected");
            server->handleQUIT(quitMsg, uit->second);
        }
        return;
    }

    if (server->connectedUsers.find(clientSocket) == server->connectedUsers.end())
        server->connectedUsers[clientSocket] = new IRCUser(clientSocket);

    std::string& buf = server->recvBuffers[clientSocket];
    buf.append(data, length);

    size_t pos;
    while ((pos = buf.find("\n")) != std::string::npos)
	{
        std::string line = buf.substr(0, pos);
        buf.erase(0, pos + 1); 

        IRCMessage msg = server->parser(line);
        std::cout << ">> " << msg.command;
    
		for (size_t i = 0; i < msg.params.size(); ++i)
            std::cout << " [" << msg.params[i] << "]";
        std::cout << std::endl;

        server->handler(msg, clientSocket);
    }
}

void IRC_Protocol::handler(const IRCMessage& msg, int clientSocket)
{
    std::map<int, IRCUser*>::iterator uit = connectedUsers.find(clientSocket);
    if (uit == connectedUsers.end())
	{
        if (connectedUsers.find(clientSocket) == connectedUsers.end())
            connectedUsers[clientSocket] = new IRCUser(clientSocket);
        uit = connectedUsers.find(clientSocket);
        if (uit == connectedUsers.end())
		{
            std::string response = ":" + serverName + " 451 :User not found\r\n";
            send(clientSocket, response.c_str());
            return;
        }
    }
    IRCUser* user = uit->second;

    if (user->registrationState < (REG_STATE_PASS + REG_STATE_NICK + REG_STATE_USER))
	{
        if (msg.command != "PASS" && msg.command != "NICK" && msg.command != "USER")
		{
            std::string nickForReply = user->nickname.empty() ? "*" : user->nickname;
            std::string response = ":" + serverName + " 451 " + nickForReply + " :You are not registered\r\n";
            send(clientSocket, response.c_str());
            return;
        }
    }
    
    std::map<std::string, void (IRC_Protocol::*)(const IRCMessage&, IRCUser*)>::iterator it = 
        commandHandlers.find(msg.command);
    
	if (it != commandHandlers.end())
        (this->*(it->second))(msg, user);
	else
	{
        std::string nickForReply = user->nickname.empty() ? "*" : user->nickname;
        std::string response = ":" + serverName + " 421 " + nickForReply + " " + msg.command + " :Unknown command\r\n";
        send(clientSocket, response.c_str());
    }
}
