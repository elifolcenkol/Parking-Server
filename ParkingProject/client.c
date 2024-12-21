//gcc client.c -o client -lws2_32 .\client

#define _WINSOCK_DEPRECATED_NO_WARNINGS // winsock un eski fonksiyonları için uyarıları kapatır
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>  // windows sockets apı için gerekli kütüphane. TCP/IP bağlantısı için kullanılır

#pragma comment(lib, "ws2_32.lib")

#define PORT 8080  // istemcinin bağlanacağı sunucu port numarası
#define BUFFER_SIZE 1024  // veri gönderme/alma işlemlerinde kullanılan tampon boyutu

int main() {
    WSADATA wsaData; //winsock başlatma ve konfigürasyon için kullanılan yapı
    SOCKET clientSocket;  // sunucuya bağlanacak istemci soketi
    struct sockaddr_in serverAddr; //sunucunun adres ve port bilgilerini tutan yapı
    char buffer[BUFFER_SIZE]; // komutları göndermek ve sunucu yanıtlarını almak için kullanılan tampon

    WSAStartup(MAKEWORD(2, 2), &wsaData);  //winsock kütüphanesini başlatır, winsockun 2.2 sürümünü kullanmayı belirtir. eğer başarısız olursa program devam edemez

    clientSocket = socket(AF_INET, SOCK_STREAM, 0); // AFN_INET iile Ipv4 protokolü kullanacağı bilinir. SOCK_STREAM TCP bağlantısı için kullanılacağını belirtir.
    serverAddr.sin_family = AF_INET;  // sin_family, IPv4 protokülünü belirtir.
    serverAddr.sin_port = htons(PORT);  //port atanır,  htons ile ağ bayt sırasına çevrilir.
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");  //sunucu adresi 127.0.0.1 olarak ayarlanır.

    connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr));// sunucuya bağlanmaya çalışılır clientsocket=bağlanılacak soket, serverAddr=sunucu adres bilgisi

    printf("Connected to the server.\n"); // kullanıcıya bağlantının başarılı olduğunu bildirir.
    while (1) {
        printf("\nAvailable commands:\n");                                     
        printf("1. ENTRY <plate> - Park a vehicle\n");                          //bir aracı park etmek
        printf("2. STATUS <spot> - Check the status of a specific spot\n");     //belirli bir park yerinin durumunu kontrol etmek.
        printf("3. EXIT <plate> - Remove a parked vehicle\n");                  // park etmiş bir aracı çıkarmak
        printf("4. LIST - Display the status of all parking spots\n");          // tüm park yerlerinin durumunu listelemek.
        printf("5. EXIT - Disconnect from the server\n");                       // sunucudan bağlantıyı kesmek
        printf("Enter command: ");
        fgets(buffer, BUFFER_SIZE, stdin);                                      // fgets kullanımı ile giriş satırını tampon boyutunda alır ve \n karakterini temizler.
        buffer[strcspn(buffer, "\n")] = 0;

        send(clientSocket, buffer, strlen(buffer), 0); //kullanıcı tarafından girilen komut sunucuya gönderilir
        if (strcmp(buffer, "EXIT") == 0) break; // eğer komut exit ise döngüden çıkılır ve bağlantı sonlanır

        memset(buffer, 0, BUFFER_SIZE); // buffer sıfırlanarak önceki içerik temizlenir
        recv(clientSocket, buffer, BUFFER_SIZE, 0); // sunucudan gelen yanıt alınır ve buffer içerisine yazılır
        printf("\nServer Response:\n%s\n", buffer);  // yanıt kullanıcıya gösterilir
    }

    closesocket(clientSocket); // istemci soketini kapatır
    WSACleanup();  // winsock kaynaklarını temizler
    return 0;
}
