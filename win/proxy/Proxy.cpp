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
							// ���Ҷ�Ӧ�ķ���������
							targetSocket = client_to_server_map[i];
							printf("Message from client %llu: %s\n", (unsigned long long)i, buffer);
						}

                        //SOCKET serverSocket = client_to_server_map[i];
                        if (targetSocket == INVALID_SOCKET || targetSocket == 0) {
                            // ���ӵ������
                            targetSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
                            if (targetSocket == INVALID_SOCKET) {
                                printf("Error at socket(): %ld\n", WSAGetLastError());
                                closesocket(i);
                                FD_CLR(i, &masterfds);
                                continue;
                            }

                            struct sockaddr_in serverAddr;
                            serverAddr.sin_family = AF_INET;
                            serverAddr.sin_port = htons(12346); // ����˶˿� 12346
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

                        // ��������
                        size_t len = bytesReceived;
						char* pbuff = buffer;
						printf("Foward message \"%s\": %llu\n", buffer, (unsigned long long)targetSocket);
                        while (len > 0) {
                            if (consume_tokens(&tb, len)) { // ���������㹻������
                                int sent = send(targetSocket, pbuff, len, 0);
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

