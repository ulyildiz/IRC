#include "SimpleTcpServerListener.h"
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
#include <poll.h>
#include <errno.h>

// Global pointer: Sinyal handler için (basitlik açısından).
static SimpleTcpServer* globalServerPtr = NULL;

SimpleTcpServer::SimpleTcpServer()
    : serverSocket(-1), running(false), dataHandler(0), handlerUserData(0)
{
}

SimpleTcpServer::~SimpleTcpServer() {
    stop();
}

int SimpleTcpServer::start(const char *ip, const char* port) {
    struct addrinfo hints;
    struct addrinfo* res = NULL;
    int ret;

    // getprotobyname() kullanarak "tcp" protokol numarasını alalım.
    struct protoent* proto = getprotobyname("tcp");
    if (!proto) {
        std::cerr << "getprotobyname() failed." << std::endl;
        return -1;
    }

    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;         // IPv4
    hints.ai_socktype = SOCK_STREAM;   // TCP
    hints.ai_protocol = proto->p_proto;  // getprotobyname'dan aldığımız protokol
    hints.ai_flags = AI_PASSIVE;       // Yerel IP (wildcard)

    ret = getaddrinfo(ip, port, &hints, &res);
    if (ret != 0) {
        std::cerr << "getaddrinfo: " << gai_strerror(ret) << std::endl;
        return -1;
    }

    // Yerel host bilgisini gethostbyname() ile de alalım (örnek kullanım)
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == 0) {
        struct hostent* he = gethostbyname(hostname);
        if (he) {
            std::cout << "Local host: " << he->h_name << std::endl;
        }
    }

    // Soketi oluştur.
    serverSocket = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (serverSocket < 0) {
        std::cerr << "socket() failed." << std::endl;
        freeaddrinfo(res);
        return -1;
    }

    // setsockopt: SO_REUSEADDR
    int opt = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (const void*)&opt, sizeof(opt)) < 0) {
        std::cerr << "setsockopt() failed." << std::endl;
        freeaddrinfo(res);
        close(serverSocket);
        return -1;
    }

    // bind() ile soketi adresine bağla.
    if (bind(serverSocket, res->ai_addr, res->ai_addrlen) < 0) {
        std::cerr << "bind() failed." << std::endl;
        freeaddrinfo(res);
        close(serverSocket);
        return -1;
    }
    freeaddrinfo(res);

    // Socket'i non-blocking moda alıyoruz.
    if (setNonBlocking(serverSocket) < 0) {
        std::cerr << "Failed to set non-blocking mode on serverSocket." << std::endl;
        close(serverSocket);
        return -1;
    }

    // listen() ile dinlemeye başla.
    if (listen(serverSocket, 10) < 0) {
        std::cerr << "listen() failed." << std::endl;
        close(serverSocket);
        return -1;
    }

    running = true;

    // SIGINT sinyali yakalamak için sigaction() kullan.
    struct sigaction sa;
    std::memset(&sa, 0, sizeof(sa));
    sa.sa_handler = SimpleTcpServer::signalHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGINT, &sa, NULL) < 0) {
        std::cerr << "sigaction() failed." << std::endl;
    }
    globalServerPtr = this;

    printBoundAddress();

    return 0;
}

void SimpleTcpServer::stop() {
    if (running) {
        running = false;
        close(serverSocket);
    }
}

void SimpleTcpServer::setDataHandler(DataHandler handler, void* userData) {
    dataHandler = handler;
    handlerUserData = userData;
}
void SimpleTcpServer::run() {
    const int MAX_CLIENTS = 1000;
    struct pollfd fds[MAX_CLIENTS];
    int nfds = 1;

    // Server socket setup
    fds[0].fd = serverSocket;
    fds[0].events = POLLIN;
    for (int i = 1; i < MAX_CLIENTS; i++) {
        fds[i].fd = -1;
        fds[i].events = 0;
        fds[i].revents = 0;
    }

    while (running) {
        int pollCount = poll(fds, nfds, -1);
        if (pollCount < 0) {
            if (errno == EINTR)
                continue;
            std::cerr << "poll() error." << std::endl;
            break;
        }

        if (pollCount == 0)
            continue;

        // Yeni bağlantı
        if (fds[0].revents & POLLIN) {
            struct sockaddr_in clientAddr;
            socklen_t clientLen = sizeof(clientAddr);
            int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientLen);
            if (clientSocket >= 0) {
                setNonBlocking(clientSocket);
                if (nfds < MAX_CLIENTS) {
                    fds[nfds].fd = clientSocket;
                    fds[nfds].events = POLLIN;
                    fds[nfds].revents = 0;
                    nfds++;
                } else {
                    std::cerr << "Too many clients." << std::endl;
                    close(clientSocket);
                }
            }
        }

        // Client socket işlemleri
        for (int i = 1; i < nfds; i++) {
            if (fds[i].fd == -1)
                continue;

            if (fds[i].revents & POLLIN) {
                char buffer[1024];
                int n = recv(fds[i].fd, buffer, sizeof(buffer) - 1, 0);
                if (n > 0) {
                    buffer[n] = '\0';
                    if (dataHandler) {
                        dataHandler(fds[i].fd, buffer, n, handlerUserData);
                    }
                } else if (n == 0 || (n < 0 && errno != EWOULDBLOCK)) {
                    close(fds[i].fd);
                    fds[i].fd = -1;
                }
            }
        }

        // pollfd dizisini sıkıştır (fd == -1 olanları at)
        for (int i = 1; i < nfds; i++) {
            if (fds[i].fd == -1) {
                for (int j = i; j < nfds - 1; j++) {
                    fds[j] = fds[j + 1];
                }
                nfds--;
                i--; // tekrar aynı indexi kontrol et
            }
        }

        // ✅ Kalan alanları sıfırla (Valgrind uyarılarını önler)
        for (int i = nfds; i < MAX_CLIENTS; ++i) {
            fds[i].fd = -1;
            fds[i].events = 0;
            fds[i].revents = 0;
        }
    }
}


// Sinyal yakalayıcı: SIGINT geldiğinde sunucuyu durdurur.
void SimpleTcpServer::signalHandler(int signum) {
    (void)signum;
    if (globalServerPtr) {
        std::cout << "\nSIGINT received, shutting down server." << std::endl;
        globalServerPtr->stop();
        
    }
}

// Socket'i non-blocking moda almak için fcntl() kullanımı.
int SimpleTcpServer::setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

// getsockname() kullanarak sunucunun bağlı olduğu adres bilgisini ekrana yazdırır.
void SimpleTcpServer::printBoundAddress() {
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
    if (getsockname(serverSocket, (struct sockaddr*)&addr, &addrlen) == 0) {
        std::cout << "Server bound to: " << inet_ntoa(addr.sin_addr)
                  << ":" << ntohs(addr.sin_port) << std::endl;
    } else {
        std::cerr << "getsockname() failed." << std::endl;
    }
}

// fstat ve lseek kullanımı örneği: Verilen dosya (soket) hakkında bilgi yazdırır.
// (Not: Soketler için lseek genellikle geçerli değildir; bu sadece örnek amaçlıdır.)
void SimpleTcpServer::printFileDescriptorInfo(int fd) {
    struct stat statbuf;
    if (fstat(fd, &statbuf) == 0) {
        std::cout << "FD " << fd << " inode: " << statbuf.st_ino << std::endl;
        // lseek ile mevcut konumu almaya çalışalım (muhtemelen -1 döner).
        off_t pos = lseek(fd, 0, SEEK_CUR);
        if (pos != (off_t)-1)
            std::cout << "FD " << fd << " current offset: " << pos << std::endl;
        else
            std::cout << "lseek() failed on FD " << fd << std::endl;
    } else {
        std::cerr << "fstat() failed on FD " << fd << std::endl;
    }
}

int SimpleTcpServer::send(int clientSocket, const char* data) {
    return send(clientSocket, data, std::strlen(data));
}

int SimpleTcpServer::send(int clientSocket, const char* data, size_t dataSize) {
    ssize_t sentBytes = ::send(clientSocket, data, dataSize, 0);
    if (sentBytes < 0) {
        std::cerr << "send() failed: " << strerror(errno) << std::endl;
    }
    return sentBytes;
}
