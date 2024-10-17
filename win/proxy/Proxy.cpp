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

// 令牌桶结构体
typedef struct {
    unsigned long long tokens; // 当前令牌数量
    unsigned long long max_tokens; // 桶的最大容量
    unsigned long long refill_rate; // 每秒添加的令牌数
    time_t last_refill_time; // 上次添加令牌的时间
} TokenBucket;

// 初始化令牌桶
void init_token_bucket(TokenBucket *bucket, unsigned long long max_tokens, unsigned long long refill_rate) {
    bucket->tokens = max_tokens;
    bucket->max_tokens = max_tokens;
    bucket->refill_rate = refill_rate;
    bucket->last_refill_time = time(NULL);
}

// 添加令牌
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

// 尝试消费令牌
int consume_tokens(TokenBucket *bucket, unsigned long long tokens_to_consume) {
    refill_tokens(bucket); // 先尝试补充令牌
    if (bucket->tokens >= tokens_to_consume) {
        bucket->tokens -= tokens_to_consume;
        return 1; // 成功
    } else {
        return 0; // 失败
    }
}

// 处理客户端连接的函数
unsigned __stdcall handle_client(void *arg) {
    SOCKET client_socket = *(SOCKET*)arg;
    TokenBucket *bucket = (TokenBucket*)(((SOCKET*)arg) + 1);

    char buffer[1024];
    int bytesReceived = recv(client_socket, buffer, sizeof(buffer), 0);
    if (bytesReceived > 0) {
        buffer[bytesReceived] = '\0';
        printf("Message from client: %s\n", buffer);

        // 连接到服务端
        SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (serverSocket == INVALID_SOCKET) {
            printf("Error at socket(): %ld\n", WSAGetLastError());
            closesocket(client_socket);
            _endthreadex(0);
            return 0;
        }

        struct sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(12346); // 服务端端口 12346
        inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

        if (connect(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
            printf("Connect failed: %d\n", WSAGetLastError());
            closesocket(client_socket);
            closesocket(serverSocket);
            _endthreadex(0);
            return 0;
        }

        // 发送请求到服务端
        size_t len = bytesReceived;
		char* pBuff = buffer;
        while (len > 0) {
            if (consume_tokens(bucket, len)) { // 尝试消费足够的令牌
                int sent = send(serverSocket, pBuff, len, 0);
                if (sent == SOCKET_ERROR) {
                    printf("Send failed: %d\n", WSAGetLastError());
                    break;
                }
                pBuff += sent;
                len -= sent;
            } else {
                Sleep(100); // 如果没有足够的令牌，等待一段时间再试
            }
        }

        // 接收服务端响应
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
    proxyAddr.sin_port = htons(12345); // 监听端口 12345
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

    // 初始化令牌桶，模拟1MB带宽
    TokenBucket tb;
    init_token_bucket(&tb, 1000000, 1000000); // 最大1MB，每秒填充1MB

    while (true) {
        SOCKET ClientSocket = accept(ListenSocket, NULL, NULL);
        if (ClientSocket == INVALID_SOCKET) {
            printf("Accept failed with error code : %d", WSAGetLastError());
            continue;
        }

        printf("Client connected: %llu\n", (unsigned long long)ClientSocket);

        // 创建新线程处理客户端连接
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

        // 不需要等待线程结束，让它们在后台运行
    }

    // 清理资源
    closesocket(ListenSocket);
    WSACleanup();

    return 0;
}