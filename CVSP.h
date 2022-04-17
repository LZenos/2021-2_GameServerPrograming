#ifndef _CVSP_1_0_0_H_
#define _CVSP_1_0_0_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <assert.h>
#include <fcntl.h>	// OS 관련 API를 담고 있는 헤더

#define WIN32 1
#ifdef WIN32

#pragma comment(lib, "ws2_32.lib")
#include <winsock.h>
#include <io.h>

#endif

#ifndef WIN32

#include <sys/socket.h>
#include <unistd.h>

#endif

#define CVSP_MONITORING_MESSAGE			700
#define CVSP_MONITORING_LOAD			701

// version 
#define CVSP_VER						0x01
// Port	
#define CVSP_PORT						9000
// payload size 
#define CVSP_STANDARD_PAYLOAD_LENGTH	4096
// Command 
#define CVSP_JOINREQ					0x01
#define CVSP_JOINRES					0x02
#define CVSP_SENDREQ					0x03
#define CVSP_SENDRES					0x04
#define CVSP_OPERATIONREQ				0x05
#define CVSP_MONITORINGMSG				0x06
#define CVSP_LEAVEREQ					0x07
#define CVSP_LEAVERES					0x08
#define CVSP_BEGINREQ					0x09
#define CVSP_BEGINRES					0x10
#define CVSP_GAMESTARTREQ				0x11
#define CVSP_GAMESTARTRES				0x12
#define CVSP_TURNENDREQ					0x13
#define CVSP_TURNENDRES					0x14
// option  
#define CVSP_SUCCESS					0x01
#define CVSP_FAILE						0x02
#define CVSP_NEWUSER					0x03
#define CVSP_OLDUSER					0x04


/* variable style */
#ifdef WIN32
typedef unsigned char  u_char;
typedef unsigned int   u_int;
typedef unsigned short u_short;
typedef unsigned long  u_long;
#endif

/*	Header(32bit)
		|0   |8	   |12	|16      |24     |
		|____|_____|______|______|_______|____________________|
		|Ver  |cmd |Option|Packet|Length |PayLoad			  |
		|__________|______|______|_______|____________________|
*/
typedef struct CVSPHeader_t
{
	u_char			_cmd;
	u_char			_option;
	u_short			_packetLength;
} CVSPHeader;

// #define MFCSOCK

// API interface
int sendCVSP(unsigned int sockfd, unsigned char cmd, unsigned char option, void* payload, unsigned short len);
int recvCVSP(unsigned int sockfd, unsigned char* cmd, unsigned char* option, void* payload, unsigned short len);

// 여기에 통신할 데이터들을 정의할 수 있음
struct PlayerInfo
{
	// 문자열의 경우, C# 동적할당이 아니면 표현하기 어렵고 서버에서의 중복 가능성이 있기 때문에 ID(Token)로 관리
	char id[50];
	float posX;
	float posY;
	float posZ;
	float quatX;
	float quatY;
	float quatZ;
	float quatW;
};


#endif