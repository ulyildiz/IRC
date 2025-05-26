#ifndef SIMPLE_TCP_SERVER_HPP
#define SIMPLE_TCP_SERVER_HPP

#include <sys/types.h>
#include <sys/socket.h>
#include <poll.h>

#define MAX_CLIENTS 3

typedef void (*DataHandler)(int clientSocket, const char* data, int length, void* userData);

class SimpleTcpServer {
	private:
		void printBoundAddress();
		void createClientSocket();
		int setNonBlocking(int fd);
		
	protected:
		struct pollfd fds[MAX_CLIENTS]; // Poll için file descriptor array
		nfds_t	nfds; // Aktif file descriptor sayısı
		
		DataHandler dataHandler;
		void* handlerUserData;
		
		int start(const char *ip, const char* port, void *IRC, DataHandler handler);
		
		int send(int clientSocket, const char* data);
		int send(int clientSocket, const char* data, size_t dataSize);
		
	public:
		SimpleTcpServer();
		~SimpleTcpServer();
	
		static void signalHandler(int signum);

		void run();
};

#endif
