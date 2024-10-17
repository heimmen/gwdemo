#include "Proxy.h"

// Proxy.cpp
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <time.h>
#include <windows.h>
#include <process.h>

#pragma comment(lib, "Ws2_32.lib")

CProxy::CProxy(void)
{
}


CProxy::~CProxy(void)
{
}

// ����Ͱ�ṹ��
typedef struct {
    unsigned long long tokens; // ��ǰ��������
    unsigned long long max_tokens; // Ͱ���������
    unsigned long long refill_rate; // ÿ����ӵ�������
    time_t last_refill_time; // �ϴ�������Ƶ�ʱ��
} TokenBucket;

// ��ʼ������Ͱ
void init_token_bucket(TokenBucket *bucket, unsigned long long max_tokens, unsigned long long refill_rate) {
    bucket->tokens = max_tokens;
    bucket->max_tokens = max_tokens;
    bucket->refill_rate = refill_rate;
    bucket->last_refill_time = time(NULL);
}

// �������
void refill_tokens(TokenBucket *bucket) {
    time_t now = time(NULL);
    unsigned long long elapsed_seconds = now - bucket->last_refill_time;
    if (elapsed_seconds > 0) {
        bucket->tokens += elapsed_seconds * bucket->refill_rate;
        if (bucket->tokens > bucket->max_tokens) {
            bucket->tokens = bucket->max_tokens;
        }
        bucket->last_refill_time = now;
    }
}

// ������������
int consume_tokens(TokenBucket *bucket, unsigned long long tokens_to_consume) {
    refill_tokens(bucket); // �ȳ��Բ�������
    if (bucket->tokens >= tokens_to_consume) {
        bucket->tokens -= tokens_to_consume;
        return 1; // �ɹ�
    } else {
        return 0; // ʧ��
    }
}

// ����ͻ������ӵĺ���
unsigned __stdcall handle_client(void *arg) {
    SOCKET client_socket = *(SOCKET*)arg;
    TokenBucket *bucket = (TokenBucket*)(((SOCKET*)arg) + 1);

    char buffer[1024];
    int bytesReceived = recv(client_socket, buffer, sizeof(buffer), 0);
    if (bytesReceived > 0) {
        buffer[bytesReceived] = '\0';
        printf("Message from client: %s\n", buffer);

        // ���ӵ������
        SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (serverSocket == INVALID_SOCKET) {
            printf("Error at socket(): %ld\n", WSAGetLastError());
            closesocket(client_socket);
            _endthreadex(0);
            return 0;
        }

        struct sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(12346); // ����˶˿� 12346
        inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

        if (connect(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
            printf("Connect failed: %d\n", WSAGetLastError());
            closesocket(client_socket);
            closesocket(serverSocket);
            _endthreadex(0);
            return 0;
        }

        // �������󵽷����
        size_t len = bytesReceived;
		char* pBuff = buffer;
        while (len > 0) {
            if (consume_tokens(bucket, len)) { // ���������㹻������
                int sent = send(serverSocket, pBuff, len, 0);
                if (sent == SOCKET_ERROR) {
                    printf("Send failed: %d\n", WSAGetLastError());
                    break;
                }
                pBuff += sent;
                len -= sent;
            } else {
                Sleep(100); // ���û���㹻�����ƣ��ȴ�һ��ʱ������
            }
        }

        // ���շ������Ӧ
        char response_buffer[1024];
        int bytesReceivedFromServer = recv(serverSocket, response_buffer, sizeof(response_buffer), 0);
        if (bytesReceivedFromServer > 0) {
            response_buffer[bytesReceivedFromServer] = '\0';
            send(client_socket, response_buffer, bytesReceivedFromServer, 0);
        } else if (bytesReceivedFromServer == 0) {
            printf("Connection closed by server.\n");
        } else {
            printf("Receive failed: %d\n", WSAGetLastError());
        }

        closesocket(serverSocket);
    } else if (bytesReceived == 0) {
        printf("Connection closed by client.\n");
    } else {
        printf("Receive failed: %d\n", WSAGetLastError());
    }

    closesocket(client_socket);
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

    SOCKET ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ListenSocket == INVALID_SOCKET) {
        printf("Error at socket(): %ld\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    struct sockaddr_in proxyAddr;
    proxyAddr.sin_family = AF_INET;
    proxyAddr.sin_port = htons(12345); // �����˿� 12345
    proxyAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(ListenSocket, (struct sockaddr*)&proxyAddr, sizeof(proxyAddr)) == SOCKET_ERROR) {
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

    printf("Proxy is listening on port 12345...\n");

    // ��ʼ������Ͱ��ģ��1MB����
    TokenBucket tb;
    init_token_bucket(&tb, 1000000, 1000000); // ���1MB��ÿ�����1MB

    while (true) {
        SOCKET ClientSocket = accept(ListenSocket, NULL, NULL);
        if (ClientSocket == INVALID_SOCKET) {
            printf("Accept failed with error code : %d", WSAGetLastError());
            continue;
        }

        printf("Client connected: %llu\n", (unsigned long long)ClientSocket);

        // �������̴߳���ͻ�������
        SOCKET *client_socket_ptr = new SOCKET[2];
        client_socket_ptr[0] = ClientSocket;
        client_socket_ptr[1] = (SOCKET)&tb;

        HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, handle_client, client_socket_ptr, 0, NULL);
        if (hThread == NULL) {
            printf("Failed to create thread: %d\n", GetLastError());
            delete[] client_socket_ptr;
            closesocket(ClientSocket);
            continue;
        }

        // ����Ҫ�ȴ��߳̽������������ں�̨����
    }

    // ������Դ
    closesocket(ListenSocket);
    WSACleanup();

    return 0;
}