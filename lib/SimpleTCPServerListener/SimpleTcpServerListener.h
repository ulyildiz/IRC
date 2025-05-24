#ifndef SIMPLE_TCP_SERVER_H
#define SIMPLE_TCP_SERVER_H

#include <sys/types.h>
#include <sys/socket.h>
#include <poll.h>

// Client'tan veri alındığında çağrılacak callback fonksiyonu.
typedef void (*DataHandler)(int clientSocket, const char* data, int length, void* userData);

class SimpleTcpServer {
public:
    SimpleTcpServer();
    ~SimpleTcpServer();

    // Belirtilen portta sunucuyu başlatır (port parametresi string olarak, örn. "6667").
    // Başarı durumunda 0, hata durumunda -1 döner.
    int start(const char *ip, const char* port);
    
    // Sunucuyu durdurur.
    void stop();

    // Veri geldiğinde çalışacak callback fonksiyonunu ayarlar.
    void setDataHandler(DataHandler handler, void* userData);
    int send(int clientSocket, const char* data);
    int send(int clientSocket, const char* data, size_t dataSize);
    // Event loop: poll() kullanılarak sunucu ve bağlı client’ları izler.
    void run();

private:
    int serverSocket;
    bool running;
    DataHandler dataHandler;
    void* handlerUserData;

    // Socket’i non-blocking moda almak için fcntl() kullanımı.
    int setNonBlocking(int fd);

    // getsockname() ile sunucunun bağlı olduğu IP:port bilgisini yazdırır.
    void printBoundAddress();

    // Örnek olarak, fstat ve lseek kullanarak verilen dosya (soket) hakkında bilgi yazdırır.
    void printFileDescriptorInfo(int fd);

    // Sinyalleri yakalamak için static sinyal handler.
    static void signalHandler(int signum);

    // Kopyalamayı engellemek için.
    SimpleTcpServer(const SimpleTcpServer&);
    SimpleTcpServer& operator=(const SimpleTcpServer&);
};

#endif // SIMPLE_TCP_SERVER_H
