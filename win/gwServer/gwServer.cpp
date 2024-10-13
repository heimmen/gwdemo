// gwServer.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"


#include <iostream>
#include <winsock2.h>  // Winsock库
#pragma comment(lib, "ws2_32.lib")  // 加载 Winsock 库

int main() {
    // 初始化 Winsock 库
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Winsock 初始化失败" << std::endl;
        return 1;
    }

    // 创建 UDP 套接字
    SOCKET udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (udpSocket == INVALID_SOCKET) {
        std::cerr << "创建套接字失败" << std::endl;
        WSACleanup();
        return 1;
    }

    // 设置接收端地址信息
    sockaddr_in receiverAddr;
    receiverAddr.sin_family = AF_INET;
    receiverAddr.sin_port = htons(8888);  // 接收端监听端口
    receiverAddr.sin_addr.s_addr = INADDR_ANY;  // 监听所有可用接口

    // 绑定套接字
    if (bind(udpSocket, (sockaddr*)&receiverAddr, sizeof(receiverAddr)) == SOCKET_ERROR) {
        std::cerr << "绑定失败" << std::endl;
        closesocket(udpSocket);
        WSACleanup();
        return 1;
    }

    // 接收数据
    char buffer[1024] = {0};
    sockaddr_in senderAddr;
    int senderAddrSize = sizeof(senderAddr);

    std::cout << "等待数据..." << std::endl;

    int receiveResult = recvfrom(udpSocket, buffer, sizeof(buffer) - 1, 0, (sockaddr*)&senderAddr, &senderAddrSize);
    if (receiveResult == SOCKET_ERROR) {
        std::cerr << "接收数据失败" << std::endl;
        closesocket(udpSocket);
        WSACleanup();
        return 1;
    }

    // 输出接收到的数据
    buffer[receiveResult] = '\0';  // 确保字符串末尾有空终止符
    std::cout << "接收到消息：" << buffer << std::endl;

    // 关闭套接字
    closesocket(udpSocket);

    // 清理 Winsock 库
    WSACleanup();

    return 0;
}


