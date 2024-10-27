#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <vector>
#include <windows.h>

#pragma comment(lib, "Ws2_32.lib")

#include "SelectServer.cpp"
#include "SelectClient.cpp"

// 定义一个线程来启动服务器
unsigned __stdcall start_server_thread(void* arg) {
    Server* server = static_cast<Server*>(arg);
    server->start();
    return 0;
}

unsigned __stdcall startClient(void *arg) 
{
	Client client;
    client.start();
    return 0;
}

// 测试用例：测试多客户端并发访问
void test_client_server_communication(int threadCount) {
	Server svr;
    HANDLE serverThread = (HANDLE)_beginthreadex(nullptr, 0, start_server_thread, &svr, 0, nullptr);
    if (serverThread == nullptr) {
        std::cerr << "Failed to create server thread" << std::endl;
        return;
    }

    // 给服务器一些时间来启动
    Sleep(1000);

    // 启动多个客户端
	const int THREAD_COUNT = threadCount;
    std::vector<HANDLE> clientThreads;
    for (int i = 0; i < THREAD_COUNT; ++i) {
        HANDLE clientThread = (HANDLE)_beginthreadex(nullptr, 0, startClient, nullptr, 0, nullptr);
        if (clientThread == nullptr) {
            std::cerr << "Failed to create client thread" << std::endl;
            continue;
        }
		//Sleep(10);
        clientThreads.push_back(clientThread);
    }

    // 等待所有客户端线程结束
    for (int i = 0; i < THREAD_COUNT; ++i) {
		HANDLE clientThread = clientThreads[i];
        WaitForSingleObject(clientThread, INFINITE);
        CloseHandle(clientThread);
    }


    // 等待服务器线程结束
	svr.stopServer();
    WaitForSingleObject(serverThread, 10000);
    CloseHandle(serverThread);

	printf("Pass test_client_server_communication.");
}

int main() {
    // 初始化 Winsock
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cerr << "WSAStartup failed: " << result << std::endl;
        return 1;
    }

    // 运行测试用例
    test_client_server_communication(128);
	
    test_client_server_communication(256);

    test_client_server_communication(512);

    // 清理 Winsock
    WSACleanup();

    return 0;
}