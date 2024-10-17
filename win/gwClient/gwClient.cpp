// gwClient.cpp : �������̨Ӧ�ó������ڵ㡣
//
//
#include "stdafx.h"
//
//
//int _tmain(int argc, _TCHAR* argv[])
//{
//	return 0;
//}
//
#include <iostream>
#include <winsock2.h>  // Winsock��
#pragma comment(lib, "ws2_32.lib")  // ���� Winsock ��

int main() {
    // ��ʼ�� Winsock ��
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Winsock ��ʼ��ʧ��" << std::endl;
        return 1;
    }

    // ���� UDP �׽���
    SOCKET udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (udpSocket == INVALID_SOCKET) {
        std::cerr << "�����׽���ʧ��" << std::endl;
        WSACleanup();
        return 1;
    }

    // Ŀ���ַ��Ϣ�����շ��� IP �Ͷ˿ڣ�
    sockaddr_in receiverAddr;
    receiverAddr.sin_family = AF_INET;
    receiverAddr.sin_port = htons(8888);  // Ŀ��˿�
    receiverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");  // Ŀ�� IP ��ַ�����أ�

    // Ҫ���͵�����
    const char* message = "Hello, this is a UDP message!";
    
    // �������ݵ�Ŀ���ַ
    int sendResult = sendto(udpSocket, message, strlen(message), 0, (sockaddr*)&receiverAddr, sizeof(receiverAddr));
    if (sendResult == SOCKET_ERROR) {
        std::cerr << "��������ʧ��" << std::endl;
        closesocket(udpSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "��Ϣ���ͳɹ���" << std::endl;

    // �ر��׽���
    closesocket(udpSocket);

    // ���� Winsock ��
    WSACleanup();

    return 0;
}
