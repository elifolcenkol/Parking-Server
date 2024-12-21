//gcc server.c -o server -lws2_32 .\server

//gerkli kütüphanelrin eklenmesi
#define _WINSOCK_DEPRECATED_NO_WARNINGS //bu kütüphaneyle TCP sunucusu başlatılır
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>
#include <process.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 8080
#define MAX_SPOTS 100 // Park alanı sayısını artırdık
#define BUFFER_SIZE 1024

typedef struct {
    char plate[20];
    int occupied; //doluluk durumu 1 or 0
} ParkingSpot;

ParkingSpot parkingLot[MAX_SPOTS]; //tüm park yerlerini saklar
CRITICAL_SECTION lock; //çoklu veri iş parçacığı arasında veri erişimini güvenli hale getirir

unsigned __stdcall handleClient(void* clientSocket) { // gelen komutları okuyarak komuta göre yönlendirir örnegin :ENTRY ve ya STATUS
    SOCKET clientSock = *(SOCKET*)clientSocket;
    free(clientSocket);
    char buffer[BUFFER_SIZE], response[BUFFER_SIZE];

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int readBytes = recv(clientSock, buffer, BUFFER_SIZE, 0);// istemciden gelen mesaj recv ile okunur
        if (readBytes <= 0) break;

        EnterCriticalSection(&lock); //park yeri durumu güncellemesi
        if (strncmp(buffer, "ENTRY ", 6) == 0) { //strncmp ile komutlar ayrıştırılır
            char* plate = buffer + 6; // plaka bilgisi komuttan ayrıştırılıyor
            int found = 0;
            for (int i = 0; i < MAX_SPOTS; ++i) { // parking lot dizisi taranarak ilk boş yer bulunuyor
                if (!parkingLot[i].occupied) {
                    parkingLot[i].occupied = 1;  // eğer boş yer varsa occupied=1 yapılarak dolu olark işaretleniyor.
                    strncpy(parkingLot[i].plate, plate, 20);    //plaka bilgisi park yerine atanıyor
                    snprintf(response, BUFFER_SIZE, "Vehicle %s parked at spot %d", plate, i + 1);
                    found = 1;
                    break;
                }
            }
            if (!found) {
                snprintf(response, BUFFER_SIZE, "No available spots for %s", plate);  // eğer boş yer yoksa bu yanıt veriliyor.
            }
        } else if (strncmp(buffer, "STATUS ", 7) == 0) {
            int spot = atoi(buffer + 7) - 1; // park yeri numarası alınıyor.
            if (spot >= 0 && spot < MAX_SPOTS) {   // girilen numaranın geçerli olup olmadığı kontrol diliyor.
                if (parkingLot[spot].occupied) {
                    snprintf(response, BUFFER_SIZE, "Spot %d is occupied by %s", spot + 1, parkingLot[spot].plate);
                } else {
                    snprintf(response, BUFFER_SIZE, "Spot %d is available", spot + 1);
                }
            } else {   // eğer geçersiz ise bu mesaj yazdırılıyor.
                snprintf(response, BUFFER_SIZE, "Invalid spot number %d", spot + 1);
            }
        } else if (strncmp(buffer, "EXIT ", 5) == 0) {
            char* plate = buffer + 5;  //plaka bilgisi komuttan ayrıştırılıyor
            int found = 0;
            for (int i = 0; i < MAX_SPOTS; ++i) {  
                if (parkingLot[i].occupied && strcmp(parkingLot[i].plate, plate) == 0) { //parkinglot dizisi taranarak belirtilen plakayı içeren yer aranıyor.
                    parkingLot[i].occupied = 0; //park yeri boş olarak işaretleniyor.
                    snprintf(response, BUFFER_SIZE, "Vehicle %s has exited from spot %d", plate, i + 1); // çıkış mesajı oluşturuluyor.
                    found = 1;
                    break;
                }
            }
            if (!found) {
                snprintf(response, BUFFER_SIZE, "Vehicle %s not found", plate); // eğer plaka bulunamazsa bu mesaj oluşturuluyor.
            }
        } else if (strncmp(buffer, "LIST", 4) == 0) { // LIST komutunu ele alma
            strcpy(response, "Parking lot status:\n");
            for (int i = 0; i < MAX_SPOTS; ++i) { // tüm park yerleri taranıyor
                char spotInfo[50];
                if (parkingLot[i].occupied) {
                    snprintf(spotInfo, sizeof(spotInfo), "Spot %d: Occupied by %s\n", i + 1, parkingLot[i].plate); // dolu park yerleri için bu yazdırılıyor ekrana
                } else {
                    snprintf(spotInfo, sizeof(spotInfo), "Spot %d: Available\n", i + 1); // boş park yerleri için de bu yazdırılır
                }
                strncat(response, spotInfo, BUFFER_SIZE - strlen(response) - 1); // liste bir yanıt stringine ekleniyor.
            }
        } else {
            snprintf(response, BUFFER_SIZE, "Unknown command");  // tanınmayan bir komut geldiğinde bu yanıt verilir
        }
        LeaveCriticalSection(&lock);  // kritik bölgeden çıkış

        send(clientSock, response, strlen(response), 0);// send işlemin sonucuna göre istemciye yanıt gönderir
    }
    closesocket(clientSock);
    return 0;
}

int main() {
    WSADATA wsaData; 
    SOCKET serverSocket, clientSocket;
    struct sockaddr_in serverAddr, clientAddr;
    int clientAddrSize = sizeof(clientAddr);

    WSAStartup(MAKEWORD(2, 2), &wsaData); //windows ağ kütüphanesi başlatılır
    InitializeCriticalSection(&lock); //veri tutarlılığı için kritik bölüm tanımlanır

    serverSocket = socket(AF_INET, SOCK_STREAM, 0); //socket ile bir TCP bağlantısı olusturulur
    serverAddr.sin_family = AF_INET; // serverAddr ile sunucu adresi ve port bilgisi ayarlanır
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)); //bind ile socket belirtilen adrese bağlanır
    listen(serverSocket, SOMAXCONN); // listen ile gelen bağlantılar için kuyruğa alınır

    for (int i = 0; i < MAX_SPOTS; ++i) { //bu döngüyle park yerleri dolaşılır ve tüm park yerleri 0 olarak işaretlenir
        parkingLot[i].occupied = 0;
    }

    printf("Server running on port %d\n", PORT);
    while (1) {
        clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrSize); //accept gelen istemci bağıntısını kabul eder
        SOCKET* clientSockPtr = malloc(sizeof(SOCKET));
        *clientSockPtr = clientSocket;
        _beginthreadex(NULL, 0, handleClient, clientSockPtr, 0, NULL);// her istemci için handleclient fonksiyonuna yeni bir iş parçacığı atanır
    }

    DeleteCriticalSection(&lock); // kritik bölgeler silinir ve bellek temizlenir
    closesocket(serverSocket); //socket kapatılır
    WSACleanup(); // winsock işlemleri sonlandırılır
    return 0;
}


