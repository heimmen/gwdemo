#include "Proxy.h"

// Proxy.cpp
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <time.h>
#include <windows.h>
#include <process.h>
#include <map>


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

    fd_set readfds, masterfds;
    FD_ZERO(&masterfds);
    FD_SET(ListenSocket, &masterfds);
    SOCKET max_socket = ListenSocket;

    std::map<SOCKET, SOCKET> client_to_server_map;

    while (true) {
        readfds = masterfds;

        if (select(0, &readfds, NULL, NULL, NULL) == SOCKET_ERROR) {
            printf("Select failed: %d\n", WSAGetLastError());
            break;
        }

        for (SOCKET i = ListenSocket; i <= max_socket; ++i) {
            if (FD_ISSET(i, &readfds)) {
                if (i == ListenSocket) {
                    // ������
                    SOCKET ClientSocket = accept(ListenSocket, NULL, NULL);
                    if (ClientSocket == INVALID_SOCKET) {
                        printf("Accept failed with error code : %d", WSAGetLastError());
                        continue;
                    }

                    printf("Client connected: %llu\n", (unsigned long long)ClientSocket);
                    FD_SET(ClientSocket, &masterfds);
                    if (ClientSocket > max_socket) {
                        max_socket = ClientSocket;
                    }
                } else {
                    // ���������ӵĿͻ���
                    char buffer[1024];
                    int bytesReceived = recv(i, buffer, sizeof(buffer), 0);
                    if (bytesReceived > 0) {
                        buffer[bytesReceived] = '\0';
                        printf("Message from client: %s\n", buffer);

                        // ���Ҷ�Ӧ�ķ���������
                        SOCKET serverSocket = client_to_server_map[i];
                        if (serverSocket == INVALID_SOCKET || serverSocket == 0) {
                            // ���ӵ������
                            serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
                            if (serverSocket == INVALID_SOCKET) {
                                printf("Error at socket(): %ld\n", WSAGetLastError());
                                closesocket(i);
                                FD_CLR(i, &masterfds);
                                continue;
                            }

                            struct sockaddr_in serverAddr;
                            serverAddr.sin_family = AF_INET;
                            serverAddr.sin_port = htons(12346); // ����˶˿� 12346
                            inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

                            if (connect(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
                                printf("Connect failed: %d\n", WSAGetLastError());
                                closesocket(i);
                                closesocket(serverSocket);
                                FD_CLR(i, &masterfds);
                                continue;
                            }

                            client_to_server_map[i] = serverSocket;
                            FD_SET(serverSocket, &masterfds);
                            if (serverSocket > max_socket) {
                                max_socket = serverSocket;
                            }
                        }

                        // �������󵽷����
                        size_t len = bytesReceived;
						char* pbuff = buffer;
                        while (len > 0) {
                            if (consume_tokens(&tb, len)) { // ���������㹻������
                                int sent = send(serverSocket, pbuff, len, 0);
                                if (sent == SOCKET_ERROR) {
                                    printf("Send failed: %d\n", WSAGetLastError());
                                    break;
                                }
                                pbuff += sent;
                                len -= sent;
                            } else {
                                Sleep(100); // ���û���㹻�����ƣ��ȴ�һ��ʱ������
                            }
                        }
                    } else if (bytesReceived == 0) {
                        printf("Connection closed by client.\n");
                        SOCKET serverSocket = client_to_server_map[i];
                        if (serverSocket != INVALID_SOCKET) {
                            closesocket(serverSocket);
                            FD_CLR(serverSocket, &masterfds);
                            client_to_server_map.erase(i);
                        }
                        closesocket(i);
                        FD_CLR(i, &masterfds);
                    } else {
                        printf("Receive failed: %d\n", WSAGetLastError());
                        SOCKET serverSocket = client_to_server_map[i];
                        if (serverSocket != INVALID_SOCKET) {
                            closesocket(serverSocket);
                            FD_CLR(serverSocket, &masterfds);
                            client_to_server_map.erase(i);
                        }
                        closesocket(i);
                        FD_CLR(i, &masterfds);
                    }
                }
            }
        }
    }

    // ������Դ
	for (std::map<SOCKET, SOCKET>::iterator it = client_to_server_map.begin();
		it != client_to_server_map.end(); ++it) {
        closesocket(it->first);
        closesocket(it->second);
    }
    closesocket(ListenSocket);
    WSACleanup();

    return 0;
}

