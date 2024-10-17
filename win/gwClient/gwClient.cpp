// gwClient.cpp : 定义控制台应用程序的入口点。
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

    // 目标地址信息（接收方的 IP 和端口）
    sockaddr_in receiverAddr;
    receiverAddr.sin_family = AF_INET;
    receiverAddr.sin_port = htons(8888);  // 目标端口
    receiverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");  // 目标 IP 地址（本地）

    // 要发送的数据
    const char* message = "Hello, this is a UDP message!";
    
    // 发送数据到目标地址
    int sendResult = sendto(udpSocket, message, strlen(message), 0, (sockaddr*)&receiverAddr, sizeof(receiverAddr));
    if (sendResult == SOCKET_ERROR) {
        std::cerr << "发送数据失败" << std::endl;
        closesocket(udpSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "消息发送成功！" << std::endl;

    // 关闭套接字
    closesocket(udpSocket);

    // 清理 Winsock 库
    WSACleanup();

    return 0;
}
