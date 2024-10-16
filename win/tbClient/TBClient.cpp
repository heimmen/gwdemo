#include "TBClient.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>

#pragma comment(lib, "Ws2_32.lib")
#include <iostream>

using namespace std;

CTBClient::CTBClient(void)
{
}


CTBClient::~CTBClient(void)
{
}


int main() {
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        printf("WSAStartup failed: %d\n", result);
        return 1;
    }

    SOCKET ConnectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ConnectSocket == INVALID_SOCKET) {
        printf("Socket creation failed: %ld\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(12345);
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

    if (connect(ConnectSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        printf("Connect failed: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }

    char buffer[1024];
    int bytesReceived = recv(ConnectSocket, buffer, sizeof(buffer), 0);
    if (bytesReceived > 0) {
        buffer[bytesReceived] = '\0'; // Null terminate the received data
        printf("Message from server: %s\n", buffer);
    } else if (bytesReceived == 0) {
        printf("Connection closed by server.\n");
    } else {
        printf("Receive failed: %d\n", WSAGetLastError());
    }

    closesocket(ConnectSocket);
    WSACleanup();

    char input;

    cout << "请输入字符，输入 'm' 退出程序：" << endl;

    while (true) {
        cin >> input;

        if (input == 'm' || input == 'M') {
            cout << "退出程序..." << endl;
            break;
        } else {
            cout << "你输入的是: " << input << endl;
        }
    }

    return 0;
}