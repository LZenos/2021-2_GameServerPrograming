#pragma comment(lib, "ws2_32")
#pragma warning(disable:4996)

#include <stdio.h>
#include <stdlib.h>
#include <WinSock2.h>
#include <process.h>


unsigned __stdcall Run(void* p);
bool isRunning;

int main(int argc, char** argv)
{
	WSADATA w_data;
	SOCKET server_socket, client_socket;
	SOCKADDR_IN server_addr, client_addr;
	int port_num = 5004;
	int sz_client_addr;
	HANDLE h_thread;

	char message[] = "20161637 박진호";
	char send_message[100];


	printf("...[Server]...\n");


	// 서버 시작 시 WSAStartup 호출
	if (WSAStartup(MAKEWORD(2, 2), &w_data) != 0)
	{
		printf("[ERR] WSAStartup Error\n");
		return -1;
	}

	// 소켓 생성
	server_socket = socket(PF_INET, SOCK_STREAM, 0);
	if (server_socket == INVALID_SOCKET)
	{
		printf("[ERR] Failed to create socket\n");
		return -1;
	}

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port_num);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(server_socket, (SOCKADDR*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR)
	{
		printf("[ERR] Binding Error\n");
		return -1;
	}

	if (listen(server_socket, 5) == SOCKET_ERROR)
	{
		printf("[ERR] Listening Error\n");
		return -1;
	}

	sz_client_addr = sizeof(client_addr);
	client_socket = accept(server_socket, (SOCKADDR*)&client_addr, &sz_client_addr);
	if (client_socket == INVALID_SOCKET)
	{
		printf("[ERR] Accepting Error\n");
		return - 1;
	}
	else
	{
		printf("[SYS] Connection complete successfully!\n");
	}

	send(client_socket, message, sizeof(message), 0);

	// Thread 생성
	h_thread = (HANDLE)_beginthreadex(NULL, 0, Run, (void*)client_socket, CREATE_SUSPENDED, NULL);
	if (!h_thread)
	{
		printf("[ERR] Failed to create thread\n");
		return -1;
	}
	// Thread 실행
	isRunning = true;
	ResumeThread(h_thread);


	while (isRunning)
	{
		printf("[MSG] ME\t: ");
		gets_s(send_message, sizeof(send_message));

		send(client_socket, send_message, (int)strlen(send_message), 0);

		if (strcmp(send_message, "exit") == 0)
		{
			printf("\r[SYS] Disconnected\n");
			isRunning = false;
			break;
		}
	}

	// Thread 종료 대기
	WaitForSingleObject(h_thread, INFINITE);

	// Thread 및 Socket 해제
	CloseHandle(h_thread);
	closesocket(client_socket);
	closesocket(server_socket);

	// 서버 종료 시 WSACleanup 호출
	if (WSACleanup() == SOCKET_ERROR)
	{
		printf("[ERR] WSACleanup failed and it occurs an Error: %d\n", WSAGetLastError());
		return -1;
	}
	printf("\r[SYS] Connection End\n");
}


unsigned __stdcall Run(void* p)
{
	SOCKET client_socket = (SOCKET)p;
	char recv_message[100];
	int length = 0;


	while (isRunning)
	{
		memset(recv_message, 0, sizeof(recv_message));
		length = recv(client_socket, recv_message, sizeof(recv_message) - 1, 0);
		if (length == SOCKET_ERROR)
		{
			printf("[ERR] Recieve Error\n");
			break;
		}

		recv_message[length] = '\0';

		if (strcmp(recv_message, "exit") == 0)
		{
			printf("\r[SYS] Disconnected\n");
			printf("[SYS] Input ENTER to end this process");
			isRunning = false;
			break;
		}

		printf("\r[MSG] CLIENT\t: %s\n", recv_message);
		printf("[MSG] ME\t: ");
	}

	return 0;
}