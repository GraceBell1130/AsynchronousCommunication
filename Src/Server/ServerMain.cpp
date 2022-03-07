#include "stdafx.h"
#include <WinSock2.h>
#include <Windows.h>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>
#pragma comment(lib, "ws2_32.lib")

const uint8_t BUF_SIZE = 100;
void CompressSockets(SOCKET hSocketArray[], int idx, int total)
{
	for (int i = idx; i < total; i++)
	{
		hSocketArray[i] = hSocketArray[i + 1];
	}
}
void CompressEvents(WSAEVENT hEventArray[], int idx, int total)
{
	for (int i = idx; i < total; i++)
	{
		hEventArray[i] = hEventArray[i + 1];
	}
}
void ErrorHandling(std::string_view strMsg)
{
	std::cerr << strMsg << std::endl;
	exit(1);
}

int main(int argc, char* argv[])
{
	SOCKET hServerSocket, hClientSocket;

	if (argc != 2)
	{
		std::cout << "Usage " << argv[0] << " <port>" << std::endl;
		exit(1);
	}

	WSADATA wsaData;
	if (0 != WSAStartup(MAKEWORD(2, 2), &wsaData))
	{
		ErrorHandling("WSAStartup() error");
	}

	hServerSocket = socket(PF_INET, SOCK_STREAM, 0);
	SOCKADDR_IN serverAddr{ 0 }, clinetAddr{0};
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddr.sin_port = htons(atoi(argv[1]));

	if (SOCKET_ERROR == bind(hServerSocket, (SOCKADDR*) & serverAddr, sizeof(serverAddr)))
	{
		ErrorHandling("bind() error");
	}

	if (SOCKET_ERROR == listen(hServerSocket, 5))
	{
		ErrorHandling("listen() error");
	}

	WSAEVENT newEvent = WSACreateEvent();
	if (SOCKET_ERROR == WSAEventSelect(hServerSocket, newEvent, FD_ACCEPT))
	{
		ErrorHandling("WSAEventSelect() error");
	}

	int numOfClinetSocket = 0;
	SOCKET hSocketArray[WSA_MAXIMUM_WAIT_EVENTS] = {hServerSocket,};
	WSAEVENT hEventArray[WSA_MAXIMUM_WAIT_EVENTS] = {newEvent,};
	numOfClinetSocket++;

	WSANETWORKEVENTS networkEvenvts;
	int strLen;
	DWORD posInfo, startIndex;
	int clientAddrLen;
	std::string msg(BUF_SIZE, 0);

	while (true)
	{
		posInfo = WSAWaitForMultipleEvents(numOfClinetSocket, hEventArray, FALSE, WSA_INFINITE, FALSE);
		startIndex = posInfo - WSA_WAIT_EVENT_0;

		for (int i = startIndex; i < numOfClinetSocket; i++)
		{
			int signalEventIndex = WSAWaitForMultipleEvents(1, &hEventArray[i], TRUE, 0, FALSE);
			if (WSA_WAIT_FAILED == signalEventIndex || WSA_WAIT_TIMEOUT == signalEventIndex)
			{
				continue;
			}
			else
			{
				signalEventIndex = i;
				WSAEnumNetworkEvents(hSocketArray[signalEventIndex], hEventArray[signalEventIndex], &networkEvenvts);
				if (FD_ACCEPT & networkEvenvts.lNetworkEvents)
				{
					if (0 != networkEvenvts.iErrorCode[FD_ACCEPT_BIT])
					{
						std::cout << "Accept Error" << std::endl;
						break;
					}
					clientAddrLen = sizeof(clinetAddr);
					hClientSocket = accept(hSocketArray[signalEventIndex], (SOCKADDR*)&clinetAddr, &clientAddrLen);
					newEvent = WSACreateEvent();
					WSAEventSelect(hClientSocket, newEvent, FD_READ | FD_CLOSE);
					hEventArray[numOfClinetSocket] = newEvent;
					hSocketArray[numOfClinetSocket] = hClientSocket;
					numOfClinetSocket++;
					std::cout << "connected new client..." << std::endl;
				}

				if (FD_READ & networkEvenvts.lNetworkEvents)
				{
					if (0 != networkEvenvts.iErrorCode[FD_READ_BIT])
					{
						std::cout << "Read Error" << std::endl;
						break;
					}
					strLen = recv(hSocketArray[signalEventIndex], msg.data(), msg.size(), 0);
					send(hSocketArray[signalEventIndex], msg.data(), strLen, 0);
				}

				if (FD_CLOSE & networkEvenvts.lNetworkEvents)
				{
					if (0 != networkEvenvts.iErrorCode[FD_CLOSE_BIT])
					{
						std::cout << "Close Error" << std::endl;
						break;
					}
					WSACloseEvent(hEventArray[signalEventIndex]);
					closesocket(hSocketArray[signalEventIndex]);
					numOfClinetSocket--;
					CompressSockets(hSocketArray, signalEventIndex, numOfClinetSocket);
					CompressEvents(hEventArray, signalEventIndex, numOfClinetSocket);
				}
			}
		}
	}
	WSACleanup();
	exit(0);
}