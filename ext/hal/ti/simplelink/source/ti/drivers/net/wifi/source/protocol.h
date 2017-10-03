/*
 * protocol.h - CC31xx/CC32xx Host Driver Implementation
 *
 * Copyright (C) 2017 Texas Instruments Incorporated - http://www.ti.com/ 
 * 
 * 
 *  Redistribution and use in source and binary forms, with or without 
 *  modification, are permitted provided that the following conditions 
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright 
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the 
 *    documentation and/or other materials provided with the   
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
*/


/*******************************************************************************\
*
*   FILE NAME:      protocol.h
*
*   DESCRIPTION:    Constant and data structure definitions and function
*                   prototypes for the SL protocol module, which implements
*                   processing of SimpleLink Commands.
*
*   AUTHOR:
*
\*******************************************************************************/

#ifndef _SL_PROTOCOL_TYPES_H_
#define _SL_PROTOCOL_TYPES_H_

#ifdef    __cplusplus
extern "C" {
#endif

/****************************************************************************
**
**  User I/F pools definitions
**
****************************************************************************/

/****************************************************************************
**
**  Definitions for SimpleLink Commands
**
****************************************************************************/


/* pattern for LE 8/16/32 or BE*/
#define H2N_SYNC_PATTERN     {0xBBDDEEFF,0x4321,0x34,0x12}
#define H2N_CNYS_PATTERN     {0xBBDDEEFF,0x8765,0x78,0x56}

#define H2N_DUMMY_PATTERN    (_u32)0xFFFFFFFF
#define N2H_SYNC_PATTERN     (_u32)0xABCDDCBA
#define SYNC_PATTERN_LEN     (_u32)sizeof(_u32)
#define UART_SET_MODE_MAGIC_CODE    (_u32)0xAA55AA55
#define SPI_16BITS_BUG(pattern)     (_u32)((_u32)pattern & (_u32)0xFFFF7FFF)
#define SPI_8BITS_BUG(pattern)      (_u32)((_u32)pattern & (_u32)0xFFFFFF7F)



typedef struct
{
	_u16 Opcode;
	_u16 Len;
}_SlGenericHeader_t;


typedef struct
{
    _u32  Long;
    _u16  Short;
    _u8  Byte1;
    _u8  Byte2;
}_SlSyncPattern_t;

typedef _SlGenericHeader_t _SlCommandHeader_t;

typedef struct
{
    _SlGenericHeader_t  GenHeader;
    _u8               TxPoolCnt;
    _u8               DevStatus;
	_u16              MinMaxPayload;
    _u16              SocketTXFailure;  
    _u16              SocketNonBlocking;
}_SlResponseHeader_t;

#define _SL_RESP_SPEC_HDR_SIZE (sizeof(_SlResponseHeader_t) - sizeof(_SlGenericHeader_t))
#define _SL_RESP_HDR_SIZE       sizeof(_SlResponseHeader_t)
#define _SL_CMD_HDR_SIZE        sizeof(_SlCommandHeader_t)

#define _SL_RESP_ARGS_START(_pMsg) (((_SlResponseHeader_t *)(_pMsg)) + 1)

/* Used only in NWP! */
typedef struct
{
    _SlCommandHeader_t  sl_hdr;
    _u8   func_args_start;
} T_SCMD;

/* _SlResponseHeader_t DevStatus bits */
#define _SL_DEV_STATUS_BIT_WLAN_CONN					0x01 
#define _SL_DEV_STATUS_BIT_DROPPED_EVENTS				0x02
#define _SL_DEV_STATUS_BIT_LOCKED						0x04
#define _SL_DEV_STATUS_BIT_PROVISIONING_ACTIVE			0x08
#define _SL_DEV_STATUS_BIT_PROVISIONING_USER_INITIATED	0x10
#define _SL_DEV_STATUS_BIT_PRESERVATION					0x20
#define _SL_DEV_STATUS_BIT_PROVISIONING_ENABLE_API      0x40


/* Internal driver bits status  (g_SlDeviceStatus) */
#define _SL_DRV_STATUS_BIT_RESTART_REQUIRED       0x100   
#define _SL_DRV_STATUS_BIT_DEVICE_STARTED         0x200
#define _SL_DRV_STATUS_BIT_STOP_IN_PROGRESS       0x400 
#define _SL_DRV_STATUS_BIT_START_IN_PROGRESS      0x800

/****************************************************************************
**  OPCODES
****************************************************************************/
#define SL_IPV4_IPV6_OFFSET                            ( 9 )
#define SL_OPCODE_IPV4							       ( 0x0 << SL_IPV4_IPV6_OFFSET )
#define SL_OPCODE_IPV6							       ( 0x1 << SL_IPV4_IPV6_OFFSET )

#define SL_SYNC_ASYNC_OFFSET                           ( 10 )
#define SL_OPCODE_SYNC							       (0x1 << SL_SYNC_ASYNC_OFFSET )
#define SL_OPCODE_SILO_OFFSET                           ( 11 )
#define SL_OPCODE_SILO_MASK                             ( 0xF << SL_OPCODE_SILO_OFFSET )
#define SL_OPCODE_SILO_DEVICE                           ( 0x0 << SL_OPCODE_SILO_OFFSET )
#define SL_OPCODE_SILO_WLAN                             ( 0x1 << SL_OPCODE_SILO_OFFSET )
#define SL_OPCODE_SILO_SOCKET                           ( 0x2 << SL_OPCODE_SILO_OFFSET )
#define SL_OPCODE_SILO_NETAPP                           ( 0x3 << SL_OPCODE_SILO_OFFSET )
#define SL_OPCODE_SILO_FS                               ( 0x4 << SL_OPCODE_SILO_OFFSET )
#define SL_OPCODE_SILO_NETCFG                           ( 0x5 << SL_OPCODE_SILO_OFFSET )
#define SL_OPCODE_SILO_NETUTIL	                        ( 0x6 << SL_OPCODE_SILO_OFFSET )

#define SL_FAMILY_SHIFT                            (0x4)
#define SL_FLAGS_MASK                              (0xF)

#define SL_OPCODE_DEVICE_INITCOMPLETE                               	0x0008
#define SL_OPCODE_DEVICE_ABORT			                               	0x000C
#define SL_OPCODE_DEVICE_STOP_COMMAND                               	0x8473
#define SL_OPCODE_DEVICE_STOP_RESPONSE                              	0x0473
#define SL_OPCODE_DEVICE_STOP_ASYNC_RESPONSE                        	0x0073
#define SL_OPCODE_DEVICE_DEVICEASYNCDUMMY                           	0x0063

#define SL_OPCODE_DEVICE_VERSIONREADCOMMAND	                            0x8470
#define SL_OPCODE_DEVICE_VERSIONREADRESPONSE	                        0x0470
#define SL_OPCODE_DEVICE_DEVICE_ASYNC_GENERAL_ERROR                     0x0078
#define SL_OPCODE_DEVICE_FLOW_CTRL_ASYNC_EVENT             				0x0079

#define SL_OPCODE_WLAN_WLANCONNECTCOMMAND                           	0x8C80
#define SL_OPCODE_WLAN_WLANCONNECTRESPONSE                          	0x0C80
#define SL_OPCODE_WLAN_STA_ASYNCCONNECTEDRESPONSE                   	0x0880
#define SL_OPCODE_WLAN_P2PCL_ASYNCCONNECTEDRESPONSE                   	0x0892

#define SL_OPCODE_WLAN_WLANDISCONNECTCOMMAND                        	0x8C81
#define SL_OPCODE_WLAN_WLANDISCONNECTRESPONSE                       	0x0C81
#define SL_OPCODE_WLAN_STA_ASYNCDISCONNECTEDRESPONSE                	0x0881
#define SL_OPCODE_WLAN_P2PCL_ASYNCDISCONNECTEDRESPONSE                	0x0894

#define SL_OPCODE_WLAN_ASYNC_STA_ADDED                                  0x082E
#define SL_OPCODE_WLAN_ASYNC_P2PCL_ADDED                                0x0896
#define SL_OPCODE_WLAN_ASYNC_STA_REMOVED                                0x082F
#define SL_OPCODE_WLAN_ASYNC_P2PCL_REMOVED                              0x0898

#define SL_OPCODE_WLAN_P2P_DEV_FOUND                                    0x0830
#define SL_OPCODE_WLAN_P2P_CONNECTION_FAILED                            0x0831
#define SL_OPCODE_WLAN_P2P_NEG_REQ_RECEIVED                             0x0832

#define SL_OPCODE_WLAN_WLANCONNECTEAPCOMMAND                        	0x8C82
#define SL_OPCODE_WLAN_WLANCONNECTEAPCRESPONSE                      	0x0C82
#define SL_OPCODE_WLAN_PROFILEADDCOMMAND                            	0x8C83
#define SL_OPCODE_WLAN_PROFILEADDRESPONSE                           	0x0C83
#define SL_OPCODE_WLAN_PROFILEGETCOMMAND                            	0x8C84
#define SL_OPCODE_WLAN_PROFILEGETRESPONSE                           	0x0C84
#define SL_OPCODE_WLAN_PROFILEDELCOMMAND                            	0x8C85
#define SL_OPCODE_WLAN_PROFILEDELRESPONSE                           	0x0C85
#define SL_OPCODE_WLAN_POLICYSETCOMMAND                             	0x8C86
#define SL_OPCODE_WLAN_POLICYSETRESPONSE                            	0x0C86
#define SL_OPCODE_WLAN_POLICYGETCOMMAND                             	0x8C87
#define SL_OPCODE_WLAN_POLICYGETRESPONSE                            	0x0C87
#define SL_OPCODE_WLAN_FILTERADD                                    	0x8C88
#define SL_OPCODE_WLAN_FILTERADDRESPONSE                            	0x0C88
#define SL_OPCODE_WLAN_FILTERGET                                    	0x8C89
#define SL_OPCODE_WLAN_FILTERGETRESPONSE                            	0x0C89
#define SL_OPCODE_WLAN_FILTERDELETE                                 	0x8C8A
#define SL_OPCODE_WLAN_FILTERDELETERESPOSNE                         	0x0C8A
#define SL_OPCODE_WLAN_WLANGETSTATUSCOMMAND                         	0x8C8F
#define SL_OPCODE_WLAN_WLANGETSTATUSRESPONSE                        	0x0C8F
#define SL_OPCODE_WLAN_STARTTXCONTINUESCOMMAND                      	0x8CAA
#define SL_OPCODE_WLAN_STARTTXCONTINUESRESPONSE                     	0x0CAA
#define SL_OPCODE_WLAN_STOPTXCONTINUESCOMMAND                       	0x8CAB
#define SL_OPCODE_WLAN_STOPTXCONTINUESRESPONSE                      	0x0CAB
#define SL_OPCODE_WLAN_STARTRXSTATCOMMAND                           	0x8CAC
#define SL_OPCODE_WLAN_STARTRXSTATRESPONSE                          	0x0CAC
#define SL_OPCODE_WLAN_STOPRXSTATCOMMAND                            	0x8CAD
#define SL_OPCODE_WLAN_STOPRXSTATRESPONSE                           	0x0CAD
#define SL_OPCODE_WLAN_GETRXSTATCOMMAND                             	0x8CAF
#define SL_OPCODE_WLAN_GETRXSTATRESPONSE                            	0x0CAF
#define SL_OPCODE_WLAN_POLICYSETCOMMANDNEW                          	0x8CB0
#define SL_OPCODE_WLAN_POLICYSETRESPONSENEW                         	0x0CB0
#define SL_OPCODE_WLAN_POLICYGETCOMMANDNEW                          	0x8CB1
#define SL_OPCODE_WLAN_POLICYGETRESPONSENEW                         	0x0CB1

#define SL_OPCODE_WLAN_PROVISIONING_PROFILE_ADDED_ASYNC_RESPONSE        0x08B2
#define SL_OPCODE_WLAN_SET_MODE                                         0x8CB4
#define SL_OPCODE_WLAN_SET_MODE_RESPONSE                                0x0CB4
#define SL_OPCODE_WLAN_CFG_SET                                          0x8CB5
#define SL_OPCODE_WLAN_CFG_SET_RESPONSE                                 0x0CB5
#define SL_OPCODE_WLAN_CFG_GET                                          0x8CB6
#define SL_OPCODE_WLAN_CFG_GET_RESPONSE                                 0x0CB6
#define SL_OPCODE_WLAN_EAP_PROFILEADDCOMMAND                            0x8C67
#define SL_OPCODE_WLAN_EAP_PROFILEADDCOMMAND_RESPONSE                   0x0C67

#define SL_OPCODE_SOCKET_SOCKET                                     	0x9401
#define SL_OPCODE_SOCKET_SOCKETRESPONSE                             	0x1401
#define SL_OPCODE_SOCKET_CLOSE                                      	0x9402
#define SL_OPCODE_SOCKET_CLOSERESPONSE                              	0x1402
#define SL_OPCODE_SOCKET_ACCEPT                                     	0x9403
#define SL_OPCODE_SOCKET_ACCEPTRESPONSE                             	0x1403
#define SL_OPCODE_SOCKET_ACCEPTASYNCRESPONSE                        	0x1003
#define SL_OPCODE_SOCKET_ACCEPTASYNCRESPONSE_V6                     	0x1203
#define SL_OPCODE_SOCKET_BIND                                       	0x9404
#define SL_OPCODE_SOCKET_BIND_V6                                    	0x9604
#define SL_OPCODE_SOCKET_BINDRESPONSE                               	0x1404
#define SL_OPCODE_SOCKET_LISTEN                                     	0x9405
#define SL_OPCODE_SOCKET_LISTENRESPONSE                             	0x1405
#define SL_OPCODE_SOCKET_CONNECT                                    	0x9406
#define SL_OPCODE_SOCKET_CONNECT_V6                                 	0x9606
#define SL_OPCODE_SOCKET_CONNECTRESPONSE                            	0x1406
#define SL_OPCODE_SOCKET_CONNECTASYNCRESPONSE                       	0x1006
#define SL_OPCODE_SOCKET_SELECT                                     	0x9407
#define SL_OPCODE_SOCKET_SELECTRESPONSE                             	0x1407
#define SL_OPCODE_SOCKET_SELECTASYNCRESPONSE                        	0x1007
#define SL_OPCODE_SOCKET_SETSOCKOPT                                 	0x9408
#define SL_OPCODE_SOCKET_SETSOCKOPTRESPONSE                         	0x1408
#define SL_OPCODE_SOCKET_GETSOCKOPT                                 	0x9409
#define SL_OPCODE_SOCKET_GETSOCKOPTRESPONSE                         	0x1409
#define SL_OPCODE_SOCKET_RECV                                       	0x940A
#define SL_OPCODE_SOCKET_RECVASYNCRESPONSE                          	0x100A
#define SL_OPCODE_SOCKET_RECVFROM                                   	0x940B
#define SL_OPCODE_SOCKET_RECVFROMASYNCRESPONSE                      	0x100B
#define SL_OPCODE_SOCKET_RECVFROMASYNCRESPONSE_V6                   	0x120B
#define SL_OPCODE_SOCKET_SEND                                       	0x940C
#define SL_OPCODE_SOCKET_SENDTO                                     	0x940D
#define SL_OPCODE_SOCKET_SENDTO_V6                                  	0x960D
#define SL_OPCODE_SOCKET_TXFAILEDASYNCRESPONSE                      	0x100E
#define SL_OPCODE_SOCKET_SOCKETASYNCEVENT                               0x100F
#define SL_OPCODE_SOCKET_SOCKETCLOSEASYNCEVENT                          0x1010
#define SL_OPCODE_NETAPP_START_COMMAND                                  0x9C0A
#define SL_OPCODE_NETAPP_START_RESPONSE                                	0x1C0A
#define SL_OPCODE_NETAPP_NETAPPSTARTRESPONSE                        	0x1C0A
#define SL_OPCODE_NETAPP_STOP_COMMAND                               	0x9C61
#define SL_OPCODE_NETAPP_STOP_RESPONSE                              	0x1C61
#define SL_OPCODE_NETAPP_NETAPPSET                            	        0x9C0B
#define SL_OPCODE_NETAPP_NETAPPSETRESPONSE                    	        0x1C0B
#define SL_OPCODE_NETAPP_NETAPPGET                            	        0x9C27
#define SL_OPCODE_NETAPP_NETAPPGETRESPONSE                    	        0x1C27
#define SL_OPCODE_NETAPP_DNSGETHOSTBYNAME                           	0x9C20
#define SL_OPCODE_NETAPP_DNSGETHOSTBYNAMERESPONSE                   	0x1C20
#define SL_OPCODE_NETAPP_DNSGETHOSTBYNAMEASYNCRESPONSE              	0x1820
#define SL_OPCODE_NETAPP_DNSGETHOSTBYNAMEASYNCRESPONSE_V6           	0x1A20
#define SL_OPCODE_NETAPP_NETAPP_MDNS_LOOKUP_SERVICE                     0x9C71
#define SL_OPCODE_NETAPP_NETAPP_MDNS_LOOKUP_SERVICE_RESPONSE            0x1C72
#define SL_OPCODE_NETAPP_MDNSREGISTERSERVICE                            0x9C34
#define SL_OPCODE_NETAPP_MDNSREGISTERSERVICERESPONSE                    0x1C34
#define SL_OPCODE_NETAPP_MDNSGETHOSTBYSERVICE                           0x9C35
#define SL_OPCODE_NETAPP_MDNSGETHOSTBYSERVICERESPONSE                   0x1C35
#define SL_OPCODE_NETAPP_MDNSGETHOSTBYSERVICEASYNCRESPONSE              0x1835
#define SL_OPCODE_NETAPP_MDNSGETHOSTBYSERVICEASYNCRESPONSE_V6           0x1A35
#define SL_OPCODE_NETAPP_DNSGETHOSTBYADDR                           	0x9C26
#define SL_OPCODE_NETAPP_DNSGETHOSTBYADDR_V6                        	0x9E26
#define SL_OPCODE_NETAPP_DNSGETHOSTBYADDRRESPONSE                   	0x1C26
#define SL_OPCODE_NETAPP_DNSGETHOSTBYADDRASYNCRESPONSE              	0x1826
#define SL_OPCODE_NETAPP_PINGSTART                                  	0x9C21
#define SL_OPCODE_NETAPP_PINGSTART_V6                               	0x9E21
#define SL_OPCODE_NETAPP_PINGSTARTRESPONSE                          	0x1C21
#define SL_OPCODE_NETAPP_PINGREPORTREQUEST                          	0x9C22
#define SL_OPCODE_NETAPP_PINGREPORTREQUESTRESPONSE                  	0x1822
#define SL_OPCODE_NETAPP_ARPFLUSH                                   	0x9C24
#define SL_OPCODE_NETAPP_ARPFLUSHRESPONSE                           	0x1C24
#define SL_OPCODE_NETAPP_IPACQUIRED                                 	0x1825
#define SL_OPCODE_NETAPP_IPV4_LOST	                                 	0x1832
#define SL_OPCODE_NETAPP_DHCP_IPV4_ACQUIRE_TIMEOUT                  	0x1833
#define SL_OPCODE_NETAPP_IPACQUIRED_V6                              	0x1A25
#define SL_OPCODE_NETAPP_IPV6_LOST_V6									0x1A32
#define SL_OPCODE_NETAPP_IPERFSTARTCOMMAND                          	0x9C28
#define SL_OPCODE_NETAPP_IPERFSTARTRESPONSE                         	0x1C28
#define SL_OPCODE_NETAPP_IPERFSTOPCOMMAND                           	0x9C29
#define SL_OPCODE_NETAPP_IPERFSTOPRESPONSE                          	0x1C29
#define SL_OPCODE_NETAPP_CTESTSTARTCOMMAND                          	0x9C2A
#define SL_OPCODE_NETAPP_CTESTSTARTRESPONSE                         	0x1C2A
#define SL_OPCODE_NETAPP_CTESTASYNCRESPONSE                         	0x182A
#define SL_OPCODE_NETAPP_CTESTSTOPCOMMAND                           	0x9C2B
#define SL_OPCODE_NETAPP_CTESTSTOPRESPONSE                          	0x1C2B
#define SL_OPCODE_NETAPP_IP_LEASED                                  	0x182C
#define SL_OPCODE_NETAPP_IP_RELEASED                                	0x182D
#define SL_OPCODE_NETAPP_HTTPGETTOKENVALUE                          	0x182E
#define SL_OPCODE_NETAPP_HTTPSENDTOKENVALUE                         	0x9C2F
#define SL_OPCODE_NETAPP_HTTPPOSTTOKENVALUE                         	0x1830
#define SL_OPCODE_NETAPP_IP_COLLISION                                	0x1831

#define SL_OPCODE_NETAPP_REQUEST                                    	0x1878
#define SL_OPCODE_NETAPP_RESPONSE                                   	0x9C78
#define SL_OPCODE_NETAPP_SEND                                       	0x9C79
#define SL_OPCODE_NETAPP_SENDRESPONSE                               	0x1C79
#define SL_OPCODE_NETAPP_RECEIVEREQUEST                             	0x9C7A
#define SL_OPCODE_NETAPP_RECEIVE                                    	0x187B

#define SL_OPCODE_NVMEM_FILEOPEN                                    	0xA43C
#define SL_OPCODE_NVMEM_FILEOPENRESPONSE                             	0x243C
#define SL_OPCODE_NVMEM_FILECLOSE                                    	0xA43D
#define SL_OPCODE_NVMEM_FILECLOSERESPONSE                           	0x243D
#define SL_OPCODE_NVMEM_FILEREADCOMMAND                              	0xA440
#define SL_OPCODE_NVMEM_FILEREADRESPONSE                            	0x2440
#define SL_OPCODE_NVMEM_FILEWRITECOMMAND                            	0xA441
#define SL_OPCODE_NVMEM_FILEWRITERESPONSE                           	0x2441
#define SL_OPCODE_NVMEM_FILEGETINFOCOMMAND                          	0xA442
#define SL_OPCODE_NVMEM_FILEGETINFORESPONSE                         	0x2442
#define SL_OPCODE_NVMEM_FILEDELCOMMAND                              	0xA443
#define SL_OPCODE_NVMEM_FILEDELRESPONSE                             	0x2443
#define SL_OPCODE_NVMEM_NVMEMFORMATCOMMAND                          	0xA444
#define SL_OPCODE_NVMEM_NVMEMFORMATRESPONSE                         	0x2444
#define SL_OPCODE_NVMEM_NVMEMGETFILELISTCOMMAND                        	0xA448
#define SL_OPCODE_NVMEM_NVMEMGETFILELISTRESPONSE                       	0x2448

#define SL_OPCODE_NVMEM_NVMEMFSPROGRAMMINGCOMMAND              	0xA44A
#define SL_OPCODE_NVMEM_NVMEMFSPROGRAMMINGRESPONSE             	0x244A
#define SL_OPCODE_NVMEM_NVMEMFILESYSTEMCONTROLCOMMAND                   0xA44B
#define SL_OPCODE_NVMEM_NVMEMFILESYSTEMCONTROLRESPONSE                  0x244B
#define SL_OPCODE_NVMEM_NVMEMBUNDLECONTROLCOMMAND                  	    0xA44C
#define SL_OPCODE_NVMEM_NVMEMBUNDLECONTROLRESPONSE                 	    0x244C


#define SL_OPCODE_DEVICE_SETDEBUGLEVELCOMMAND                       	0x846A
#define SL_OPCODE_DEVICE_SETDEBUGLEVELRESPONSE                      	0x046A

#define SL_OPCODE_DEVICE_NETCFG_SET_COMMAND                 	        0x8432
#define SL_OPCODE_DEVICE_NETCFG_SET_RESPONSE                	        0x0432
#define SL_OPCODE_DEVICE_NETCFG_GET_COMMAND                 	        0x8433
#define SL_OPCODE_DEVICE_NETCFG_GET_RESPONSE                	        0x0433
/*  */
#define SL_OPCODE_DEVICE_SETUARTMODECOMMAND                         	0x846B
#define SL_OPCODE_DEVICE_SETUARTMODERESPONSE                        	0x046B
#define SL_OPCODE_DEVICE_SSISIZESETCOMMAND	                            0x846B
#define SL_OPCODE_DEVICE_SSISIZESETRESPONSE	                            0x046B

/*  */
#define SL_OPCODE_DEVICE_EVENTMASKSET                               	0x8464
#define SL_OPCODE_DEVICE_EVENTMASKSETRESPONSE                       	0x0464
#define SL_OPCODE_DEVICE_EVENTMASKGET                               	0x8465
#define SL_OPCODE_DEVICE_EVENTMASKGETRESPONSE                       	0x0465

#define SL_OPCODE_DEVICE_DEVICEGET                                  	0x8466
#define SL_OPCODE_DEVICE_DEVICEGETRESPONSE                              0x0466
#define SL_OPCODE_DEVICE_DEVICESET										0x84B7
#define SL_OPCODE_DEVICE_DEVICESETRESPONSE								0x04B7

#define SL_OPCODE_WLAN_SCANRESULTSGETCOMMAND                        	0x8C8C
#define SL_OPCODE_WLAN_SCANRESULTSGETRESPONSE                       	0x0C8C
#define SL_OPCODE_WLAN_SMARTCONFIGOPTSET                            	0x8C8D
#define SL_OPCODE_WLAN_SMARTCONFIGOPTSETRESPONSE                    	0x0C8D
#define SL_OPCODE_WLAN_SMARTCONFIGOPTGET                            	0x8C8E
#define SL_OPCODE_WLAN_SMARTCONFIGOPTGETRESPONSE                    	0x0C8E

#define SL_OPCODE_WLAN_PROVISIONING_COMMAND                   			0x8C98
#define SL_OPCODE_WLAN_PROVISIONING_RESPONSE                  			0x0C98
#define SL_OPCODE_DEVICE_RESET_REQUEST_ASYNC_EVENT                  	0x0099
#define SL_OPCODE_WLAN_PROVISIONING_STATUS_ASYNC_EVENT              	0x089A

#define SL_OPCODE_FREE_BSD_RECV_BUFFER                                  0xCCCB
#define SL_OPCODE_FREE_NON_BSD_READ_BUFFER                              0xCCCD


/* Rx Filters opcodes */
#define SL_OPCODE_WLAN_WLANRXFILTERADDCOMMAND                           0x8C6C
#define SL_OPCODE_WLAN_WLANRXFILTERADDRESPONSE                          0x0C6C
#define SL_OPCODE_WLAN_WLANRXFILTERGETSTATISTICSINFOCOMMAND             0x8C6E
#define SL_OPCODE_WLAN_WLANRXFILTERGETSTATISTICSINFORESPONSE            0x0C6E
#define SL_OPCODE_WLAN_WLANRXFILTERGETINFO                              0x8C70
#define SL_OPCODE_WLAN_WLANRXFILTERGETINFORESPONSE                      0x0C70
#define SL_OPCODE_WLAN_RX_FILTER_ASYNC_RESPONSE                         0x089D

/* Utils */
#define SL_OPCODE_NETUTIL_SET                                    		0xB4BE
#define SL_OPCODE_NETUTIL_SETRESPONSE                            		0x34BE
#define SL_OPCODE_NETUTIL_GET                                    		0xB4C0
#define SL_OPCODE_NETUTIL_GETRESPONSE                            		0x34C0
#define SL_OPCODE_NETUTIL_COMMAND                                		0xB4C1
#define SL_OPCODE_NETUTIL_COMMANDRESPONSE                        		0x34C1
#define SL_OPCODE_NETUTIL_COMMANDASYNCRESPONSE                   		0x30C1

/******************************************************************************************/
/*   Device structs  */
/******************************************************************************************/
typedef _u32 InitStatus_t;


typedef struct
{
    _i32 Status;
    _i32 ChipId;
    _i32 MoreData;
}InitComplete_t;

typedef struct
{
  _i16 status;
  _u16 sender;

}_BasicResponse_t;

typedef struct
{
    _u32 SessionNumber;
    _u16 Caller;
    _u16 Padding;
}SlDeviceResetRequestData_t;

typedef struct
{
  _u16 Timeout;
  _u16 Padding;
}SlDeviceStopCommand_t;

typedef struct
{
  _u32 Group;
  _u32 Mask;
}SlDeviceMaskEventSetCommand_t;

typedef _BasicResponse_t _DevMaskEventSetResponse_t;


typedef struct
{
  _u32 Group;
} SlDeviceMaskEventGetCommand_t;


typedef struct
{
  _u32 Group;
  _u32 Mask;
} SlDeviceMaskEventGetResponse_t;


typedef struct
{
  _u32 Group;
} SlDeviceStatusGetCommand_t;


typedef struct
{
  _u32 Group;
  _u32 Status;
} SlDeviceStatusGetResponse_t;

typedef struct
{
    _u32  ChipId;
    _u32  FwVersion[4];
    _u8   PhyVersion[4];
} SlDeviceVersionReadResponsePart_t;

typedef struct
{
    SlDeviceVersionReadResponsePart_t	part;
    _u32								NwpVersion[4];
    _u16								RomVersion;
    _u16								Padding;
} SlDeviceVersionReadResponseFull_t;

typedef struct
{
  _u16    MinTxPayloadSize;
  _u8     padding[6];
} SlDeviceFlowCtrlAsyncEvent_t;



typedef struct
{
	_u32 BaudRate;
	_u8  FlowControlEnable;
} SlDeviceUartSetModeCommand_t;

typedef _BasicResponse_t SlDeviceUartSetModeResponse_t;

/******************************************************/

typedef struct
{
    _u8 SsiSizeInBytes;
    _u8 Padding[3];
}_StellarisSsiSizeSet_t;

/*****************************************************************************************/
/*   WLAN structs */
/*****************************************************************************************/
#define MAXIMAL_PASSWORD_LENGTH					(64)

typedef struct
{
	_u8	 ProvisioningCmd;
	_u8	 RequestedRoleAfterSuccess;
	_u16 InactivityTimeoutSec;
	_u32 Flags;
} SlWlanProvisioningParams_t;

typedef struct{
	_u8	SecType;
	_u8	SsidLen;
    _u8	Bssid[6];
	_u8	PasswordLen;
} SlWlanConnectCommon_t;

#define SSID_STRING(pCmd)       (_i8 *)((SlWlanConnectCommon_t *)(pCmd) + 1)
#define PASSWORD_STRING(pCmd)   (SSID_STRING(pCmd) + ((SlWlanConnectCommon_t *)(pCmd))->SsidLen)

typedef struct{
	SlWlanConnectCommon_t       Common;
	_u8							UserLen;
	_u8							AnonUserLen;
    _u8   						CertIndex;
    _u32  						EapBitmask;
} SlWlanConnectEapCommand_t;

#define EAP_SSID_STRING(pCmd)       (_i8 *)((SlWlanConnectEapCommand_t *)(pCmd) + 1)
#define EAP_PASSWORD_STRING(pCmd)   (EAP_SSID_STRING(pCmd) + ((SlWlanConnectEapCommand_t *)(pCmd))->Common.SsidLen)
#define EAP_USER_STRING(pCmd)       (EAP_PASSWORD_STRING(pCmd) + ((SlWlanConnectEapCommand_t *)(pCmd))->Common.PasswordLen)
#define EAP_ANON_USER_STRING(pCmd)  (EAP_USER_STRING(pCmd) + ((SlWlanConnectEapCommand_t *)(pCmd))->UserLen)


typedef struct
{
    _u8	PolicyType;
    _u8 Padding;
    _u8	PolicyOption;
    _u8	PolicyOptionLen;
} SlWlanPolicySetGet_t;


typedef struct{
	_u32	MinDwellTime;
	_u32    MaxDwellTime;
	_u32    NumProbeResponse;
	_u32    G_Channels_mask;
	_i32    RssiThershold;
	_i32    SnrThershold;
	_i32    DefaultTXPower;
	_u16    IntervalList[16];
} SlWlanScanParamSetCommand_t;


typedef struct{
	_i16    SecType;
	_u8		SsidLen;
	_u8		Priority;
	_u8		Bssid[6];
    _u8		PasswordLen;
    _u8		WepKeyId;
} SlWlanAddGetProfile_t;


typedef struct{
       SlWlanAddGetProfile_t           Common;
       _u8                             UserLen;
       _u8                             AnonUserLen;
       _u8                             CertIndex;
       _u8                             padding;
       _u32                            EapBitmask;
} SlWlanAddGetEapProfile_t;




#define PROFILE_SSID_STRING(pCmd)       ((_i8 *)((SlWlanAddGetProfile_t *)(pCmd) + 1))
#define PROFILE_PASSWORD_STRING(pCmd)   (PROFILE_SSID_STRING(pCmd) + ((SlWlanAddGetProfile_t *)(pCmd))->SsidLen)

#define EAP_PROFILE_SSID_STRING(pCmd)       (_i8 *)((SlWlanAddGetEapProfile_t *)(pCmd) + 1)
#define EAP_PROFILE_PASSWORD_STRING(pCmd)   (EAP_PROFILE_SSID_STRING(pCmd) + ((SlWlanAddGetEapProfile_t *)(pCmd))->Common.SsidLen)
#define EAP_PROFILE_USER_STRING(pCmd)       (EAP_PROFILE_PASSWORD_STRING(pCmd) + ((SlWlanAddGetEapProfile_t *)(pCmd))->Common.PasswordLen)
#define EAP_PROFILE_ANON_USER_STRING(pCmd)  (EAP_PROFILE_USER_STRING(pCmd) + ((SlWlanAddGetEapProfile_t *)(pCmd))->UserLen)



typedef struct
{
	_u8	Index;
	_u8	Padding[3];
} SlWlanProfileDelGetCommand_t;

typedef _BasicResponse_t _WlanGetNetworkListResponse_t;

typedef struct
{
	_u8 	Index;
	_u8 	Count;
 _i8 	padding[2];
} SlWlanGetNetworkListCommand_t;


typedef struct
{
	_u32  						  GroupIdBitmask;
	_u8                           Cipher;
	_u8                           PublicKeyLen;		
	_u8							  Padding[2];
} SlWlanSmartConfigParams_t;

#define SMART_CONFIG_START_PUBLIC_KEY_STRING(pCmd)       ((_i8 *)((SlWlanSmartConfigParams_t *)(pCmd) + 1))

typedef	struct
{
	_u8	  Mode;
    _u8   Padding[3];
} SlWlanSetMode_t;




typedef struct
{
    _u16  Status;
    _u16  ConfigId;
    _u16  ConfigOpt;
    _u16  ConfigLen;
} SlWlanCfgSetGet_t;


/* ******************************************************************************/
/*     RX filters - Start  */
/* ******************************************************************************/

typedef struct
{
	SlWlanRxFilterRuleType_t RuleType;
	SlWlanRxFilterFlags_u Flags;
	SlWlanRxFilterID_t FilterId;
	_u8 Padding;
	SlWlanRxFilterRule_u Rule;
	SlWlanRxFilterTrigger_t Trigger;
	SlWlanRxFilterAction_t Action;
} SlWlanRxFilterAddCommand_t;


typedef struct
{
	SlWlanRxFilterID_t FilterId;
	_i16          Status;
	_u8  Padding[1];

} SlWlanRxFilterAddCommandReponse_t;


typedef struct
{
	_i16  Status;
	_u8 Padding[2];

} SlWlanRxFilterSetCommandReponse_t;


typedef struct
{
	_i16  Status;
	_u16 OutputBufferLength;

} SlWlanRxFilterGetCommandReponse_t;



/* ******************************************************************************/
/*     RX filters -- End  */
/* ******************************************************************************/

typedef struct
{
    _u16 Status;
    _u8  WlanRole;     /* 0 = station, 2 = AP */
    _u8  Ipv6Enabled;
    _u8  DhcpEnabled;

    _u32 Global[4];
    _u32 Local[4];
    _u32 DnsServer[4];
    _u8  DhcpState;

} SlNetappIpV6configRetArgs_t;


typedef struct
{
    _u8  Ip[4];
    _u8  IpMask[4];
    _u8  IpGateway[4];
    _u8  IpDnsServer[4];
	_u8  IpStart[4];
	_u8  IpEnd[4];
} SlNetCfgIpV4APArgs_t;



typedef struct
{
  _u16 Status;
  _u8  MacAddr[6];
} SlMacAddressSetGet_t;


typedef struct
{
    _u16    Status;
    _u16	ConfigId;
	_u16	ConfigOpt;
    _u16	ConfigLen;
} SlNetCfgSetGet_t;

typedef struct
{
	_u16  Status;
	_u16  DeviceSetId;
	_u16  Option;
	_u16  ConfigLen;
} SlDeviceSetGet_t;




/******************************************************************************************/
/*   Socket structs  */
/******************************************************************************************/

typedef struct
{
  _u8 Domain;
  _u8 Type;
  _u8 Protocol;
  _u8 Padding;
} SlSocketCommand_t;


typedef struct
{
  _i16 StatusOrLen;
  _u8  Sd;
  _u8  Padding;
} SlSocketResponse_t;

typedef struct
{
  _u8 Sd;
  _u8 Family;
  _u8 Padding1;
  _u8 Padding2;
} SlAcceptCommand_t;


typedef struct
{
  _i16 StatusOrLen;
  _u8  Sd;
  _u8  Family;
  _u16 Port;
  _u16 PaddingOrAddr;
  _u32 Address;
} SlSocketAddrAsyncIPv4Response_t;

typedef struct
{
  _i16 StatusOrLen;
  _u8  Sd;
  _u8  Family;
  _u16 Port;
  _u8  Address[6];
} SlSocketAddrAsyncIPv6EUI48Response_t;
typedef struct
{
  _i16 StatusOrLen;
  _u8  Sd;
  _u8  Family;
  _u16 Port;
  _u16 PaddingOrAddr;
  _u32 Address[4];
} SlSocketAddrAsyncIPv6Response_t;


typedef struct
{
  _i16 LenOrPadding;
  _u8  Sd;
  _u8  FamilyAndFlags;
  _u16 Port;
  _u16 PaddingOrAddr;
  _u32 Address;
} SlSocketAddrIPv4Command_t;

typedef struct
{
  _i16 LenOrPadding;
  _u8  Sd;
  _u8  FamilyAndFlags;
  _u16 Port;
  _u8  Address[6];
} SlSocketAddrIPv6EUI48Command_t;
typedef struct
{
  _i16 LenOrPadding;
  _u8  Sd;
  _u8  FamilyAndFlags;
  _u16 Port;
  _u16 PaddingOrAddr;
  _u32 Address[4];
} SlSocketAddrIPv6Command_t;

typedef union {
    SlSocketAddrIPv4Command_t      IpV4;
    SlSocketAddrIPv6EUI48Command_t IpV6EUI48;
#ifdef SL_SUPPORT_IPV6
    SlSocketAddrIPv6Command_t      IpV6;
#endif
} SlSocketAddrCommand_u;

typedef union {
    SlSocketAddrAsyncIPv4Response_t      IpV4;
    SlSocketAddrAsyncIPv6EUI48Response_t IpV6EUI48;
#ifdef SL_SUPPORT_IPV6
    SlSocketAddrAsyncIPv6Response_t IpV6;
#endif
} SlSocketAddrResponse_u;

typedef struct
{
  _u8 Sd;
  _u8 Backlog;
  _u8 Padding1;
  _u8 Padding2;
} SlListenCommand_t;

typedef struct
{
  _u8 Sd;
  _u8 Padding0;
  _u8 Padding1;
  _u8 Padding2;
} SlCloseCommand_t;


typedef struct
{
  _u8  Nfds;
  _u8  ReadFdsCount;
  _u8  WriteFdsCount;
  _u8  Padding;
  _u16 ReadFds;
  _u16 WriteFds;
  _u16 tv_usec;
  _u16 tv_sec;
} SlSelectCommand_t;


typedef struct
{
  _u16 Status;
  _u8  ReadFdsCount;
  _u8  WriteFdsCount;
  _u16 ReadFds;
  _u16 WriteFds;
} SlSelectAsyncResponse_t;

typedef struct
{
  _u8 Sd;
  _u8 Level;
  _u8 OptionName;
  _u8 OptionLen;
} SlSetSockOptCommand_t;

typedef struct
{
  _u8 Sd;
  _u8 Level;
  _u8 OptionName;
  _u8 OptionLen;
} SlGetSockOptCommand_t;

typedef struct
{
  _i16 Status;
  _u8  Sd;
  _u8  OptionLen;
} SlGetSockOptResponse_t;


typedef struct
{
  _u16 StatusOrLen;
  _u8  Sd;
  _u8  FamilyAndFlags;
} SlSendRecvCommand_t;

/*****************************************************************************************
*   NETAPP structs
******************************************************************************************/


typedef _BasicResponse_t _NetAppStartStopResponse_t;

typedef struct
{
    _u32  AppId;
}_NetAppStartStopCommand_t;

typedef struct
{
    _u16    Status;
    _u16	AppId;
    _u16	ConfigOpt;
    _u16	ConfigLen;
} SlNetAppSetGet_t;
typedef struct
{
    _u16  PortNumber;
} SlNetAppHttpServerGetSetPortNum_t;

typedef struct
{
    _u8  AuthEnable;
} SlNetAppHttpServerGetSetAuthEnable_t;

typedef struct _SlNetAppHttpServerGetToken_t
{
	_u8		TokenNameLen;
	_u8		Padd1;
	_u16	Padd2;
}SlNetAppHttpServerGetToken_t;

typedef struct _SlNetAppHttpServerSendToken_t
{
	_u8	  TokenValueLen;
	_u8	  TokenNameLen;
	_u8   TokenName[SL_NETAPP_MAX_TOKEN_NAME_LEN];
	_u16  Padd;
} SlNetAppHttpServerSendToken_t;

typedef struct _SlNetAppHttpServerPostToken_t
{
	_u8 PostActionLen;
	_u8 TokenNameLen;
	_u8 TokenValueLen;
	_u8 padding;
} SlNetAppHttpServerPostToken_t;

/*****************************************************************************************
*   NETAPP Request/Response/Send/Receive
******************************************************************************************/
typedef struct _SlProtocolNetAppRequest_t
{
	_u8 	AppId;
	_u8     RequestType;
	_u16	Handle;
	_u16	MetadataLen;
	_u16    PayloadLen;
	_u32    Flags;
} SlProtocolNetAppRequest_t;

typedef struct _SlProtocolNetAppResponse_t
{
	_u16	Handle;
	_u16    status;
	_u16	MetadataLen;
	_u16    PayloadLen;
	_u32    Flags;
} SlProtocolNetAppResponse_t;

typedef struct _SlProtocolNetAppSend_t
{
	_u16	Handle;
	_u16    DataLen;  /* can be data payload or metadata, depends on bit 1 in flags */
	_u32    Flags;
} SlProtocolNetAppSend_t;

typedef struct _SlProtocolNetAppReceiveRequest_t
{
	_u16 Handle;
	_u16 MaxBufferLen;
	_u32 Flags;
} SlProtocolNetAppReceiveRequest_t;

typedef struct _SlProtocolNetAppReceive_t
{
	_u16 Handle;
	_u16 PayloadLen;
	_u32 Flags;
} SlProtocolNetAppReceive_t;


typedef struct
{
  _u16 Len;
  _u8  Family;
  _u8  Padding;
} NetAppGetHostByNameCommand_t;

typedef struct
{
  _u16 Status;
  _u16 Padding;
  _u32 Ip0;
  _u32 Ip1;
  _u32 Ip2;
  _u32 Ip3;
} NetAppGetHostByNameIPv6AsyncResponse_t;

typedef struct
{
  _u16 Status;
  _u8  Padding1;
  _u8  Padding2;
  _u32 Ip0;
} NetAppGetHostByNameIPv4AsyncResponse_t;




typedef enum
{
    CTST_BSD_UDP_TX,
    CTST_BSD_UDP_RX,
    CTST_BSD_TCP_TX,
    CTST_BSD_TCP_RX,
    CTST_BSD_TCP_SERVER_BI_DIR,
    CTST_BSD_TCP_CLIENT_BI_DIR,
    CTST_BSD_UDP_BI_DIR,
    CTST_BSD_RAW_TX,
    CTST_BSD_RAW_RX,
    CTST_BSD_RAW_BI_DIR,
    CTST_BSD_SECURED_TCP_TX,
    CTST_BSD_SECURED_TCP_RX,
    CTST_BSD_SECURED_TCP_SERVER_BI_DIR,
    CTST_BSD_SECURED_TCP_CLIENT_BI_DIR,
	CTST_BSD_UDP_TX_IPV6,
    CTST_BSD_UDP_RX_IPV6,
    CTST_BSD_TCP_TX_IPV6,
    CTST_BSD_TCP_RX_IPV6,
    CTST_BSD_TCP_SERVER_BI_DIR_IPV6,
    CTST_BSD_TCP_CLIENT_BI_DIR_IPV6,
    CTST_BSD_UDP_BI_DIR_IPV6,
    CTST_BSD_RAW_TX_IPV6,
    CTST_BSD_RAW_RX_IPV6,
    CTST_BSD_RAW_BI_DIR_IPV6,
    CTST_BSD_SECURED_TCP_TX_IPV6,
    CTST_BSD_SECURED_TCP_RX_IPV6,
    CTST_BSD_SECURED_TCP_SERVER_BI_DIR_IPV6,
    CTST_BSD_SECURED_TCP_CLIENT_BI_DIR_IPV6,
	CTST_RAW_TX,
    CTST_RAW_RX
 }CommTest_e;

typedef struct _sl_protocol_CtestStartCommand_t
{
    _u32 Test;
    _u16 DestPort;
    _u16 SrcPort;
    _u32 DestAddr[4];
    _u32 PayloadSize;
    _u32 Timeout;
    _u32 CsEnabled;
    _u32 Secure;
    _u32 RawProtocol;
    _u8  Reserved1[4];
}_CtestStartCommand_t;

typedef struct
{
  _u8  Test;
  _u8  Socket;
  _i16 Status;
  _u32 StartTime;
  _u32 EndTime;
  _u16 TxKbitsSec;
  _u16 RxKbitsSec;
  _u32 OutOfOrderPackets;
  _u32 MissedPackets;
  _i16 Token;
}_CtestAsyncResponse_t;


typedef struct
{
    _u16 Status;
    _u16 RttMin;
    _u16 RttMax;
    _u16 RttAvg;
    _u32 NumSuccsessPings;
    _u32 NumSendsPings;
    _u32 TestTime;
} SlPingReportResponse_t;


typedef struct
{
    _u32 Ip;
    _u32 Gateway;
    _u32 Dns;
} IpV4AcquiredAsync_t;


typedef enum
{
  ACQUIRED_IPV6_LOCAL = 1,
  ACQUIRED_IPV6_GLOBAL
}IpV6AcquiredType_e;


typedef struct
{
    _u32 Type;
    _u32 Ip[4];
    _u32 Gateway[4];
    _u32 Dns[4];
} IpV6AcquiredAsync_t;


typedef union
{
    SlSocketCommand_t     EventMask;
    SlSendRecvCommand_t   DeviceInit;
}_device_commands_t;

/*****************************************************************************************
*   FS structs
******************************************************************************************/

typedef struct
{
    _u32 FileHandle;
    _u32 Offset;
    _u16 Len;
    _u16 Padding;
} SlFsReadCommand_t;

typedef struct
{
  _u32 Mode;
  _u32 Token;
} SlFsOpenCommand_t;

typedef struct
{
  _u32 FileHandle;
  _u32 Token;
} SlFsOpenResponse_t;


typedef struct
{
  _u32 FileHandle;
  _u32 CertificFileNameLength;
  _u32 SignatureLen;
} SlFsCloseCommand_t;


typedef _BasicResponse_t  SlFsReadResponse_t;
typedef _BasicResponse_t  SlFsDeleteResponse_t;
typedef _BasicResponse_t  SlFsCloseResponse_t;

typedef struct
{
    _u16 Status;
    _u16 Flags;
    _u32 FileLen;
    _u32 AllocatedLen;
    _u32 Token[4];
    _u32 FileStorageSize; /* The total size that the file required on the storage */
    _u32 FileWriteCounter; /* number of times in which the file have been written successfully */
} SlFsGetInfoResponse_t;

typedef struct
{
    _u8 DeviceID;
    _u8 Padding[3];
} SlFsFormatCommand_t;

typedef _BasicResponse_t  SlFsFormatResponse_t;

typedef struct
{
    _u32 Token;
} SlFsDeleteCommand_t;

typedef  SlFsDeleteCommand_t  SlFsGetInfoCommand_t;

typedef struct
{
    _u32 FileHandle;
    _u32 Offset;
    _u16 Len;
    _u16 Padding;
} SlFsWriteCommand_t;

typedef _BasicResponse_t SlFsWriteResponse_t;

typedef struct
{
	_u32         Token;
	_u8          Operation;
	_u8          Padding[3];
	_u32         FileNameLength;
	_u32         BufferLength;
} SlFsFileSysControlCommand_t;

typedef struct
{
	_i32          Status;
	_u32          Token;
	_u32          Len;
} SlFsFileSysControlResponse_t;


typedef struct
{
	_u16         IncludeFileFilters;
	_u8          Operation;
	_u8          Padding;
} SlFsBundleControlCommand_t;



typedef struct
{
	_i32         Status;
	_u8          BundleState;
	_u8          Padding[3];
} SlFsBundleControlResponse_t;


typedef struct
{
  _u16        KeyLen;
  _u16        ChunkLen;
  _u32        Flags;
} SlFsProgramCommand_t;



typedef struct
{
	_i32          Status;
} SlFsProgramResponse_t;


typedef struct
{
	_i32  Index;  /* start point is -1 */
	_u8   Count;
	_u8   MaxEntryLen;
	_u8   Flags;
	_u8   Padding;
} SlFsGetFileListCommand_t;

typedef struct
{
	_i32    NumOfEntriesOrError;
	_i32    Index; /* -1 , nothing was read */
	_u32    OutputBufferLength;
} SlFsGetFileListResponse_t;


/* TODO: Set MAx Async Payload length depending on flavor (Tiny, Small, etc.) */


#ifdef SL_TINY
#define SL_ASYNC_MAX_PAYLOAD_LEN        120  /* size must be aligned to 4 */
#else
    #if defined(slcb_NetAppHttpServerHdlr) || defined(EXT_LIB_REGISTERED_HTTP_SERVER_EVENTS) || defined(slcb_NetAppRequestHdlr) || defined(EXT_LIB_REGISTERED_NETAPP_REQUEST_EVENTS)
    #define SL_ASYNC_MAX_PAYLOAD_LEN    1600 /* size must be aligned to 4 */
    #else
    #define SL_ASYNC_MAX_PAYLOAD_LEN    220 /* size must be aligned to 4 */
    #endif
#endif

#define SL_ASYNC_MAX_MSG_LEN            (_SL_RESP_HDR_SIZE + SL_ASYNC_MAX_PAYLOAD_LEN)

#define RECV_ARGS_SIZE                  (sizeof(SlSocketResponse_t))
#define RECVFROM_IPV4_ARGS_SIZE         (sizeof(SlSocketAddrAsyncIPv4Response_t))
#define RECVFROM_IPV6_ARGS_SIZE         (sizeof(SlSocketAddrAsyncIPv6Response_t))

#define SL_IPV4_ADDRESS_SIZE 			(sizeof(_u32))
#define SL_IPV6_ADDRESS_SIZE 			(4 * sizeof(_u32))


/*****************************************************************************************
*   NetUtil structures
******************************************************************************************/
/* Utils Set Get Header */
typedef struct
{
    _u32	ObjId;
    _i16	Status;
    _u16	Option;
    _u16	ValueLen;
    _u8		Padding[2];
} SlNetUtilSetGet_t;


/* NetUtil Command Header */
typedef struct
{
    _u16	Cmd;
    _u16	AttribLen;
    _u16	InputLen;
    _u16	OutputLen;
} SlNetUtilCmd_t;

/* NetUtil Command Response Header */
typedef struct
{
    _u32	ObjId;
    _i16	Status;
    _u16	Cmd;
    _u16	OutputLen;
    _u8		Padding[2];
} SlNetUtilCmdRsp_t;

#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif /*  _SL_PROTOCOL_TYPES_H_  */
