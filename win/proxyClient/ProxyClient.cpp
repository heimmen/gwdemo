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

// ����ͻ������ӵĺ���
unsigned __stdcall send_request(void *arg) {
    int thread_id = *(int*)arg;
    free(arg); // �ͷŴ��ݵĲ����ڴ�

    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket == INVALID_SOCKET) {
        printf("Error at socket(): %ld\n", WSAGetLastError());
        _endthreadex(0);
        return 0;
    }

    struct sockaddr_in proxyAddr;
    proxyAddr.sin_family = AF_INET;
    proxyAddr.sin_port = htons(12345); // ����˿� 12345
    inet_pton(AF_INET, "127.0.0.1", &proxyAddr.sin_addr);

    if (connect(clientSocket, (struct sockaddr*)&proxyAddr, sizeof(proxyAddr)) == SOCKET_ERROR) {
        printf("Connect failed: %d\n", WSAGetLastError());
        closesocket(clientSocket);
        _endthreadex(0);
        return 0;
    }

    char message[100];
    // ��ʽ����Ϣ�ַ������������
    sprintf(message, "Hello, this is a test message from thread %d.\n", thread_id);
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

	const int THREAD_COUNT = 1;
    HANDLE threads[THREAD_COUNT];
    for (int i = 0; i < THREAD_COUNT; ++i) {
        int *thread_id = (int*)malloc(sizeof(int));
        *thread_id = i;
        threads[i] = (HANDLE)_beginthreadex(NULL, 0, send_request, thread_id, 0, NULL);
        if (threads[i] == NULL) {
            printf("Failed to create thread: %d\n", GetLastError());
            free(thread_id); // �ͷ��ڴ�
            continue;
        }
    }

    // �ȴ������߳̽���
    WaitForMultipleObjects(THREAD_COUNT, threads, TRUE, INFINITE);

    // �ر��߳̾��
    for (int i = 0; i < THREAD_COUNT; ++i) {
        CloseHandle(threads[i]);
    }

    // ������Դ
    WSACleanup();

    return 0;
}