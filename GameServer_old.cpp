#include "GameServer.h"

#include <sys\types.h>
#include <sys\stat.h>
#include <mmsystem.h>


CGameServer::CGameServer()
{
	_portNum = 0;
	_isRun = true;
	InitSocketLayer();

	for (int i = 0; i < 100; i++)
	{
		ClientInfo info;
		_clientLists.push_back(info);
	}

	ClientIter iter;
	iter = _clientLists.begin();
	while (iter != _clientLists.end())
	{
		_clientPools.push(iter);
		++iter;
	}
}
CGameServer::~CGameServer()
{
	_isRun = false;

	while (!_clientLists.empty())
		_clientPools.pop();

	// ClientLists를 순회하며 연결 해제
	ClientIter iter;
	iter = _clientLists.begin();
	while (iter != _clientLists.end())
	{
		if (iter->_isConnected)
		{
			WaitForSingleObject(iter->_clientHandle, INFINITE);
			CloseHandle(iter->_clientHandle);
			closesocket(iter->_clientSock);
			++iter;
		}
	}
	WaitForSingleObject(_listenHandle, INFINITE);
	CloseHandle(_listenHandle);
	closesocket(_serverSock);
	CloseSocketLayer();
}

int CGameServer::InitSocketLayer()
{
	int retval = 0;

	printf("...[Server]...\n");

	// WinSock 버전 2.2 설정
	WORD ver_request = MAKEWORD(2, 2);
	WSADATA wsa_data;

	if (WSAStartup(ver_request, &wsa_data) != 0)
	{
		printf("[ERR] WSAStartup Error\n");
		return -1;
	}

	return 0;
}
void CGameServer::CloseSocketLayer()
{
	WSACleanup();
}

void CGameServer::Wait()
{
	WaitForSingleObject(_listenHandle, INFINITE);
}
void CGameServer::Listen(int port)
{
	_portNum = port;
	_listenHandle = (HANDLE)_beginthreadex(NULL, 0, CGameServer::ListenThread, this, 0, NULL);
	if (!_listenHandle)
	{
		printf("[ERR] Failed to create thread\n");
		return;
	}
}

UINT WINAPI CGameServer::ListenThread(LPVOID p)
{
	CGameServer* server;
	server = (CGameServer*)p;

	server->_serverSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (server->_serverSock == INVALID_SOCKET)
	{
		WSACleanup();
		return -1;
	}

	sockaddr_in service;
	service.sin_family = AF_INET;
	service.sin_addr.s_addr = htonl(INADDR_ANY);
	service.sin_port = htons(server->_portNum);

	if (bind(server->_serverSock, (SOCKADDR*)&service, sizeof(service)) == SOCKET_ERROR)
	{
		printf("[ERR] Binding Error\n");
		return -1;
	}

	if (listen(server->_serverSock, 5) == SOCKET_ERROR)
	{
		printf("[ERR] Listening Error\n");
		return -1;
	}

	while (server->_isRun)
	{
		SOCKET connected_sock = accept(server->_serverSock, NULL, NULL);
		if (connected_sock > 0)
		{
			// Connection Pooling 확인
			if (server->_clientPools.empty())
			{
				closesocket(connected_sock);
				continue;
			}
			else
			{
				// 연결 완료
				ClientIter iter;
				iter = server->_clientPools.top();
				server->_clientPools.pop();

				iter->_clientSock = connected_sock;
				server->_lastSock = connected_sock;

				iter->_isConnected = true;

				iter->_clientHandle = (HANDLE)_beginthreadex(NULL, 0, CGameServer::ControlThread, server, 0, NULL);
			}
		}
		Sleep(50);
	}

	return 0;
}
UINT WINAPI CGameServer::ControlThread(LPVOID p)
{
	CGameServer* server = (CGameServer*)p;

	SOCKET connected_sock = server->_lastSock;

	char recv_message[100];
	int length = 0;
	bool is_found = false;


	ClientIter iter = server->_clientLists.begin();
	while (iter != server->_clientLists.end())
	{
		if (((ClientInfo)(*iter))._clientSock == connected_sock)
		{
			is_found = true;

			// 연결 성공 메시지 전송
			char connection_success_message[] = "Connection Complete";
			send(connected_sock, connection_success_message, sizeof(connection_success_message), 0);
			break;
		}

		++iter;
	}

	#pragma region Thread Solution
	//while (iter->_isConnected && is_found)
	//{
	//	memset(recv_message, 0, sizeof(recv_message));
	//	length = recv(connected_sock, recv_message, sizeof(recv_message) - 1, 0);
	//	if (length == SOCKET_ERROR)
	//	{
	//		printf("[ERR] Recieve Error\n");
	//		break;
	//	}
	//	recv_message[length] = '\0';

	//	if (strcmp(recv_message, "exit") == 0)
	//	{
	//		printf("\r[SYS] Disconnected\n");
	//		iter->_isConnected = false;
	//		break;
	//	}

	//	printf("\r[MSG] CLIENT\t: %s\n", recv_message);

	//	// 모든 클라이언트에 메시지 뿌리기
	//	ClientIter c_iter = server->_clientLists.begin();
	//	while (c_iter != server->_clientLists.end())
	//	{
	//		if (c_iter->_isConnected && c_iter != iter)
	//			send(c_iter->_clientSock, recv_message, sizeof(recv_message), 0);

	//		++c_iter;
	//	}
	//}
	#pragma endregion


	fd_set fd_read_set, fd_error_set, fd_master;
	timeval time_interval;

	FD_ZERO(&fd_master);
	FD_SET(connected_sock, &fd_master);

	time_interval.tv_sec = 0;
	time_interval.tv_usec = 100;

	while (iter->_isConnected && is_found)
	{
		fd_read_set = fd_master;
		fd_error_set = fd_master;

		select((int)connected_sock, &fd_read_set, NULL, &fd_error_set, &time_interval);
		if (FD_ISSET(connected_sock, &fd_read_set))
		{
			memset(recv_message, 0, sizeof(recv_message));
			length = recv(connected_sock, recv_message, sizeof(recv_message) - 1, 0);
			if (length == SOCKET_ERROR)
			{
				printf("[ERR] Recieve Error\n");
				break;
			}
			recv_message[length] = '\0';


			if (strcmp(recv_message, "exit") == 0)
			{
				printf("\r[SYS] Disconnected\n");
				iter->_isConnected = false;
				break;
			}

			// 모든 클라이언트에 메시지 뿌리기
			ClientIter c_iter = server->_clientLists.begin();
			while (c_iter != server->_clientLists.end())
			{
				if (c_iter->_isConnected && c_iter != iter)
					send(c_iter->_clientSock, recv_message, sizeof(recv_message), 0);

				++c_iter;
			}

			printf("\r[MSG] CLIENT\t: %s\n", recv_message);
		}
		else if (FD_ISSET(connected_sock, &fd_error_set))
		{
			printf("[ERR] Error Occured\n");
			break;
		}
	}

	closesocket(iter->_clientSock);
	server->_clientPools.push(iter);
	printf("\r[SYS] Client Disconnect\n");

	return 0;
}