#ifndef SIMPLE_TCP_SERVER_HPP
# define SIMPLE_TCP_SERVER_HPP

# include <sys/types.h>
# include <sys/socket.h>
# include <poll.h>

# define MAX_CLIENTS 1024

typedef void (*DataHandler)(int clientSocket, const char* data, int length, void* userData);

class SimpleTcpServer {
	private:
		void printBoundAddress();
		void createClientSocket();
		int setNonBlocking(int fd);
		
	protected:
		struct pollfd fds[MAX_CLIENTS + 1];
		nfds_t	nfds;
		
		void* handlerUserData;
		DataHandler dataHandler;
		
		void start(const char *ip, const char* port, void *IRC, DataHandler handler);
		
		int send(int clientSocket, const char* data);
		int send(int clientSocket, const char* data, size_t dataSize);
		
	public:
		SimpleTcpServer();
		~SimpleTcpServer();
	
		static void signalHandler(int signum);

		void run();
};

#endif
