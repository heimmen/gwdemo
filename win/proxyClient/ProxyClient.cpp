#include "ProxyClient.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <windows.h>
#include <process.h>

#pragma comment(lib, "Ws2_32.lib")

CProxyClient::CProxyClient(void)
{
}


CProxyClient::~CProxyClient(void)
{
}


// 处理客户端连接的函数
unsigned __stdcall send_request(void *arg) {
    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket == INVALID_SOCKET) {
        printf("Error at socket(): %ld\n", WSAGetLastError());
        _endthreadex(0);
        return 0;
    }

    struct sockaddr_in proxyAddr;
    proxyAddr.sin_family = AF_INET;
    proxyAddr.sin_port = htons(12345); // 代理端口 12345
    inet_pton(AF_INET, "127.0.0.1", &proxyAddr.sin_addr);

    if (connect(clientSocket, (struct sockaddr*)&proxyAddr, sizeof(proxyAddr)) == SOCKET_ERROR) {
        printf("Connect failed: %d\n", WSAGetLastError());
        closesocket(clientSocket);
        _endthreadex(0);
        return 0;
    }

    const char* message = "Hello, this is a test message.\n";
    size_t len = strlen(message);
    int sent = send(clientSocket, message, len, 0);
    if (sent == SOCKET_ERROR) {
        printf("Send failed: %d\n", WSAGetLastError());
        closesocket(clientSocket);
        _endthreadex(0);
        return 0;
    }

    char buffer[1024];
    int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
    if (bytesReceived > 0) {
        buffer[bytesReceived] = '\0';
        printf("Response from server: %s\n", buffer);
    } else if (bytesReceived == 0) {
        printf("Connection closed by server.\n");
    } else {
        printf("Receive failed: %d\n", WSAGetLastError());
    }

    closesocket(clientSocket);
    _endthreadex(0);
    return 0;
}

int main() {
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        printf("WSAStartup failed: %d\n", result);
        return 1;
    }
	const int THREAD_COUNT = 10;
    HANDLE threads[THREAD_COUNT];
    for (int i = 0; i < THREAD_COUNT; ++i) {
        threads[i] = (HANDLE)_beginthreadex(NULL, 0, send_request, NULL, 0, NULL);
        if (threads[i] == NULL) {
            printf("Failed to create thread: %d\n", GetLastError());
            continue;
        }
    }

    // 等待所有线程结束
    WaitForMultipleObjects(500, threads, TRUE, INFINITE);

    // 清理资源
    WSACleanup();

    return 0;
}