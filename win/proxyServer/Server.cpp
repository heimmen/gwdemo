#include "Server.h"

// Server.cpp
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>

#pragma comment(lib, "Ws2_32.lib")

CServer::CServer(void)
{
}


CServer::~CServer(void)
{
}


int main() {
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        printf("WSAStartup failed: %d\n", result);
        return 1;
    }

    SOCKET ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ListenSocket == INVALID_SOCKET) {
        printf("Error at socket(): %ld\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(12346); // ¼àÌý¶Ë¿Ú 12346
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(ListenSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        printf("Bind failed with error code : %d", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    if (listen(ListenSocket, SOMAXCONN) == SOCKET_ERROR) {
        printf("Listen failed with error code : %d", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    printf("Server is listening on port 12346...\n");

    while (true) {
        SOCKET ClientSocket = accept(ListenSocket, NULL, NULL);
        if (ClientSocket == INVALID_SOCKET) {
            printf("Accept failed with error code : %d", WSAGetLastError());
            continue;
        }

        printf("Client connected: %llu\n", (unsigned long long)ClientSocket);

        char buffer[1024];
        int bytesReceived = recv(ClientSocket, buffer, sizeof(buffer), 0);
        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0';
            printf("Message from client: %s\n", buffer);
            const char* response = "Response from server\n";
            send(ClientSocket, response, strlen(response), 0);
        } else if (bytesReceived == 0) {
            printf("Connection closed by client.\n");
        } else {
            printf("Receive failed: %d\n", WSAGetLastError());
        }

        closesocket(ClientSocket);
    }

    closesocket(ListenSocket);
    WSACleanup();

    return 0;
}