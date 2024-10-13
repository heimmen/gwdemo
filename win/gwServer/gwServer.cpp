// gwServer.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"


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

    // ���ý��ն˵�ַ��Ϣ
    sockaddr_in receiverAddr;
    receiverAddr.sin_family = AF_INET;
    receiverAddr.sin_port = htons(8888);  // ���ն˼����˿�
    receiverAddr.sin_addr.s_addr = INADDR_ANY;  // �������п��ýӿ�

    // ���׽���
    if (bind(udpSocket, (sockaddr*)&receiverAddr, sizeof(receiverAddr)) == SOCKET_ERROR) {
        std::cerr << "��ʧ��" << std::endl;
        closesocket(udpSocket);
        WSACleanup();
        return 1;
    }

    // ��������
    char buffer[1024] = {0};
    sockaddr_in senderAddr;
    int senderAddrSize = sizeof(senderAddr);

    std::cout << "�ȴ�����..." << std::endl;

    int receiveResult = recvfrom(udpSocket, buffer, sizeof(buffer) - 1, 0, (sockaddr*)&senderAddr, &senderAddrSize);
    if (receiveResult == SOCKET_ERROR) {
        std::cerr << "��������ʧ��" << std::endl;
        closesocket(udpSocket);
        WSACleanup();
        return 1;
    }

    // ������յ�������
    buffer[receiveResult] = '\0';  // ȷ���ַ���ĩβ�п���ֹ��
    std::cout << "���յ���Ϣ��" << buffer << std::endl;

    // �ر��׽���
    closesocket(udpSocket);

    // ���� Winsock ��
    WSACleanup();

    return 0;
}


