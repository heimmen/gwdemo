#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <vector>
#include <windows.h>
#include <string>

#pragma comment(lib, "Ws2_32.lib")

#define PORT 12346
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

class Server {
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

        SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (serverSocket == INVALID_SOCKET) {
            std::cerr << "Socket creation failed: " << WSAGetLastError() << std::endl;
            WSACleanup();
            return;
        }

        struct sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(PORT);
        serverAddr.sin_addr.s_addr = INADDR_ANY;

        if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
            std::cerr << "Bind failed: " << WSAGetLastError() << std::endl;
            closesocket(serverSocket);
            WSACleanup();
            return;
        }

        if (listen(serverSocket, MAX_CLIENTS) == SOCKET_ERROR) {
            std::cerr << "Listen failed: " << WSAGetLastError() << std::endl;
            closesocket(serverSocket);
            WSACleanup();
            return;
        }

        std::vector<SOCKET> clientSockets;
        fd_set master;
        FD_ZERO(&master);
        FD_SET(serverSocket, &master);

        SOCKET maxSocket = serverSocket;

        while (true && !m_toStop) {
            fd_set readfds = master;
            if (select(0, &readfds, nullptr, nullptr, nullptr) == SOCKET_ERROR) {
                std::cerr << "Select failed: " << WSAGetLastError() << std::endl;
                break;
            }

            for (int i = 0; i <= maxSocket; ++i) {
                if (FD_ISSET(i, &readfds)) {
                    if (i == serverSocket) {
                        SOCKET clientSocket = accept(serverSocket, nullptr, nullptr);
                        if (clientSocket == INVALID_SOCKET) {
                            std::cerr << "Accept failed: " << WSAGetLastError() << std::endl;
                            continue;
                        }
                        FD_SET(clientSocket, &master);
                        if (clientSocket > maxSocket) {
                            maxSocket = clientSocket;
                        }
                        std::cout << "Server: New client connected: " << clientSocket << std::endl;
                    } else {
                        char buffer[BUFFER_SIZE] = {0};
                        int bytesReceived = recv(i, buffer, BUFFER_SIZE, 0);
                        if (bytesReceived <= 0) {
                            std::cout << "Server: Client disconnected: " << i << std::endl;
                            closesocket(i);
                            FD_CLR(i, &master);
                        } else {
                            std::cout << "Server: Message received from " << i << ": " << buffer << std::endl;
                            const char *response = "Hello response from server";
							//std::string serverResponse = "Message reply from server for: " + ss;
							int sent = send(i, response, strlen(response), 0);
                            if (sent == SOCKET_ERROR)
                            {
                                std::cerr << "Send failed: " << WSAGetLastError() << std::endl;
                            }
                        }
                    }
                }
            }
        }

        //for (auto& clientSocket : clientSockets) {
        //    closesocket(clientSocket);
        //}
		for (int i = 0; i < clientSockets.size(); ++i) {
			SOCKET clientSocket = clientSockets[i];
			//WaitForSingleObject(clientThread, INFINITE);
			closesocket(clientSocket);
		}
        closesocket(serverSocket);
        WSACleanup();
    }
};