#include "stdafx.h"
#include <WinSock2.h>
#include <Windows.h>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>
#pragma comment(lib, "ws2_32.lib")
const uint16_t BUF_SIZE{1024};
void ErrorHandling(std::string_view msg)
{
	std::cerr << msg.data() << std::endl;
	exit(1);
}

int main(int argc, char* argv[])
{
	if (3 != argc)
	{
		std::cout << "Usage: " << argv[0] << "<IP> <PORT>" << std::endl;
		exit(1);
	}

	WSADATA wsaData;
	if (0 != WSAStartup(MAKEWORD(2, 2), &wsaData))
	{
		ErrorHandling("WSAStartup() error!");
	}

	SOCKET hSocket = socket(PF_INET, SOCK_STREAM, 0);
	SOCKADDR_IN sendAdr{ 0 };
	sendAdr.sin_family = AF_INET;
	sendAdr.sin_addr.S_un.S_addr = inet_addr(argv[1]);
	sendAdr.sin_port = htons(atoi(argv[2]));

	if (SOCKET_ERROR == connect(hSocket, (SOCKADDR*)&sendAdr, sizeof(sendAdr)))
	{
		ErrorHandling("connect() error!");
	}
	else
	{
		std::cout << "Connected........" << std::endl;
	}

	char message[BUF_SIZE];
	int strLen, readLen;
	while (TRUE)
	{
		std::cout << "Input message (Q to quit) : ";
		fgets(message, BUF_SIZE, stdin);
		if (!strcmp(message, "q\n") || !strcmp(message, "Q\n"))
		{
			break;
		}

		strLen = strlen(message);
		send(hSocket, message, strLen, 0);
		readLen = 0;
		while (TRUE)
		{
			readLen += recv(hSocket, &message[readLen], BUF_SIZE - 1, 0);
			std::cout << readLen << std::endl;
			if (readLen >= strLen)
			{
				break;
			}
		}
		message[strLen] = 0;
		std::cout << "Message from server : " << message << std::endl;
	}

	closesocket(hSocket);
	WSACleanup();
	return 0;
}
