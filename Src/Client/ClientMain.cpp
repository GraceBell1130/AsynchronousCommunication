#include "stdafx.h"
#include <WinSock2.h>
#include <Windows.h>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>
#pragma comment(lib, "ws2_32.lib")

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

	char msg[] = "Network is computer!";

	WSAEVENT evObj;

	if (0 != WSAStartup(MAKEWORD(2, 2), &wsaData))
	{
		ErrorHandling("WSAStartup() error!");
	}

	SOCKET hSocket = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	SOCKADDR_IN sendAdr{ 0 };
	sendAdr.sin_family = AF_INET;
	sendAdr.sin_addr.S_un.S_addr = inet_addr(argv[1]);
	sendAdr.sin_port = htons(atoi(argv[2]));

	if (SOCKET_ERROR == connect(hSocket, (SOCKADDR*)&sendAdr, sizeof(sendAdr)))
	{
		ErrorHandling("connect() error!");
	}

	evObj = WSACreateEvent();
	WSAOVERLAPPED overlapped{ 0 };
	overlapped.hEvent = evObj;
	WSABUF dataBuf;
	dataBuf.len = strlen(msg) + 1;
	dataBuf.buf = msg;

	DWORD sendBytes = 0;
	if (SOCKET_ERROR == WSASend(hSocket, &dataBuf, 1, &sendBytes, 0, &overlapped, NULL))
	{
		if (WSA_IO_PENDING == WSAGetLastError())
		{
			std::cout << "Background data send" << std::endl;
			WSAWaitForMultipleEvents(1, &evObj, TRUE, WSA_INFINITE, FALSE);
			WSAGetOverlappedResult(hSocket, &overlapped, &sendBytes, FALSE, NULL);
		}
		else
		{
			ErrorHandling("WSASend() error");
		}
	}

	std::cout << "Send data size : " << sendBytes << std::endl;
	WSACloseEvent(evObj);
	closesocket(hSocket);
	WSACleanup();

	return 0;

}
