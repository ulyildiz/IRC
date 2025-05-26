#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>

#define SERVER "10.12.5.3"  // IRC sunucun (localhost veya IP)
#define PORT 6060           // IRC portu (genellikle 6667)
#define NICK "botum"
#define USER "botum 0 * :Test Bot"

int main() {
    int sock;
    struct sockaddr_in server_addr;
    char buffer[1024];

    // 1. Socket oluştur
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return 1;
    }

    // 2. Sunucu adresini ayarla
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, SERVER, &server_addr.sin_addr);

    // 3. Bağlan
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        return 1;
    }

    // 4. NICK ve USER komutlarını gönder
    snprintf(buffer, sizeof(buffer), "PASS secret\r\n");
    send(sock, buffer, strlen(buffer), 0);
    snprintf(buffer, sizeof(buffer), "NICK %s\r\n", NICK);
    send(sock, buffer, strlen(buffer), 0);
    snprintf(buffer, sizeof(buffer), "USER %s\r\n", USER);
    send(sock, buffer, strlen(buffer), 0);

    // 5. Sunucudan gelenleri (birkaç satır) oku
    recv(sock, buffer, sizeof(buffer) - 1, 0);
    // Burada gelen PING varsa PONG ile cevaplaman gerekebilir (örneği basit tuttuk)

    // 6. Mesaj gönderme döngüsü
    int i = 0;
    while (1) {
        snprintf(buffer, sizeof(buffer), "PRIVMSG jacop :%d\r\n", i++);
        send(sock, buffer, strlen(buffer), 0);
        printf("Mesaj gönderildi: %s", buffer);
        usleep(100);
    }

    close(sock);
    return 0;
}
