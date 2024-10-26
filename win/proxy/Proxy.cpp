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

		// TODO: Seperate the code to handle server and client socket.
        for (SOCKET i = ListenSocket; i <= max_socket; ++i) {
			printf("Check ListenSocket %d Socket max_socket %d i %d \n", ListenSocket, max_socket, i);
			bool isServerSocket = false;
			for (std::map<SOCKET, SOCKET>::iterator it = client_to_server_map.begin();
				it != client_to_server_map.end(); ++it) {
				if (i == it->second)
				{
					isServerSocket = true;
				}
			}
            if (FD_ISSET(i, &readfds)) {
                if (i == ListenSocket) {
                    // 新连接
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
                    // 处理已连接的客户端
                    char buffer[1024];
                    int bytesReceived = recv(i, buffer, sizeof(buffer), 0);
                    if (bytesReceived > 0) {
                        buffer[bytesReceived] = '\0';
						SOCKET targetSocket = INVALID_SOCKET;
						if (isServerSocket)
						{
							//targetSocket = i;
							for (std::map<SOCKET, SOCKET>::iterator it = client_to_server_map.begin();
								it != client_to_server_map.end(); ++it) {
								// To find the client
								if (i == it->second)
								{
									targetSocket = it->first;
								}
							}
							printf("Message from server %llu: %s\n", (unsigned long long)i, buffer);
						}
						else
						{
							// 查找对应的服务器连接
							targetSocket = client_to_server_map[i];
							printf("Message from client %llu: %s\n", (unsigned long long)i, buffer);
						}

                        //SOCKET serverSocket = client_to_server_map[i];
                        if (targetSocket == INVALID_SOCKET || targetSocket == 0) {
                            // 连接到服务端
                            targetSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
                            if (targetSocket == INVALID_SOCKET) {
                                printf("Error at socket(): %ld\n", WSAGetLastError());
                                closesocket(i);
                                FD_CLR(i, &masterfds);
                                continue;
                            }

                            struct sockaddr_in serverAddr;
                            serverAddr.sin_family = AF_INET;
                            serverAddr.sin_port = htons(12346); // 服务端端口 12346
                            inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

                            if (connect(targetSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
                                printf("Connect failed: %d\n", WSAGetLastError());
                                closesocket(i);
                                closesocket(targetSocket);
                                FD_CLR(i, &masterfds);
                                continue;
                            }

							printf("Connected to server socket: %llu\n", (unsigned long long)targetSocket);

                            client_to_server_map[i] = targetSocket;
                            FD_SET(targetSocket, &masterfds);
                            if (targetSocket > max_socket) {
                                max_socket = targetSocket;
                            }
                        }

                        // 发送请求
                        size_t len = bytesReceived;
						char* pbuff = buffer;
						printf("Foward message \"%s\": %llu\n", buffer, (unsigned long long)targetSocket);
                        while (len > 0) {
                            if (consume_tokens(&tb, len)) { // 尝试消费足够的令牌
                                int sent = send(targetSocket, pbuff, len, 0);
                                if (sent == SOCKET_ERROR) {
                                    printf("Send failed: %d\n", WSAGetLastError());
                                    break;
                                }
                                pbuff += sent;
                                len -= sent;
                            } else {
                                Sleep(100); // 如果没有足够的令牌，等待一段时间再试
                            }
                        }
                    } else if (bytesReceived == 0) {
                        printf("Received zero from socket: %llu\n", (unsigned long long)i);
                        SOCKET serverSocket = client_to_server_map[i];
                        if (serverSocket != INVALID_SOCKET && serverSocket != 0) {
                            closesocket(serverSocket);
                            FD_CLR(serverSocket, &masterfds);
                            client_to_server_map.erase(i);
                        }
						if (isServerSocket)
						{
							printf("Close server socket: %llu\n", (unsigned long long)i);
						}
						else
						{
							printf("Close socket: %llu\n", (unsigned long long)i);
						}
                        closesocket(i);
                        FD_CLR(i, &masterfds);
                    } else {
                        printf("Receive failed: %d\n", WSAGetLastError());
						printf("Try to close by socket: %llu\n", (unsigned long long)i);
                        SOCKET serverSocket = client_to_server_map[i];
                        if (serverSocket != INVALID_SOCKET && serverSocket != 0) {
                            closesocket(serverSocket);
                            FD_CLR(serverSocket, &masterfds);
                            client_to_server_map.erase(i);
                        }
						if (isServerSocket)
						{
							printf("Close server socket: %llu\n", (unsigned long long)i);
						}
						else
						{
							printf("Close socket: %llu\n", (unsigned long long)i);
						}
                        closesocket(i);
                        FD_CLR(i, &masterfds);
                    }
                }
            }
        }
    }

    // 清理资源
	for (std::map<SOCKET, SOCKET>::iterator it = client_to_server_map.begin();
		it != client_to_server_map.end(); ++it) {
        closesocket(it->first);
        closesocket(it->second);
    }
    closesocket(ListenSocket);
    WSACleanup();

    return 0;
}

