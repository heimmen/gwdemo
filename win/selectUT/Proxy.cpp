#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <vector>
#include <windows.h>
#include <string>

#pragma comment(lib, "Ws2_32.lib")

#define PROXY_PORT 12345
#define SERVER_PORT 12346
#define MAX_CLIENTS 16
#define BUFFER_SIZE 1024
using namespace std;

class Proxy
{
private:
    bool m_toStop;

public:
    void stopServer()
    {
        m_toStop = true;
    }
    void start()
    {
        m_toStop = false;
        WSADATA wsaData;
        int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (result != 0)
        {
            std::cerr << "WSAStartup failed: " << result << std::endl;
            return;
        }

        SOCKET proxySocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (proxySocket == INVALID_SOCKET)
        {
            std::cerr << "Socket creation failed: " << WSAGetLastError() << std::endl;
            WSACleanup();
            return;
        }

        struct sockaddr_in proxyAddr;
        proxyAddr.sin_family = AF_INET;
        proxyAddr.sin_port = htons(PROXY_PORT);
        proxyAddr.sin_addr.s_addr = INADDR_ANY;

        if (bind(proxySocket, (struct sockaddr *)&proxyAddr, sizeof(proxyAddr)) == SOCKET_ERROR)
        {
            std::cerr << "Bind proxy socket address failed: " << WSAGetLastError() << std::endl;
            closesocket(proxySocket);
            WSACleanup();
            return;
        }

        if (listen(proxySocket, MAX_CLIENTS) == SOCKET_ERROR)
        {
            std::cerr << "Listen proxy socket failed: " << WSAGetLastError() << std::endl;
            closesocket(proxySocket);
            WSACleanup();
            return;
        }

        std::vector<SOCKET> clientSockets;
        fd_set masterfds;
        FD_ZERO(&masterfds);
        FD_SET(proxySocket, &masterfds);

        SOCKET maxSocket = proxySocket;
        SOCKET serverSocket = INVALID_SOCKET;

        while (true && !m_toStop)
        {
            fd_set readfds = masterfds;
            if (select(0, &readfds, nullptr, nullptr, nullptr) == SOCKET_ERROR)
            {
                std::cerr << "Select failed: " << WSAGetLastError() << std::endl;
                break;
            }

            for (int i = 0; i <= maxSocket; ++i)
            {
                if (FD_ISSET(i, &readfds))
                {
                    if (i == proxySocket)
                    {
                        SOCKET clientSocket = accept(proxySocket, nullptr, nullptr);
                        if (clientSocket == INVALID_SOCKET)
                        {
                            std::cerr << "Accept failed: " << WSAGetLastError() << std::endl;
                            continue;
                        }
                        FD_SET(clientSocket, &masterfds);
                        if (clientSocket > maxSocket)
                        {
                            maxSocket = clientSocket;
                        }
                        std::cout << "PROXY: New client connected: " << clientSocket << std::endl;
                    }
                    else
                    {
                        char buffer[BUFFER_SIZE] = {0};
                        int bytesReceived = recv(i, buffer, BUFFER_SIZE, 0);
                        if (bytesReceived <= 0)
                        {
                            std::cout << "Client disconnected: " << i << std::endl;
                            closesocket(i);
                            FD_CLR(i, &masterfds);
                        }
                        else
                        {
                            if (serverSocket == INVALID_SOCKET)
                            {
                                // Connect to server if not yet
                                serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
                                if (serverSocket == INVALID_SOCKET)
                                {
                                    printf("Error at socket(): %ld\n", WSAGetLastError());
                                    closesocket(i);
                                    FD_CLR(i, &masterfds);
                                    break;
                                }

                                struct sockaddr_in serverAddr;
                                serverAddr.sin_family = AF_INET;
                                serverAddr.sin_port = htons(SERVER_PORT);
                                inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

                                if (connect(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
                                {
                                    printf("Connect failed: %d\n", WSAGetLastError());
                                    closesocket(i);
                                    closesocket(serverSocket);
                                    FD_CLR(i, &masterfds);
                                    break;
                                }

                                printf("Connected to server socket: %llu\n", (unsigned long long)serverSocket);

                                // client_to_server_map[i] = serverSocket;
                                FD_SET(serverSocket, &masterfds);
                                if (serverSocket > maxSocket)
                                {
                                    maxSocket = serverSocket;
                                }
                            }
                            std::cout << "PROXY: Message received from client " << i << ": " << buffer << std::endl;
                            const char *response = "Hello from server";
                            string forwardContent = "Message forwarded from proxy: " + string(buffer);

                            // Forward message to the server
                            int sent = send(serverSocket, forwardContent.c_str(), forwardContent.length(), 0);
                            if (sent == SOCKET_ERROR)
                            {
                                std::cerr << "Send message to server failed: " << WSAGetLastError() << std::endl;
                            }
                            // TODO: Receive server response and send it back to client.
                            sent = send(i, response, strlen(response), 0);
                            if (sent == SOCKET_ERROR)
                            {
                                std::cerr << "Send message to client failed: " << WSAGetLastError() << std::endl;
                            }
                        }
                    }
                }
            }
        }

        for (int i = 0; i < clientSockets.size(); ++i)
        {
            SOCKET clientSocket = clientSockets[i];
            closesocket(clientSocket);
        }
        closesocket(proxySocket);
        closesocket(serverSocket);
        WSACleanup();
    }
};
