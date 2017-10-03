/*
 * sl_socket.c - CC31xx/CC32xx Host Driver Implementation
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

static void     _SlSocketBuildAddress(const SlSockAddr_t *addr, SlSocketAddrCommand_u    *pCmd);
_SlReturnVal_t  _SlSocketHandleAsync_Connect(void *pVoidBuf);
_SlReturnVal_t _SlSocketHandleAsync_Close(void *pVoidBuf);

#ifndef SL_TINY
void             _SlSocketParseAddress(SlSocketAddrResponse_u *pRsp, SlSockAddr_t *addr, SlSocklen_t *addrlen);
_SlReturnVal_t   _SlSocketHandleAsync_Accept(void *pVoidBuf);
_SlReturnVal_t   _SlSocketHandleAsync_Select(void *pVoidBuf);
#if (defined(SL_PLATFORM_MULTI_THREADED) && !defined(slcb_SocketTriggerEventHandler))
static _i16      _SlDrvClearCtrlSocket(void);
static _i8       _SlDrvGetNextTimeoutValue(void);
#endif
#endif

/*******************************************************************************/
/* Functions                                                                   */
/*******************************************************************************/





/* ******************************************************************************/
/*  _SlSocketBuildAddress */
/* ******************************************************************************/
static void _SlSocketBuildAddress(const SlSockAddr_t *addr, SlSocketAddrCommand_u    *pCmd)
{

    /* Note: parsing of family and port in the generic way for all IPV4, IPV6 and EUI48
           is possible as long as these parameters are in the same offset and size for these
           three families. */
    pCmd->IpV4.FamilyAndFlags = (_u8)((addr->sa_family << 4) & 0xF0);
    pCmd->IpV4.Port = ((SlSockAddrIn_t *)addr)->sin_port;

    if(SL_AF_INET == addr->sa_family)
    {
        pCmd->IpV4.Address  = ((SlSockAddrIn_t *)addr)->sin_addr.s_addr;
    }
#ifdef SL_SUPPORT_IPV6
    else
    {
		sl_Memcpy(pCmd->IpV6.Address, ((SlSockAddrIn6_t *)addr)->sin6_addr._S6_un._S6_u32, 16 );
    }
#endif
}


/*******************************************************************************/
/*  _SlSocketParseAddress */
/*******************************************************************************/

#ifndef SL_TINY
void _SlSocketParseAddress(SlSocketAddrResponse_u    *pRsp, SlSockAddr_t *addr, SlSocklen_t *addrlen)
{
    /*  Note: parsing of family and port in the generic way for all IPV4, IPV6 and EUI48 */
    /*  is possible as long as these parameters are in the same offset and size for these */
    /*  three families. */
    addr->sa_family                 = pRsp->IpV4.Family;
    ((SlSockAddrIn_t *)addr)->sin_port = pRsp->IpV4.Port;

    *addrlen = (SL_AF_INET == addr->sa_family) ? sizeof(SlSockAddrIn_t) : sizeof(SlSockAddrIn6_t);

    if(SL_AF_INET == addr->sa_family)
    {
        ((SlSockAddrIn_t *)addr)->sin_addr.s_addr  = pRsp->IpV4.Address;
    }
#ifdef SL_SUPPORT_IPV6
    else
    {
	     sl_Memcpy(((SlSockAddrIn6_t *)addr)->sin6_addr._S6_un._S6_u32, pRsp->IpV6.Address, 16);
    }
#endif
}

#endif

/*******************************************************************************/
/* sl_Socket */
/*******************************************************************************/
typedef union
{
    _u32                Dummy;
	SlSocketCommand_t 	Cmd;
	SlSocketResponse_t	Rsp;
}_SlSockSocketMsg_u;



#if _SL_INCLUDE_FUNC(sl_Socket)

static const _SlCmdCtrl_t _SlSockSocketCmdCtrl =
{
    SL_OPCODE_SOCKET_SOCKET,
    (_SlArgSize_t)sizeof(SlSocketCommand_t),
    (_SlArgSize_t)sizeof(SlSocketResponse_t)
};

_i16 sl_Socket(_i16 Domain, _i16 Type, _i16 Protocol)
{
    _SlSockSocketMsg_u  Msg;

    Msg.Cmd.Domain	    = (_u8)Domain;
    Msg.Cmd.Type     	= (_u8)Type;
    Msg.Cmd.Protocol 	= (_u8)Protocol;

    /* verify that this api is allowed. if not allowed then
    ignore the API execution and return immediately with an error */
    VERIFY_API_ALLOWED(SL_OPCODE_SILO_SOCKET);

    VERIFY_RET_OK(_SlDrvCmdOp((_SlCmdCtrl_t *)&_SlSockSocketCmdCtrl, &Msg, NULL));

    if( Msg.Rsp.StatusOrLen < 0 )
	{
    	return ( Msg.Rsp.StatusOrLen);
	}
	else
	{
	    return (_i16)((_u8)Msg.Rsp.Sd);
	}
}
#endif

/*******************************************************************************/
/*  sl_Close  */
/*******************************************************************************/
typedef union
{
	SlCloseCommand_t    Cmd;
	SlSocketResponse_t	Rsp;
}_SlSockCloseMsg_u;


#if _SL_INCLUDE_FUNC(sl_Close)

static const _SlCmdCtrl_t _SlSockCloseCmdCtrl =
{
	SL_OPCODE_SOCKET_CLOSE,
    (_SlArgSize_t)sizeof(SlCloseCommand_t),
    (_SlArgSize_t)sizeof(SlSocketResponse_t)
};

_i16 sl_Close(_i16 sd)
{
	_SlSockCloseMsg_u    Msg;
	_u8 ObjIdx =		 MAX_CONCURRENT_ACTIONS;
	SlSocketResponse_t   AsyncRsp;
	_SlReturnVal_t       RetVal;
	_u8 bSocketInAction = FALSE;

    /* verify that this api is allowed. if not allowed then
    ignore the API execution and return immediately with an error */
    VERIFY_API_ALLOWED(SL_OPCODE_SILO_SOCKET);

    Msg.Cmd.Sd = (_u8)sd;
    _SlDrvMemZero(&AsyncRsp, sizeof(SlSocketResponse_t));

	/* check if the socket has already action in progress */
	bSocketInAction = !!(g_pCB->ActiveActionsBitmap & (1<<sd));

	if (bSocketInAction == FALSE)
	{
		ObjIdx = _SlDrvProtectAsyncRespSetting((_u8*)&AsyncRsp, CLOSE_ID, (_u8)(sd  & SL_BSD_SOCKET_ID_MASK));

		if (MAX_CONCURRENT_ACTIONS == ObjIdx)
		{
		    return SL_POOL_IS_EMPTY;
		}
	}

	VERIFY_RET_OK(_SlDrvCmdOp((_SlCmdCtrl_t *)&_SlSockCloseCmdCtrl, &Msg, NULL));
	
	RetVal = Msg.Rsp.StatusOrLen;

	if (bSocketInAction == FALSE)
	{
		 if( SL_RET_CODE_OK == RetVal)
		{	
			 SL_DRV_SYNC_OBJ_WAIT_TIMEOUT(&g_pCB->ObjPool[ObjIdx].SyncObj,
							SL_DRIVER_TIMEOUT_LONG,
							SL_OPCODE_SOCKET_SOCKETCLOSEASYNCEVENT
							);
			RetVal = AsyncRsp.StatusOrLen;
		}

		_SlDrvReleasePoolObj(ObjIdx);
	}

	return RetVal;
}
#endif

/*******************************************************************************/
/*  sl_Bind */
/*******************************************************************************/
typedef union
{
	SlSocketAddrCommand_u    Cmd;
	SlSocketResponse_t	     Rsp;
}_SlSockBindMsg_u;

#if _SL_INCLUDE_FUNC(sl_Bind)
_i16 sl_Bind(_i16 sd, const SlSockAddr_t *addr, _i16 addrlen)
{
	_SlSockBindMsg_u    Msg;
    _SlCmdCtrl_t        CmdCtrl = {0, 0, (_SlArgSize_t)sizeof(SlSocketResponse_t)};

    /* verify that this api is allowed. if not allowed then
    ignore the API execution and return immediately with an error */
    VERIFY_API_ALLOWED(SL_OPCODE_SILO_SOCKET);

    switch(addr->sa_family)
    {
        case SL_AF_INET :
            CmdCtrl.Opcode = SL_OPCODE_SOCKET_BIND;
            CmdCtrl.TxDescLen = (_SlArgSize_t)sizeof(SlSocketAddrIPv4Command_t);
            break;
#ifndef SL_TINY
#ifdef SL_SUPPORT_IPV6
        case SL_AF_INET6:
            CmdCtrl.Opcode = SL_OPCODE_SOCKET_BIND_V6;
            CmdCtrl.TxDescLen = (_SlArgSize_t)sizeof(SlSocketAddrIPv6Command_t);
            break;
#endif
#endif

        case SL_AF_RF   :
        default:
        	return SL_RET_CODE_INVALID_INPUT;
    }

    Msg.Cmd.IpV4.LenOrPadding = 0;
    Msg.Cmd.IpV4.Sd = (_u8)sd;

    _SlSocketBuildAddress(addr, &Msg.Cmd);
    VERIFY_RET_OK(_SlDrvCmdOp((_SlCmdCtrl_t *)&CmdCtrl, &Msg, NULL));

    return Msg.Rsp.StatusOrLen;
}
#endif

/*******************************************************************************/
/*  sl_Sendto */
/*******************************************************************************/
typedef union
{
    SlSocketAddrCommand_u    Cmd;
    /*  no response for 'sendto' commands*/
}_SlSendtoMsg_u;

#if _SL_INCLUDE_FUNC(sl_SendTo)
_i16 sl_SendTo(_i16 sd, const void *pBuf, _i16 Len, _i16 flags, const SlSockAddr_t *to, SlSocklen_t tolen)
{
    _SlSendtoMsg_u   Msg;
    _SlCmdCtrl_t     CmdCtrl = {0, 0, 0};
    _SlCmdExt_t      CmdExt;
    _i16             RetVal;

    /* verify that this api is allowed. if not allowed then
    ignore the API execution and return immediately with an error */
    VERIFY_API_ALLOWED(SL_OPCODE_SILO_SOCKET);
   /* RAW transceiver use only sl_Send  */
    if ((sd & SL_SOCKET_PAYLOAD_TYPE_MASK) == SL_SOCKET_PAYLOAD_TYPE_RAW_TRANCEIVER)
	{
    	return SL_ERROR_BSD_SOC_ERROR;
	}
    else
    {
        if (Len < 1)
        {
    	  /* ignore */
    	  return 0;
        }
    }

    _SlDrvResetCmdExt(&CmdExt);
    CmdExt.TxPayload1Len = (_u16)Len;
    CmdExt.pTxPayload1 = (_u8 *)pBuf;

    switch(to->sa_family)
    {
        case SL_AF_INET:
            CmdCtrl.Opcode = SL_OPCODE_SOCKET_SENDTO;
            CmdCtrl.TxDescLen = (_SlArgSize_t)sizeof(SlSocketAddrIPv4Command_t);
            break;
#ifndef SL_TINY               
#ifdef SL_SUPPORT_IPV6
        case SL_AF_INET6:
            CmdCtrl.Opcode = SL_OPCODE_SOCKET_SENDTO_V6;
            CmdCtrl.TxDescLen = (_SlArgSize_t)sizeof(SlSocketAddrIPv6Command_t);
            break;
#endif
#endif
        case SL_AF_RF:
        default:
        	return SL_RET_CODE_INVALID_INPUT;
    }

    Msg.Cmd.IpV4.LenOrPadding = Len;
    Msg.Cmd.IpV4.Sd = (_u8)sd;
    _SlSocketBuildAddress(to, &Msg.Cmd);
    Msg.Cmd.IpV4.FamilyAndFlags |= flags & 0x0F;

    RetVal = _SlDrvDataWriteOp((_SlSd_t)sd, &CmdCtrl, &Msg, &CmdExt);
    if(SL_OS_RET_CODE_OK != RetVal)
    {
    	return RetVal;
    }

    return (_i16)Len;
}
#endif

/*******************************************************************************/
/*  sl_Recvfrom */
/*******************************************************************************/
typedef union
{
    SlSendRecvCommand_t	    Cmd;
    SlSocketAddrResponse_u	Rsp;
}_SlRecvfromMsg_u;

static const _SlCmdCtrl_t _SlRecvfomCmdCtrl =
{
	SL_OPCODE_SOCKET_RECVFROM,
    (_SlArgSize_t)sizeof(SlSendRecvCommand_t),
    (_SlArgSize_t)sizeof(SlSocketAddrResponse_u)
};

#if _SL_INCLUDE_FUNC(sl_RecvFrom)
_i16 sl_RecvFrom(_i16 sd, void *buf, _i16 Len, _i16 flags, SlSockAddr_t *from, SlSocklen_t *fromlen)
{
    _SlRecvfromMsg_u    Msg;
    _SlCmdExt_t         CmdExt;
    _i16                 RetVal;

    /* verify that this api is allowed. if not allowed then
    ignore the API execution and return immediately with an error */
    VERIFY_API_ALLOWED(SL_OPCODE_SILO_SOCKET);

	/* RAW transceiver use only sl_Recv  */
    if ((sd & SL_SOCKET_PAYLOAD_TYPE_MASK) == SL_SOCKET_PAYLOAD_TYPE_RAW_TRANCEIVER)
	{
    	return SL_ERROR_BSD_SOC_ERROR;
	}

    _SlDrvResetCmdExt(&CmdExt);
    CmdExt.RxPayloadLen = Len;
    CmdExt.pRxPayload = (_u8 *)buf;

    Msg.Cmd.Sd = (_u8)sd;
    Msg.Cmd.StatusOrLen = (_u16)Len;
    
    /*  no size truncation in recv path */
    CmdExt.RxPayloadLen = (_i16)Msg.Cmd.StatusOrLen;

    Msg.Cmd.FamilyAndFlags = (_u8)(flags & 0x0F);

    if(sizeof(SlSockAddrIn_t) == *fromlen)
    {
        Msg.Cmd.FamilyAndFlags |= (SL_AF_INET << 4);
    }
    else if (sizeof(SlSockAddrIn6_t) == *fromlen)
    {
        Msg.Cmd.FamilyAndFlags |= (SL_AF_INET6 << 4);
    }
    else
    {
    	return SL_RET_CODE_INVALID_INPUT;
    }

    RetVal = _SlDrvDataReadOp((_SlSd_t)sd, (_SlCmdCtrl_t *)&_SlRecvfomCmdCtrl, &Msg, &CmdExt);
    if( RetVal != SL_OS_RET_CODE_OK )
    {
  	   return RetVal;
    }

    RetVal = Msg.Rsp.IpV4.StatusOrLen;

    if(RetVal >= 0)
    {
        VERIFY_PROTOCOL(sd == (_i16)Msg.Rsp.IpV4.Sd);
#if 0
        _SlSocketParseAddress(&Msg.Rsp, from, fromlen);
#else
        from->sa_family = Msg.Rsp.IpV4.Family;
        if(SL_AF_INET == from->sa_family)
        {
            ((SlSockAddrIn_t *)from)->sin_port = Msg.Rsp.IpV4.Port;
            ((SlSockAddrIn_t *)from)->sin_addr.s_addr = Msg.Rsp.IpV4.Address;
            *fromlen = (SlSocklen_t)sizeof(SlSockAddrIn_t);
        }
#ifdef SL_SUPPORT_IPV6
        else if(SL_AF_INET6 == from->sa_family)
        {
            VERIFY_PROTOCOL(*fromlen >= sizeof(SlSockAddrIn6_t));

            ((SlSockAddrIn6_t *)from)->sin6_port = Msg.Rsp.IpV6.Port;
            sl_Memcpy(((SlSockAddrIn6_t *)from)->sin6_addr._S6_un._S6_u32, Msg.Rsp.IpV6.Address, 16);
            *fromlen = sizeof(SlSockAddrIn6_t);
        }
#endif
#endif
    }

    return (_i16)RetVal;
}
#endif

/*******************************************************************************/
/*  sl_Connect */
/*******************************************************************************/
typedef union
{
	SlSocketAddrCommand_u    Cmd;
	SlSocketResponse_t	    Rsp;
}_SlSockConnectMsg_u;

#if _SL_INCLUDE_FUNC(sl_Connect)
_i16 sl_Connect(_i16 sd, const SlSockAddr_t *addr, _i16 addrlen)
{
      _SlSockConnectMsg_u  Msg;
      _SlReturnVal_t       RetVal;
      _SlCmdCtrl_t         CmdCtrl = {0, (_SlArgSize_t)0, (_SlArgSize_t)sizeof(SlSocketResponse_t)};
      SlSocketResponse_t   AsyncRsp;
      _u8 ObjIdx = MAX_CONCURRENT_ACTIONS;

    /* verify that this api is allowed. if not allowed then
    ignore the API execution and return immediately with an error */
    VERIFY_API_ALLOWED(SL_OPCODE_SILO_SOCKET);
    _SlDrvMemZero(&AsyncRsp, sizeof(SlSocketResponse_t));

    switch(addr->sa_family)
    {
        case SL_AF_INET :
            CmdCtrl.Opcode = SL_OPCODE_SOCKET_CONNECT;
            CmdCtrl.TxDescLen = (_SlArgSize_t)sizeof(SlSocketAddrIPv4Command_t);
            /* Do nothing - cmd already initialized to this type */
            break;
#ifdef SL_SUPPORT_IPV6
        case SL_AF_INET6:
            CmdCtrl.Opcode = SL_OPCODE_SOCKET_CONNECT_V6;
            CmdCtrl.TxDescLen = (_SlArgSize_t)sizeof(SlSocketAddrIPv6Command_t);
            break;
#endif
        case SL_AF_RF:
        default:
        	return SL_RET_CODE_INVALID_INPUT;
    }

    Msg.Cmd.IpV4.LenOrPadding = 0;
    Msg.Cmd.IpV4.Sd = (_u8)sd;

    _SlSocketBuildAddress(addr, &Msg.Cmd);

    ObjIdx = _SlDrvProtectAsyncRespSetting((_u8*)&AsyncRsp, CONNECT_ID, (_u8)(sd  & SL_BSD_SOCKET_ID_MASK));

    if (MAX_CONCURRENT_ACTIONS == ObjIdx)
    {
    	return SL_POOL_IS_EMPTY;
    }

    /* send the command */
    VERIFY_RET_OK(_SlDrvCmdOp((_SlCmdCtrl_t *)&CmdCtrl, &Msg, NULL));
    VERIFY_PROTOCOL(Msg.Rsp.Sd == (_u8)sd);

	RetVal = Msg.Rsp.StatusOrLen;

    if(SL_RET_CODE_OK == RetVal)
    {
#ifndef SL_TINY    
    	/*In case socket is non-blocking one, the async event should be received immediately */
    	if( g_pCB->SocketNonBlocking & (1<<(sd & SL_BSD_SOCKET_ID_MASK) ))
		{
    		SL_DRV_SYNC_OBJ_WAIT_TIMEOUT(&g_pCB->ObjPool[ObjIdx].SyncObj,
                                          SL_DRIVER_TIMEOUT_SHORT,
                                          SL_OPCODE_SOCKET_CONNECTASYNCRESPONSE
                                          );
		}
    	else
#endif         
    	{

    		/* wait for async and get Data Read parameters */
    		SL_DRV_SYNC_OBJ_WAIT_FOREVER(&g_pCB->ObjPool[ObjIdx].SyncObj);
    	}

        VERIFY_PROTOCOL(AsyncRsp.Sd == (_u8)sd);
        RetVal = AsyncRsp.StatusOrLen;
    }

    _SlDrvReleasePoolObj(ObjIdx);
    return RetVal;
}

#endif


/*******************************************************************************/
/*   _SlSocketHandleAsync_Connect */
/*******************************************************************************/
_SlReturnVal_t _SlSocketHandleAsync_Connect(void *pVoidBuf)
{
    SlSocketResponse_t          *pMsgArgs   = (SlSocketResponse_t *)_SL_RESP_ARGS_START(pVoidBuf);

    SL_DRV_PROTECTION_OBJ_LOCK_FOREVER();

    VERIFY_PROTOCOL((pMsgArgs->Sd & SL_BSD_SOCKET_ID_MASK) <= SL_MAX_SOCKETS);
    VERIFY_SOCKET_CB(NULL != g_pCB->ObjPool[g_pCB->FunctionParams.AsyncExt.ActionIndex].pRespArgs);
    

    ((SlSocketResponse_t *)(g_pCB->ObjPool[g_pCB->FunctionParams.AsyncExt.ActionIndex].pRespArgs))->Sd = pMsgArgs->Sd;
    ((SlSocketResponse_t *)(g_pCB->ObjPool[g_pCB->FunctionParams.AsyncExt.ActionIndex].pRespArgs))->StatusOrLen = pMsgArgs->StatusOrLen;


    SL_DRV_SYNC_OBJ_SIGNAL(&g_pCB->ObjPool[g_pCB->FunctionParams.AsyncExt.ActionIndex].SyncObj);
    SL_DRV_PROTECTION_OBJ_UNLOCK();
    
	return SL_OS_RET_CODE_OK;
}

/*******************************************************************************/
/*   _SlSocketHandleAsync_Close */
/*******************************************************************************/
_SlReturnVal_t _SlSocketHandleAsync_Close(void *pVoidBuf)
{
    SlSocketResponse_t          *pMsgArgs   = (SlSocketResponse_t *)_SL_RESP_ARGS_START(pVoidBuf);

    SL_DRV_PROTECTION_OBJ_LOCK_FOREVER();

    VERIFY_PROTOCOL((pMsgArgs->Sd & SL_BSD_SOCKET_ID_MASK) <= SL_MAX_SOCKETS);
    VERIFY_SOCKET_CB(NULL != g_pCB->ObjPool[g_pCB->FunctionParams.AsyncExt.ActionIndex].pRespArgs);
    

    ((SlSocketResponse_t *)(g_pCB->ObjPool[g_pCB->FunctionParams.AsyncExt.ActionIndex].pRespArgs))->Sd = pMsgArgs->Sd;
    ((SlSocketResponse_t *)(g_pCB->ObjPool[g_pCB->FunctionParams.AsyncExt.ActionIndex].pRespArgs))->StatusOrLen = pMsgArgs->StatusOrLen;


    SL_DRV_SYNC_OBJ_SIGNAL(&g_pCB->ObjPool[g_pCB->FunctionParams.AsyncExt.ActionIndex].SyncObj);
    SL_DRV_PROTECTION_OBJ_UNLOCK();
    
	return SL_OS_RET_CODE_OK;
}

/*******************************************************************************/
/*  sl_Send */
/*******************************************************************************/
typedef union
{
	SlSendRecvCommand_t    Cmd;
    /*  no response for 'sendto' commands*/
}_SlSendMsg_u;

static const _SlCmdCtrl_t _SlSendCmdCtrl =
{
    SL_OPCODE_SOCKET_SEND,
    (_SlArgSize_t)sizeof(SlSendRecvCommand_t),
    (_SlArgSize_t)0
};

#if _SL_INCLUDE_FUNC(sl_Send)
_i16 sl_Send(_i16 sd, const void *pBuf, _i16 Len, _i16 flags)
{
    _SlSendMsg_u   Msg;
    _SlCmdExt_t    CmdExt;
    _i16            RetVal;
	_u32         tempVal;

    /* verify that this api is allowed. if not allowed then
    ignore the API execution and return immediately with an error */
    VERIFY_API_ALLOWED(SL_OPCODE_SILO_SOCKET);

    _SlDrvResetCmdExt(&CmdExt);
    CmdExt.TxPayload1Len = (_u16)Len;
    CmdExt.pTxPayload1 = (_u8 *)pBuf;
    
    /* Only for RAW transceiver type socket, relay the flags parameter in the 2 bytes (4 byte aligned) before the actual payload */
    if ((sd & SL_SOCKET_PAYLOAD_TYPE_MASK) == SL_SOCKET_PAYLOAD_TYPE_RAW_TRANCEIVER)
    {
		tempVal = (_u32)flags;
        CmdExt.pRxPayload = (_u8 *)&tempVal;
		CmdExt.RxPayloadLen = -4; /* the (-) sign is used to mark the rx buff as output buff as well*/
    }
    else
    {
        CmdExt.pRxPayload = NULL;
        if (Len < 1)
        {
    	  /* ignore */
    	  return 0;
        }
    }

    Msg.Cmd.StatusOrLen = Len;
    Msg.Cmd.Sd = (_u8)sd;
    Msg.Cmd.FamilyAndFlags |= flags & 0x0F;

    RetVal = _SlDrvDataWriteOp((_u8)sd, (_SlCmdCtrl_t *)&_SlSendCmdCtrl, &Msg, &CmdExt);
    if(SL_OS_RET_CODE_OK != RetVal)
    {
    	return RetVal;
    }
    
    return (_i16)Len;
}
#endif

/*******************************************************************************/
/*  sl_Listen */
/*******************************************************************************/
typedef union
{
	SlListenCommand_t    Cmd;
    _BasicResponse_t     Rsp;
}_SlListenMsg_u;



#if _SL_INCLUDE_FUNC(sl_Listen)

static const _SlCmdCtrl_t _SlListenCmdCtrl =
{
    SL_OPCODE_SOCKET_LISTEN,
    (_SlArgSize_t)sizeof(SlListenCommand_t),
    (_SlArgSize_t)sizeof(_BasicResponse_t),
};

_i16 sl_Listen(_i16 sd, _i16 backlog)
{
    _SlListenMsg_u  Msg;

    /* verify that this api is allowed. if not allowed then
    ignore the API execution and return immediately with an error */
    VERIFY_API_ALLOWED(SL_OPCODE_SILO_SOCKET);

    Msg.Cmd.Sd = (_u8)sd;
    Msg.Cmd.Backlog = (_u8)backlog;

    VERIFY_RET_OK(_SlDrvCmdOp((_SlCmdCtrl_t *)&_SlListenCmdCtrl, &Msg, NULL));
    return (_i16)Msg.Rsp.status;
}
#endif

/*******************************************************************************/
/*  sl_Accept */
/*******************************************************************************/
typedef union
{
	SlAcceptCommand_t    Cmd;
	SlSocketResponse_t  Rsp;
}_SlSockAcceptMsg_u;



#if _SL_INCLUDE_FUNC(sl_Accept)

static const _SlCmdCtrl_t _SlAcceptCmdCtrl =
{
    SL_OPCODE_SOCKET_ACCEPT,
    (_SlArgSize_t)sizeof(SlAcceptCommand_t),
    (_SlArgSize_t)sizeof(_BasicResponse_t),
};

_i16 sl_Accept(_i16 sd, SlSockAddr_t *addr, SlSocklen_t *addrlen)
{
	_SlSockAcceptMsg_u       Msg;
    _SlReturnVal_t           RetVal;
    SlSocketAddrResponse_u   AsyncRsp;

	_u8 ObjIdx = MAX_CONCURRENT_ACTIONS;

    /* verify that this api is allowed. if not allowed then
    ignore the API execution and return immediately with an error */
    VERIFY_API_ALLOWED(SL_OPCODE_SILO_SOCKET);

    Msg.Cmd.Sd = (_u8)sd;

    if((addr != NULL) && (addrlen != NULL))
    {
        /* If addr is present, addrlen has to be provided */
        Msg.Cmd.Family = (_u8)((sizeof(SlSockAddrIn_t) == *addrlen) ? SL_AF_INET : SL_AF_INET6);
    }
    else
    {
        /* In any other case, addrlen is ignored */
        Msg.Cmd.Family = (_u8)0;
    }

    ObjIdx = _SlDrvProtectAsyncRespSetting((_u8*)&AsyncRsp, ACCEPT_ID, (_u8)sd  & SL_BSD_SOCKET_ID_MASK);

    if (MAX_CONCURRENT_ACTIONS == ObjIdx)
    {
    	return SL_POOL_IS_EMPTY;
    }
    
	/* send the command */
    VERIFY_RET_OK(_SlDrvCmdOp((_SlCmdCtrl_t *)&_SlAcceptCmdCtrl, &Msg, NULL));
    VERIFY_PROTOCOL(Msg.Rsp.Sd == (_u8)sd);

    RetVal = Msg.Rsp.StatusOrLen;

    if(SL_OS_RET_CODE_OK == RetVal)
    {
#ifndef SL_TINY    
    	/* in case socket is non-blocking one, the async event should be received immediately */
    	if( g_pCB->SocketNonBlocking & (1<<(sd & SL_BSD_SOCKET_ID_MASK) ))
    	{
    		SL_DRV_SYNC_OBJ_WAIT_TIMEOUT(&g_pCB->ObjPool[ObjIdx].SyncObj,
                                         SL_DRIVER_TIMEOUT_SHORT,
                                         SL_OPCODE_SOCKET_ACCEPTASYNCRESPONSE
                                         );
    	}
    	else
#endif         
    	{
    		/* wait for async and get Data Read parameters */
    		SL_DRV_SYNC_OBJ_WAIT_FOREVER(&g_pCB->ObjPool[ObjIdx].SyncObj);
    	}

        VERIFY_PROTOCOL(AsyncRsp.IpV4.Sd == (_u8)sd);

        RetVal = AsyncRsp.IpV4.StatusOrLen;
#if 0 /*  Kept for backup */
            _SlSocketParseAddress(&AsyncRsp, addr, addrlen);
#else
         if((addr != NULL) && (addrlen != NULL))
         {
    	   addr->sa_family = AsyncRsp.IpV4.Family;

    	    if(SL_AF_INET == addr->sa_family)
    	    {
              if( *addrlen == (SlSocklen_t)sizeof( SlSockAddrIn_t ) )
              {
                ((SlSockAddrIn_t *)addr)->sin_port         = AsyncRsp.IpV4.Port;
                ((SlSockAddrIn_t *)addr)->sin_addr.s_addr  = AsyncRsp.IpV4.Address;
              }
              else
              {
                *addrlen = 0;
              }
    	    }
#ifdef SL_SUPPORT_IPV6
    	    else if(SL_AF_INET6 == addr->sa_family)
    	    {
              if( *addrlen == sizeof( SlSockAddrIn6_t ) )
              {
    	        ((SlSockAddrIn6_t *)addr)->sin6_port                   = AsyncRsp.IpV6.Port    ;
    	        sl_Memcpy(((SlSockAddrIn6_t *)addr)->sin6_addr._S6_un._S6_u32, AsyncRsp.IpV6.Address, 16);
              }
              else
              {
                *addrlen = 0;
              }
    	    }
#endif
         }
#endif
    }

    _SlDrvReleasePoolObj(ObjIdx);
    return (_i16)RetVal;
}
#endif


/*******************************************************************************/
/*  sl_Htonl */
/*******************************************************************************/
_u32 sl_Htonl( _u32 val )
{
  _u32 i = 1; 
  _i8 *p = (_i8 *)&i;  
  if (p[0] == 1) /* little endian */
  {
    p[0] = ((_i8* )&val)[3];
    p[1] = ((_i8* )&val)[2];
    p[2] = ((_i8* )&val)[1];
    p[3] = ((_i8* )&val)[0];
    return i;
  }
  else /* big endian */
  {
    return val; 
  }
}

/*******************************************************************************/
/*  sl_Htonl */
/*******************************************************************************/
_u16 sl_Htons( _u16 val )
{
  _i16 i = 1; 
  _i8 *p = (_i8 *)&i;  
  if (p[0] == 1) /* little endian */
  {
    p[0] = ((_i8* )&val)[1];
    p[1] = ((_i8* )&val)[0];
    return (_u16)i;
  }
  else /* big endian */
  {
    return val; 
  }
}

/*******************************************************************************/
/*   _SlSocketHandleAsync_Accept */
/*******************************************************************************/
#ifndef SL_TINY
_SlReturnVal_t _SlSocketHandleAsync_Accept(void *pVoidBuf)
{
    SlSocketAddrResponse_u      *pMsgArgs   = (SlSocketAddrResponse_u *)_SL_RESP_ARGS_START(pVoidBuf);

    SL_DRV_PROTECTION_OBJ_LOCK_FOREVER();

    VERIFY_PROTOCOL(( pMsgArgs->IpV4.Sd & SL_BSD_SOCKET_ID_MASK) <= SL_MAX_SOCKETS);
    VERIFY_SOCKET_CB(NULL != g_pCB->ObjPool[g_pCB->FunctionParams.AsyncExt.ActionIndex].pRespArgs);

	sl_Memcpy(g_pCB->ObjPool[g_pCB->FunctionParams.AsyncExt.ActionIndex].pRespArgs, pMsgArgs,sizeof(SlSocketAddrResponse_u));
	SL_DRV_SYNC_OBJ_SIGNAL(&g_pCB->ObjPool[g_pCB->FunctionParams.AsyncExt.ActionIndex].SyncObj);

    SL_DRV_PROTECTION_OBJ_UNLOCK();

    return SL_OS_RET_CODE_OK;
}
#endif

/*******************************************************************************/
/*  sl_Recv */
/*******************************************************************************/
typedef union
{
	SlSendRecvCommand_t  Cmd;
	SlSocketResponse_t   Rsp;    
}_SlRecvMsg_u;


#if _SL_INCLUDE_FUNC(sl_Recv)

static const _SlCmdCtrl_t _SlRecvCmdCtrl =
{
    SL_OPCODE_SOCKET_RECV,
    (_SlArgSize_t)sizeof(SlSendRecvCommand_t),
    (_SlArgSize_t)sizeof(SlSocketResponse_t)
};


_i16 sl_Recv(_i16 sd, void *pBuf, _i16 Len, _i16 flags)
{
    _SlRecvMsg_u    Msg;
    _SlCmdExt_t     CmdExt;
    _SlReturnVal_t status;

    /* verify that this api is allowed. if not allowed then
    ignore the API execution and return immediately with an error */
    VERIFY_API_ALLOWED(SL_OPCODE_SILO_SOCKET);

    _SlDrvResetCmdExt(&CmdExt);
    CmdExt.RxPayloadLen = Len;
    CmdExt.pRxPayload = (_u8 *)pBuf;

    Msg.Cmd.Sd = (_u8)sd;
    Msg.Cmd.StatusOrLen = (_u16)Len;

    /*  no size truncation in recv path */
    CmdExt.RxPayloadLen = (_i16)Msg.Cmd.StatusOrLen;

    Msg.Cmd.FamilyAndFlags = (_u8)(flags & 0x0F);

    status = _SlDrvDataReadOp((_SlSd_t)sd, (_SlCmdCtrl_t *)&_SlRecvCmdCtrl, &Msg, &CmdExt);
    if( status != SL_OS_RET_CODE_OK )
    {
    	return status;
    }
     
    /*  if the Device side sends less than expected it is not the Driver's role */
    /*  the returned value could be smaller than the requested size */
    return (_i16)Msg.Rsp.StatusOrLen;
}
#endif

/*******************************************************************************/
/*  sl_SetSockOpt */
/*******************************************************************************/
typedef union
{
	SlSetSockOptCommand_t    Cmd;
	SlSocketResponse_t       Rsp;    
}_SlSetSockOptMsg_u;

static const _SlCmdCtrl_t _SlSetSockOptCmdCtrl =
{
    SL_OPCODE_SOCKET_SETSOCKOPT,
    (_SlArgSize_t)sizeof(SlSetSockOptCommand_t),
    (_SlArgSize_t)sizeof(SlSocketResponse_t)
};

#if _SL_INCLUDE_FUNC(sl_SetSockOpt)
_i16 sl_SetSockOpt(_i16 sd, _i16 level, _i16 optname, const void *optval, SlSocklen_t optlen)
{
    _SlSetSockOptMsg_u    Msg;
    _SlCmdExt_t           CmdExt;

    /* verify that this api is allowed. if not allowed then
    ignore the API execution and return immediately with an error */
    VERIFY_API_ALLOWED(SL_OPCODE_SILO_SOCKET);

    _SlDrvResetCmdExt(&CmdExt);
    CmdExt.TxPayload1Len = optlen;
    CmdExt.pTxPayload1 = (_u8 *)optval;

    Msg.Cmd.Sd = (_u8)sd;
    Msg.Cmd.Level = (_u8)level;
    Msg.Cmd.OptionLen = (_u8)optlen;
    Msg.Cmd.OptionName = (_u8)optname;

    VERIFY_RET_OK(_SlDrvCmdOp((_SlCmdCtrl_t *)&_SlSetSockOptCmdCtrl, &Msg, &CmdExt));

    return (_i16)Msg.Rsp.StatusOrLen;
}
#endif

/*******************************************************************************/
/*  sl_GetSockOpt */
/*******************************************************************************/
typedef union
{
	SlGetSockOptCommand_t     Cmd;
	SlGetSockOptResponse_t    Rsp;    
}_SlGetSockOptMsg_u;


#if _SL_INCLUDE_FUNC(sl_GetSockOpt)

static const _SlCmdCtrl_t _SlGetSockOptCmdCtrl =
{
    SL_OPCODE_SOCKET_GETSOCKOPT,
    (_SlArgSize_t)sizeof(SlGetSockOptCommand_t),
    (_SlArgSize_t)sizeof(SlGetSockOptResponse_t)
};

_i16 sl_GetSockOpt(_i16 sd, _i16 level, _i16 optname, void *optval, SlSocklen_t *optlen)
{
    _SlGetSockOptMsg_u    Msg;
    _SlCmdExt_t           CmdExt;

    /* verify that this api is allowed. if not allowed then
    ignore the API execution and return immediately with an error */
    VERIFY_API_ALLOWED(SL_OPCODE_SILO_SOCKET);
	if (*optlen == 0)
	{
		return SL_EZEROLEN;
	}

    _SlDrvResetCmdExt(&CmdExt);
    CmdExt.RxPayloadLen = (_i16)(*optlen);
    CmdExt.pRxPayload = optval;

    Msg.Cmd.Sd = (_u8)sd;
    Msg.Cmd.Level = (_u8)level;
    Msg.Cmd.OptionLen = (_u8)(*optlen);
    Msg.Cmd.OptionName = (_u8)optname;

    VERIFY_RET_OK(_SlDrvCmdOp((_SlCmdCtrl_t *)&_SlGetSockOptCmdCtrl, &Msg, &CmdExt));

	if (CmdExt.RxPayloadLen < CmdExt.ActualRxPayloadLen) 
	{
	    *optlen = Msg.Rsp.OptionLen;
	    return SL_ESMALLBUF;
	}
	else
	{
		*optlen = (_u8)CmdExt.ActualRxPayloadLen;
	}
	return (_i16)Msg.Rsp.Status;
}
#endif

/********************************************************************************/
/*  sl_Select */
/* ******************************************************************************/
#ifndef SL_TINY
#if _SL_INCLUDE_FUNC(sl_Select)

typedef union
{
    SlSelectCommand_t   Cmd;
    _BasicResponse_t    Rsp;
}_SlSelectMsg_u;

static const _SlCmdCtrl_t _SlSelectCmdCtrl =
{
    SL_OPCODE_SOCKET_SELECT,
    (_SlArgSize_t)sizeof(SlSelectCommand_t),
    (_SlArgSize_t)sizeof(_BasicResponse_t)
};

/********************************************************************************/
/*  SL_SOCKET_FD_SET */
/* ******************************************************************************/
void SL_SOCKET_FD_SET(_i16 fd, SlFdSet_t *fdset)
{
   fdset->fd_array[0] |=  (1<< (fd & SL_BSD_SOCKET_ID_MASK));
}

/*******************************************************************************/
/*  SL_SOCKET_FD_CLR */
/*******************************************************************************/
void SL_SOCKET_FD_CLR(_i16 fd, SlFdSet_t *fdset)
{
  fdset->fd_array[0] &=  ~(1<< (fd & SL_BSD_SOCKET_ID_MASK));
}

/*******************************************************************************/
/*  SL_SOCKET_FD_ISSET */
/*******************************************************************************/
_i16 SL_SOCKET_FD_ISSET(_i16 fd, SlFdSet_t *fdset)
{
  if( fdset->fd_array[0] & (1<< (fd & SL_BSD_SOCKET_ID_MASK)) )
  {
    return 1;
  }
  return 0;
}

/*******************************************************************************/
/*  SL_SOCKET_FD_ZERO */
/*******************************************************************************/  
void SL_SOCKET_FD_ZERO(SlFdSet_t *fdset)
{
  fdset->fd_array[0] = 0;
}

#if (defined(SL_PLATFORM_MULTI_THREADED) && !defined(slcb_SocketTriggerEventHandler))

/*******************************************************************************/
/*  Multiple Select  */
/*******************************************************************************/

/* Multiple Select Defines */
#define LOCAL_CTRL_PORT                   (3632)
#define SL_LOOPBACK_ADDR                  (0x0100007F)
#define DUMMY_BUF_SIZE                    (4)
#define CTRL_SOCK_FD                      (((_u16)(1)) << g_pCB->MultiSelectCB.CtrlSockFD)
#define SELECT_TIMEOUT                    ((_u16)0)
#define SELECT_NO_TIMEOUT                 (0xFFFFFFFF)

/* Multiple Select Structures */
_SlSelectMsg_u  Msg;

static const SlSockAddrIn_t _SlCtrlSockAddr =
{
    SL_AF_INET,
    LOCAL_CTRL_PORT,
    SL_INADDR_ANY,
    (_u32)0x0,
    (_u32)0x0
};

static const SlSockAddrIn_t _SlCtrlSockRelese =
{
    SL_AF_INET,
    LOCAL_CTRL_PORT,
    SL_LOOPBACK_ADDR,
    (_u32)0x0,
    (_u32)0x0
};

/*******************************************************************************/
/*  CountSetBits */
/*******************************************************************************/
static inline _u8 CountSetBits(_u16 fdList)
{
    _u8 Count = 0;

    while(fdList)
    {
        Count += (fdList & ((_u16)1));
        fdList = fdList >> 1;
    }

    return Count;
}

/*******************************************************************************/
/*   _SlSocketHandleAsync_Select */
/*******************************************************************************/
_SlReturnVal_t _SlSocketHandleAsync_Select(void *pVoidBuf)
{
    _SlReturnVal_t               RetVal;
    SlSelectAsyncResponse_t     *pMsgArgs   = (SlSelectAsyncResponse_t *)_SL_RESP_ARGS_START(pVoidBuf);
    _u8                          RegIdx = 0;
    _u32                         time_now;
    _u8                          TimeoutEvent = 0;
    _u16                         SelectEvent = 0;

    _SlDrvMemZero(&Msg, sizeof(_SlSelectMsg_u));

    SL_DRV_PROTECTION_OBJ_LOCK_FOREVER();

    SL_DRV_OBJ_LOCK_FOREVER(&g_pCB->MultiSelectCB.SelectLockObj);

    /* Check if this context was triggered by a 'select joiner' only,
     * without timeout occuering, in order to launch the next select as quick as possible */
    if((CTRL_SOCK_FD == pMsgArgs->ReadFds) && (pMsgArgs->Status != SELECT_TIMEOUT))
    {
        RetVal = _SlDrvClearCtrlSocket();
        Msg.Cmd.ReadFds  = g_pCB->MultiSelectCB.readsds;
        Msg.Cmd.WriteFds = g_pCB->MultiSelectCB.writesds;
        Msg.Cmd.ReadFds |= CTRL_SOCK_FD;
        Msg.Cmd.tv_sec  = 0xFFFF;
        Msg.Cmd.tv_usec = 0xFFFF;

        RegIdx = _SlDrvGetNextTimeoutValue();

        SL_TRACE3(DBG_MSG, MSG_312, "\n\rAdded caller: call Select with: Write:%x Sec:%d uSec:%d\n\r",
                                                    Msg.Cmd.WriteFds, Msg.Cmd.tv_sec, Msg.Cmd.tv_usec);

        RetVal = _SlDrvCmdSend_noWait((_SlCmdCtrl_t *)&_SlSelectCmdCtrl, &Msg, NULL);

        SL_DRV_OBJ_UNLOCK(&g_pCB->MultiSelectCB.SelectLockObj);

        SL_DRV_PROTECTION_OBJ_UNLOCK();

        return RetVal;
    }

    /* If we're triggered by the NWP, take time-stamps to monitor the time-outs */
    time_now = ((slcb_GetTimestamp() / SL_TIMESTAMP_TICKS_IN_10_MILLISECONDS) * 10);

    /* If it's a proper select response, or if timeout occurred, release the relevant waiters */
    for(RegIdx = 0 ; RegIdx < MAX_CONCURRENT_ACTIONS ; RegIdx++)
    {
        if(g_pCB->MultiSelectCB.SelectEntry[RegIdx] != NULL)
        {
            /*  In case a certain entry has 100 mSec or less until it's timeout, the overhead
             *  caused by calling select again with it's fd lists is redundant, just return a time-out. */

            TimeoutEvent = ((time_now + 100) >= g_pCB->MultiSelectCB.SelectEntry[RegIdx]->TimeStamp);

            if(pMsgArgs->Status != SELECT_TIMEOUT)
            {
                SelectEvent = ((g_pCB->MultiSelectCB.SelectEntry[RegIdx]->readlist & pMsgArgs->ReadFds) ||
                               (g_pCB->MultiSelectCB.SelectEntry[RegIdx]->writelist & pMsgArgs->WriteFds));
            }

            if(SelectEvent || TimeoutEvent)
            {
                SL_TRACE4(DBG_MSG, MSG_312, "\n\rg: %x, sl:%d, run:%d, idx:%d\n\r", g_pCB->MultiSelectCB.writesds, SelectEvent , times, RegIdx);

                /* Clear the global select socket descriptor bitmaps */
                g_pCB->MultiSelectCB.readsds  &= ~(g_pCB->MultiSelectCB.SelectEntry[RegIdx]->readlist);
                g_pCB->MultiSelectCB.writesds &= ~(g_pCB->MultiSelectCB.SelectEntry[RegIdx]->writelist);

                if(SelectEvent)
                {
                    /* set the corresponding fd lists. */
                    g_pCB->MultiSelectCB.SelectEntry[RegIdx]->Response.ReadFds = (pMsgArgs->ReadFds & g_pCB->MultiSelectCB.SelectEntry[RegIdx]->readlist);
                    g_pCB->MultiSelectCB.SelectEntry[RegIdx]->Response.WriteFds = (pMsgArgs->WriteFds & g_pCB->MultiSelectCB.SelectEntry[RegIdx]->writelist);
                    g_pCB->MultiSelectCB.SelectEntry[RegIdx]->Response.ReadFdsCount = CountSetBits(g_pCB->MultiSelectCB.SelectEntry[RegIdx]->readlist);
                    g_pCB->MultiSelectCB.SelectEntry[RegIdx]->Response.WriteFdsCount = CountSetBits(g_pCB->MultiSelectCB.SelectEntry[RegIdx]->writelist);
                    g_pCB->MultiSelectCB.SelectEntry[RegIdx]->Response.Status = (g_pCB->MultiSelectCB.SelectEntry[RegIdx]->Response.ReadFdsCount +
                                                                                 g_pCB->MultiSelectCB.SelectEntry[RegIdx]->Response.WriteFdsCount);
                }
                else
                {
                    g_pCB->MultiSelectCB.SelectEntry[RegIdx]->Response.Status = SELECT_TIMEOUT;
                }

                g_pCB->MultiSelectCB.SelectEntry[RegIdx]->Response.ReadFds &= ~(CTRL_SOCK_FD);

                /* Signal the waiting caller. */
                SL_DRV_SYNC_OBJ_SIGNAL(&g_pCB->ObjPool[g_pCB->MultiSelectCB.SelectEntry[RegIdx]->ObjIdx].SyncObj);

                /* Clean it's table entry */
                g_pCB->MultiSelectCB.SelectEntry[RegIdx] = NULL;
            }
        }
    }

    /* In case where A caller was added, but also some sockfd were set on the NWP,
     * We clear the control socket. */
    if((pMsgArgs->ReadFds & CTRL_SOCK_FD) && (pMsgArgs->Status != SELECT_TIMEOUT))
    {
        RetVal = _SlDrvClearCtrlSocket();
    }

    /* If more readers/Writers are present, send select again */
    if((0 != g_pCB->MultiSelectCB.readsds) || (0 != g_pCB->MultiSelectCB.writesds))
    {
        Msg.Cmd.ReadFds  = g_pCB->MultiSelectCB.readsds;
        Msg.Cmd.ReadFds |= CTRL_SOCK_FD;
        Msg.Cmd.WriteFds = g_pCB->MultiSelectCB.writesds;

        /* Set timeout to blocking, in case there is no caller with timeout value. */
        Msg.Cmd.tv_sec  = 0xFFFF;
        Msg.Cmd.tv_usec = 0xFFFF;

        /* Get the next awaiting timeout caller */
        RegIdx = _SlDrvGetNextTimeoutValue();

        SL_TRACE3(DBG_MSG, MSG_312, "\n\rRelease Partial: call Select with: Read:%x Sec:%d uSec:%d\n\r",
                                    Msg.Cmd.ReadFds, Msg.Cmd.tv_sec, Msg.Cmd.tv_usec);

        RetVal = _SlDrvCmdSend_noWait((_SlCmdCtrl_t *)&_SlSelectCmdCtrl, &Msg, NULL);
    }
    else
    {
        while(g_pCB->MultiSelectCB.ActiveWaiters)
        {
            SL_DRV_SYNC_OBJ_SIGNAL(&g_pCB->MultiSelectCB.SelectSyncObj);
            g_pCB->MultiSelectCB.ActiveWaiters--;
        }

        g_pCB->MultiSelectCB.ActiveSelect = FALSE;

        SL_TRACE1(DBG_MSG, MSG_312, "\n\rSelect isn't Active: %d\n\r", g_pCB->MultiSelectCB.ActiveSelect);
    }

    SL_DRV_OBJ_UNLOCK(&g_pCB->MultiSelectCB.SelectLockObj);

    SL_DRV_PROTECTION_OBJ_UNLOCK();

    return SL_OS_RET_CODE_OK;
}

/*******************************************************************************/
/*  SlDrvGetNextTimeoutValue */
/*******************************************************************************/
static _i8 _SlDrvGetNextTimeoutValue(void)
{
    _u32 time_now;
    _i8  Found = -1;
    _u8  idx = 0;

    /* Take a timestamp */
    time_now = ((slcb_GetTimestamp() / SL_TIMESTAMP_TICKS_IN_10_MILLISECONDS) * 10);

    /* Go through all waiting time-outs, and select the closest */
    for(idx = 0 ; idx < MAX_CONCURRENT_ACTIONS ; idx++)
    {
        if(NULL != g_pCB->MultiSelectCB.SelectEntry[idx])
        {
            /* Check if the time-stamp is bigger or equal to current time, and if it's the minimal time-stamp (closest event) */
            if(g_pCB->MultiSelectCB.SelectEntry[idx]->TimeStamp >= time_now)
            {
                if(Found == -1)
                {
                    Found = idx;
                }
                else
                {
                    if(g_pCB->MultiSelectCB.SelectEntry[idx]->TimeStamp <= g_pCB->MultiSelectCB.SelectEntry[Found]->TimeStamp)
                    {
                        Found = idx;
                    }
                }
            }
        }
    }

    /* If a non-wait-forever index was found, calculate delta until closest event */
    if(g_pCB->MultiSelectCB.SelectEntry[Found]->TimeStamp != SELECT_NO_TIMEOUT)
    {
        _i32 delta = (g_pCB->MultiSelectCB.SelectEntry[Found]->TimeStamp - time_now);

        if(delta >= 0)
        {
            Msg.Cmd.tv_sec  = (delta / 1000);
            Msg.Cmd.tv_usec = (((delta % 1000) * 1000) >> 10);
        }
        else
        {
            /* if delta time calculated is negative, call a non-blocking select */
            Msg.Cmd.tv_sec  = 0;
            Msg.Cmd.tv_usec = 0;
        }
    }

    return Found;
}

/*******************************************************************************/
/*  _SlDrvClearCtrlSocket */
/*******************************************************************************/
static _i16 _SlDrvClearCtrlSocket(void)
{
    _SlRecvfromMsg_u    Msg;
    _SlCmdExt_t         CmdExt;
    _u8                 dummyBuf[DUMMY_BUF_SIZE];
    _SlReturnVal_t      RetVal;

    /* Prepare a recvFrom Cmd */
    _SlDrvResetCmdExt(&CmdExt);
    _SlDrvMemZero(&Msg, sizeof(_SlRecvfromMsg_u));

    CmdExt.RxPayloadLen = DUMMY_BUF_SIZE;
    CmdExt.pRxPayload = (_u8 *)&dummyBuf;

    Msg.Cmd.Sd = (_u8)g_pCB->MultiSelectCB.CtrlSockFD;
    Msg.Cmd.StatusOrLen = (_u16)DUMMY_BUF_SIZE;
    Msg.Cmd.FamilyAndFlags = (SL_AF_INET << 4);

    RetVal = _SlDrvCmdSend_noWait((_SlCmdCtrl_t *)&_SlRecvfomCmdCtrl, &Msg, &CmdExt);

    return RetVal;
}

/*******************************************************************************/
/*  _SlDrvOpenCtrlSocket */
/*******************************************************************************/
static _i16 _SlDrvOpenCtrlSocket(void)
{
    _i16 retVal;

   /* In case a control socket is already open, return. */
   if(g_pCB->MultiSelectCB.CtrlSockFD != 0xFF)
   {
       return 0;
   }

   /* Open a local control socket */
   retVal = sl_Socket(SL_AF_INET, SL_SOCK_DGRAM, 0);

   if(retVal == SL_ERROR_BSD_ENSOCK)
   {
        return 0;
   }
   else if(retVal < 0)
   {
       return retVal;
   }
   else
   {
       g_pCB->MultiSelectCB.CtrlSockFD = retVal;
   }

   /* Bind it to local control port */
   retVal = sl_Bind(g_pCB->MultiSelectCB.CtrlSockFD, (const SlSockAddr_t *)&_SlCtrlSockAddr, sizeof(SlSockAddrIn_t));

   return retVal;
}

/*******************************************************************************/
/*  _SlDrvCloseCtrlSocket */
/*******************************************************************************/
static _i16 _SlDrvCloseCtrlSocket(void)
{
    _i16 retVal = 0;
    _i16 sockfd = 0xFF;

   /* Close the internal Control socket */
   sockfd = g_pCB->MultiSelectCB.CtrlSockFD;

   if(sockfd != 0xFF)
   {
       /* Close the local control socket */
       retVal = sl_Close(sockfd);
   }

   g_pCB->MultiSelectCB.CtrlSockFD = 0xFF;

   if(retVal < 0)
   {
       return SL_ERROR_BSD_SOC_ERROR;
   }

   return retVal;
}

/*******************************************************************************/
/*  to_Msec */
/*******************************************************************************/
static inline _u32 to_mSec(struct SlTimeval_t* timeout)
{
    return (((slcb_GetTimestamp() / SL_TIMESTAMP_TICKS_IN_10_MILLISECONDS) * 10) + (timeout->tv_sec * 1000) + (timeout->tv_usec / 1000));
}

/*******************************************************************************/
/*  _SlDrvUnRegisterForSelectAsync */
/*******************************************************************************/
static _i16 _SlDrvUnRegisterForSelectAsync(_SlSelectEntry_t* pEntry, _u8 SelectInProgress)
{
    SL_DRV_OBJ_LOCK_FOREVER(&g_pCB->MultiSelectCB.SelectLockObj);

    /* Clear the global select fd lists */
    g_pCB->MultiSelectCB.readsds  &= ~(pEntry->readlist);
    g_pCB->MultiSelectCB.writesds &= ~(pEntry->writelist);

    /* Empty the caller's table entry. */
    g_pCB->MultiSelectCB.SelectEntry[pEntry->ObjIdx] = NULL;

    if(g_pCB->MultiSelectCB.ActiveSelect == FALSE)
    {
        _SlDrvCloseCtrlSocket();
    }

    SL_DRV_OBJ_UNLOCK(&g_pCB->MultiSelectCB.SelectLockObj);

    /* Release it's pool object */
    _SlDrvReleasePoolObj(pEntry->ObjIdx);

    return SL_ERROR_BSD_SOC_ERROR;
}

/*******************************************************************************/
/*  _SlDrvRegisterForSelectAsync */
/*******************************************************************************/
static _i16 _SlDrvRegisterForSelectAsync(_SlSelectEntry_t* pEntry, _SlSelectMsg_u* pMsg, struct SlTimeval_t *timeout, _u8 SelectInProgress)
{
    _SlReturnVal_t _RetVal = 0;
    _u8            dummyBuf[4] = {0};

    /* Register this caller's parameters */
    pEntry->readlist  = pMsg->Cmd.ReadFds;
    pEntry->writelist = pMsg->Cmd.WriteFds;

    if((pMsg->Cmd.tv_sec != 0xFFFF) && (timeout != NULL))
    {
        pEntry->TimeStamp = to_mSec(timeout);
    }
    else
    {
        pEntry->TimeStamp = SELECT_NO_TIMEOUT;
    }

    g_pCB->MultiSelectCB.readsds  |= pMsg->Cmd.ReadFds;
    g_pCB->MultiSelectCB.writesds |= pMsg->Cmd.WriteFds;
    g_pCB->MultiSelectCB.SelectEntry[pEntry->ObjIdx] = pEntry;

    SL_TRACE3(DBG_MSG, MSG_312, "\n\rRegistered: Objidx:%d, sec:%d, usec%d\n\r",
              pEntry->ObjIdx, pMsg->Cmd.tv_sec, pMsg->Cmd.tv_usec);

    if((!SelectInProgress) || (g_pCB->MultiSelectCB.ActiveSelect == FALSE))
    {
        /* Add ctrl socket to the read list for this 'select' call */
        pMsg->Cmd.ReadFds |= CTRL_SOCK_FD;

        SL_DRV_OBJ_UNLOCK(&g_pCB->MultiSelectCB.SelectLockObj);

        _RetVal = _SlDrvCmdOp((_SlCmdCtrl_t *)&_SlSelectCmdCtrl, pMsg, NULL);

        if((_RetVal == SL_RET_CODE_OK) && (g_pCB->MultiSelectCB.CtrlSockFD != 0xFF))
        {
            /* Signal any waiting "Select" callers */
            SL_DRV_SYNC_OBJ_SIGNAL(&g_pCB->MultiSelectCB.SelectSyncObj);
        }
    }
    else
    {
        SL_DRV_OBJ_UNLOCK(&g_pCB->MultiSelectCB.SelectLockObj);

        /* Wait here to be signaled by a successfully completed select caller */
        SL_DRV_SYNC_OBJ_WAIT_FOREVER(&g_pCB->MultiSelectCB.SelectSyncObj);

        _RetVal = sl_SendTo(g_pCB->MultiSelectCB.CtrlSockFD,
                                &dummyBuf[0],
                                sizeof(dummyBuf),
                                0,
                                (const SlSockAddr_t *)&_SlCtrlSockRelese,
                                sizeof(SlSockAddrIn_t));
    }

    return _RetVal;
}

/********************************************************************************/
/*  sl_Select */
/* ******************************************************************************/
_i16 sl_Select(_i16 nfds, SlFdSet_t *readsds, SlFdSet_t *writesds, SlFdSet_t *exceptsds, struct SlTimeval_t *timeout)
{
    _i16                      ret;
    _u8                       isCaller = FALSE;
    _SlSelectMsg_u            Msg;
    _SlSelectEntry_t          SelectParams;
    _u8                       SelectInProgress = FALSE;

    /* verify that this API is allowed. if not allowed then
    ignore the API execution and return immediately with an error */
    VERIFY_API_ALLOWED(SL_OPCODE_SILO_SOCKET);
    _SlDrvMemZero(&Msg,          sizeof(_SlSelectMsg_u));
    _SlDrvMemZero(&SelectParams, sizeof(_SlSelectEntry_t));

    Msg.Cmd.Nfds = (_u8)nfds;

    if(readsds)
    {
        Msg.Cmd.ReadFds = (_u16)readsds->fd_array[0];
    }

    if(writesds)
    {
       Msg.Cmd.WriteFds = (_u16)writesds->fd_array[0];
    }

    if(NULL == timeout)
    {
        Msg.Cmd.tv_sec = 0xffff;
        Msg.Cmd.tv_usec = 0xffff;
    }
    else
    {
        if(0xffff <= timeout->tv_sec)
        {
            Msg.Cmd.tv_sec = 0xffff;
        }
        else
        {
            Msg.Cmd.tv_sec = (_u16)timeout->tv_sec;
        }

        /* this divides by 1024 to fit the result in a int16_t.
         * Upon receiving, the NWP multiply this value by 1024.         */
        timeout->tv_usec = (timeout->tv_usec >> 10);

        if(0xffff <= timeout->tv_usec)
        {
            Msg.Cmd.tv_usec = 0xffff;
        }
        else
        {
            Msg.Cmd.tv_usec = (_u16)timeout->tv_usec;
        }
    }

    while(FALSE == isCaller)
    {
        SelectParams.ObjIdx = _SlDrvProtectAsyncRespSetting((_u8*)&SelectParams.Response, SELECT_ID, SL_MAX_SOCKETS);

        if(MAX_CONCURRENT_ACTIONS == SelectParams.ObjIdx)
        {
            return SL_POOL_IS_EMPTY;
        }

        SL_DRV_OBJ_LOCK_FOREVER(&g_pCB->MultiSelectCB.SelectLockObj);

        /* Check if no other 'Select' calls are in progress */
        if(FALSE == g_pCB->MultiSelectCB.ActiveSelect)
        {
            g_pCB->MultiSelectCB.ActiveSelect = TRUE;
        }
        else
        {
            SelectInProgress = TRUE;
        }

        if(!SelectInProgress)
        {
           ret = _SlDrvOpenCtrlSocket();

           if(ret < 0)
           {
               _SlDrvCloseCtrlSocket();
               g_pCB->MultiSelectCB.ActiveSelect = FALSE;
               SL_DRV_OBJ_UNLOCK(&g_pCB->MultiSelectCB.SelectLockObj);
               _SlDrvReleasePoolObj(SelectParams.ObjIdx);
               return ret;
           }
           else
           {
               /* All conditions are met for calling "Select" */
               isCaller = TRUE;
           }
        }
        else if(g_pCB->MultiSelectCB.CtrlSockFD == 0xFF)
        {
            _SlDrvReleasePoolObj(SelectParams.ObjIdx);
            
            /* This is not a first select caller and all sockets are open,
             * caller is expected to wait until select is inactive,
             * before trying to register again.                         */
            g_pCB->MultiSelectCB.ActiveWaiters++;

            SL_DRV_OBJ_UNLOCK(&g_pCB->MultiSelectCB.SelectLockObj);

            SL_DRV_SYNC_OBJ_WAIT_FOREVER(&g_pCB->MultiSelectCB.SelectSyncObj);

            if((_i16)g_pCB->MultiSelectCB.SelectCmdResp.status != SL_RET_CODE_OK)
            {
                return (_i16)(g_pCB->MultiSelectCB.SelectCmdResp.status);
            }

            SelectInProgress = FALSE;
        }
        else
        {
            /* All conditions are met for calling "Select" */
            isCaller = TRUE;
        }
    }

    /* Register this caller details for an select Async event.
     * SelectLockObj is released inside this function,
     * right before sending 'Select' command.                 */
    ret = _SlDrvRegisterForSelectAsync(&SelectParams, &Msg, timeout, SelectInProgress);

    if(ret < 0)
    {
        return (_SlDrvUnRegisterForSelectAsync(&SelectParams, SelectInProgress));
    }

    /* Wait here for a Async event, or command response in case select fails.*/
    SL_DRV_SYNC_OBJ_WAIT_FOREVER(&g_pCB->ObjPool[SelectParams.ObjIdx].SyncObj);

    _SlDrvReleasePoolObj(SelectParams.ObjIdx);

    ret = (_i16)g_pCB->MultiSelectCB.SelectCmdResp.status;

    if(ret == SL_RET_CODE_OK)
    {
        ret = (_i16)SelectParams.Response.Status;

        if(ret > SELECT_TIMEOUT)
        {
            if(readsds)
            {
                readsds->fd_array[0] = SelectParams.Response.ReadFds;
            }

            if(writesds)
            {
               writesds->fd_array[0] = SelectParams.Response.WriteFds;
            }
        }
    }

    return ret;
}

#else

/*******************************************************************************/
/*   _SlSocketHandleAsync_Select */
/*******************************************************************************/
_SlReturnVal_t _SlSocketHandleAsync_Select(void *pVoidBuf)
{
    SlSelectAsyncResponse_t     *pMsgArgs   = (SlSelectAsyncResponse_t *)_SL_RESP_ARGS_START(pVoidBuf);
#if ((defined(SL_RUNTIME_EVENT_REGISTERATION) || defined(slcb_SocketTriggerEventHandler)))
    SlSockTriggerEvent_t SockTriggerEvent;
#endif

    SL_DRV_PROTECTION_OBJ_LOCK_FOREVER();

    VERIFY_SOCKET_CB(NULL != g_pCB->ObjPool[g_pCB->FunctionParams.AsyncExt.ActionIndex].pRespArgs);

    sl_Memcpy(g_pCB->ObjPool[g_pCB->FunctionParams.AsyncExt.ActionIndex].pRespArgs, pMsgArgs, sizeof(SlSelectAsyncResponse_t));

#if ((defined(SL_RUNTIME_EVENT_REGISTERATION) || defined(slcb_SocketTriggerEventHandler)))
    if(1 == _SlIsEventRegistered(SL_EVENT_HDL_SOCKET_TRIGGER))
    {
        if (g_pCB->SocketTriggerSelect.Info.State == SOCK_TRIGGER_WAITING_FOR_RESP)
        {

            SockTriggerEvent.Event = SL_SOCKET_TRIGGER_EVENT_SELECT;
            SockTriggerEvent.EventData = 0;


            g_pCB->SocketTriggerSelect.Info.State = SOCK_TRIGGER_RESP_RECEIVED;

            SL_DRV_PROTECTION_OBJ_UNLOCK();

            /* call the user handler */
            _SlDrvHandleSocketTriggerEvents(&SockTriggerEvent);

            return SL_OS_RET_CODE_OK;
        }
        else
        {
            SL_DRV_SYNC_OBJ_SIGNAL(&g_pCB->ObjPool[g_pCB->FunctionParams.AsyncExt.ActionIndex].SyncObj);
        }
    }
    else
#endif
    {

        SL_DRV_SYNC_OBJ_SIGNAL(&g_pCB->ObjPool[g_pCB->FunctionParams.AsyncExt.ActionIndex].SyncObj);
    }

    SL_DRV_PROTECTION_OBJ_UNLOCK();

    return SL_OS_RET_CODE_OK;
}

_i16 sl_Select(_i16 nfds, SlFdSet_t *readsds, SlFdSet_t *writesds, SlFdSet_t *exceptsds, struct SlTimeval_t *timeout)
{
    _SlSelectMsg_u            Msg;
    SlSelectAsyncResponse_t   AsyncRsp;
    _u8 ObjIdx = MAX_CONCURRENT_ACTIONS;
#if ((defined(SL_RUNTIME_EVENT_REGISTERATION) || defined(slcb_SocketTriggerEventHandler)))
    _u8 IsNonBlocking = FALSE;
#endif

    /* verify that this API is allowed. if not allowed then
    ignore the API execution and return immediately with an error */
    VERIFY_API_ALLOWED(SL_OPCODE_SILO_SOCKET);

#if ((defined(SL_RUNTIME_EVENT_REGISTERATION) || defined(slcb_SocketTriggerEventHandler)))
    if(1 == _SlIsEventRegistered(SL_EVENT_HDL_SOCKET_TRIGGER))
    {
        if( NULL != timeout )
        {
            /* Set that we are in Non-Blocking mode */
            if ( (0 == timeout->tv_sec) && (0 == timeout->tv_usec) )
            {
                IsNonBlocking = TRUE;
            }
            else
            {
               SL_DRV_PROTECTION_OBJ_LOCK_FOREVER();

               /* If there is a trigger select running in the progress abort the new blocking request */
               if (g_pCB->SocketTriggerSelect.Info.State > SOCK_TRIGGER_READY)
               {
                    SL_DRV_PROTECTION_OBJ_UNLOCK();
                    return SL_RET_CODE_SOCKET_SELECT_IN_PROGRESS_ERROR;
               }

               SL_DRV_PROTECTION_OBJ_UNLOCK();
            }

            if (IsNonBlocking == TRUE)
            {
                /* return EAGAIN if we alreay have select trigger in progress */
                if (g_pCB->SocketTriggerSelect.Info.State == SOCK_TRIGGER_WAITING_FOR_RESP)
                {
                    return SL_ERROR_BSD_EAGAIN;
                }
                /* return the stored response if already received */
                else if (g_pCB->SocketTriggerSelect.Info.State == SOCK_TRIGGER_RESP_RECEIVED)
                {

                    if(  ((_i16)g_pCB->SocketTriggerSelect.Resp.Status) >= 0 )
                    {
                        if( readsds )
                        {
                           readsds->fd_array[0]  = g_pCB->SocketTriggerSelect.Resp.ReadFds;
                        }
                        if( writesds )
                        {
                           writesds->fd_array[0] = g_pCB->SocketTriggerSelect.Resp.WriteFds;
                        }
                    }

                    /* Now relaese the pool object */
                    _SlDrvReleasePoolObj(g_pCB->SocketTriggerSelect.Info.ObjPoolIdx);

                    g_pCB->SocketTriggerSelect.Info.ObjPoolIdx = MAX_CONCURRENT_ACTIONS;

                    /* Reset the socket select trigger object */
                    g_pCB->SocketTriggerSelect.Info.State = SOCK_TRIGGER_READY;

                    return (_i16)g_pCB->SocketTriggerSelect.Resp.Status;
                }
            }
        }
    }
#endif

    Msg.Cmd.Nfds          = (_u8)nfds;
    Msg.Cmd.ReadFdsCount  = 0;
    Msg.Cmd.WriteFdsCount = 0;

    Msg.Cmd.ReadFds = 0;
    Msg.Cmd.WriteFds = 0;


    if( readsds )
    {
       Msg.Cmd.ReadFds = (_u16)readsds->fd_array[0];
    }
    if( writesds )
    {
       Msg.Cmd.WriteFds = (_u16)writesds->fd_array[0];
    }
    if( NULL == timeout )
    {
        Msg.Cmd.tv_sec = 0xffff;
        Msg.Cmd.tv_usec = 0xffff;
    }
    else
    {
        if( 0xffff <= timeout->tv_sec )
        {
            Msg.Cmd.tv_sec = 0xffff;
        }
        else
        {
            Msg.Cmd.tv_sec = (_u16)timeout->tv_sec;
        }

        /*  convert to milliseconds */
        timeout->tv_usec = timeout->tv_usec >> 10;

        if( 0xffff <= timeout->tv_usec )
        {
            Msg.Cmd.tv_usec = 0xffff;
        }
        else
        {
            Msg.Cmd.tv_usec = (_u16)timeout->tv_usec;
        }

    }

    /* Use Obj to issue the command, if not available try later */
    ObjIdx = _SlDrvProtectAsyncRespSetting((_u8*)&AsyncRsp, SELECT_ID, SL_MAX_SOCKETS);

    if (MAX_CONCURRENT_ACTIONS == ObjIdx)
    {
        return SL_POOL_IS_EMPTY;
    }

    /* send the command */
    VERIFY_RET_OK(_SlDrvCmdOp((_SlCmdCtrl_t *)&_SlSelectCmdCtrl, &Msg, NULL));

    if(SL_OS_RET_CODE_OK == (_i16)Msg.Rsp.status)
    {
        SL_DRV_SYNC_OBJ_WAIT_FOREVER(&g_pCB->ObjPool[ObjIdx].SyncObj);

        Msg.Rsp.status = (_i16)AsyncRsp.Status;

        /* this code handles the socket trigger mode case */
#if((defined(SL_RUNTIME_EVENT_REGISTERATION) || defined(slcb_SocketTriggerEventHandler)))
        if(1 == _SlIsEventRegistered(SL_EVENT_HDL_SOCKET_TRIGGER))
        {
            /* if no data returned and we are in trigger mode,
               send another select cmd but now with timeout infinite,
               and return immediately with EAGAIN to the user */
            if ((IsNonBlocking == TRUE) && (AsyncRsp.Status == 0))
            {
                /* set the select trigger-in-progress bit */
                g_pCB->SocketTriggerSelect.Info.State  = SOCK_TRIGGER_WAITING_FOR_RESP;

                Msg.Cmd.tv_sec = 0xffff;
                Msg.Cmd.tv_usec = 0xffff;

                /* Release pool object and try to take another call */
                _SlDrvReleasePoolObj(ObjIdx);

                /* Use Obj to issue the command, if not available try later */
                ObjIdx = _SlDrvProtectAsyncRespSetting((_u8*)&g_pCB->SocketTriggerSelect.Resp, SELECT_ID, SL_MAX_SOCKETS);

                if (MAX_CONCURRENT_ACTIONS == ObjIdx)
                {
                    return SL_POOL_IS_EMPTY;
                }

                /* Save the pool index to be released only after the user read the response  */
                g_pCB->SocketTriggerSelect.Info.ObjPoolIdx = ObjIdx;

                /* send the command */
                VERIFY_RET_OK(_SlDrvCmdOp((_SlCmdCtrl_t *)&_SlSelectCmdCtrl, &Msg, NULL));
                return SL_ERROR_BSD_EAGAIN;

            }
        }
#endif

        if(  ((_i16)Msg.Rsp.status) >= 0 )
        {
            if( readsds )
            {
               readsds->fd_array[0]  = AsyncRsp.ReadFds;
            }
            if( writesds )
            {
               writesds->fd_array[0] = AsyncRsp.WriteFds;
            }
        }
    }

    _SlDrvReleasePoolObj(ObjIdx);
    return (_i16)Msg.Rsp.status;
}

#endif /* defined(SL_PLATFORM_MULTI_THREADED) || !defined(slcb_SocketTriggerEventHandler) */
#endif /* _SL_INCLUDE_FUNC(sl_Select) */
#endif /* SL_TINY */
