#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <windows.h>

#include <process.h>

#pragma comment(lib, "Ws2_32.lib")

#define PORT 12345

class Client {
public:
    void start() {
        WSADATA wsaData;
        int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (result != 0) {
            std::cerr << "WSAStartup failed: " << result << std::endl;
            return;
        }

        SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "Socket creation failed: " << WSAGetLastError() << std::endl;
            WSACleanup();
            return;
        }

        struct sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(PORT);
        inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

        if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
            std::cerr << "Connect failed: " << WSAGetLastError() << std::endl;
            closesocket(clientSocket);
            WSACleanup();
            return;
        }

        const char* message = "Hello from client";
        int sent = send(clientSocket, message, strlen(message), 0);
        if (sent == SOCKET_ERROR) {
            std::cerr << "Send failed: " << WSAGetLastError() << std::endl;
        } else {
            std::cout << "Message sent" << std::endl;
        }

        char buffer[1024] = {0};
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesReceived > 0) {
            std::cout << "Message received: " << buffer << std::endl;
        } else {
            std::cerr << "Receive failed: " << WSAGetLastError() << std::endl;
        }

        closesocket(clientSocket);
        WSACleanup();
    }
};