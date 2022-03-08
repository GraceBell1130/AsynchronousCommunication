#include "stdafx.h"
#include <iostream>
#include <WinSock2.h>
#include <Windows.h>
#include <string>
#include <sstream>
#include <string_view>
#pragma comment(lib, "ws2_32.lib")

const uint16_t BUF_SIZE{ 1024 };
const uint8_t READ{ 3 };
const uint8_t WRITE{ 5 };
uint8_t THREAD_NUMBER = 0;
CRITICAL_SECTION cs;

class exceptionFormatter : public std::exception
{
	char messageBuffer[1024]{ 0 };
public:
	explicit exceptionFormatter(const char* parameter,...)
	{
		va_list arg;
		va_start(arg, parameter);
		vsnprintf(messageBuffer, 1024, parameter, arg);
		va_end(arg);
	}
	~exceptionFormatter() = default;
	char const* what() const
	{
		return messageBuffer;
	}
};

struct PER_HANDLE_DATA
{
	SOCKET hClntSock{0};
	SOCKADDR_IN clntAdr{0};
	PER_HANDLE_DATA() = default;
	~PER_HANDLE_DATA() = default;
};
typedef PER_HANDLE_DATA* PPER_HANDLE_DATA;
struct PER_IO_DATA
{
	OVERLAPPED overlapped{0};
	WSABUF wsaBuf{0};
	char buffer[BUF_SIZE];
	int rwMode;
	PER_IO_DATA() = default;
	~PER_IO_DATA() = default;
};
using PPER_IO_DATA = PER_IO_DATA;

DWORD WINAPI EchoThreadMain(LPVOID CompletionProtIO) 
{
	HANDLE hComPort = reinterpret_cast<HANDLE>(CompletionProtIO);
	SOCKET sock;
	DWORD bytesTrans;
	PER_HANDLE_DATA* handleInfo = nullptr;
	PER_IO_DATA* ioInfo = nullptr;
	DWORD flags{0};
	EnterCriticalSection(&cs);
	const int threadNumber = THREAD_NUMBER++;
	std::cout << threadNumber << " : start" << std::endl;
	LeaveCriticalSection(&cs);

	while (TRUE)
	{
		GetQueuedCompletionStatus(hComPort, &bytesTrans, (LPDWORD)&handleInfo, (LPOVERLAPPED*)&ioInfo, INFINITE);
		sock = handleInfo->hClntSock;

		if (READ == ioInfo->rwMode)
		{
			std::cout <<  threadNumber << " : message received!" << std::endl;
			if (0 == bytesTrans)
			{
				closesocket(sock);
				delete handleInfo;
				delete ioInfo;
				continue;
			}

			memset(&(ioInfo->overlapped), 0 , sizeof(OVERLAPPED));
			ioInfo->wsaBuf.len = bytesTrans;
			ioInfo->rwMode = WRITE;
			WSASend(sock, &(ioInfo->wsaBuf), 1, NULL, 0 , &(ioInfo->overlapped), NULL);

			ioInfo = new PER_IO_DATA();
			ioInfo->wsaBuf.len = BUF_SIZE;
			ioInfo->wsaBuf.buf = ioInfo->buffer;
			ioInfo->rwMode = READ;
			WSARecv(sock, &(ioInfo->wsaBuf), 1, NULL, &flags, &(ioInfo->overlapped), NULL);
		}
		else
		{
			std::cout << "message sent!" << std::endl;
			delete ioInfo;
		}
	}
	std::cout << threadNumber << "end" << std::endl;
	EnterCriticalSection(&cs);
	THREAD_NUMBER--;
	LeaveCriticalSection(&cs);
	return 0;
}

int main(int argc, char* argv[])
{
	int nReturnCode = EXIT_SUCCESS;
	InitializeCriticalSection(&cs);
	try 
	{
		if (2 != argc)
		{
			throw exceptionFormatter("Usage %s <Port>", argv[0]);
		}
		WSADATA wsaData;
		if (0 != WSAStartup(MAKEWORD(2, 2), &wsaData))
		{
			throw exceptionFormatter("WSAStartup() error!");
		}
		HANDLE hComPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
		SYSTEM_INFO sysInfo;
		GetSystemInfo(&sysInfo);
		for (DWORD i = 0; i < sysInfo.dwNumberOfProcessors; i++)
		{
			CreateThread(NULL, 0, EchoThreadMain, hComPort, 0, NULL);
		}
		SOCKET hServSock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
		SOCKADDR_IN lisnAdr{ 0 };
		lisnAdr.sin_family = AF_INET;
		lisnAdr.sin_addr.s_addr = htonl(INADDR_ANY);
		lisnAdr.sin_port = htons(atoi(argv[1]));

		if (SOCKET_ERROR == bind(hServSock, (SOCKADDR*)&lisnAdr, sizeof(lisnAdr)))
		{
			throw exceptionFormatter("bind() error!");
		}

		if (SOCKET_ERROR == listen(hServSock, 5))
		{
			throw exceptionFormatter("listen() error!");
		}

		PER_HANDLE_DATA* handleInfo = nullptr;
		PER_IO_DATA* ioInfo = nullptr;
		DWORD recvBytes, flags{ 0 };
		while (TRUE)
		{
			SOCKADDR_IN clntAdr;
			int addrLen = sizeof(clntAdr);
			SOCKET hClntSock = accept(hServSock, (SOCKADDR*)&clntAdr, &addrLen);
			handleInfo = new PER_HANDLE_DATA();
			handleInfo->hClntSock = hClntSock;
			handleInfo->clntAdr = clntAdr;

			CreateIoCompletionPort(reinterpret_cast<HANDLE>(handleInfo->hClntSock), hComPort,
				reinterpret_cast<ULONG_PTR>(handleInfo), 0);

			ioInfo = new PER_IO_DATA();
			ioInfo->overlapped = { 0, };
			ioInfo->wsaBuf.len = BUF_SIZE;
			ioInfo->wsaBuf.buf = ioInfo->buffer;
			ioInfo->rwMode = READ;
			WSARecv(handleInfo->hClntSock, &(ioInfo->wsaBuf), 1, &recvBytes, &flags, &(ioInfo->overlapped), NULL);
		}
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		nReturnCode = EXIT_FAILURE;
	}
	DeleteCriticalSection(&cs);
	exit(nReturnCode);
}