/*
 * netapp.c - CC31xx/CC32xx Host Driver Implementation
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
    


/*****************************************************************************/
/* Include files                                                             */
/*****************************************************************************/
#include <ti/drivers/net/wifi/simplelink.h>
#include <ti/drivers/net/wifi/source/protocol.h>
#include <ti/drivers/net/wifi/source/driver.h>

/*****************************************************************************/
/* Macro declarations                                                        */
/*****************************************************************************/

/*****************************************************************************/
/* Functions prototypes                                                      */
/*****************************************************************************/
_SlReturnVal_t _SlNetAppHandleAsync_DnsGetHostByName(void *pVoidBuf);

#ifndef SL_TINY
_SlReturnVal_t _SlNetAppHandleAsync_DnsGetHostByService(void *pVoidBuf);
_SlReturnVal_t _SlNetAppHandleAsync_PingResponse(void *pVoidBuf);
static void _SlNetAppCopyPingResultsToReport(SlPingReportResponse_t *pResults,SlNetAppPingReport_t *pReport);
#endif


_i16 _SlNetAppMDNSRegisterUnregisterService(const _i8* 		pServiceName, 
											const _u8   ServiceNameLen,
											const _i8* 		pText,
											const _u8   TextLen,
											const _u16  Port,
											const _u32    TTL,
											const _u32    Options);


_u16 _SlNetAppSendTokenValue(SlNetAppHttpServerData_t * Token);

_u16 _SlNetAppSendResponse( _u16 handle, SlNetAppResponse_t *NetAppResponse);


#define SL_NETAPP_SERVICE_SIZE_MASK (0x7)
#define SL_NETAPP_PING_GUARD_INTERVAL (20000)

static _u16 NetAppServiceSizeLUT[] =
{
	(_u16)sizeof(_BasicResponse_t),                          /* 0 - Default value */
	(_u16)sizeof(SlNetAppGetFullServiceWithTextIpv4List_t),  /* 1 - SL_NETAPP_FULL_SERVICE_WITH_TEXT_IPV4_TYPE */
	(_u16)sizeof(SlNetAppGetFullServiceIpv4List_t),          /* 2 - SL_NETAPP_FULL_SERVICE_IPV4_TYPE */
	(_u16)sizeof(SlNetAppGetShortServiceIpv4List_t),         /* 3 - SL_NETAPP_SHORT_SERVICE_IPV4_TYPE */
	(_u16)sizeof(SlNetAppGetFullServiceWithTextIpv6List_t),  /* 4 - SL_NETAPP_FULL_SERVICE_WITH_TEXT_IPV6_TYPE */
	(_u16)sizeof(SlNetAppGetFullServiceIpv6List_t),          /* 5 - SL_NETAPP_FULL_SERVICE_IPV6_TYPE */
	(_u16)sizeof(SlNetAppGetShortServiceIpv6List_t),         /* 6 - SL_NETAPP_SHORT_SERVICE_IPV6_TYPE */
	(_u16)sizeof(_BasicResponse_t),                          /* 7 - Default value */
};

typedef union
{
	_NetAppStartStopCommand_t    Cmd;
	_NetAppStartStopResponse_t   Rsp;
}_SlNetAppStartStopMsg_u;


#if _SL_INCLUDE_FUNC(sl_NetAppStart)

static const _SlCmdCtrl_t _SlNetAppStartCtrl =

{
    SL_OPCODE_NETAPP_START_COMMAND,
    (_SlArgSize_t)sizeof(_NetAppStartStopCommand_t),
    (_SlArgSize_t)sizeof(_NetAppStartStopResponse_t)
};

_i16 sl_NetAppStart(const _u32 AppBitMap)
{
    _SlNetAppStartStopMsg_u Msg;
    Msg.Cmd.AppId = AppBitMap;

    /* verify that this api is allowed. if not allowed then
    ignore the API execution and return immediately with an error */
    VERIFY_API_ALLOWED(SL_OPCODE_SILO_NETAPP);
    VERIFY_RET_OK(_SlDrvCmdOp((_SlCmdCtrl_t *)&_SlNetAppStartCtrl, &Msg, NULL));

    return Msg.Rsp.status;
}
#endif

/*****************************************************************************
 sl_NetAppStop
*****************************************************************************/
#if _SL_INCLUDE_FUNC(sl_NetAppStop)


static const _SlCmdCtrl_t _SlNetAppStopCtrl =
{
    SL_OPCODE_NETAPP_STOP_COMMAND,
    (_SlArgSize_t)sizeof(_NetAppStartStopCommand_t),
    (_SlArgSize_t)sizeof(_NetAppStartStopResponse_t)
};



_i16 sl_NetAppStop(const _u32 AppBitMap)
{
    _SlNetAppStartStopMsg_u Msg;

    /* verify that this api is allowed. if not allowed then
    ignore the API execution and return immediately with an error */
    VERIFY_API_ALLOWED(SL_OPCODE_SILO_NETAPP);
    Msg.Cmd.AppId = AppBitMap;
    VERIFY_RET_OK(_SlDrvCmdOp((_SlCmdCtrl_t *)&_SlNetAppStopCtrl, &Msg, NULL));

    return Msg.Rsp.status;
}
#endif


/******************************************************************************/
/* sl_NetAppGetServiceList */
/******************************************************************************/
typedef struct
{
    _u8  IndexOffest;
    _u8  MaxServiceCount;
    _u8  Flags;
    _i8  Padding;
}NetappGetServiceListCMD_t;

typedef union
{
	 NetappGetServiceListCMD_t      Cmd;
	_BasicResponse_t                Rsp;
}_SlNetappGetServiceListMsg_u;


#if _SL_INCLUDE_FUNC(sl_NetAppGetServiceList)

static const _SlCmdCtrl_t _SlGetServiceListeCtrl =
{
    SL_OPCODE_NETAPP_NETAPP_MDNS_LOOKUP_SERVICE,
    (_SlArgSize_t)sizeof(NetappGetServiceListCMD_t),
    (_SlArgSize_t)sizeof(_BasicResponse_t)
};

_i16 sl_NetAppGetServiceList(const _u8  IndexOffest,
						     const _u8  MaxServiceCount,
							 const _u8  Flags,
						           _i8  *pBuffer,
							 const _u32  BufferLength
							)
{

    _i32 					 retVal= 0;
    _SlNetappGetServiceListMsg_u Msg;
    _SlCmdExt_t                  CmdExt;
	_u16               ServiceSize = 0;
	_u16               BufferSize = 0;

    /* verify that this api is allowed. if not allowed then
    ignore the API execution and return immediately with an error */
    VERIFY_API_ALLOWED(SL_OPCODE_SILO_NETAPP);

	/*
	Calculate RX pBuffer size
    WARNING:
    if this size is BufferSize than 1480 error should be returned because there
    is no place in the RX packet.
    */
    ServiceSize = NetAppServiceSizeLUT[Flags & SL_NETAPP_SERVICE_SIZE_MASK];
	BufferSize =  MaxServiceCount * ServiceSize;

	/* Check the size of the requested services is smaller than size of the user buffer.
	  If not an error is returned in order to avoid overwriting memory. */
	if(BufferLength < BufferSize)
	{
		return SL_ERROR_NET_APP_RX_BUFFER_LENGTH;
	}

    _SlDrvResetCmdExt(&CmdExt);
    CmdExt.RxPayloadLen = (_i16)BufferSize;
    CmdExt.pRxPayload = (_u8 *)pBuffer; 

    Msg.Cmd.IndexOffest		= IndexOffest;
    Msg.Cmd.MaxServiceCount = MaxServiceCount;
    Msg.Cmd.Flags			= Flags;
    Msg.Cmd.Padding			= 0;

    VERIFY_RET_OK(_SlDrvCmdOp((_SlCmdCtrl_t *)&_SlGetServiceListeCtrl, &Msg, &CmdExt));
    retVal = Msg.Rsp.status;

    return (_i16)retVal;
}

#endif

/*****************************************************************************/
/* sl_mDNSRegisterService */
/*****************************************************************************/
/*
 * The below struct depicts the constant parameters of the command/API RegisterService.
 *
   1. ServiceLen                      - The length of the service should be smaller than SL_NETAPP_MDNS_MAX_SERVICE_NAME_AND_TEXT_LENGTH.
   2. TextLen                         - The length of the text should be smaller than SL_NETAPP_MDNS_MAX_SERVICE_NAME_AND_TEXT_LENGTH.
   3. port                            - The port on this target host.
   4. TTL                             - The TTL of the service
   5. Options                         - bitwise parameters:
                                        bit 0  - is unique (means if the service needs to be unique)
										bit 31  - for internal use if the service should be added or deleted (set means ADD).
                                        bit 1-30 for future.

   NOTE:

   1. There are another variable parameter is this API which is the service name and the text.
   2. According to now there is no warning and Async event to user on if the service is a unique.
*
 */


typedef struct
{
    _u8   ServiceNameLen;
    _u8   TextLen;
    _u16  Port;
    _u32   TTL;
    _u32   Options;
}NetappMdnsSetService_t;

typedef union
{
	 NetappMdnsSetService_t         Cmd;
	_BasicResponse_t                Rsp;
}_SlNetappMdnsRegisterServiceMsg_u;


#if (_SL_INCLUDE_FUNC(sl_NetAppMDNSRegisterService) || _SL_INCLUDE_FUNC(sl_NetAppMDNSUnregisterService))

static const _SlCmdCtrl_t _SlRegisterServiceCtrl =
{
    SL_OPCODE_NETAPP_MDNSREGISTERSERVICE,
    (_SlArgSize_t)sizeof(NetappMdnsSetService_t),
    (_SlArgSize_t)sizeof(_BasicResponse_t)
};

/******************************************************************************

    sl_NetAppMDNSRegisterService

    CALLER          user from its host


    DESCRIPTION:
                    Add/delete  service
					The function manipulates the command that register the service and call
					to the NWP in order to add/delete the service to/from the mDNS package and to/from the DB.
                    
					This register service is a service offered by the application.
					This unregister service is a service offered by the application before.
                     
					The service name should be full service name according to RFC
                    of the DNS-SD - means the value in name field in SRV answer.
                    
					Example for service name:
                    1. PC1._ipp._tcp.local
                    2. PC2_server._ftp._tcp.local

                    If the option is_unique is set, mDNS probes the service name to make sure
                    it is unique before starting to announce the service on the network.
                    Instance is the instance portion of the service name.

 


    PARAMETERS:

                    The command is from constant parameters and variables parameters.

					Constant parameters are:

                    ServiceLen                          - The length of the service.
                    TextLen                             - The length of the service should be smaller than 64.
                    port                                - The port on this target host.
                    TTL                                 - The TTL of the service
                    Options                             - bitwise parameters:
                                                            bit 0  - is unique (means if the service needs to be unique)
                                                            bit 31  - for internal use if the service should be added or deleted (set means ADD).
                                                            bit 1-30 for future.

                   The variables parameters are:

                    Service name(full service name)     - The service name.
                                                          Example for service name:
                                                          1. PC1._ipp._tcp.local
                                                          2. PC2_server._ftp._tcp.local

                    Text                                - The description of the service.
                                                          should be as mentioned in the RFC
                                                          (according to type of the service IPP,FTP...)

					NOTE - pay attention

						1. Temporary -  there is an allocation on stack of internal buffer.
						Its size is SL_NETAPP_MDNS_MAX_SERVICE_NAME_AND_TEXT_LENGTH.
						It means that the sum of the text length and service name length cannot be bigger than
						SL_NETAPP_MDNS_MAX_SERVICE_NAME_AND_TEXT_LENGTH.
						If it is - An error is returned.

					    2. According to now from certain constraints the variables parameters are set in the
					    attribute part (contain constant parameters)



	  RETURNS:        Status - the immediate response of the command status.
							   0 means success. 



******************************************************************************/
_i16 _SlNetAppMDNSRegisterUnregisterService(	const _i8* 		pServiceName, 
											const _u8   ServiceNameLen,
											const _i8* 		pText,
											const _u8   TextLen,
											const _u16  Port,
											const _u32   TTL,
											const _u32   Options)

{
    _SlNetappMdnsRegisterServiceMsg_u			Msg;
    _SlCmdExt_t									CmdExt ;
 _i8 									ServiceNameAndTextBuffer[SL_NETAPP_MDNS_MAX_SERVICE_NAME_AND_TEXT_LENGTH];
 _i8 									*TextPtr;

    /* verify that this api is allowed. if not allowed then
    ignore the API execution and return immediately with an error */
    VERIFY_API_ALLOWED(SL_OPCODE_SILO_NETAPP);
	/*

	NOTE - pay attention

		1. Temporary -  there is an allocation on stack of internal buffer.
		Its size is SL_NETAPP_MDNS_MAX_SERVICE_NAME_AND_TEXT_LENGTH.
		It means that the sum of the text length and service name length cannot be bigger than
		SL_NETAPP_MDNS_MAX_SERVICE_NAME_AND_TEXT_LENGTH.
		If it is - An error is returned.

		2. According to now from certain constraints the variables parameters are set in the
		attribute part (contain constant parameters)


	*/

	/*build the attribute part of the command.
	  It contains the constant parameters of the command*/

	Msg.Cmd.ServiceNameLen	= ServiceNameLen;
	Msg.Cmd.Options			= Options;
	Msg.Cmd.Port			= Port;
	Msg.Cmd.TextLen			= TextLen;
	Msg.Cmd.TTL				= TTL;

	/*Build the payload part of the command
	 Copy the service name and text to one buffer.
	 NOTE - pay attention
	 			The size of the service length + the text length should be smaller than 255,
	 			Until the simplelink drive supports to variable length through SPI command. */
	if(TextLen + ServiceNameLen > (SL_NETAPP_MDNS_MAX_SERVICE_NAME_AND_TEXT_LENGTH - 1 )) /*-1 is for giving a place to set null termination at the end of the text*/
	{
		return -1;
	}

    _SlDrvMemZero(ServiceNameAndTextBuffer, (_u16)SL_NETAPP_MDNS_MAX_SERVICE_NAME_AND_TEXT_LENGTH);

	
	/*Copy the service name*/
	sl_Memcpy(ServiceNameAndTextBuffer,
		      pServiceName,   
			  ServiceNameLen);

	if(TextLen > 0 )
	{
		
		TextPtr = &ServiceNameAndTextBuffer[ServiceNameLen];
		/*Copy the text just after the service name*/
		sl_Memcpy(TextPtr,
				  pText,   
				  TextLen);

  
	}

    _SlDrvResetCmdExt(&CmdExt);
    CmdExt.TxPayload1Len = (TextLen + ServiceNameLen);
    CmdExt.pTxPayload1   = (_u8 *)ServiceNameAndTextBuffer;

	
	VERIFY_RET_OK(_SlDrvCmdOp((_SlCmdCtrl_t *)&_SlRegisterServiceCtrl, &Msg, &CmdExt));

	return (_i16)Msg.Rsp.status;

	
}
#endif

/**********************************************************************************************/
#if _SL_INCLUDE_FUNC(sl_NetAppMDNSRegisterService)

_i16 sl_NetAppMDNSRegisterService(	const _i8* 	pServiceName, 
									const _u8   ServiceNameLen,
									const _i8* 	pText,
									const _u8   TextLen,
									const _u16  Port,
									const _u32  TTL,
									     _u32   Options)

{

    /* verify that this api is allowed. if not allowed then
    ignore the API execution and return immediately with an error */
    VERIFY_API_ALLOWED(SL_OPCODE_SILO_NETAPP);

	/*

	NOTE - pay attention

	1. Temporary -  there is an allocation on stack of internal buffer.
	Its size is SL_NETAPP_MDNS_MAX_SERVICE_NAME_AND_TEXT_LENGTH.
	It means that the sum of the text length and service name length cannot be bigger than
	SL_NETAPP_MDNS_MAX_SERVICE_NAME_AND_TEXT_LENGTH.
	If it is - An error is returned.

	2. According to now from certain constraints the variables parameters are set in the
	attribute part (contain constant parameters)

	*/

	/*Set the add service bit in the options parameter.
	  In order not use different opcodes for the register service and unregister service
	  bit 31 in option is taken for this purpose. if it is set it means in NWP that the service should be added
	  if it is cleared it means that the service should be deleted and there is only meaning to pServiceName
	  and ServiceNameLen values. */
	Options |=  SL_NETAPP_MDNS_OPTIONS_ADD_SERVICE_BIT;

    return  _SlNetAppMDNSRegisterUnregisterService(	pServiceName, 
											        ServiceNameLen,
													pText,
													TextLen,
													Port,
													TTL,
													Options);

	
}
#endif
/**********************************************************************************************/



/**********************************************************************************************/
#if _SL_INCLUDE_FUNC(sl_NetAppMDNSUnRegisterService)

_i16 sl_NetAppMDNSUnRegisterService(	const _i8* 		pServiceName, 
									const _u8   ServiceNameLen,_u32 Options)


{

    /* verify that this api is allowed. if not allowed then
    ignore the API execution and return immediately with an error */
    VERIFY_API_ALLOWED(SL_OPCODE_SILO_NETAPP);

	/*
	
	NOTE - pay attention

			The size of the service length  should be smaller than 255,
			Until the simplelink drive supports to variable length through SPI command.


	*/

	/*Clear the add service bit in the options parameter.
	  In order not use different opcodes for the register service and unregister service
	  bit 31 in option is taken for this purpose. if it is set it means in NWP that the service should be added
	  if it is cleared it means that the service should be deleted and there is only meaning to pServiceName
	  and ServiceNameLen values.*/
	
	Options &=  (~SL_NETAPP_MDNS_OPTIONS_ADD_SERVICE_BIT);

    return  _SlNetAppMDNSRegisterUnregisterService(	pServiceName, 
											        ServiceNameLen,
													NULL,
													0,
													0,
													0,
													Options);

	
}
#endif
/**********************************************************************************************/



/*****************************************************************************/
/* sl_DnsGetHostByService */
/*****************************************************************************/
/*
 * The below struct depicts the constant parameters of the command/API sl_DnsGetHostByService.
 *
   1. ServiceLen                      - The length of the service should be smaller than 255.
   2. AddrLen                         - TIPv4 or IPv6 (SL_AF_INET , SL_AF_INET6).
*
 */

typedef struct 
{
	 _u8   ServiceLen;
	 _u8   AddrLen;
	 _u16  Padding;
}_GetHostByServiceCommand_t;



/*
 * The below structure depict the constant parameters that are returned in the Async event answer
 * according to command/API sl_DnsGetHostByService for IPv4 and IPv6.
 *
	1Status						- The status of the response.
	2.Address						- Contains the IP address of the service.
	3.Port							- Contains the port of the service.
	4.TextLen						- Contains the max length of the text that the user wants to get.
												it means that if the test of service is bigger that its value than
												the text is cut to inout_TextLen value.
										Output: Contain the length of the text that is returned. Can be full text or part
												of the text (see above).
															   
*
 */

typedef struct 
{
	_u16   Status;
	_u16   TextLen;
	_u32    Port;
	_u32    Address[4];
}_GetHostByServiceIPv6AsyncResponse_t;

/*
 * The below struct contains pointers to the output parameters that the user gives 
 *
 */
typedef struct
{
    _i16           Status;
	_u32   *out_pAddr;
	_u32   *out_pPort;
	_u16   *inout_TextLen; /* in: max len , out: actual len */
 _i8            *out_pText;
}_GetHostByServiceAsyncResponse_t;



typedef union
{
	_GetHostByServiceCommand_t      Cmd;
	_BasicResponse_t                Rsp;
}_SlGetHostByServiceMsg_u;


#if _SL_INCLUDE_FUNC(sl_NetAppDnsGetHostByService)

static const _SlCmdCtrl_t _SlGetHostByServiceCtrl =
{
    SL_OPCODE_NETAPP_MDNSGETHOSTBYSERVICE,
    (_SlArgSize_t)sizeof(_GetHostByServiceCommand_t),
    (_SlArgSize_t)sizeof(_BasicResponse_t)
};

/******************************************************************************/

_i16 sl_NetAppDnsGetHostByService(_i8 		*pServiceName,	/* string containing all (or only part): name + subtype + service */
								  const _u8  ServiceLen,
								  const _u8  Family,			/* 4-IPv4 , 16-IPv6 */
								  _u32  pAddr[], 
								  _u32  *pPort,
								  _u16 *pTextLen, /* in: max len , out: actual len */
								  _i8          *pText
						         )
{
    _SlGetHostByServiceMsg_u         Msg;
    _SlCmdExt_t                      CmdExt ;
    _GetHostByServiceAsyncResponse_t AsyncRsp;
	_u8 ObjIdx = MAX_CONCURRENT_ACTIONS;

    /* verify that this api is allowed. if not allowed then
    ignore the API execution and return immediately with an error */
    VERIFY_API_ALLOWED(SL_OPCODE_SILO_NETAPP);
    _SlDrvMemZero(&AsyncRsp, sizeof(_GetHostByServiceAsyncResponse_t));

/*
	Note:
	1. The return's attributes are belonged to first service that is found.
	It can be other services with the same service name will response to
	the query. The results of these responses are saved in the peer cache of the NWP, and
	should be read by another API.

	2. Text length can be 120 bytes only - not more
	It is because of constraints in the NWP on the buffer that is allocated for the Async event.

	3.The API waits to Async event by blocking. It means that the API is finished only after an Async event
	is sent by the NWP.

	4.No rolling option!!! - only PTR type is sent.

 
*/
	/*build the attribute part of the command.
	  It contains the constant parameters of the command */

	Msg.Cmd.ServiceLen = ServiceLen;
	Msg.Cmd.AddrLen    = Family;

	/*Build the payload part of the command
	  Copy the service name and text to one buffer.*/

    _SlDrvResetCmdExt(&CmdExt);
	CmdExt.TxPayload1Len = ServiceLen;
    CmdExt.pTxPayload1   = (_u8 *)pServiceName;

	/*set pointers to the output parameters (the returned parameters).
	  This pointers are belonged to local struct that is set to global Async response parameter.
	  It is done in order not to run more than one sl_DnsGetHostByService at the same time.
	  The API should be run only if global parameter is pointed to NULL. */
	AsyncRsp.out_pText     = pText;
	AsyncRsp.inout_TextLen = (_u16* )pTextLen;
	AsyncRsp.out_pPort     = pPort;
	AsyncRsp.out_pAddr     = (_u32 *)&pAddr[0];

    ObjIdx = _SlDrvProtectAsyncRespSetting((_u8*)&AsyncRsp, GETHOSYBYSERVICE_ID, SL_MAX_SOCKETS);

    if (MAX_CONCURRENT_ACTIONS == ObjIdx)
    {
        return SL_POOL_IS_EMPTY;
    }

    
	if (SL_AF_INET6 == Family)  
	{
		g_pCB->ObjPool[ObjIdx].AdditionalData |= SL_NETAPP_FAMILY_MASK;
	}
    /* Send the command */
	VERIFY_RET_OK(_SlDrvCmdOp((_SlCmdCtrl_t *)&_SlGetHostByServiceCtrl, &Msg, &CmdExt));

 
	 
    /* If the immediate reponse is O.K. than  wait for aSYNC event response. */
	if(SL_RET_CODE_OK == Msg.Rsp.status)
    {        
		SL_DRV_SYNC_OBJ_WAIT_FOREVER(&g_pCB->ObjPool[ObjIdx].SyncObj);
        
		/* If we are - it means that Async event was sent.
		   The results are copied in the Async handle return functions */
		
		Msg.Rsp.status = AsyncRsp.Status;
    }

    _SlDrvReleasePoolObj(ObjIdx);
    return Msg.Rsp.status;
}
#endif

/******************************************************************************/

/******************************************************************************
    _SlNetAppHandleAsync_DnsGetHostByService

    CALLER          NWP - Async event on sl_DnsGetHostByService with IPv4 Family


    DESCRIPTION: 
					
					Async event on sl_DnsGetHostByService command with IPv4 Family.
					Return service attributes like IP address, port and text according to service name.
					The user sets a service name Full/Part (see example below), and should get the:
					1. IP of the service
					2. The port of service.
					3. The text of service.

					Hence it can make a connection to the specific service and use it.
					It is similar to get host by name method.

					It is done by a single shot query with PTR type on the service name.



					Note:
					1. The return's attributes are belonged to first service that is found.
					It can be other services with the same service name will response to
					the query. The results of these responses are saved in the peer cache of the NWP, and
					should be read by another API.

	
	    PARAMETERS:

                  pVoidBuf - is point to opcode of the event.
				  it contains the outputs that are given to the user

				  outputs description:

				   1.out_pAddr[]					- output: Contain the IP address of the service.
				   2.out_pPort						- output: Contain the port of the service.
				   3.inout_TextLen					- Input:  Contain the max length of the text that the user wants to get.
															  it means that if the test of service is bigger that its value than
															  the text is cut to inout_TextLen value.
													  Output: Contain the length of the text that is returned. Can be full text or part
														      of the text (see above).
															   
				   4.out_pText						- Contain the text of the service (full or part see above- inout_TextLen description).

  *


    RETURNS:        success or fail.





******************************************************************************/
#ifndef SL_TINY
_SlReturnVal_t _SlNetAppHandleAsync_DnsGetHostByService(void *pVoidBuf)
{
	_u16 				  TextLen;
	_u16 				  UserTextLen;
    _GetHostByServiceAsyncResponse_t* Res= NULL;
    _GetHostByServiceIPv6AsyncResponse_t    *pMsgArgs = (_GetHostByServiceIPv6AsyncResponse_t *)_SL_RESP_ARGS_START(pVoidBuf);

    VERIFY_SOCKET_CB(NULL != g_pCB->ObjPool[g_pCB->FunctionParams.AsyncExt.ActionIndex].pRespArgs);
    /*Res pointed to mDNS global object struct */
    Res = (_GetHostByServiceAsyncResponse_t*)g_pCB->ObjPool[g_pCB->FunctionParams.AsyncExt.ActionIndex].pRespArgs;
    /*IPv6*/
    if(g_pCB->ObjPool[g_pCB->FunctionParams.AsyncExt.ActionIndex].AdditionalData & SL_NETAPP_FAMILY_MASK)
    {
        Res->out_pAddr[1]	= pMsgArgs->Address[1]; /* Copy data from IPv6 address to Host user's pAddr. The array must be at least 4 cells of _u32 */
        Res->out_pAddr[2]	= pMsgArgs->Address[2];
        Res->out_pAddr[3]	= pMsgArgs->Address[3];
    }
   
    TextLen = pMsgArgs->TextLen;
    
    /*It is 4 bytes so we avoid from memcpy*/
    Res->out_pAddr[0]	= pMsgArgs->Address[0];   /* Copy first cell data from IPv4/6 address to Host user's pAddr */
    Res->out_pPort[0]	= pMsgArgs->Port;
	Res->Status			= (_i16)pMsgArgs->Status;
    /*set to TextLen the text length of the user (input fromthe user).*/
    UserTextLen			= Res->inout_TextLen[0];

    /*Cut the service text if the user requested for smaller text.*/
    UserTextLen = (TextLen <= UserTextLen) ? TextLen : UserTextLen;
    Res->inout_TextLen[0] = UserTextLen ;

    /**************************************************************************************************

    2. Copy the payload part of the evnt (the text) to the payload part of the response
    the lenght of the copy is according to the text length in the attribute part. */


    /*IPv6*/
    if (g_pCB->ObjPool[g_pCB->FunctionParams.AsyncExt.ActionIndex].AdditionalData & SL_NETAPP_FAMILY_MASK)
    {
           sl_Memcpy(Res->out_pText          ,
     (_i8 *)(& pMsgArgs[1])  ,   /* & pMsgArgs[1] -> 1st byte after the fixed header = 1st byte of variable text.*/
     UserTextLen              );
    }
    else
    {
           sl_Memcpy(Res->out_pText          ,
                  (_i8 *)(& pMsgArgs->Address[1])  ,   /* & pMsgArgs[1] -> 1st byte after the fixed header = 1st byte of variable text.*/
     UserTextLen              );
    }


		SL_DRV_SYNC_OBJ_SIGNAL(&g_pCB->ObjPool[g_pCB->FunctionParams.AsyncExt.ActionIndex].SyncObj);
		
    return SL_OS_RET_CODE_OK;
}


/*****************************************************************************/
/*  _SlNetAppHandleAsync_DnsGetHostByAddr */
/*****************************************************************************/
_SlReturnVal_t _SlNetAppHandleAsync_DnsGetHostByAddr(void *pVoidBuf)
{
    SL_TRACE0(DBG_MSG, MSG_303, "STUB: _SlNetAppHandleAsync_DnsGetHostByAddr not implemented yet!");
    return SL_OS_RET_CODE_OK;
}
#endif

/*****************************************************************************/
/* sl_DnsGetHostByName */
/*****************************************************************************/
typedef union
{
    NetAppGetHostByNameIPv4AsyncResponse_t IpV4;
    NetAppGetHostByNameIPv6AsyncResponse_t IpV6;
}_GetHostByNameAsyncResponse_u;

typedef union
{
	NetAppGetHostByNameCommand_t         Cmd;
	_BasicResponse_t                Rsp;
}_SlGetHostByNameMsg_u;


#if _SL_INCLUDE_FUNC(sl_NetAppDnsGetHostByName)
static const _SlCmdCtrl_t _SlGetHostByNameCtrl =
{
    SL_OPCODE_NETAPP_DNSGETHOSTBYNAME,
    (_SlArgSize_t)sizeof(NetAppGetHostByNameCommand_t),
    (_SlArgSize_t)sizeof(_BasicResponse_t)
};

_i16 sl_NetAppDnsGetHostByName(_i8 * pHostName,const  _u16 NameLen, _u32*  OutIpAddr,const _u8 Family )
{
    _SlGetHostByNameMsg_u           Msg;
    _SlCmdExt_t                     ExtCtrl;
    _GetHostByNameAsyncResponse_u   AsyncRsp;
	_u8 ObjIdx = MAX_CONCURRENT_ACTIONS;

    /* verify that this api is allowed. if not allowed then
    ignore the API execution and return immediately with an error */
    VERIFY_API_ALLOWED(SL_OPCODE_SILO_NETAPP);

    _SlDrvResetCmdExt(&ExtCtrl);
    ExtCtrl.TxPayload1Len = NameLen;
    ExtCtrl.pTxPayload1 = (_u8 *)pHostName;

    Msg.Cmd.Len = NameLen;
    Msg.Cmd.Family = Family;

	/*Use Obj to issue the command, if not available try later */
	ObjIdx = (_u8)_SlDrvWaitForPoolObj(GETHOSYBYNAME_ID,SL_MAX_SOCKETS);
	if (MAX_CONCURRENT_ACTIONS == ObjIdx)
	{
		return SL_POOL_IS_EMPTY;
	}

    SL_DRV_PROTECTION_OBJ_LOCK_FOREVER();

	g_pCB->ObjPool[ObjIdx].pRespArgs =  (_u8 *)&AsyncRsp;
	/*set bit to indicate IPv6 address is expected */
	if (SL_AF_INET6 == Family)
	{
		g_pCB->ObjPool[ObjIdx].AdditionalData |= SL_NETAPP_FAMILY_MASK;
	}
	
    SL_DRV_PROTECTION_OBJ_UNLOCK();

    VERIFY_RET_OK(_SlDrvCmdOp((_SlCmdCtrl_t *)&_SlGetHostByNameCtrl, &Msg, &ExtCtrl));

    if(SL_RET_CODE_OK == Msg.Rsp.status)
    {
    	SL_DRV_SYNC_OBJ_WAIT_FOREVER(&g_pCB->ObjPool[ObjIdx].SyncObj);
        
        Msg.Rsp.status = (_i16)AsyncRsp.IpV4.Status;

        if(SL_OS_RET_CODE_OK == (_i16)Msg.Rsp.status)
        {
            sl_Memcpy((_i8 *)OutIpAddr,
                      (_i8 *)&AsyncRsp.IpV4.Ip0, 
                      (SL_AF_INET == Family) ? SL_IPV4_ADDRESS_SIZE : SL_IPV6_ADDRESS_SIZE);
        }
    }
    _SlDrvReleasePoolObj(ObjIdx);
    return Msg.Rsp.status;
}
#endif


/******************************************************************************/
/*  _SlNetAppHandleAsync_DnsGetHostByName */
/******************************************************************************/
_SlReturnVal_t _SlNetAppHandleAsync_DnsGetHostByName(void *pVoidBuf)
{
    NetAppGetHostByNameIPv4AsyncResponse_t     *pMsgArgs   = (NetAppGetHostByNameIPv4AsyncResponse_t *)_SL_RESP_ARGS_START(pVoidBuf);

    SL_DRV_PROTECTION_OBJ_LOCK_FOREVER();

    VERIFY_SOCKET_CB(NULL != g_pCB->ObjPool[g_pCB->FunctionParams.AsyncExt.ActionIndex].pRespArgs);

	/*IPv6 */
	if(g_pCB->ObjPool[g_pCB->FunctionParams.AsyncExt.ActionIndex].AdditionalData & SL_NETAPP_FAMILY_MASK)
	{
		sl_Memcpy(g_pCB->ObjPool[g_pCB->FunctionParams.AsyncExt.ActionIndex].pRespArgs, pMsgArgs, sizeof(NetAppGetHostByNameIPv6AsyncResponse_t));
	}
	/*IPv4 */
	else
	{
		sl_Memcpy(g_pCB->ObjPool[g_pCB->FunctionParams.AsyncExt.ActionIndex].pRespArgs, pMsgArgs, sizeof(NetAppGetHostByNameIPv4AsyncResponse_t));
	}
    
	SL_DRV_SYNC_OBJ_SIGNAL(&g_pCB->ObjPool[g_pCB->FunctionParams.AsyncExt.ActionIndex].SyncObj);
    SL_DRV_PROTECTION_OBJ_UNLOCK();
    return SL_OS_RET_CODE_OK;
}


#ifndef SL_TINY
static void _SlNetAppCopyPingResultsToReport(SlPingReportResponse_t *pResults,SlNetAppPingReport_t *pReport)
{
    pReport->PacketsSent     = pResults->NumSendsPings;
    pReport->PacketsReceived = pResults->NumSuccsessPings;
    pReport->MinRoundTime    = pResults->RttMin;
    pReport->MaxRoundTime    = pResults->RttMax;
    pReport->AvgRoundTime    = pResults->RttAvg;
    pReport->TestTime        = pResults->TestTime;
}

/*****************************************************************************/
/*  _SlNetAppHandleAsync_PingResponse */
/*****************************************************************************/
_SlReturnVal_t _SlNetAppHandleAsync_PingResponse(void *pVoidBuf)
{
    SlPingReportResponse_t     *pMsgArgs   = (SlPingReportResponse_t *)_SL_RESP_ARGS_START(pVoidBuf);
    SlNetAppPingReport_t        pingReport;
    
    if(pPingCallBackFunc)
    {
        _SlNetAppCopyPingResultsToReport(pMsgArgs,&pingReport);
        pPingCallBackFunc(&pingReport);
    }
    else
    {
       
        SL_DRV_PROTECTION_OBJ_LOCK_FOREVER();
        
        VERIFY_SOCKET_CB(NULL != g_pCB->PingCB.PingAsync.pAsyncRsp);

		if (NULL != g_pCB->ObjPool[g_pCB->FunctionParams.AsyncExt.ActionIndex].pRespArgs)
		{
		   sl_Memcpy(g_pCB->ObjPool[g_pCB->FunctionParams.AsyncExt.ActionIndex].pRespArgs, pMsgArgs, sizeof(SlPingReportResponse_t));
		   SL_DRV_SYNC_OBJ_SIGNAL(&g_pCB->ObjPool[g_pCB->FunctionParams.AsyncExt.ActionIndex].SyncObj);
		}
        SL_DRV_PROTECTION_OBJ_UNLOCK();
    }

    return SL_OS_RET_CODE_OK;
}
#endif

/*****************************************************************************/
/* sl_NetAppPing */
/*****************************************************************************/
typedef union
{
	SlNetAppPingCommand_t   Cmd;
	SlPingReportResponse_t Rsp;
}_SlPingStartMsg_u;


typedef enum
{
  CMD_PING_TEST_RUNNING = 0,
  CMD_PING_TEST_STOPPED
}_SlPingStatus_e;


#if _SL_INCLUDE_FUNC(sl_NetAppPing)
_i16 sl_NetAppPing(const SlNetAppPingCommand_t* pPingParams, const _u8 Family, SlNetAppPingReport_t *pReport, const P_SL_DEV_PING_CALLBACK pPingCallback)
{
    _SlCmdCtrl_t                CmdCtrl = {0, (_SlArgSize_t)sizeof(SlNetAppPingCommand_t), (_SlArgSize_t)sizeof(_BasicResponse_t)};
    _SlPingStartMsg_u           Msg;
    SlPingReportResponse_t      PingRsp;
    _u8 ObjIdx = MAX_CONCURRENT_ACTIONS;
	_u32 PingTimeout = 0;

    /* verify that this api is allowed. if not allowed then
    ignore the API execution and return immediately with an error */
    VERIFY_API_ALLOWED(SL_OPCODE_SILO_NETAPP);

	if(NULL != pPingParams)
	{

		if(SL_AF_INET == Family)
		{
			CmdCtrl.Opcode = SL_OPCODE_NETAPP_PINGSTART;
			sl_Memcpy(&Msg.Cmd.Ip, &pPingParams->Ip, SL_IPV4_ADDRESS_SIZE);
		}
		else
		{
			CmdCtrl.Opcode = SL_OPCODE_NETAPP_PINGSTART_V6;
			sl_Memcpy(&Msg.Cmd.Ip, &pPingParams->Ip, SL_IPV6_ADDRESS_SIZE);
		}

		Msg.Cmd.PingIntervalTime        = pPingParams->PingIntervalTime;
		Msg.Cmd.PingSize                = pPingParams->PingSize;
		Msg.Cmd.PingRequestTimeout      = pPingParams->PingRequestTimeout;
		Msg.Cmd.TotalNumberOfAttempts   = pPingParams->TotalNumberOfAttempts;
		Msg.Cmd.Flags                   = pPingParams->Flags;

    
		/* calculate the ping timeout according to the parmas + the guard interval */
		PingTimeout = SL_NETAPP_PING_GUARD_INTERVAL + (pPingParams->PingIntervalTime * pPingParams->TotalNumberOfAttempts);

		if (Msg.Cmd.Ip != 0)
		{
		
			/*	If the following conditions are met, return an error 
				Wrong ping parameters - ping cannot be called with the following parameters: 
				1. infinite ping packet
				2. report only when finished 
				3. no callback supplied  */
			if ((pPingCallback == NULL) && (pPingParams->Flags == 0) && (pPingParams->TotalNumberOfAttempts == 0))
			{
				return SL_RET_CODE_NET_APP_PING_INVALID_PARAMS;
			}
	
			if( pPingCallback )
			{	
				pPingCallBackFunc = pPingCallback;
			} 
			else
			{
			   /* Use Obj to issue the command, if not available try later */
			   ObjIdx = (_u8)_SlDrvWaitForPoolObj(PING_ID,SL_MAX_SOCKETS);
			   if (MAX_CONCURRENT_ACTIONS == ObjIdx)
			   {
				  return SL_POOL_IS_EMPTY;
			   }
			   OSI_RET_OK_CHECK(sl_LockObjLock(&g_pCB->ProtectionLockObj, SL_OS_WAIT_FOREVER));
				/* async response handler for non callback mode */
			   g_pCB->ObjPool[ObjIdx].pRespArgs = (_u8 *)&PingRsp;
			   pPingCallBackFunc = NULL;
			   OSI_RET_OK_CHECK(sl_LockObjUnlock(&g_pCB->ProtectionLockObj));
			}
		}
	}
	/* Issue Stop Command */
	else
	{
		CmdCtrl.Opcode = SL_OPCODE_NETAPP_PINGSTART;
		Msg.Cmd.Ip = 0;
	}
    /* send the command */
    VERIFY_RET_OK(_SlDrvCmdOp(&CmdCtrl, &Msg, NULL));
	if (Msg.Cmd.Ip != 0)
	{
		if(CMD_PING_TEST_RUNNING == (_i16)Msg.Rsp.Status || CMD_PING_TEST_STOPPED == (_i16)Msg.Rsp.Status )
		{
			/* block waiting for results if no callback function is used */
			if( NULL == pPingCallback ) 
			{
#ifdef SL_TINY        
				_SlDrvSyncObjWaitForever(&g_pCB->ObjPool[ObjIdx].SyncObj);
#else       
				SL_DRV_SYNC_OBJ_WAIT_TIMEOUT(&g_pCB->ObjPool[ObjIdx].SyncObj,
										PingTimeout,
										SL_OPCODE_NETAPP_PINGREPORTREQUESTRESPONSE
										);
#endif

				if( SL_OS_RET_CODE_OK == (_i16)PingRsp.Status )
				{
					_SlNetAppCopyPingResultsToReport(&PingRsp,pReport);
				}
				_SlDrvReleasePoolObj(ObjIdx);
				
			}
		}
		else
		{   /* ping failure, no async response */
			if( NULL == pPingCallback ) 
			{	
				_SlDrvReleasePoolObj(ObjIdx);
			}
		}
	
	}
    return (_i16)Msg.Rsp.Status;
}
#endif

/*****************************************************************************/
/* sl_NetAppSet */
/*****************************************************************************/
typedef union
{
    SlNetAppSetGet_t    Cmd;
    _BasicResponse_t    Rsp;
}_SlNetAppMsgSet_u;


#if _SL_INCLUDE_FUNC(sl_NetAppSet)

static const _SlCmdCtrl_t _SlNetAppSetCmdCtrl =
{
    SL_OPCODE_NETAPP_NETAPPSET,
    (_SlArgSize_t)sizeof(SlNetAppSetGet_t),
    (_SlArgSize_t)sizeof(_BasicResponse_t)
};

_i16 sl_NetAppSet(const _u8 AppId ,const _u8 Option, const _u8 OptionLen, const _u8 *pOptionValue)
{
    _SlNetAppMsgSet_u         Msg;
    _SlCmdExt_t               CmdExt;

    /* verify that this api is allowed. if not allowed then
    ignore the API execution and return immediately with an error */
    VERIFY_API_ALLOWED(SL_OPCODE_SILO_NETAPP);

    _SlDrvResetCmdExt(&CmdExt);
	CmdExt.TxPayload1Len = (OptionLen+3) & (~3);
    CmdExt.pTxPayload1 = (_u8 *)pOptionValue;


    Msg.Cmd.AppId    = AppId;
    Msg.Cmd.ConfigLen   = OptionLen;
	Msg.Cmd.ConfigOpt   = Option;

    VERIFY_RET_OK(_SlDrvCmdOp((_SlCmdCtrl_t *)&_SlNetAppSetCmdCtrl, &Msg, &CmdExt));

    return (_i16)Msg.Rsp.status;
}
#endif

/*****************************************************************************/
/* sl_NetAppSendTokenValue */
/*****************************************************************************/
typedef union
{
    SlNetAppHttpServerSendToken_t    Cmd;
    _BasicResponse_t   Rsp;
}_SlNetAppMsgSendTokenValue_u;


const _SlCmdCtrl_t _SlNetAppSendTokenValueCmdCtrl =
{
    SL_OPCODE_NETAPP_HTTPSENDTOKENVALUE,
    (_SlArgSize_t)sizeof(SlNetAppHttpServerSendToken_t),
    (_SlArgSize_t)sizeof(_BasicResponse_t)
};

_u16 _SlNetAppSendTokenValue(SlNetAppHttpServerData_t * Token_value)
{
	_SlNetAppMsgSendTokenValue_u    Msg;
    _SlCmdExt_t     		        CmdExt;

	_SlDrvMemZero(&CmdExt, (_u16)sizeof(_SlCmdExt_t));

	CmdExt.TxPayload1Len = (Token_value->ValueLen+3) & (~3);
	CmdExt.pTxPayload1 = (_u8 *) Token_value->pTokenValue;

	Msg.Cmd.TokenValueLen = Token_value->ValueLen;
	Msg.Cmd.TokenNameLen = Token_value->NameLen;
	sl_Memcpy(&Msg.Cmd.TokenName[0], Token_value->pTokenName, Token_value->NameLen);
	

	VERIFY_RET_OK(_SlDrvCmdSend_noLock((_SlCmdCtrl_t *)&_SlNetAppSendTokenValueCmdCtrl, &Msg, &CmdExt));

	return Msg.Rsp.status;
}


/*****************************************************************************/
/* sl_NetAppSendResponse */
/*****************************************************************************/
#ifndef SL_TINY 
typedef union
{
    SlProtocolNetAppResponse_t Cmd;
    _BasicResponse_t   Rsp;
}_SlNetAppMsgSendResponse_u;


const _SlCmdCtrl_t _SlNetAppSendResponseCmdCtrl =
{
    SL_OPCODE_NETAPP_RESPONSE,
    sizeof(SlProtocolNetAppResponse_t),
    sizeof(_BasicResponse_t)
};

_u16 _SlNetAppSendResponse( _u16 handle, SlNetAppResponse_t *NetAppResponse)
{
    _SlNetAppMsgSendResponse_u    Msg;
    _SlCmdExt_t		              CmdExt;
    _SlReturnVal_t                RetVal;
    _u16 dataLen;
   
	_SlDrvMemZero(&CmdExt, (_u16)sizeof(_SlCmdExt_t));

    dataLen = NetAppResponse->ResponseData.MetadataLen + NetAppResponse->ResponseData.PayloadLen;

    if ((NetAppResponse->ResponseData.MetadataLen <= SL_NETAPP_REQUEST_MAX_METADATA_LEN) && (dataLen <= SL_NETAPP_REQUEST_MAX_DATA_LEN))
    {
        if (dataLen > 0)
        {
			/* Zero copy of the two parts: metadata + payload */
			CmdExt.pTxPayload1 = NetAppResponse->ResponseData.pMetadata;
			CmdExt.TxPayload1Len =  NetAppResponse->ResponseData.MetadataLen;

			CmdExt.pTxPayload2 = NetAppResponse->ResponseData.pPayload;
			CmdExt.TxPayload2Len = NetAppResponse->ResponseData.PayloadLen;    
        }
        else
        {
            CmdExt.pTxPayload1 = NULL;
			CmdExt.pTxPayload2 = NULL;
        }
 
        CmdExt.RxPayloadLen = 0;
        CmdExt.pRxPayload = NULL;

		Msg.Cmd.Handle = handle;
        Msg.Cmd.status = NetAppResponse->Status;
        Msg.Cmd.MetadataLen = NetAppResponse->ResponseData.MetadataLen;
        Msg.Cmd.PayloadLen = NetAppResponse->ResponseData.PayloadLen;
        Msg.Cmd.Flags = NetAppResponse->ResponseData.Flags;

		RetVal = _SlDrvCmdSend_noLock((_SlCmdCtrl_t *)&_SlNetAppSendResponseCmdCtrl, &Msg, &CmdExt);

    }
    else
    {
        /* TODO: how to return the error code asynchronously? */
        RetVal = SL_ERROR_BSD_ENOMEM;
    }

	return RetVal;
}


/*****************************************************************************/
/* sl_NetAppRecv */
/*****************************************************************************/
typedef union
{
	SlProtocolNetAppReceiveRequest_t  Cmd;
    _BasicResponse_t                    Rsp; /* Not used. do we need it? */
}_SlNetAppReceiveMsg_u;

#if _SL_INCLUDE_FUNC(sl_NetAppRecv)

const _SlCmdCtrl_t _SlNetAppReceiveCmdCtrl =
{
    SL_OPCODE_NETAPP_RECEIVEREQUEST,
    sizeof(SlProtocolNetAppReceiveRequest_t),
    sizeof(_BasicResponse_t)     /* Where is this used? */
};

_SlReturnVal_t sl_NetAppRecv( _u16 Handle, _u16 *DataLen, _u8 *pData, _u32 *Flags)
{
    _SlNetAppReceiveMsg_u   Msg;
    _SlCmdExt_t             CmdExt;
    SlProtocolNetAppReceive_t AsyncRsp; /* Will be filled when SL_OPCODE_NETAPP_RECEIVE async event is arrived */

    _SlReturnVal_t RetVal;
    _u8 ObjIdx = MAX_CONCURRENT_ACTIONS;
    _SlArgsData_t pArgsData;

    /* Validate input arguments */
    if ((NULL == pData) || (0==DataLen))
    {
        return SL_ERROR_BSD_EINVAL;
    }

    /* Save the user RX bufer. Rx data will be copied into it on the SL_OPCODE_NETAPP_RECEIVE async event */
    _SlDrvResetCmdExt(&CmdExt);
    CmdExt.RxPayloadLen = *DataLen;
    CmdExt.pRxPayload = pData;

    /* Prepare the command args */
    Msg.Cmd.Handle = Handle;
    Msg.Cmd.MaxBufferLen = *DataLen;
    Msg.Cmd.Flags = *Flags;

    /* Use Obj to issue the command, if not available try later */
    ObjIdx = (_u8)_SlDrvWaitForPoolObj(NETAPP_RECEIVE_ID, SL_MAX_SOCKETS);

    if (MAX_CONCURRENT_ACTIONS == ObjIdx)
    {
        return SL_POOL_IS_EMPTY;
    }

    /* Save the AsyncRsp and cmdExt information for the SL_OPCODE_NETAPP_RECEIVE async event */
    AsyncRsp.Handle = Handle; /* Handle we are waiting for */
    AsyncRsp.Flags = 0; 
    AsyncRsp.PayloadLen = 0; /* 0 will indicate an error in the SL_OPCODE_NETAPP_RECEIVE async event and that no data arrived. */

    _SlDrvProtectionObjLockWaitForever();

    pArgsData.pData = (_u8 *) &CmdExt;
    pArgsData.pArgs = (_u8 *) &AsyncRsp;

    g_pCB->ObjPool[ObjIdx].pRespArgs =  (_u8 *)&pArgsData;

    _SlDrvProtectionObjUnLock();

    /* Send the command */
    RetVal = _SlDrvCmdSend((_SlCmdCtrl_t *)&_SlNetAppReceiveCmdCtrl, &Msg, &CmdExt);

    if(SL_OS_RET_CODE_OK == RetVal)
    {
        /* Wait for SL_OPCODE_NETAPP_RECEIVE async event. Will be signaled by _SlNetAppHandleAsync_NetAppReceive. */
        _SlDrvSyncObjWaitForever(&g_pCB->ObjPool[ObjIdx].SyncObj);

        /* Update information for the user */
        *DataLen = AsyncRsp.PayloadLen;
        *Flags = AsyncRsp.Flags;
    }

    _SlDrvReleasePoolObj(ObjIdx);

    return RetVal;
}

#endif

/*****************************************************************************/
/*  _SlNetAppHandleAsync_NetAppReceive */
/*****************************************************************************/
void _SlNetAppHandleAsync_NetAppReceive(void *pVoidBuf)
{
    _u8 *pData;
    _u16 len;
    SlProtocolNetAppReceive_t *AsyncRsp;
    _SlCmdExt_t                 *CmdExt;
    SlProtocolNetAppReceive_t *pMsgArgs = (SlProtocolNetAppReceive_t *)_SL_RESP_ARGS_START(pVoidBuf);

    pData = (_u8 *)((SlProtocolNetAppReceive_t *)pMsgArgs + 1); /* Points to the netapp receive payload */

    _SlDrvProtectionObjLockWaitForever();

    if (NULL != g_pCB->ObjPool[g_pCB->FunctionParams.AsyncExt.ActionIndex].pRespArgs)
    {
        AsyncRsp =  (SlProtocolNetAppReceive_t *) ((_SlArgsData_t *)(g_pCB->ObjPool[g_pCB->FunctionParams.AsyncExt.ActionIndex].pRespArgs))-> pArgs;
        CmdExt = (_SlCmdExt_t *) ((_SlArgsData_t *)(g_pCB->ObjPool[g_pCB->FunctionParams.AsyncExt.ActionIndex].pRespArgs))-> pData;

        if (pMsgArgs->Handle == AsyncRsp->Handle)
        {
            if (pMsgArgs->PayloadLen <= CmdExt->RxPayloadLen)
            {
                len = pMsgArgs->PayloadLen;
            }
            else
            {
                len = CmdExt->RxPayloadLen;
            }

            /* Copy the data to the user buffer */
            sl_Memcpy (CmdExt->pRxPayload, pData, len);

            /* Update len and flags */
            AsyncRsp->PayloadLen = len;
            AsyncRsp->Flags = pMsgArgs->Flags;
        }
    }

    _SlDrvSyncObjSignal(&g_pCB->ObjPool[g_pCB->FunctionParams.AsyncExt.ActionIndex].SyncObj);
    _SlDrvProtectionObjUnLock();

    return;
}

#endif

/*****************************************************************************/
/* sl_NetAppSend */
/*****************************************************************************/
typedef union
{
     SlProtocolNetAppSend_t    Cmd;
    _BasicResponse_t           Rsp;
}_SlNetAppMsgSend_u;


const _SlCmdCtrl_t _SlNetAppSendCmdCtrl =
{
    SL_OPCODE_NETAPP_SEND,
    sizeof(SlProtocolNetAppSend_t),
    sizeof(_BasicResponse_t)
};

_u16 sl_NetAppSend( _u16 Handle, _u16 DataLen, _u8* pData, _u32 Flags)
{
	_SlNetAppMsgSend_u      Msg;
    _SlCmdExt_t			    CmdExt;

	_SlDrvMemZero(&CmdExt, (_u16)sizeof(_SlCmdExt_t));

    if  ((((Flags & SL_NETAPP_REQUEST_RESPONSE_FLAGS_METADATA) == SL_NETAPP_REQUEST_RESPONSE_FLAGS_METADATA) && (DataLen <= SL_NETAPP_REQUEST_MAX_METADATA_LEN)) || 
            (((Flags & SL_NETAPP_REQUEST_RESPONSE_FLAGS_METADATA) == 0) && (DataLen <= SL_NETAPP_REQUEST_MAX_DATA_LEN)))
    {
	    CmdExt.TxPayload1Len = (DataLen+3) & (~3);
	    CmdExt.pTxPayload1 = (_u8 *) pData;

	    Msg.Cmd.Handle = Handle;
        Msg.Cmd.DataLen = DataLen;    
        Msg.Cmd.Flags = Flags;

	    VERIFY_RET_OK(_SlDrvCmdOp((_SlCmdCtrl_t *)&_SlNetAppSendCmdCtrl, &Msg, &CmdExt));
    }
    else
    {
        Msg.Rsp.status = SL_ERROR_BSD_ENOMEM;
    }

	return Msg.Rsp.status;
}


/*****************************************************************************/
/* sl_NetAppGet */
/*****************************************************************************/
typedef union
{
	SlNetAppSetGet_t	    Cmd;
	SlNetAppSetGet_t	    Rsp;
}_SlNetAppMsgGet_u;


#if _SL_INCLUDE_FUNC(sl_NetAppGet)
static const _SlCmdCtrl_t _SlNetAppGetCmdCtrl =
{
    SL_OPCODE_NETAPP_NETAPPGET,
    (_SlArgSize_t)sizeof(SlNetAppSetGet_t),
    (_SlArgSize_t)sizeof(SlNetAppSetGet_t)
};

_i16 sl_NetAppGet(const _u8 AppId, const _u8 Option,_u8 *pOptionLen, _u8 *pOptionValue)
{
    _SlNetAppMsgGet_u         Msg;
    _SlCmdExt_t               CmdExt;

    /* verify that this api is allowed. if not allowed then
    ignore the API execution and return immediately with an error */
    VERIFY_API_ALLOWED(SL_OPCODE_SILO_NETAPP);

       if (*pOptionLen == 0)
       {
              return SL_EZEROLEN;
       }

    _SlDrvResetCmdExt(&CmdExt);
    CmdExt.RxPayloadLen = (_i16)(*pOptionLen);
    CmdExt.pRxPayload = (_u8 *)pOptionValue;

    Msg.Cmd.AppId    = AppId;
    Msg.Cmd.ConfigOpt   = Option;
    VERIFY_RET_OK(_SlDrvCmdOp((_SlCmdCtrl_t *)&_SlNetAppGetCmdCtrl, &Msg, &CmdExt));
    

       if (CmdExt.RxPayloadLen < CmdExt.ActualRxPayloadLen) 
       {
              *pOptionLen = (_u8)CmdExt.RxPayloadLen;
              return SL_ESMALLBUF;
       }
       else
       {
              *pOptionLen = (_u8)CmdExt.ActualRxPayloadLen;
       }
  
    return (_i16)Msg.Rsp.Status;
}
#endif





/*****************************************************************************/
/* _SlNetAppEventHandler */
/*****************************************************************************/
_SlReturnVal_t _SlNetAppEventHandler(void* pArgs)
{
    _SlResponseHeader_t     *pHdr       = (_SlResponseHeader_t *)pArgs;
#if defined(slcb_NetAppHttpServerHdlr) || defined(EXT_LIB_REGISTERED_HTTP_SERVER_EVENTS)
    SlNetAppHttpServerEvent_t		httpServerEvent;
    SlNetAppHttpServerResponse_t	httpServerResponse;
#endif
    switch(pHdr->GenHeader.Opcode)
    {
        case SL_OPCODE_NETAPP_DNSGETHOSTBYNAMEASYNCRESPONSE:
        case SL_OPCODE_NETAPP_DNSGETHOSTBYNAMEASYNCRESPONSE_V6:
            _SlNetAppHandleAsync_DnsGetHostByName(pArgs);
            break;
#ifndef SL_TINY            
        case SL_OPCODE_NETAPP_MDNSGETHOSTBYSERVICEASYNCRESPONSE:
        case SL_OPCODE_NETAPP_MDNSGETHOSTBYSERVICEASYNCRESPONSE_V6:
            _SlNetAppHandleAsync_DnsGetHostByService(pArgs);
            break;
        case SL_OPCODE_NETAPP_PINGREPORTREQUESTRESPONSE:
            _SlNetAppHandleAsync_PingResponse(pArgs);
            break;
#endif

		case SL_OPCODE_NETAPP_HTTPGETTOKENVALUE:
		{              
#if defined(slcb_NetAppHttpServerHdlr) || defined(EXT_LIB_REGISTERED_HTTP_SERVER_EVENTS)
                    _u8 *pTokenName;
                    SlNetAppHttpServerData_t Token_value;
                    SlNetAppHttpServerGetToken_t *httpGetToken = (SlNetAppHttpServerGetToken_t *)_SL_RESP_ARGS_START(pHdr);
                    pTokenName = (_u8 *)((SlNetAppHttpServerGetToken_t *)httpGetToken + 1);

                    httpServerResponse.Response = SL_NETAPP_HTTPSETTOKENVALUE;
                    httpServerResponse.ResponseData.TokenValue.Len = SL_NETAPP_MAX_TOKEN_VALUE_LEN;

                    /* Reuse the async buffer for getting the token value response from the user */
                    httpServerResponse.ResponseData.TokenValue.pData = (_u8 *)_SL_RESP_ARGS_START(pHdr) + SL_NETAPP_MAX_TOKEN_NAME_LEN;

                    httpServerEvent.Event = SL_NETAPP_EVENT_HTTP_TOKEN_GET;
                    httpServerEvent.EventData.HttpTokenName.Len = httpGetToken->TokenNameLen;
                    httpServerEvent.EventData.HttpTokenName.pData = pTokenName;

                    Token_value.pTokenName =  pTokenName;

                    _SlDrvDispatchHttpServerEvents (&httpServerEvent, &httpServerResponse);

                    Token_value.ValueLen = httpServerResponse.ResponseData.TokenValue.Len;
                    Token_value.NameLen = httpServerEvent.EventData.HttpTokenName.Len;
                    Token_value.pTokenValue = httpServerResponse.ResponseData.TokenValue.pData;
                        
                    _SlNetAppSendTokenValue(&Token_value);
#else

                    _u8 *pTokenName;
                    SlNetAppHttpServerData_t Token_value;
                    SlNetAppHttpServerGetToken_t *httpGetToken = (SlNetAppHttpServerGetToken_t*)_SL_RESP_ARGS_START(pHdr);
                    pTokenName = (_u8 *)((SlNetAppHttpServerGetToken_t *)httpGetToken + 1);

                    Token_value.pTokenName =  pTokenName;
                    Token_value.ValueLen = 0;
                    Token_value.NameLen = httpGetToken->TokenNameLen;
                    Token_value.pTokenValue = NULL;
                        
                    _SlNetAppSendTokenValue(&Token_value);
#endif
		}
		break;

		case SL_OPCODE_NETAPP_HTTPPOSTTOKENVALUE:
		{
#if defined(slcb_NetAppHttpServerHdlr) || defined(EXT_LIB_REGISTERED_HTTP_SERVER_EVENTS)
			_u8 *pPostParams;

			SlNetAppHttpServerPostToken_t *httpPostTokenArgs = (SlNetAppHttpServerPostToken_t *)_SL_RESP_ARGS_START(pHdr);
			pPostParams = (_u8 *)((SlNetAppHttpServerPostToken_t *)httpPostTokenArgs + 1);

			httpServerEvent.Event = SL_NETAPP_EVENT_HTTP_TOKEN_POST;

			httpServerEvent.EventData.HttpPostData.Action.Len = httpPostTokenArgs->PostActionLen;
			httpServerEvent.EventData.HttpPostData.Action.pData = pPostParams;
			pPostParams+=httpPostTokenArgs->PostActionLen;

			httpServerEvent.EventData.HttpPostData.TokenName.Len = httpPostTokenArgs->TokenNameLen;
			httpServerEvent.EventData.HttpPostData.TokenName.pData = pPostParams;
			pPostParams+=httpPostTokenArgs->TokenNameLen;

			httpServerEvent.EventData.HttpPostData.TokenValue.Len = httpPostTokenArgs->TokenValueLen;
			httpServerEvent.EventData.HttpPostData.TokenValue.pData = pPostParams;

			httpServerResponse.Response = SL_NETAPP_HTTPRESPONSE_NONE;

			_SlDrvDispatchHttpServerEvents (&httpServerEvent, &httpServerResponse);
#endif
		}
		break;
#ifndef SL_TINY  
        case SL_OPCODE_NETAPP_REQUEST:

            {
#if defined(slcb_NetAppRequestHdlr) || defined(EXT_LIB_REGISTERED_NETAPP_REQUEST_EVENTS)
                _u8 *pData;
                SlNetAppRequest_t  NetAppRequest;
                SlNetAppResponse_t NetAppResponse;
                _u16 status;

	        
                /* Points to the Netapp request Arguments */
                SlProtocolNetAppRequest_t *protocol_NetAppRequest = (SlProtocolNetAppRequest_t*)_SL_RESP_ARGS_START(pHdr);

                NetAppRequest.AppId = protocol_NetAppRequest->AppId;
                NetAppRequest.Type = protocol_NetAppRequest->RequestType;
                NetAppRequest.Handle = protocol_NetAppRequest->Handle;
                NetAppRequest.requestData.Flags = protocol_NetAppRequest->Flags;

                /* Prepare the Metadata*/
                pData = (_u8 *)((SlProtocolNetAppRequest_t *)protocol_NetAppRequest + 1);/* Points to the netapp request Data (start of Metadata + payload) */
                NetAppRequest.requestData.pMetadata = pData;  /* Just pass the pointer */
                NetAppRequest.requestData.MetadataLen = protocol_NetAppRequest->MetadataLen;

                /* Preare the Payload */
                pData+=protocol_NetAppRequest->MetadataLen; 
                NetAppRequest.requestData.pPayload = pData; /* Just pass the pointer */
                NetAppRequest.requestData.PayloadLen = protocol_NetAppRequest->PayloadLen;


				/* Just in case - clear the response outout data  */
				sl_Memset(&NetAppResponse, 0, sizeof (NetAppResponse));
				NetAppResponse.Status = SL_NETAPP_HTTP_RESPONSE_404_NOT_FOUND;


                /* Call the request handler dispatcher */
                _SlDrvDispatchNetAppRequestEvents (&NetAppRequest, &NetAppResponse);

                /* Handle the response */
                status = _SlNetAppSendResponse(protocol_NetAppRequest->Handle, &NetAppResponse);

#if defined(SL_RUNTIME_EVENT_REGISTERATION)
                if(1 == _SlIsEventRegistered(SL_EVENT_HDL_MEM_FREE))
                {
					if ((NetAppResponse.ResponseData.MetadataLen > 0) && (NetAppResponse.ResponseData.pMetadata != NULL))
					{

						_SlDrvHandleNetAppRequestMemFreeEvents (NetAppResponse.ResponseData.pMetadata);

					}

					if ((NetAppResponse.ResponseData.PayloadLen > 0) && (NetAppResponse.ResponseData.pPayload != NULL))
					{

						_SlDrvHandleNetAppRequestMemFreeEvents (NetAppResponse.ResponseData.pPayload);

					}
                }
#endif

                if (status != 0 )
                {
                    /* Error - just send resource not found */
                    NetAppResponse.Status = SL_NETAPP_HTTP_RESPONSE_404_NOT_FOUND;
                    NetAppResponse.ResponseData.pMetadata = NULL;
                    NetAppResponse.ResponseData.MetadataLen = 0;
                    NetAppResponse.ResponseData.pPayload = NULL;
                    NetAppResponse.ResponseData.PayloadLen = 0;
                    NetAppResponse.ResponseData.Flags = 0;

                    /* Handle the response */
                    _SlNetAppSendResponse(protocol_NetAppRequest->Handle, &NetAppResponse);
                }
#else

                SlNetAppResponse_t NetAppResponse;

			
                /* Points to the Netapp request Arguments */
                SlProtocolNetAppRequest_t *protocol_NetAppRequest = (SlProtocolNetAppRequest_t *)_SL_RESP_ARGS_START(pHdr); 

                /* Prepare the response */
                NetAppResponse.Status = SL_NETAPP_HTTP_RESPONSE_404_NOT_FOUND;
                NetAppResponse.ResponseData.pMetadata = NULL;
                NetAppResponse.ResponseData.MetadataLen = 0;
                NetAppResponse.ResponseData.pPayload = NULL;
                NetAppResponse.ResponseData.PayloadLen = 0;
                NetAppResponse.ResponseData.Flags = 0;

                /* Handle the response */
                _SlNetAppSendResponse(protocol_NetAppRequest->Handle, &NetAppResponse);
#endif

            }
        break;
#endif

        
        default:
      //      SL_ERROR_TRACE2(MSG_305, "ASSERT: _SlNetAppEventHandler : invalid opcode = 0x%x = %1", pHdr->GenHeader.Opcode, pHdr->GenHeader.Opcode);
            VERIFY_PROTOCOL(0);
    }

	return SL_OS_RET_CODE_OK;
}

