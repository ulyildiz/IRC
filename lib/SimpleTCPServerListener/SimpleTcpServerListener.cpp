#include "SimpleTcpServerListener.hpp"
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/stat.h>
#include <errno.h>


SimpleTcpServer::SimpleTcpServer()
: nfds(1), handlerUserData(NULL), dataHandler(NULL) 
{
	struct sigaction sa;
	std::memset(&sa, 0, sizeof(sa));
	sa.sa_handler = SimpleTcpServer::signalHandler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	if (sigaction(SIGINT, &sa, NULL) < 0) {
		throw std::runtime_error("sigaction() failed: " + std::string(std::strerror(errno)));
	}
}

SimpleTcpServer::~SimpleTcpServer() {
	close(fds[0].fd);
}

void SimpleTcpServer::start(const char *ip, const char* port, void *IRC, DataHandler handler) {
    struct addrinfo hints;
    struct addrinfo* res = NULL;
    int serverSocket;

	handlerUserData = IRC;
	dataHandler = handler;

	struct protoent* proto = getprotobyname("tcp");
    if (!proto)
        throw std::runtime_error("getprotobyname() failed: " + std::string(std::strerror(errno)));

    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;           // IPv4
    hints.ai_socktype = SOCK_STREAM;     // TCP
    hints.ai_protocol = proto->p_proto;  // getprotobyname's return value
    hints.ai_flags = AI_PASSIVE;         // Yerel IP (wildcard)

	char hostname[256];
	if (gethostname(hostname, sizeof(hostname)) == 0) {
		struct hostent* he = gethostbyname(hostname);
		if (he) {
			std::cout << "Local host: " << he->h_name << std::endl;
		}
	}

    if (getaddrinfo(ip, port, &hints, &res))
        throw std::runtime_error("getaddrinfo() failed: " + std::string(std::strerror(errno)));

    serverSocket = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (serverSocket < 0) {
        freeaddrinfo(res);
        throw std::runtime_error("socket() failed: " + std::string(std::strerror(errno)));
    }

    int opt = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (const void*)&opt, sizeof(opt)) < 0) {
		freeaddrinfo(res);
        close(serverSocket);
		throw std::runtime_error("setsockopt() failed: " + std::string(std::strerror(errno)));
    }

    if (bind(serverSocket, res->ai_addr, res->ai_addrlen) < 0) {
		freeaddrinfo(res);
        close(serverSocket);
        throw std::runtime_error("bind() failed: " + std::string(std::strerror(errno)));
    }
    freeaddrinfo(res);

	setNonBlocking(serverSocket);

    if (listen(serverSocket, 10) < 0) {
		close(serverSocket);
        throw std::runtime_error("listen() failed: " + std::string(std::strerror(errno)));
    }

	fds[0].fd = serverSocket;
	fds[0].events = POLLIN;
	for (int i = 1; i < MAX_CLIENTS + 1; i++) {
		fds[i].fd = -1;
		fds[i].events = 0;
		fds[i].revents = 0;
	}
    printBoundAddress();
}

void SimpleTcpServer::createClientSocket()
{
	struct sockaddr_in clientAddr;
	socklen_t clientLen = sizeof(clientAddr);
	
	int clientSocket = accept(fds[0].fd, (struct sockaddr*)&clientAddr, &clientLen);
    if (clientSocket >= 0) {
        setNonBlocking(clientSocket);
    
		if (nfds < MAX_CLIENTS + 1)
        {
            fds[nfds].fd = clientSocket;
            fds[nfds].events = POLLIN;
            fds[nfds].revents = 0;
            nfds++;
        } 
        else
        {
            std::cerr << "Too many clients." << std::endl;
            send(clientSocket, "Server is full. Try again later.\n");
			close(clientSocket);
        }
    }
}

void SimpleTcpServer::run() {
	
	while (true) {
		if (poll(fds, nfds, -1) < 0) {
			if (errno == EINTR)
				continue;
            throw std::runtime_error("poll() failed: " + std::string(std::strerror(errno)));
        }
		
        if (fds[0].revents & POLLIN)
			createClientSocket();

        for (nfds_t i = 1; i < nfds; i++) {
            if (fds[i].fd == -1)
                continue;

            if (fds[i].revents & POLLIN) {
                char buffer[1024];
                int n = recv(fds[i].fd, buffer, sizeof(buffer) - 1, 0);
                if (n > 0) {
                    buffer[n] = '\0';
					dataHandler(fds[i].fd, buffer, n, handlerUserData);
				} else if (n == 0 || (n < 0 && errno != EWOULDBLOCK)) {
                    close(fds[i].fd);
                    fds[i].fd = -1;
                }
            }
        }
    
		for (nfds_t i = 1; i < nfds; i++) {
            if (fds[i].fd == -1) {
                for (nfds_t j = i; j < nfds - 1; j++) {
                    fds[j] = fds[j + 1];
                }
                nfds--;
                i--;
            }
        }

    }
}


void SimpleTcpServer::signalHandler(int signum) {
    (void)signum;
    throw std::runtime_error("Server interrupted by signal.");
}

int SimpleTcpServer::setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    
	if (flags == -1)
	{
		close(fd);
        throw std::runtime_error("fcntl() failed: " + std::string(std::strerror(errno)));
	}
	
	return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void SimpleTcpServer::printBoundAddress() {
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
    
	if (getsockname(fds[0].fd, (struct sockaddr*)&addr, &addrlen) == 0) {
        std::cout << "Server bound to: " << inet_ntoa(addr.sin_addr)
                  << ":" << ntohs(addr.sin_port) << std::endl;
    } else {
        std::cerr << "getsockname() failed." << std::endl;
    }
}

int SimpleTcpServer::send(int clientSocket, const char* data) {
    return send(clientSocket, data, std::strlen(data));
}

int SimpleTcpServer::send(int clientSocket, const char* data, size_t dataSize) {
    ssize_t sentBytes = ::send(clientSocket, data, dataSize, 0);
    
	if (sentBytes < 0) {
        std::cerr << "send() failed: " << std::strerror(errno) << std::endl;
    }
    return sentBytes;
}
