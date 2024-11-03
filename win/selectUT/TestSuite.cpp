#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <vector>
#include <windows.h>

#pragma comment(lib, "Ws2_32.lib")

#include "SelectServer.cpp"
#include "SelectClient.cpp"
#include "Proxy.cpp"

// ����һ���߳�������������
unsigned __stdcall start_server_thread(void* arg) {
    Server* server = static_cast<Server*>(arg);
    server->start();
    return 0;
}

// ����һ���߳�������������
unsigned __stdcall start_proxy_thread(void* arg) {
    Proxy* pProxy = static_cast<Proxy*>(arg);
    pProxy->start();
    return 0;
}

unsigned __stdcall startClient(void *arg) 
{
	Client client;
    client.start();
    return 0;
}

// �������������Զ�ͻ��˲�������
void test_client_server_communication(int threadCount) {
    /*
    client -- 12345 --> proxy -- 12346 --> server
    */

	Server svr;
    HANDLE serverThread = (HANDLE)_beginthreadex(nullptr, 0, start_server_thread, &svr, 0, nullptr);
    if (serverThread == nullptr) {
        std::cerr << "Failed to create server thread" << std::endl;
        return;
    }

    Proxy proxy;
    HANDLE proxyThread = (HANDLE)_beginthreadex(nullptr, 0, start_proxy_thread, &proxy, 0, nullptr);
    if (proxyThread == nullptr) {
        std::cerr << "Failed to create server thread" << std::endl;
        return;
    }

    // �� Proxy һЩʱ��������
    Sleep(1000);

    // ��������ͻ���
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

    // �ȴ����пͻ����߳̽���
    for (int i = 0; i < THREAD_COUNT; ++i) {
		HANDLE clientThread = clientThreads[i];
        WaitForSingleObject(clientThread, INFINITE);
        CloseHandle(clientThread);
    }

    // �ȴ��������߳̽���
	svr.stopServer();
    WaitForSingleObject(serverThread, 1000);
    CloseHandle(serverThread);

    // �ȴ� proxy �߳̽���
	proxy.stopServer();
    WaitForSingleObject(proxyThread, 1000);
    CloseHandle(proxyThread);

	printf("Pass test_client_server_communication.");
}

int main() {
    // ��ʼ�� Winsock
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cerr << "WSAStartup failed: " << result << std::endl;
        return 1;
    }

    // ���в�������
    test_client_server_communication(2);

    // test_client_server_communication(128);
	
    //test_client_server_communication(256);

    //test_client_server_communication(512);

    // ���� Winsock
    WSACleanup();

    return 0;
}