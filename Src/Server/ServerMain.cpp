#include "stdafx.h"
#include <iostream>
#include <WinSock2.h>
#include <Windows.h>
#include <string>
#include <string_view>
#pragma comment(lib, "ws2_32.lib")

const uint16_t BUF_SIZE{ 1024 };
WSABUF dataBuf;
char buf[BUF_SIZE];
int recvBytes = 0;

void ErrorHandling(std::string_view strMsg)
{
	std::cerr << strMsg << std::endl;
	exit(1);
}

void CALLBACK CompRoutine(DWORD dwError, DWORD szRecvBytes, LPWSAOVERLAPPED lpOverlapped, DWORD flags)
{
	if (0 != dwError)
	{
		ErrorHandling("CompRoutine error");
	}
	else
	{
		recvBytes = szRecvBytes;
		std::cout << "Received message : " << buf << std::endl;
	}
}

int main(int argc, char* argv[])
{
	if (2 != argc)
	{
		std::cout << "Usage " << argv[0] << " <PORT>" << std::endl;
		exit(1);
	}

	WSADATA wsaData;
	if (0 != WSAStartup(MAKEWORD(2, 2), &wsaData))
	{
		ErrorHandling("WSAStartup() error!");
	}

	SOCKET hLisnSock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET == hLisnSock)
	{
		ErrorHandling("socket() error!");
	}

	SOCKADDR_IN lisnAdr{ 0 };
	lisnAdr.sin_family = AF_INET;
	lisnAdr.sin_addr.s_addr = htonl(INADDR_ANY);
	lisnAdr.sin_port = htons(atoi(argv[1]));

	if (SOCKET_ERROR == bind(hLisnSock, (SOCKADDR*)&lisnAdr, sizeof(lisnAdr)))
	{
		ErrorHandling("bind() error!");
	}

	if (SOCKET_ERROR == listen(hLisnSock, 5))
	{
		ErrorHandling("listen() error!");
	}

	SOCKADDR_IN recvAdr;
	int recvAdrSz = sizeof(recvAdr);
	SOCKET hRecvSock = accept(hLisnSock, (SOCKADDR*)&recvAdr, &recvAdrSz);
	if (INVALID_SOCKET == hRecvSock)
	{
		ErrorHandling("accept() error");
	}
	WSAEVENT evObj = WSACreateEvent();
	WSAOVERLAPPED overlapped{ 0 };
	overlapped.hEvent = evObj;

	WSABUF dataBuf;
	dataBuf.len = BUF_SIZE;
	dataBuf.buf = buf;

	DWORD recvBytes{ 0 }, flags{ 0 };
	if (SOCKET_ERROR == WSARecv(hRecvSock, &dataBuf, 1, &recvBytes,  &flags, &overlapped, CompRoutine))
	{
		if (WSA_IO_PENDING == WSAGetLastError())
		{
			std::cout << "Background data receive" << std::endl;
		}
	}

	int idx = WSAWaitForMultipleEvents(1, &evObj, FALSE, WSA_INFINITE, TRUE);
	if (WAIT_IO_COMPLETION == idx)
	{
		std::cout << "Overlapped I/O Completed" << std::endl;
	}
	else
	{
		ErrorHandling("WSARecv() error");
	}
	WSACloseEvent(evObj);
	closesocket(hRecvSock);
	closesocket(hLisnSock);
	WSACleanup();
	return 0;
}