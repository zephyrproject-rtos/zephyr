/**
 *
 * \file
 *
 * \brief BSD alike socket interface internal types.
 *
 * Copyright (c) 2016 Atmel Corporation. All rights reserved.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. The name of Atmel may not be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * \asf_license_stop
 *
 */
#ifndef __M2M_SOCKET_HOST_IF_H__
#define __M2M_SOCKET_HOST_IF_H__


#ifdef  __cplusplus
extern "C" {
#endif

/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
INCLUDES
*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/

#ifndef	_BOOT_
#ifndef _FIRMWARE_
#include "socket/include/socket.h"
#else
#include "m2m_types.h"
#endif
#endif

/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
MACROS
*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/

#ifdef _FIRMWARE_
#define HOSTNAME_MAX_SIZE					(64)
#endif

#define SSL_MAX_OPT_LEN						HOSTNAME_MAX_SIZE



#define SOCKET_CMD_INVALID					0x00
/*!< 
	Invlaid Socket command value.
*/


#define SOCKET_CMD_BIND						0x41
/*!< 
	Socket Binding command value.
*/


#define SOCKET_CMD_LISTEN					0x42
/*!< 
	Socket Listening command value.
*/


#define SOCKET_CMD_ACCEPT					0x43
/*!< 
	Socket Accepting command value.
*/


#define SOCKET_CMD_CONNECT					0x44
/*!< 
	Socket Connecting command value.
*/


#define SOCKET_CMD_SEND						0x45
/*!< 
	Socket send command value.
*/


#define SOCKET_CMD_RECV						0x46
/*!< 
	Socket Recieve command value.
*/


#define SOCKET_CMD_SENDTO					0x47
/*!< 
	Socket sendTo command value.
*/


#define SOCKET_CMD_RECVFROM					0x48
/*!< 
	Socket RecieveFrom command value.
*/


#define SOCKET_CMD_CLOSE					0x49
/*!< 
	Socket Close command value.
*/


#define SOCKET_CMD_DNS_RESOLVE				0x4A
/*!< 
	Socket DNS Resolve command value.
*/


#define SOCKET_CMD_SSL_CONNECT				0x4B
/*!< 
	SSL-Socket Connect command value.
*/


#define SOCKET_CMD_SSL_SEND					0x4C	
/*!< 
	SSL-Socket Send command value.
*/	


#define SOCKET_CMD_SSL_RECV					0x4D
/*!< 
	SSL-Socket Recieve command value.
*/


#define SOCKET_CMD_SSL_CLOSE				0x4E
/*!< 
	SSL-Socket Close command value.
*/


#define SOCKET_CMD_SET_SOCKET_OPTION		0x4F
/*!< 
	Set Socket Option command value.
*/


#define SOCKET_CMD_SSL_CREATE				0x50
/*!<
*/


#define SOCKET_CMD_SSL_SET_SOCK_OPT			0x51


#define SOCKET_CMD_PING						0x52

#define SOCKET_CMD_SSL_SET_CS_LIST			0x53


#define PING_ERR_SUCCESS					0
#define PING_ERR_DEST_UNREACH				1
#define PING_ERR_TIMEOUT					2

/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
DATA TYPES
*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/


/*!
*  @brief	
*/
typedef struct{	
	uint16		u16Family;
	uint16		u16Port;
	uint32		u32IPAddr;
}tstrSockAddr;


typedef sint8			SOCKET;
typedef tstrSockAddr	tstrUIPSockAddr;



/*!
@struct	\
	tstrDnsReply
	
@brief
	DNS Reply, contains hostName and HostIP.
*/
typedef struct{
	char		acHostName[HOSTNAME_MAX_SIZE];
	uint32		u32HostIP;
}tstrDnsReply;


/*!
@brief
*/
typedef struct{
	tstrSockAddr	strAddr;
	SOCKET		sock;
	uint8		u8Void;
	uint16		u16SessionID;
}tstrBindCmd;


/*!
@brief
*/
typedef struct{
	SOCKET		sock;
	sint8		s8Status;
	uint16		u16SessionID;
}tstrBindReply;


/*!
*  @brief
*/
typedef struct{
	SOCKET	sock;
	uint8	u8BackLog;
	uint16	u16SessionID;
}tstrListenCmd;


/*!
@struct	\
	tstrSocketRecvMsg
	
@brief	Socket recv status. 

	It is passed to the APPSocketEventHandler with SOCKET_MSG_RECV or SOCKET_MSG_RECVFROM message type 
	in a response to a user call to the recv or recvfrom.
	If the received data from the remote peer is larger than the USER Buffer size (given at recv call), the data is 
	delivered to the user in a number of consecutive chunks according to the USER Buffer size.
*/
typedef struct{
	SOCKET		sock;
	sint8			s8Status;
	uint16		u16SessionID;
}tstrListenReply;


/*!
*  @brief
*/
typedef struct{
	tstrSockAddr	strAddr;
	SOCKET			sListenSock;
	SOCKET			sConnectedSock;
	uint16			u16Void;
}tstrAcceptReply;


/*!
*  @brief
*/
typedef struct{
	tstrSockAddr	strAddr;
	SOCKET			sock;
	uint8			u8SslFlags;
	uint16			u16SessionID;
}tstrConnectCmd;


/*!
@struct	\
	tstrConnectReply
	
@brief
	Connect Reply, contains sock number and error value
*/
typedef struct{
	SOCKET		sock;
	sint8		s8Error;
	uint16		u16AppDataOffset;
	/*!<
		In further packet send requests the host interface should put the user application
		data at this offset in the allocated shared data packet.
	*/
}tstrConnectReply;


/*!
@brief
*/
typedef struct{
	SOCKET			sock;
	uint8			u8Void;
	uint16			u16DataSize;
	tstrSockAddr	strAddr;
	uint16			u16SessionID;
	uint16			u16Void;
}tstrSendCmd;


/*!
@struct	\
	tstrSendReply
	
@brief
	Send Reply, contains socket number and number of sent bytes.
*/
typedef struct{
	SOCKET		sock;
	uint8		u8Void;
	sint16		s16SentBytes;
	uint16		u16SessionID;
	uint16		u16Void;
}tstrSendReply;


/*!
*  @brief
*/
typedef struct{
	uint32		u32Timeoutmsec;
	SOCKET		sock;
	uint8		u8Void;
	uint16		u16SessionID;
}tstrRecvCmd;


/*!
@struct
@brief
*/
typedef struct{
	tstrSockAddr		strRemoteAddr;
	sint16			s16RecvStatus;
	uint16			u16DataOffset;
	SOCKET			sock;
	uint8			u8Void;
	uint16			u16SessionID;
}tstrRecvReply;


/*!
*  @brief
*/
typedef struct{
	uint32		u32OptionValue;
	SOCKET		sock;
	uint8 		u8Option;
	uint16		u16SessionID;
}tstrSetSocketOptCmd;


typedef struct{
	SOCKET		sslSock;
	uint8		__PAD24__[3];
}tstrSSLSocketCreateCmd;


/*!
*  @brief
*/
typedef struct{
	SOCKET		sock;
	uint8 		u8Option;
	uint16		u16SessionID;
	uint32		u32OptLen;
	uint8		au8OptVal[SSL_MAX_OPT_LEN];
}tstrSSLSetSockOptCmd;


/*!
*/
typedef struct{
	uint32	u32DestIPAddr;
	uint32	u32CmdPrivate;
	uint16	u16PingCount;
	uint8	u8TTL;
	uint8	__PAD8__;
}tstrPingCmd;


typedef struct{
	uint32	u32IPAddr;
	uint32	u32CmdPrivate;
	uint32	u32RTT;
	uint16	u16Success;
	uint16	u16Fail;
	uint8	u8ErrorCode;
	uint8	__PAD24__[3];
}tstrPingReply;


typedef struct{
	uint32	u32CsBMP;
}tstrSslSetActiveCsList;

#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif /* __M2M_SOCKET_HOST_IF_H__ */
