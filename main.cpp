#include "GameServer.h"


int main(void)
{
	CGameServer server;
	server.Listen(5004);
	server.Wait();

	return 0;
}