#pragma once
#pragma comment(lib, "ws2_32")
#pragma warning(disable:4996)

#include <stdio.h>
#include <stdlib.h>
#include <WinSock2.h>
#include <process.h>
#include <iostream>
#include <vector>
#include <stack>


struct ClientInfo
{
	SOCKET _clientSock;
	bool _isConnected;
	char _id[50];
	HANDLE _clientHandle;
	HANDLE _listenHandle;

	ClientInfo()
	{
		memset(_id, 0, sizeof(_id));
		_clientSock = NULL;
		_isConnected = false;
	}
};

typedef std::vector<ClientInfo>::iterator ClientIter;

class CGameServer
{
private:
	int _portNum;
	SOCKET _serverSock;
	HANDLE _listenHandle;
	HANDLE _mainHandle;
	SOCKET _lastSock;
	bool _isRun;
	std::vector<ClientInfo> _clientLists;
	std::stack<ClientIter, std::vector<ClientIter> > _clientPools;

	int InitSocketLayer();
	void CloseSocketLayer();

public:
	CGameServer();
	~CGameServer();

	void Wait();
	void Listen(int port_num);

	static UINT WINAPI ListenThread(LPVOID p);
	static UINT WINAPI ControlThread(LPVOID p);
};