/*
 *   Copyright (C) 2016 Texas Instruments Incorporated
 *
 *   All rights reserved. Property of Texas Instruments Incorporated.
 *   Restricted rights to use, duplicate or disclose this code are
 *   granted through contract.
 *
 *   The program may not be used without the written permission of
 *   Texas Instruments Incorporated or against the terms and conditions
 *   stipulated in the agreement under which this program has been supplied,
 *   and under no circumstances can it be used with non-TI connectivity device.
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
    Msg.Cmd.Family = (_u8)((sizeof(SlSockAddrIn_t) == *addrlen) ? SL_AF_INET : SL_AF_INET6);

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
        if( (NULL != addr) && (NULL != addrlen) )
        {
#if 0 /*  Kept for backup */
            _SlSocketParseAddress(&AsyncRsp, addr, addrlen);
#else
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
	    else
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
#endif
        }
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

/*******************************************************************************/
/*  sl_Select */
/* ******************************************************************************/
typedef union
{
	SlSelectCommand_t   Cmd;
	_BasicResponse_t    Rsp;
}_SlSelectMsg_u;

/*  Select helper functions */
/*******************************************************************************/
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

#ifndef SL_TINY
#if _SL_INCLUDE_FUNC(sl_Select)

static const _SlCmdCtrl_t _SlSelectCmdCtrl =
{
    SL_OPCODE_SOCKET_SELECT,
    (_SlArgSize_t)sizeof(SlSelectCommand_t),
    (_SlArgSize_t)sizeof(_BasicResponse_t)
};


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

#endif
#endif
