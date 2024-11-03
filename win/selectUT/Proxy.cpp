#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <vector>
#include <windows.h>

#pragma comment(lib, "Ws2_32.lib")

#define PORT 12345
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

class Proxy {
private:
	bool m_toStop;
public:
	void stopServer()
	{
		m_toStop = true;
	}
    void start() {
		m_toStop = false;
        WSADATA wsaData;
        int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (result != 0) {
            std::cerr << "WSAStartup failed: " << result << std::endl;
            return;
        }

        SOCKET proxySocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (proxySocket == INVALID_SOCKET) {
            std::cerr << "Socket creation failed: " << WSAGetLastError() << std::endl;
            WSACleanup();
            return;
        }

        struct sockaddr_in proxyAddr;
        proxyAddr.sin_family = AF_INET;
        proxyAddr.sin_port = htons(PORT);
        proxyAddr.sin_addr.s_addr = INADDR_ANY;

        if (bind(proxySocket, (struct sockaddr*)&proxyAddr, sizeof(proxyAddr)) == SOCKET_ERROR) {
            std::cerr << "Bind proxy socket address failed: " << WSAGetLastError() << std::endl;
            closesocket(proxySocket);
            WSACleanup();
            return;
        }

        if (listen(proxySocket, MAX_CLIENTS) == SOCKET_ERROR) {
            std::cerr << "Listen proxy socket failed: " << WSAGetLastError() << std::endl;
            closesocket(proxySocket);
            WSACleanup();
            return;
        }

        std::vector<SOCKET> clientSockets;
        fd_set master;
        FD_ZERO(&master);
        FD_SET(proxySocket, &master);

        SOCKET maxSocket = proxySocket;

        while (true && !m_toStop) {
            fd_set readfds = master;
            if (select(0, &readfds, nullptr, nullptr, nullptr) == SOCKET_ERROR) {
                std::cerr << "Select failed: " << WSAGetLastError() << std::endl;
                break;
            }

            for (int i = 0; i <= maxSocket; ++i) {
                if (FD_ISSET(i, &readfds)) {
                    if (i == proxySocket) {
                        SOCKET clientSocket = accept(proxySocket, nullptr, nullptr);
                        if (clientSocket == INVALID_SOCKET) {
                            std::cerr << "Accept failed: " << WSAGetLastError() << std::endl;
                            continue;
                        }
                        FD_SET(clientSocket, &master);
                        if (clientSocket > maxSocket) {
                            maxSocket = clientSocket;
                        }
                        std::cout << "PROXY: New client connected: " << clientSocket << std::endl;
                    } else {
                        char buffer[BUFFER_SIZE] = {0};
                        int bytesReceived = recv(i, buffer, BUFFER_SIZE, 0);
                        if (bytesReceived <= 0) {
                            std::cout << "Client disconnected: " << i << std::endl;
                            closesocket(i);
                            FD_CLR(i, &master);
                        } else {
                            std::cout << "Message received from " << i << ": " << buffer << std::endl;
                            const char* response = "Hello from server";
                            int sent = send(i, response, strlen(response), 0);
                            if (sent == SOCKET_ERROR) {
                                std::cerr << "Send failed: " << WSAGetLastError() << std::endl;
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
        WSACleanup();
    }
};

