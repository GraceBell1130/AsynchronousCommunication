#include "stdafx.h"
#include <iostream>
#include <WinSock2.h>
#include <Windows.h>
#include <string>
#include <string_view>
#pragma comment(lib, "ws2_32.lib")

const uint16_t BUF_SIZE{ 1024 };

void ErrorHandling(std::string_view strMsg)
{
	std::cerr << strMsg << std::endl;
	exit(1);
}

int main(int argc, char* argv[])
{
	if (3 != argc)
	{
		std::cout << "Usage " << argv[0] << "<IP> " << "<PORT>" << std::endl;
		exit(1);
	}

	WSADATA wsaData;
	if (0 != WSAStartup(MAKEWORD(2,2), &wsaData))
	{
		ErrorHandling("WSAStartup() error!");
	}

	SOCKET hSocket = socket(PF_INET, SOCK_STREAM, 0);
	if (INVALID_SOCKET == hSocket)
	{
		ErrorHandling("socket() error!");
	}

	SOCKADDR_IN serverAddr {0};
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = inet_addr(argv[1]);
	serverAddr.sin_port= htons(atoi(argv[2]));

	if (SOCKET_ERROR == connect(hSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)))
	{
		ErrorHandling("connect() error!");
	}
	else
	{
		std::cout << "Connected.........." << std::endl;
	}

	char message[BUF_SIZE];
	int strLen;
	while (true)
	{
		std::cout << "Input message(Q to quit): ";
		fgets(message, BUF_SIZE, stdin);
		
		if (!strcmp(message, "q\n") || !strcmp(message, "Q\n"))
		{
			break;
		}
		
		send(hSocket, message, strlen(message), 0);
		strLen = recv(hSocket, message, BUF_SIZE - 1, 0);
		message[strLen] = 0;
		std::cout << "Message from Server : " << message << std::endl;
	}

	closesocket(hSocket);
	WSACleanup();
	return 0;
}