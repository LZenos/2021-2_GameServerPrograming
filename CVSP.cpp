#include "CVSP.h"


int sendCVSP(unsigned int sockfd, unsigned char cmd, unsigned char option, void* payload, unsigned short len)
{
	char* packet;
	CVSPHeader header;
	u_int packet_size;
	int res;


	// ��Ŷ ������ ����
	packet_size = len + sizeof(CVSPHeader);

	// ������� ����
	header._cmd = cmd;
	header._option = option;
	header._packetLength = packet_size;

	// ��Ŷ ����
	packet = new char[packet_size];
	assert(packet);
	// ��Ŷ �ʱ�ȭ
	memset(packet, 0, packet_size);
	// ��Ŷ ����
	memcpy(packet, &header, sizeof(CVSPHeader));
	if (payload != NULL)
		memcpy(packet + sizeof(CVSPHeader), payload, len);

	// ����
	res = send(sockfd, packet, packet_size, 0);

	// ��ȯ
	delete[] packet;
	return res;
}
int recvCVSP(unsigned int sockfd, unsigned char* cmd, unsigned char* option, void* payload, unsigned short len)
{
	CVSPHeader header;
	char extra_packet[CVSP_STANDARD_PAYLOAD_LENGTH];
	int recv_size;
	int payload_size;


	// ��Ŷ ����
	assert(payload != NULL);

	// �迭 �޸� �ʱ�ȭ
	memset(extra_packet, 0, sizeof(extra_packet));

	// ��� ��ȿ�� �˻� (����� ũ�� 32bit�� ������ ���������� Ȯ��)
	recv_size = recv(sockfd, (char*)&header, sizeof(header), MSG_PEEK);
	if (recv_size < 0)
		return recv_size;

	// �������� ��� �����Ͱ� �κ������� �߷��� ���� �� �����Ƿ�, �ϳ��� �����͸� ������ ���� ������ ��� ��Ŷ�� receive
	int curr_read = 0;
	int last_read = header._packetLength;
	int ret = 0;
	while (curr_read != header._packetLength)
	{
		ret = recv(sockfd, extra_packet + curr_read, last_read, 0);
		if (ret < 0)
			return -1;

		curr_read += ret;
		last_read -= ret;
	}

	// ��� �� ����
	memcpy(&header, extra_packet, sizeof(header));
	payload_size = header._packetLength - sizeof(CVSPHeader);
	*cmd = header._cmd;
	*option = header._option;

	// ���̷ε� �� ����
	if (payload_size != 0)
		memcpy(payload, extra_packet + sizeof(header), payload_size);

	return payload_size;
}