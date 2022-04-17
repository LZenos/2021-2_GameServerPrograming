#include "CVSP.h"


int sendCVSP(unsigned int sockfd, unsigned char cmd, unsigned char option, void* payload, unsigned short len)
{
	char* packet;
	CVSPHeader header;
	u_int packet_size;
	int res;


	// 패킷 사이즈 설정
	packet_size = len + sizeof(CVSPHeader);

	// 헤더파일 설정
	header._cmd = cmd;
	header._option = option;
	header._packetLength = packet_size;

	// 패킷 생성
	packet = new char[packet_size];
	assert(packet);
	// 패킷 초기화
	memset(packet, 0, packet_size);
	// 패킷 설정
	memcpy(packet, &header, sizeof(CVSPHeader));
	if (payload != NULL)
		memcpy(packet + sizeof(CVSPHeader), payload, len);

	// 전송
	res = send(sockfd, packet, packet_size, 0);

	// 반환
	delete[] packet;
	return res;
}
int recvCVSP(unsigned int sockfd, unsigned char* cmd, unsigned char* option, void* payload, unsigned short len)
{
	CVSPHeader header;
	char extra_packet[CVSP_STANDARD_PAYLOAD_LENGTH];
	int recv_size;
	int payload_size;


	// 패킷 검증
	assert(payload != NULL);

	// 배열 메모리 초기화
	memset(extra_packet, 0, sizeof(extra_packet));

	// 헤더 유효성 검사 (헤더의 크기 32bit가 온전히 읽혀지는지 확인)
	recv_size = recv(sockfd, (char*)&header, sizeof(header), MSG_PEEK);
	if (recv_size < 0)
		return recv_size;

	// 무선망의 경우 데이터가 부분적으로 잘려서 들어올 수 있으므로, 하나의 데이터를 온전히 받을 때까지 모든 패킷을 receive
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

	// 헤더 값 복사
	memcpy(&header, extra_packet, sizeof(header));
	payload_size = header._packetLength - sizeof(CVSPHeader);
	*cmd = header._cmd;
	*option = header._option;

	// 페이로드 값 복사
	if (payload_size != 0)
		memcpy(payload, extra_packet + sizeof(header), payload_size);

	return payload_size;
}