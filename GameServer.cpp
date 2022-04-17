#include "GameServer.h"

#include "CVSP.h"

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

	// ClientLists�� ��ȸ�ϸ� ���� ����
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

	printf("...[Server]...");

	// WinSock ���� 2.2 ����
	WORD ver_request = MAKEWORD(2, 2);
	WSADATA wsa_data;

	if (WSAStartup(ver_request, &wsa_data) != 0)
	{
		printf("\n[ERR] WSAStartup Error");
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
		printf("\n[ERR] Failed to create thread");
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
		printf("\n[ERR] Binding Error");
		return -1;
	}

	if (listen(server->_serverSock, 5) == SOCKET_ERROR)
	{
		printf("\n[ERR] Listening Error");
		return -1;
	}

	while (server->_isRun)
	{
		SOCKET connected_sock = accept(server->_serverSock, NULL, NULL);
		if (connected_sock > 0)
		{
			// Connection Pooling Ȯ��
			if (server->_clientPools.empty())
			{
				closesocket(connected_sock);
				continue;
			}
			else
			{
				// ���� �Ϸ�
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
	bool is_found = false;

	ClientIter iter = server->_clientLists.begin();
	while (iter != server->_clientLists.end())
	{
		if (iter->_clientSock == connected_sock)
		{
			is_found = true;
			break;
		}
		++iter;
	}

	fd_set fd_read_set, fd_error_set, fd_master;
	timeval time_interval;
	u_char cmd;
	u_char option;
	int length;
	char extra_packet[CVSP_STANDARD_PAYLOAD_LENGTH - sizeof(CVSPHeader)];

	FD_ZERO(&fd_master);
	FD_SET(connected_sock, &fd_master);

	time_interval.tv_sec = 0;
	time_interval.tv_usec = 100;

	while (iter->_isConnected && is_found)
	{
		fd_read_set = fd_master;
		fd_error_set = fd_master;

		// add: + 1 
		select((int)connected_sock + 1, &fd_read_set, NULL, &fd_error_set, &time_interval);

		if (FD_ISSET(connected_sock, &fd_read_set))
		{
			memset(extra_packet, 0, sizeof(extra_packet));
			length = recvCVSP((u_int)connected_sock, &cmd, &option, extra_packet, sizeof(extra_packet));
			if (length == SOCKET_ERROR)
			{
				printf("\n[ERR] Recieve Error");
				break;
			}
			extra_packet[length] = '\0';

			ClientIter c_iter = server->_clientLists.begin();
			switch (cmd)
			{
			case CVSP_JOINREQ:
			{
				// DBMS�� �����Ͽ� ID, PW ���� �۾� ����
				// ���� ���� �� �α��� ó��
				//sprintf_s(iter->_id, strlen(iter->_id), "%s", extra_packet);
				memcpy(iter->_id, extra_packet, sizeof(iter->_id));
				printf("\n[SYS] Client Connected\t\t(Socket: %d | ID: %s)", iter->_clientSock, iter->_id);

				// ���� ������ Ŭ���̾�Ʈ�� ���� �����ڵ��� ���� ����
				c_iter = server->_clientLists.end();
				do
				{
					--c_iter;
					if (c_iter->_isConnected && c_iter != iter)
					{
						cmd = CVSP_JOINRES;
						option = CVSP_OLDUSER;

						if (sendCVSP((u_int)iter->_clientSock, cmd, option, c_iter->_id, strlen(c_iter->_id)) < 0)
							printf("\n[ERR] Send Error");
					}
				} while (c_iter != server->_clientLists.begin());
				c_iter = server->_clientLists.begin();

				// ���� �����ڵ鿡�� �� Ŭ���̾�Ʈ�� ���� ����
				while (c_iter != server->_clientLists.end())
				{
					if (c_iter->_isConnected && c_iter != iter)
					{
						cmd = CVSP_JOINRES;
						option = CVSP_NEWUSER;

						if (sendCVSP((u_int)c_iter->_clientSock, cmd, option, iter->_id, strlen(iter->_id)) < 0)
							printf("\n[ERR] Send Error");
					}
					++c_iter;
				}

				// �α��� �Ϸ� ����
				cmd = CVSP_JOINRES;
				option = CVSP_SUCCESS;
				if (sendCVSP((u_int)iter->_clientSock, cmd, option, iter->_id, strlen(iter->_id)) < 0)
					printf("\n[ERR] Send Error");
				break;
			}

			case CVSP_SENDREQ:
			{
				printf("\n[MSG] %s", extra_packet);

				// ��� Ŭ���̾�Ʈ�� �޽��� �Ѹ���
				while (c_iter != server->_clientLists.end())
				{
					if (c_iter->_isConnected)
					{
						cmd = CVSP_SENDRES;
						option = CVSP_SUCCESS;
						if (sendCVSP((u_int)c_iter->_clientSock, cmd, option, extra_packet, strlen(extra_packet)) < 0)
							printf("\n[ERR] Send Error");
					}
					++c_iter;
				}
				break;
			}

			case CVSP_LEAVEREQ:
			{
				char disconnection_message[100] = "Client terminated access";
				
				printf("\n[SYS] Client Disconnected\t(Socket: %d | ID: %s)", iter->_clientSock, iter->_id);
				iter->_isConnected = false;

				// ��� Ŭ���̾�Ʈ�� ���� ���� �޽��� �Ѹ���
				while (c_iter != server->_clientLists.end())
				{
					if (c_iter->_isConnected)
					{
						cmd = CVSP_LEAVERES;
						option = CVSP_SUCCESS;
						if (sendCVSP((u_int)c_iter->_clientSock, cmd, option, disconnection_message, strlen(disconnection_message)) < 0)
							printf("\n[ERR] Send Error");
					}
					++c_iter;
				}
				break;
			}

			case CVSP_OPERATIONREQ:
			{
				PlayerInfo info;
				memcpy(&info, extra_packet, length);

				// ��� Ŭ���̾�Ʈ�� ��ġ ���� �Ѹ���
				while (c_iter != server->_clientLists.end())
				{
					if (c_iter->_isConnected/* && c_iter != iter*/)
					{
						cmd = CVSP_MONITORINGMSG;
						option = CVSP_SUCCESS;
						if (sendCVSP((u_int)c_iter->_clientSock, cmd, option, &info, sizeof(info)) < 0)
							printf("\n[ERR] Send Error");
					}
					++c_iter;
				}
				break;
			}

			case CVSP_BEGINREQ:
			{
				// ��û�� Ŭ���̾�Ʈ���� �����ڸ� ������� ����
				c_iter = server->_clientLists.end();
				do
				{
					c_iter--;
					if (c_iter->_isConnected)
					{
						cmd = CVSP_BEGINRES;
						option = CVSP_OLDUSER;
						if (sendCVSP((u_int)iter->_clientSock, cmd, option, c_iter->_id, strlen(c_iter->_id)) < 0)
							printf("\n[ERR] Send Error");
					}
				} while (c_iter != server->_clientLists.begin());
				break;
			}

			case CVSP_GAMESTARTREQ:
			{
				while (c_iter != server->_clientLists.end())
				{
					if (c_iter->_isConnected)
					{
						cmd = CVSP_GAMESTARTRES;
						option = CVSP_SUCCESS;
						if (sendCVSP((u_int)c_iter->_clientSock, cmd, option, NULL, 0) < 0)
							printf("\n[ERR] Send Error");
					}
					++c_iter;
				}
				break;
			}

			case CVSP_TURNENDREQ:
			{
				while (c_iter != server->_clientLists.end())
				{
					if (c_iter->_isConnected)
					{
						cmd = CVSP_TURNENDRES;
						option = CVSP_SUCCESS;
						if (sendCVSP((u_int)c_iter->_clientSock, cmd, option, NULL, 0) < 0)
							printf("\n[ERR] Send Error");
					}
					++c_iter;
				}
				break;
			}
			}
		}
		else if (FD_ISSET(connected_sock, &fd_error_set))
		{
			printf("\n[ERR] Error Occurred");
			break;
		}
	}

	closesocket(iter->_clientSock);
	server->_clientPools.push(iter);

	return 0;
}