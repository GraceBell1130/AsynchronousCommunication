#include "stdafx.h"
#include <iostream>
#include <WinSock2.h>
#include <Windows.h>
#include <string>
#include <string_view>
#pragma comment(lib, "ws2_32.lib")

const uint16_t BUF_SIZE{ 1024 };
typedef struct
{
	SOCKET hClntSock;
	char buf[BUF_SIZE];
	WSABUF wsaBuf;
} PER_IO_DATA, * LPPER_IO_DATA;

void ErrorHandling(std::string_view strMsg)
{
	std::cerr << strMsg << std::endl;
	exit(1);
}
void CALLBACK WriteCompRoutine(DWORD dwError, DWORD szRecvBytes, LPWSAOVERLAPPED lpOverlapped, DWORD flags);
void CALLBACK ReadCompRoutine(DWORD dwError, DWORD szRecvBytes, LPWSAOVERLAPPED lpOverlapped, DWORD flags)
{
	LPPER_IO_DATA hbInfo = (LPPER_IO_DATA)(lpOverlapped->hEvent);
	SOCKET hSock = hbInfo->hClntSock;
	LPWSABUF bufInfo = &(hbInfo->wsaBuf);
	DWORD sendBytes;

	if (0 == szRecvBytes)
	{
		closesocket(hSock);
		free(lpOverlapped->hEvent);
		free(lpOverlapped);
		std::cout << "Client disconncted...." << std::endl;
	}
	else
	{
		bufInfo->len = szRecvBytes;
		WSASend(hSock, bufInfo, 1, &sendBytes, 0, lpOverlapped, WriteCompRoutine);
	}
}
void CALLBACK WriteCompRoutine(DWORD dwError, DWORD szRecvBytes, LPWSAOVERLAPPED lpOverlapped, DWORD flags)
{
	LPPER_IO_DATA hbInfo = (LPPER_IO_DATA)(lpOverlapped->hEvent);
	SOCKET hSock = hbInfo->hClntSock;
	LPWSABUF bufInfo = &(hbInfo->wsaBuf);
	DWORD recvBytes;
	DWORD flagInfo = 0;
	WSARecv(hSock, bufInfo, 1, &recvBytes, &flagInfo, lpOverlapped, ReadCompRoutine);
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
	u_long mode = 1;
	ioctlsocket(hLisnSock, FIONBIO, &mode);
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
	SOCKET hRecvSock;
	LPWSAOVERLAPPED lpOvLp;
	LPPER_IO_DATA hbInfo;
	DWORD flagInfo=0;
	DWORD recvBytes;

	int recvAdrSz = sizeof(recvAdr);
	while (TRUE)
	{
		SleepEx(100, TRUE);
		hRecvSock = accept(hLisnSock, (SOCKADDR*)&recvAdr, &recvAdrSz);
		if (INVALID_SOCKET == hRecvSock)
		{
			if (WSAEWOULDBLOCK == WSAGetLastError())
			{
				continue;
			}
			else
			{
				ErrorHandling("accept() error");
			}
		}
		std::cout << "Client conneted....." << std::endl;
		lpOvLp = (LPWSAOVERLAPPED)malloc(sizeof(WSAOVERLAPPED));
		memset(lpOvLp, 0, sizeof(WSAOVERLAPPED));

		hbInfo = (LPPER_IO_DATA)malloc(sizeof(PER_IO_DATA));
		hbInfo->hClntSock = (DWORD)hRecvSock;
		hbInfo->wsaBuf.buf = hbInfo->buf;
		hbInfo->wsaBuf.len = BUF_SIZE;

		lpOvLp->hEvent = (HANDLE)hbInfo;
		WSARecv(hRecvSock, &(hbInfo->wsaBuf), 1, &recvBytes, &flagInfo, lpOvLp, ReadCompRoutine);
	}
	closesocket(hRecvSock);
	closesocket(hLisnSock);
	WSACleanup();
	return 0;
}