/*
 * driver.c - CC31xx/CC32xx Host Driver Implementation
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
#include <ti/drivers/net/wifi/source/flowcont.h>
/*****************************************************************************/
/* Macro declarations                                                        */
/*****************************************************************************/

#ifndef SL_PLATFORM_MULTI_THREADED

#define GLOBAL_LOCK_CONTEXT_OWNER_APP     (1)
#define GLOBAL_LOCK_CONTEXT_OWNER_SPAWN   (2)

_u8 gGlobalLockContextOwner = GLOBAL_LOCK_CONTEXT_OWNER_APP;

#endif

_u8 gGlobalLockCntRequested=0;
_u8 gGlobalLockCntReleased=0;


/* static functions declaration */
static void _SlDrvUpdateApiInProgress(_i8 Value);

#define API_IN_PROGRESS_UPDATE_NONE       (0)
#define API_IN_PROGRESS_UPDATE_INCREMENT  (1)
#define API_IN_PROGRESS_UPDATE_DECREMENT  (-1)  


/*  2 LSB of the N2H_SYNC_PATTERN are for sequence number 
only in SPI interface
support backward sync pattern */
#define N2H_SYNC_PATTERN_SEQ_NUM_BITS            ((_u32)0x00000003) /* Bits 0..1    - use the 2 LBS for seq num */
#define N2H_SYNC_PATTERN_SEQ_NUM_EXISTS          ((_u32)0x00000004) /* Bit  2       - sign that sequence number exists in the sync pattern */
#define N2H_SYNC_PATTERN_MASK                    ((_u32)0xFFFFFFF8) /* Bits 3..31   - constant SYNC PATTERN */
#define N2H_SYNC_SPI_BUGS_MASK                   ((_u32)0x7FFF7F7F) /* Bits 7,15,31 - ignore the SPI (8,16,32 bites bus) error bits  */
#define BUF_SYNC_SPIM(pBuf)                      ((*(_u32 *)(pBuf)) & N2H_SYNC_SPI_BUGS_MASK)

#define N2H_SYNC_SPIM                            (N2H_SYNC_PATTERN    & N2H_SYNC_SPI_BUGS_MASK)
#define N2H_SYNC_SPIM_WITH_SEQ(TxSeqNum)         ((N2H_SYNC_SPIM & N2H_SYNC_PATTERN_MASK) | N2H_SYNC_PATTERN_SEQ_NUM_EXISTS | ((TxSeqNum) & (N2H_SYNC_PATTERN_SEQ_NUM_BITS)))
#define MATCH_WOUT_SEQ_NUM(pBuf)                 ( BUF_SYNC_SPIM(pBuf) ==  N2H_SYNC_SPIM )
#define MATCH_WITH_SEQ_NUM(pBuf, TxSeqNum)       ( BUF_SYNC_SPIM(pBuf) == (N2H_SYNC_SPIM_WITH_SEQ(TxSeqNum)) )
#define N2H_SYNC_PATTERN_MATCH(pBuf, TxSeqNum) \
    ( \
    (  (*((_u32 *)pBuf) & N2H_SYNC_PATTERN_SEQ_NUM_EXISTS) && ( MATCH_WITH_SEQ_NUM(pBuf, TxSeqNum) ) )    || \
    ( !(*((_u32 *)pBuf) & N2H_SYNC_PATTERN_SEQ_NUM_EXISTS) && ( MATCH_WOUT_SEQ_NUM(pBuf          ) ) )       \
    )

#define OPCODE(_ptr)          (((_SlResponseHeader_t *)(_ptr))->GenHeader.Opcode)         
#define RSP_PAYLOAD_LEN(_ptr) (((_SlResponseHeader_t *)(_ptr))->GenHeader.Len - _SL_RESP_SPEC_HDR_SIZE)         
#define SD(_ptr)              (((SlSocketAddrResponse_u *)(_ptr))->IpV4.Sd)
/*  Actual size of Recv/Recvfrom response data  */
#define ACT_DATA_SIZE(_ptr)   (((SlSocketAddrResponse_u *)(_ptr))->IpV4.StatusOrLen)

#if (defined(SL_PLATFORM_MULTI_THREADED) && !defined(slcb_SocketTriggerEventHandler))
#define MULTI_SELECT_MASK     (~(1 << SELECT_ID))
#else
#define MULTI_SELECT_MASK     (0xFFFFFFFF)
#endif
/* Internal function prototype declaration */

      
/* General Events handling*/
#if defined (EXT_LIB_REGISTERED_GENERAL_EVENTS)

typedef _SlEventPropogationStatus_e (*general_callback) (SlDeviceEvent_t *);

static const general_callback  general_callbacks[] =
{
#ifdef SlExtLib1GeneralEventHandler
    SlExtLib1GeneralEventHandler,
#endif

#ifdef SlExtLib2GeneralEventHandler
    SlExtLib2GeneralEventHandler,
#endif

#ifdef SlExtLib3GeneralEventHandler
    SlExtLib3GeneralEventHandler,
#endif

#ifdef SlExtLib4GeneralEventHandler
    SlExtLib4GeneralEventHandler,
#endif

#ifdef SlExtLib5GeneralEventHandler
    SlExtLib5GeneralEventHandler,
#endif
};

#undef _SlDrvHandleGeneralEvents

/********************************************************************
  _SlDrvHandleGeneralEvents
  Iterates through all the general(device) event handlers which are
  registered by   the external libs/user application.
*********************************************************************/
void _SlDrvHandleGeneralEvents(SlDeviceEvent_t *slGeneralEvent)
{
    _u8 i;

    /* Iterate over all the extenal libs handlers */
    for ( i = 0 ; i < sizeof(general_callbacks)/sizeof(general_callbacks[0]) ; i++ )
    {
        if (EVENT_PROPAGATION_BLOCK == general_callbacks[i](slGeneralEvent) )
        {
            /* exit immediately and do not call the user specific handler as well */
            return;
        }
    }

/* At last call the Application specific handler if registered */
#ifdef slcb_DeviceGeneralEvtHdlr
    slcb_DeviceGeneralEvtHdlr(slGeneralEvent);
#endif

}
#endif


/* WLAN Events handling*/

#if defined (EXT_LIB_REGISTERED_WLAN_EVENTS)

typedef _SlEventPropogationStatus_e (*wlan_callback) (SlWlanEvent_t *);

static wlan_callback  wlan_callbacks[] =
{
#ifdef SlExtLib1WlanEventHandler
        SlExtLib1WlanEventHandler,
#endif

#ifdef SlExtLib2WlanEventHandler
        SlExtLib2WlanEventHandler,
#endif

#ifdef SlExtLib3WlanEventHandler
        SlExtLib3WlanEventHandler,
#endif

#ifdef SlExtLib4WlanEventHandler
        SlExtLib4WlanEventHandler,
#endif

#ifdef SlExtLib5WlanEventHandler
        SlExtLib5WlanEventHandler,
#endif
};

#undef _SlDrvHandleWlanEvents

/***********************************************************
  _SlDrvHandleWlanEvents
  Iterates through all the wlan event handlers which are
  registered by the external libs/user application.
************************************************************/
void _SlDrvHandleWlanEvents(SlWlanEvent_t *slWlanEvent)
{
    _u8 i;

    /* Iterate over all the extenal libs handlers */
    for ( i = 0 ; i < sizeof(wlan_callbacks)/sizeof(wlan_callbacks[0]) ; i++ )
    {
        if ( EVENT_PROPAGATION_BLOCK == wlan_callbacks[i](slWlanEvent) )
        {
            /* exit immediately and do not call the user specific handler as well */
            return;
        }
    }

/* At last call the Application specific handler if registered */
#ifdef slcb_WlanEvtHdlr
    slcb_WlanEvtHdlr(slWlanEvent);
#endif

}
#endif


/* NetApp Events handling */
#if defined (EXT_LIB_REGISTERED_NETAPP_EVENTS)

typedef _SlEventPropogationStatus_e (*netApp_callback) (SlNetAppEvent_t *);

static const netApp_callback  netApp_callbacks[] =
{
#ifdef SlExtLib1NetAppEventHandler
     SlExtLib1NetAppEventHandler,
#endif

#ifdef SlExtLib2NetAppEventHandler
     SlExtLib2NetAppEventHandler,
#endif

#ifdef SlExtLib3NetAppEventHandler
     SlExtLib3NetAppEventHandler,
#endif

#ifdef SlExtLib4NetAppEventHandler
     SlExtLib4NetAppEventHandler,
#endif

#ifdef SlExtLib5NetAppEventHandler
     SlExtLib5NetAppEventHandler,
#endif
};

#undef _SlDrvHandleNetAppEvents

/************************************************************
  _SlDrvHandleNetAppEvents
  Iterates through all the net app event handlers which are
  registered by   the external libs/user application.
************************************************************/
void _SlDrvHandleNetAppEvents(SlNetAppEvent_t *slNetAppEvent)
{
    _u8 i;

    /* Iterate over all the extenal libs handlers */
    for ( i = 0 ; i < sizeof(netApp_callbacks)/sizeof(netApp_callbacks[0]) ; i++ )
    {
        if (EVENT_PROPAGATION_BLOCK == netApp_callbacks[i](slNetAppEvent) )
        {
            /* exit immediately and do not call the user specific handler as well */
            return;
        }
    }

/* At last call the Application specific handler if registered */
#ifdef slcb_NetAppEvtHdlr
    slcb_NetAppEvtHdlr(slNetAppEvent);
#endif

}
#endif


/* Http Server Events handling */
#if defined (EXT_LIB_REGISTERED_HTTP_SERVER_EVENTS)

typedef _SlEventPropogationStatus_e (*httpServer_callback) (SlNetAppHttpServerEvent_t*, SlNetAppHttpServerResponse_t*);

static const httpServer_callback  httpServer_callbacks[] =
{
#ifdef SlExtLib1HttpServerEventHandler
        SlExtLib1HttpServerEventHandler,
#endif

#ifdef SlExtLib2HttpServerEventHandler
        SlExtLib2HttpServerEventHandler,
#endif

#ifdef SlExtLib3HttpServerEventHandler
        SlExtLib3HttpServerEventHandler,
#endif

#ifdef SlExtLib4HttpServerEventHandler
        SlExtLib4HttpServerEventHandler,
#endif

#ifdef SlExtLib5HttpServerEventHandler
      SlExtLib5HttpServerEventHandler,
#endif
};

#undef _SlDrvHandleHttpServerEvents

/*******************************************************************
  _SlDrvHandleHttpServerEvents
  Iterates through all the http server event handlers which are
  registered by the external libs/user application.
********************************************************************/
void _SlDrvHandleHttpServerEvents(SlNetAppHttpServerEvent_t *slHttpServerEvent, SlNetAppHttpServerResponse_t *slHttpServerResponse)
{
    _u8 i;

    /* Iterate over all the external libs handlers */
    for ( i = 0 ; i < sizeof(httpServer_callbacks)/sizeof(httpServer_callbacks[0]) ; i++ )
    {
        if ( EVENT_PROPAGATION_BLOCK == httpServer_callbacks[i](slHttpServerEvent, slHttpServerResponse) )
        {
            /* exit immediately and do not call the user specific handler as well */
            return;
        }
    }

/* At last call the Application specific handler if registered */
#ifdef slcb_NetAppHttpServerHdlr
    slcb_NetAppHttpServerHdlr(slHttpServerEvent, slHttpServerResponse);
#endif

}
#endif


/* Socket Events */
#if defined (EXT_LIB_REGISTERED_SOCK_EVENTS)

typedef _SlEventPropogationStatus_e (*sock_callback) (SlSockEvent_t *);

static const sock_callback  sock_callbacks[] =
{
#ifdef SlExtLib1SockEventHandler
        SlExtLib1SockEventHandler,
#endif

#ifdef SlExtLib2SockEventHandler
        SlExtLib2SockEventHandler,
#endif

#ifdef SlExtLib3SockEventHandler
        SlExtLib3SockEventHandler,
#endif

#ifdef SlExtLib4SockEventHandler
        SlExtLib4SockEventHandler,
#endif

#ifdef SlExtLib5SockEventHandler
        SlExtLib5SockEventHandler,
#endif
};

/*************************************************************
  _SlDrvHandleSockEvents
  Iterates through all the socket event handlers which are
  registered by the external libs/user application.
**************************************************************/
void _SlDrvHandleSockEvents(SlSockEvent_t *slSockEvent)
{
    _u8 i;

    /* Iterate over all the external libs handlers */
    for ( i = 0 ; i < sizeof(sock_callbacks)/sizeof(sock_callbacks[0]) ; i++ )
    {
        if ( EVENT_PROPAGATION_BLOCK == sock_callbacks[i](slSockEvent) )
        {
            /* exit immediately and do not call the user specific handler as well */
            return;
        }
    }

/* At last call the Application specific handler if registered */
#ifdef slcb_SockEvtHdlr
    slcb_SockEvtHdlr(slSockEvent);
#endif

}

#endif

/* Fatal Error Events handling*/
#if defined (EXT_LIB_REGISTERED_FATAL_ERROR_EVENTS)

typedef _SlEventPropogationStatus_e (*fatal_error_callback) (SlDeviceEvent_t *);

static const fatal_error_callback  fatal_error_callbacks[] =
{
#ifdef SlExtLib1FatalErrorEventHandler
    SlExtLib1FatalErrorEventHandler,
#endif

#ifdef SlExtLib2FatalErrorEventHandler
    SlExtLib2FatalErrorEventHandler,
#endif

#ifdef SlExtLib3FatalErrorEventHandler
    SlExtLib3FatalErrorEventHandler,
#endif

#ifdef SlExtLib4FatalErrorEventHandler
    SlExtLib4FatalErrorEventHandler,
#endif

#ifdef SlExtLib5FatalErrorEventHandler
    SlExtLib5FatalErrorEventHandler,
#endif
};

#undef _SlDrvHandleFatalErrorEvents

/********************************************************************
  _SlDrvHandleFatalErrorEvents
  Iterates through all the fatal error (device) event handlers which are
  registered by the external libs/user application.
*********************************************************************/
void _SlDrvHandleFatalErrorEvents(SlDeviceFatal_t *slFatalErrorEvent)
{
    _u8 i;

    /* Iterate over all the extenal libs handlers */
    for ( i = 0 ; i < sizeof(fatal_error_callbacks)/sizeof(fatal_error_callbacks[0]) ; i++ )
    {
        if (EVENT_PROPAGATION_BLOCK == fatal_error_callbacks[i](slFatalErrorEvent) )
        {
            /* exit immediately and do not call the user specific handler as well */
            return;
        }
    }

/* At last call the Application specific handler if registered */
#ifdef slcb_DeviceFatalErrorEvtHdlr
    slcb_DeviceFatalErrorEvtHdlr(slFatalErrorEvent);
#endif

}
#endif

/* NetApp request handler */
#if defined (EXT_LIB_REGISTERED_NETAPP_REQUEST_EVENTS)

typedef _SlEventPropogationStatus_e (*netapp_request_callback) (SlNetAppRequest_t*, SlNetAppResponse_t*);

static const netapp_request_callback  netapp_request_callbacks[] =
{
#ifdef SlExtLib1NetAppRequestEventHandler
    SlExtLib1NetAppRequestEventHandler,
#endif

#ifdef SlExtLib2NetAppRequestEventHandler
    SlExtLib2NetAppRequestEventHandler,
#endif

#ifdef SlExtLib3NetAppRequestEventHandler
    SlExtLib3NetAppRequestEventHandler,
#endif

#ifdef SlExtLib4NetAppRequestEventHandler
    SlExtLib4NetAppRequestEventHandler,
#endif

#ifdef SlExtLib5NetAppRequestEventHandler
    SlExtLib5NetAppRequestEventHandler,
#endif
};

#undef _SlDrvHandleNetAppRequestEvents

/********************************************************************
  _SlDrvHandleNetAppRequest
  Iterates through all the netapp request handlers which are
  registered by the external libs/user application.
*********************************************************************/
void _SlDrvHandleNetAppRequestEvents(SlNetAppRequest_t *pNetAppRequest, SlNetAppResponse_t *pNetAppResponse)
{
    _u8 i;

    /* Iterate over all the extenal libs handlers */
    for ( i = 0 ; i < sizeof(netapp_request_callbacks)/sizeof(netapp_request_callbacks[0]) ; i++ )
    {
        if (EVENT_PROPAGATION_BLOCK == netapp_request_callbacks[i](pNetAppRequest, pNetAppResponse) )
        {
            /* exit immediately and do not call the user specific handler as well */
            return;
        }
    }

/* At last call the Application specific handler if registered */
#ifdef slcb_NetAppRequestHdlr
    slcb_NetAppRequestHdlr(pNetAppRequest, pNetAppResponse);
#endif

}
#endif


#ifndef SL_MEMORY_MGMT_DYNAMIC
typedef struct
{
    _u32 Align;
#ifdef SL_PLATFORM_MULTI_THREADED
    _SlAsyncRespBuf_t AsyncBufPool[SL_MAX_ASYNC_BUFFERS];
#endif
    _u8 AsyncRespBuf[SL_ASYNC_MAX_MSG_LEN];
}_SlStatMem_t;

static _SlStatMem_t g_StatMem;
#endif


/*****************************************************************************/
/* Variables                                                                 */
/*****************************************************************************/
_SlDriverCb_t g_CB;
static const _SlSyncPattern_t g_H2NSyncPattern = H2N_SYNC_PATTERN;

#ifndef SL_IF_TYPE_UART
static const _SlSyncPattern_t g_H2NCnysPattern = H2N_CNYS_PATTERN;
#endif
_volatile _u8           RxIrqCnt;

_u16            g_SlDeviceStatus = 0;
_SlLockObj_t    GlobalLockObj;

const _SlActionLookup_t _SlActionLookupTable[] = 
{
    {ACCEPT_ID, SL_OPCODE_SOCKET_ACCEPTASYNCRESPONSE, (_SlSpawnEntryFunc_t)_SlSocketHandleAsync_Accept},
    {CONNECT_ID, SL_OPCODE_SOCKET_CONNECTASYNCRESPONSE,(_SlSpawnEntryFunc_t)_SlSocketHandleAsync_Connect},
    {SELECT_ID, SL_OPCODE_SOCKET_SELECTASYNCRESPONSE,(_SlSpawnEntryFunc_t)_SlSocketHandleAsync_Select},
    {GETHOSYBYNAME_ID, SL_OPCODE_NETAPP_DNSGETHOSTBYNAMEASYNCRESPONSE,(_SlSpawnEntryFunc_t)_SlNetAppHandleAsync_DnsGetHostByName},
    {GETHOSYBYSERVICE_ID, SL_OPCODE_NETAPP_MDNSGETHOSTBYSERVICEASYNCRESPONSE,(_SlSpawnEntryFunc_t)_SlNetAppHandleAsync_DnsGetHostByService}, 
    {PING_ID, SL_OPCODE_NETAPP_PINGREPORTREQUESTRESPONSE, (_SlSpawnEntryFunc_t)_SlNetAppHandleAsync_PingResponse},
    {NETAPP_RECEIVE_ID, SL_OPCODE_NETAPP_RECEIVE, (_SlSpawnEntryFunc_t)_SlNetAppHandleAsync_NetAppReceive},
    {START_STOP_ID, SL_OPCODE_DEVICE_STOP_ASYNC_RESPONSE,(_SlSpawnEntryFunc_t)_SlDeviceHandleAsync_Stop},
    {NETUTIL_CMD_ID, SL_OPCODE_NETUTIL_COMMANDASYNCRESPONSE,(_SlSpawnEntryFunc_t)_SlNetUtilHandleAsync_Cmd},
    {CLOSE_ID, SL_OPCODE_SOCKET_SOCKETCLOSEASYNCEVENT,(_SlSpawnEntryFunc_t)_SlSocketHandleAsync_Close},
    {START_TLS_ID, SL_OPCODE_SOCKET_SOCKETASYNCEVENT,(_SlSpawnEntryFunc_t)_SlSocketHandleAsync_StartTLS}
};


typedef struct
{
    _u16 opcode;
    _u8  event;
} OpcodeKeyVal_t;


/* The table translates opcode to user's event type */
const OpcodeKeyVal_t OpcodeTranslateTable[] = 
{
    {SL_OPCODE_WLAN_STA_ASYNCCONNECTEDRESPONSE, SL_WLAN_EVENT_CONNECT},
    {SL_OPCODE_WLAN_P2PCL_ASYNCCONNECTEDRESPONSE, SL_WLAN_EVENT_P2P_CONNECT},
    {SL_OPCODE_WLAN_STA_ASYNCDISCONNECTEDRESPONSE, SL_WLAN_EVENT_DISCONNECT},
    {SL_OPCODE_WLAN_P2PCL_ASYNCDISCONNECTEDRESPONSE,SL_WLAN_EVENT_P2P_DISCONNECT},
    {SL_OPCODE_WLAN_ASYNC_STA_ADDED, SL_WLAN_EVENT_STA_ADDED},
    {SL_OPCODE_WLAN_ASYNC_P2PCL_ADDED,SL_WLAN_EVENT_P2P_CLIENT_ADDED},
    {SL_OPCODE_WLAN_ASYNC_STA_REMOVED, SL_WLAN_EVENT_STA_REMOVED},
    {SL_OPCODE_WLAN_ASYNC_P2PCL_REMOVED,SL_WLAN_EVENT_P2P_CLIENT_REMOVED},
    {SL_OPCODE_WLAN_P2P_DEV_FOUND,SL_WLAN_EVENT_P2P_DEVFOUND},    
    {SL_OPCODE_WLAN_P2P_NEG_REQ_RECEIVED, SL_WLAN_EVENT_P2P_REQUEST},
    {SL_OPCODE_WLAN_P2P_CONNECTION_FAILED, SL_WLAN_EVENT_P2P_CONNECTFAIL},
    {SL_OPCODE_WLAN_PROVISIONING_STATUS_ASYNC_EVENT, SL_WLAN_EVENT_PROVISIONING_STATUS},
    {SL_OPCODE_WLAN_PROVISIONING_PROFILE_ADDED_ASYNC_RESPONSE, SL_WLAN_EVENT_PROVISIONING_PROFILE_ADDED},
    {SL_OPCODE_WLAN_RX_FILTER_ASYNC_RESPONSE,SL_WLAN_EVENT_RXFILTER},
    {SL_OPCODE_WLAN_LINK_QUALITY_RESPONSE, SL_WLAN_EVENT_LINK_QUALITY_TRIGGER},

    {SL_OPCODE_NETAPP_IPACQUIRED, SL_NETAPP_EVENT_IPV4_ACQUIRED},
    {SL_OPCODE_NETAPP_IPACQUIRED_V6, SL_NETAPP_EVENT_IPV6_ACQUIRED},
    {SL_OPCODE_NETAPP_IP_LEASED, SL_NETAPP_EVENT_DHCPV4_LEASED},
    {SL_OPCODE_NETAPP_IP_RELEASED, SL_NETAPP_EVENT_DHCPV4_RELEASED},
    {SL_OPCODE_NETAPP_IP_COLLISION, SL_NETAPP_EVENT_IP_COLLISION},
    {SL_OPCODE_NETAPP_IPV4_LOST, SL_NETAPP_EVENT_IPV4_LOST},
    {SL_OPCODE_NETAPP_DHCP_IPV4_ACQUIRE_TIMEOUT, SL_NETAPP_EVENT_DHCP_IPV4_ACQUIRE_TIMEOUT},
    {SL_OPCODE_NETAPP_IPV6_LOST_V6, SL_NETAPP_EVENT_IPV6_LOST},
    {SL_OPCODE_NETAPP_NO_IP_COLLISION_DETECTED, SL_NETAPP_EVENT_NO_IPV4_COLLISION_DETECTED},
    {SL_OPCODE_NETAPP_NO_LOCAL_IP_COLLISION_DETECTED_V6, SL_NETAPP_EVENT_NO_LOCAL_IPV6_COLLISION_DETECTED},
    {SL_OPCODE_NETAPP_NO_GLOBAL_IP_COLLISION_DETECTED_V6, SL_NETAPP_EVENT_NO_GLOBAL_IPV6_COLLISION_DETECTED},
    {SL_OPCODE_SOCKET_TXFAILEDASYNCRESPONSE, SL_SOCKET_TX_FAILED_EVENT},
    {SL_OPCODE_SOCKET_SOCKETASYNCEVENT, SL_SOCKET_ASYNC_EVENT}
    
};



_SlDriverCb_t* g_pCB = NULL;
P_SL_DEV_PING_CALLBACK  pPingCallBackFunc = NULL;

/*****************************************************************************/
/* Function prototypes                                                       */
/*****************************************************************************/
static _SlReturnVal_t _SlDrvMsgRead(_u16* outMsgReadLen, _u8** pAsyncBuf);
static _SlReturnVal_t _SlDrvMsgWrite(_SlCmdCtrl_t  *pCmdCtrl,_SlCmdExt_t  *pCmdExt, _u8 *pTxRxDescBuff);
static _SlReturnVal_t _SlDrvMsgReadCmdCtx(_u16 cmdOpcode, _u8 IsLockRequired);
static _SlReturnVal_t _SlDrvClassifyRxMsg(_SlOpcode_t Opcode );
static _SlReturnVal_t _SlDrvRxHdrRead(_u8 *pBuf);
static void           _SlDrvAsyncEventGenericHandler(_u8 bInCmdContext, _u8 *pAsyncBuffer);
static void           _SlDrvRemoveFromList(_u8* ListIndex, _u8 ItemIndex);
static _SlReturnVal_t _SlDrvFindAndSetActiveObj(_SlOpcode_t  Opcode, _u8 Sd);

/*****************************************************************************/
/* Internal functions                                                        */
/*****************************************************************************/


/*****************************************************************************
_SlDrvDriverCBInit - init Driver Control Block
*****************************************************************************/

_SlReturnVal_t _SlDrvDriverCBInit(void)
{
    _u8          Idx =0;

    g_pCB = &g_CB;

#ifndef SL_PLATFORM_MULTI_THREADED
    {
        extern _SlNonOsCB_t g__SlNonOsCB;
        sl_Memset(&g__SlNonOsCB, 0, sizeof(g__SlNonOsCB));
    }
#endif

    _SlDrvMemZero(g_pCB, (_u16)sizeof(_SlDriverCb_t));
    RxIrqCnt = 0;
    OSI_RET_OK_CHECK( sl_SyncObjCreate(&g_pCB->CmdSyncObj, "CmdSyncObj") );
    SL_DRV_SYNC_OBJ_CLEAR(&g_pCB->CmdSyncObj);

    if (!SL_IS_GLOBAL_LOCK_INIT)
    {
        OSI_RET_OK_CHECK( sl_LockObjCreate(&GlobalLockObj, "GlobalLockObj") );
        SL_SET_GLOBAL_LOCK_INIT;
    }

    OSI_RET_OK_CHECK( sl_LockObjCreate(&g_pCB->ProtectionLockObj, "ProtectionLockObj") );
    g_pCB->NumOfDeletedSyncObj = 0;
#if defined(slcb_SocketTriggerEventHandler)
    g_pCB->SocketTriggerSelect.Info.ObjPoolIdx = MAX_CONCURRENT_ACTIONS;
#endif
    /* Init Drv object */
    _SlDrvMemZero(&g_pCB->ObjPool[0], (_u16)(MAX_CONCURRENT_ACTIONS*sizeof(_SlPoolObj_t)));
    /* place all Obj in the free list*/
    g_pCB->FreePoolIdx = 0;

    for (Idx = 0 ; Idx < MAX_CONCURRENT_ACTIONS ; Idx++)
    {
        g_pCB->ObjPool[Idx].NextIndex = Idx + 1;
        g_pCB->ObjPool[Idx].AdditionalData = SL_MAX_SOCKETS;

        OSI_RET_OK_CHECK( sl_SyncObjCreate(&g_pCB->ObjPool[Idx].SyncObj, "SyncObj"));
        SL_DRV_SYNC_OBJ_CLEAR(&g_pCB->ObjPool[Idx].SyncObj);
    }

     g_pCB->ActivePoolIdx = MAX_CONCURRENT_ACTIONS;
     g_pCB->PendingPoolIdx = MAX_CONCURRENT_ACTIONS;

#ifdef SL_PLATFORM_MULTI_THREADED

#ifdef SL_MEMORY_MGMT_DYNAMIC
    /* reset the spawn messages list */
    g_pCB->spawnMsgList = NULL;
#else
    for (Idx = 0; Idx < SL_MAX_ASYNC_BUFFERS; Idx++)
    {
        g_StatMem.AsyncBufPool[Idx].ActionIndex = 0xFF;
        g_StatMem.AsyncBufPool[Idx].AsyncHndlr = NULL;
    }
#endif
#else
    /* clear the global lock owner */
    _SlDrvSetGlobalLockOwner(GLOBAL_LOCK_CONTEXT_OWNER_APP);
#endif
    /* Flow control init */
    g_pCB->FlowContCB.TxPoolCnt = FLOW_CONT_MIN;
    OSI_RET_OK_CHECK(sl_LockObjCreate(&g_pCB->FlowContCB.TxLockObj, "TxLockObj"));
    OSI_RET_OK_CHECK(sl_SyncObjCreate(&g_pCB->FlowContCB.TxSyncObj, "TxSyncObj"));
    g_pCB->FlowContCB.MinTxPayloadSize = 1536; /* init maximum length */

#if (defined(SL_PLATFORM_MULTI_THREADED) && !defined(slcb_SocketTriggerEventHandler))
    OSI_RET_OK_CHECK(sl_LockObjCreate(&g_pCB->MultiSelectCB.SelectLockObj, "SelectLockObj"));
    OSI_RET_OK_CHECK(sl_SyncObjCreate(&g_pCB->MultiSelectCB.SelectSyncObj, "SelectSyncObj"));
    SL_DRV_SYNC_OBJ_CLEAR(&g_pCB->MultiSelectCB.SelectSyncObj);
    g_pCB->MultiSelectCB.CtrlSockFD = 0xFF;
#endif
    return SL_OS_RET_CODE_OK;
}

/*****************************************************************************
_SlDrvDriverCBDeinit - De init Driver Control Block
*****************************************************************************/
_SlReturnVal_t _SlDrvDriverCBDeinit(void)
{
    _SlSpawnMsgItem_t* pCurr;
    _SlSpawnMsgItem_t* pNext;

    /* Flow control de-init */
    g_pCB->FlowContCB.TxPoolCnt = 0;
    
    SL_SET_DEVICE_STATUS(0);
    
    SL_UNSET_DEVICE_STARTED;

    OSI_RET_OK_CHECK(sl_LockObjDelete(&g_pCB->FlowContCB.TxLockObj));
    OSI_RET_OK_CHECK(sl_SyncObjDelete(&g_pCB->FlowContCB.TxSyncObj));
#if (defined(SL_PLATFORM_MULTI_THREADED) && !defined(slcb_SocketTriggerEventHandler))
    OSI_RET_OK_CHECK(sl_LockObjDelete(&g_pCB->MultiSelectCB.SelectLockObj));
    OSI_RET_OK_CHECK(sl_SyncObjDelete(&g_pCB->MultiSelectCB.SelectSyncObj));
#endif
    OSI_RET_OK_CHECK( sl_SyncObjDelete(&g_pCB->CmdSyncObj));

    OSI_RET_OK_CHECK( sl_LockObjDelete(&g_pCB->ProtectionLockObj) );
   
    g_pCB->FreePoolIdx = 0;
    g_pCB->PendingPoolIdx = MAX_CONCURRENT_ACTIONS;
    g_pCB->ActivePoolIdx = MAX_CONCURRENT_ACTIONS;

#ifdef SL_MEMORY_MGMT_DYNAMIC
    /* Release linked list of async buffers */
    pCurr = g_pCB->spawnMsgList;
    while (NULL != pCurr)
    {
        pNext = pCurr->next;
        sl_Free(pCurr->Buffer);
        sl_Free(pCurr);
        pCurr = pNext;
    }
    g_pCB->spawnMsgList = NULL;
    
#endif

    g_pCB = NULL;

    /* Clear the restart device flag  */
    SL_UNSET_RESTART_REQUIRED;

    return SL_OS_RET_CODE_OK;
}

/*****************************************************************************
_SlDrvRxIrqHandler - Interrupt handler 
*****************************************************************************/
_SlReturnVal_t _SlDrvRxIrqHandler(void *pValue)
{
    (void)pValue;

    sl_IfMaskIntHdlr();

    RxIrqCnt++;

    if (TRUE == g_pCB->WaitForCmdResp)
    {
        OSI_RET_OK_CHECK( sl_SyncObjSignalFromIRQ(&g_pCB->CmdSyncObj) );
    }
    else
    {
        (void)sl_Spawn((_SlSpawnEntryFunc_t)_SlDrvMsgReadSpawnCtx, NULL, SL_SPAWN_FLAG_FROM_SL_IRQ_HANDLER);
    }
    return SL_OS_RET_CODE_OK;
}

/*****************************************************************************
_SlDrvDriverIsApiAllowed - on LOCKED state, only 3 commands are allowed
*****************************************************************************/
_SlReturnVal_t _SlDrvDriverIsApiAllowed(_u16 Silo)
{ 
    if (!SL_IS_COMMAND_ALLOWED) 
    {
        if (SL_IS_DEVICE_STOP_IN_PROGRESS)
        {
            return SL_RET_CODE_STOP_IN_PROGRESS;
        }

        if ((SL_IS_DEVICE_LOCKED) && (SL_OPCODE_SILO_FS != Silo))
        {
            /* All APIs except the FS ones must be aborted if device is locked */
            return SL_RET_CODE_DEV_LOCKED; 
        }
        if (SL_IS_RESTART_REQUIRED)
        {
            /* API has been aborted due command not allowed when Restart required */ 
            /* The opcodes allowed are: SL_OPCODE_DEVICE_STOP_COMMAND */
            return SL_API_ABORTED; 
        }

        if (!SL_IS_DEVICE_STARTED)
        {
            return SL_RET_CODE_DEV_NOT_STARTED;
        }

        if (( SL_IS_PROVISIONING_ACTIVE || SL_IS_PROVISIONING_INITIATED_BY_USER) && !(SL_IS_PROVISIONING_API_ALLOWED))
        {
            /* API has ignored due to provisioning in progress */
            return SL_RET_CODE_PROVISIONING_IN_PROGRESS;
        }

    }
    
    return SL_OS_RET_CODE_OK;
}


/*****************************************************************************
_SlDrvCmdOp
*****************************************************************************/
_SlReturnVal_t _SlDrvCmdOp(
    _SlCmdCtrl_t  *pCmdCtrl ,
    void          *pTxRxDescBuff ,
    _SlCmdExt_t   *pCmdExt)
{
    _SlReturnVal_t RetVal;
    _u8 IsLockRequired = TRUE;
    
    IsLockRequired = (SL_IS_PROVISIONING_IN_PROGRESS && (pCmdCtrl->Opcode == SL_OPCODE_DEVICE_STOP_COMMAND)) ? FALSE: TRUE;

    if (IsLockRequired)
    {
        _u32 GlobalLockFlags = GLOBAL_LOCK_FLAGS_UPDATE_API_IN_PROGRESS;

        /* check the special case of provisioning stop command */
        if (pCmdCtrl->Opcode == SL_OPCODE_WLAN_PROVISIONING_COMMAND)
        {    
            SlWlanProvisioningParams_t *pParams = (SlWlanProvisioningParams_t *)pTxRxDescBuff;

            /* No timeout specifies it is a stop provisioning command */
            if (pParams->InactivityTimeoutSec == 0)
            {
                GlobalLockFlags |= GLOBAL_LOCK_FLAGS_PROVISIONING_STOP_API;            
            }
            
        }

        GlobalLockFlags |= (((_u32)pCmdCtrl->Opcode) << 16);
        SL_DRV_LOCK_GLOBAL_LOCK_FOREVER(GlobalLockFlags);
    }

    /* In case the global was successfully taken but error in progress
    it means it has been released as part of an error handling and we should abort immediately */
    if (SL_IS_RESTART_REQUIRED)
    {
          if (IsLockRequired)
          {
              SL_DRV_LOCK_GLOBAL_UNLOCK(TRUE);
          }

          return SL_API_ABORTED;
    }
    
    g_pCB->WaitForCmdResp = TRUE;

    SL_TRACE1(DBG_MSG, MSG_312, "\n\r_SlDrvCmdOp: call _SlDrvMsgWrite: %x\n\r", pCmdCtrl->Opcode);

    /* send the message */
    RetVal = _SlDrvMsgWrite(pCmdCtrl, pCmdExt, pTxRxDescBuff);
    
    if(SL_OS_RET_CODE_OK == RetVal)
    {
        /* wait for respond */
        RetVal = _SlDrvMsgReadCmdCtx(pCmdCtrl->Opcode, IsLockRequired); /* will free global lock */
        SL_TRACE1(DBG_MSG, MSG_314, "\n\r_SlDrvCmdOp: exited _SlDrvMsgReadCmdCtx: %x\n\r", pCmdCtrl->Opcode);
    }
    else
    {
        if (IsLockRequired)
        {
            SL_DRV_LOCK_GLOBAL_UNLOCK(TRUE);
        }

    }
    
    return RetVal;
}

/*****************************************************************************
_SlDrvDataReadOp
*****************************************************************************/
_SlReturnVal_t _SlDrvDataReadOp(
    _SlSd_t             Sd,
    _SlCmdCtrl_t        *pCmdCtrl ,
    void                *pTxRxDescBuff ,
    _SlCmdExt_t         *pCmdExt)
{
    _SlReturnVal_t RetVal;
    _i16 ObjIdx = MAX_CONCURRENT_ACTIONS;
    _SlArgsData_t pArgsData;

    /* Validate input arguments */
    _SL_ASSERT_ERROR(NULL != pCmdExt->pRxPayload, SL_RET_CODE_INVALID_INPUT);
  
    /* If zero bytes is requested, return error. */
    /* This allows us not to fill remote socket's IP address in return arguments */
    VERIFY_PROTOCOL(0 != pCmdExt->RxPayloadLen);

    /* Validate socket */
    if((Sd & SL_BSD_SOCKET_ID_MASK) >= SL_MAX_SOCKETS)
    {
        return SL_ERROR_BSD_EBADF;
    }

    /*Use Obj to issue the command, if not available try later*/
    ObjIdx = _SlDrvWaitForPoolObj(RECV_ID, Sd & SL_BSD_SOCKET_ID_MASK);

    if (MAX_CONCURRENT_ACTIONS == ObjIdx)
    {
        return SL_POOL_IS_EMPTY;
    }
    if (SL_RET_CODE_STOP_IN_PROGRESS == ObjIdx)
    {
        return SL_RET_CODE_STOP_IN_PROGRESS;
    }

    SL_DRV_PROTECTION_OBJ_LOCK_FOREVER();

    pArgsData.pData = pCmdExt->pRxPayload;
    pArgsData.pArgs =  (_u8 *)pTxRxDescBuff;
    g_pCB->ObjPool[ObjIdx].pRespArgs =  (_u8 *)&pArgsData;

    SL_DRV_PROTECTION_OBJ_UNLOCK();


    /* Do Flow Control check/update for DataWrite operation */
    SL_DRV_OBJ_LOCK_FOREVER(&g_pCB->FlowContCB.TxLockObj);


    /* Clear SyncObj for the case it was signaled before TxPoolCnt */
    /* dropped below '1' (last Data buffer was taken)  */
    /* OSI_RET_OK_CHECK( sl_SyncObjClear(&g_pCB->FlowContCB.TxSyncObj) ); */
    SL_DRV_SYNC_OBJ_CLEAR(&g_pCB->FlowContCB.TxSyncObj);

    if(g_pCB->FlowContCB.TxPoolCnt <= FLOW_CONT_MIN)
    {

        /* If TxPoolCnt was increased by other thread at this moment,
                 TxSyncObj won't wait here */
#if (defined (SL_PLATFORM_MULTI_THREADED)) && (!defined (SL_PLATFORM_EXTERNAL_SPAWN))
        if (_SlDrvIsSpawnOwnGlobalLock())
        {
            while (TRUE)
            {
                /* If we are in spawn context, this is an API which was called from event handler,
                read any async event and check if we got signaled */
                _SlInternalSpawnWaitForEvent();
                /* is it mine? */
                if (0 == sl_SyncObjWait(&g_pCB->FlowContCB.TxSyncObj, SL_OS_NO_WAIT))
                {
                    break;
                }
            }
        }
        else
#endif  
        {
            SL_DRV_SYNC_OBJ_WAIT_FOREVER(&g_pCB->FlowContCB.TxSyncObj);
        }
     
    }

    SL_DRV_LOCK_GLOBAL_LOCK_FOREVER(GLOBAL_LOCK_FLAGS_UPDATE_API_IN_PROGRESS);

    /* In case the global was successfully taken but error in progress
    it means it has been released as part of an error handling and we should abort immediately */
    if (SL_IS_RESTART_REQUIRED)
    {
        SL_DRV_LOCK_GLOBAL_UNLOCK(TRUE);
        return SL_API_ABORTED;
    }

    /* Here we consider the case in which some cmd has been sent to the NWP,
       And its allocated packet has not been freed yet. */
    VERIFY_PROTOCOL(g_pCB->FlowContCB.TxPoolCnt > (FLOW_CONT_MIN - 1));
    g_pCB->FlowContCB.TxPoolCnt--;

    SL_DRV_OBJ_UNLOCK(&g_pCB->FlowContCB.TxLockObj);

    /* send the message */
    RetVal =  _SlDrvMsgWrite(pCmdCtrl, pCmdExt, (_u8 *)pTxRxDescBuff);

    SL_DRV_LOCK_GLOBAL_UNLOCK(TRUE);
    
    if(SL_OS_RET_CODE_OK == RetVal)
    {
        /* in case socket is non-blocking one, the async event should be received immediately */
        if( g_pCB->SocketNonBlocking & (1<<(Sd & SL_BSD_SOCKET_ID_MASK) ))
        {
             _u16 opcodeAsyncEvent = (pCmdCtrl->Opcode ==  SL_OPCODE_SOCKET_RECV) ? SL_OPCODE_SOCKET_RECVASYNCRESPONSE : SL_OPCODE_SOCKET_RECVFROMASYNCRESPONSE; 
             VERIFY_RET_OK(_SlDrvWaitForInternalAsyncEvent(ObjIdx, SL_DRIVER_TIMEOUT_SHORT, opcodeAsyncEvent));
        }
        else
        {
            /* Wait for response message. Will be signaled by _SlDrvMsgRead. */
            VERIFY_RET_OK(_SlDrvWaitForInternalAsyncEvent(ObjIdx, 0, 0));
        }
          
    }

    _SlDrvReleasePoolObj(ObjIdx);
    return RetVal;
}

/* ******************************************************************************/
/*   _SlDrvDataWriteOp                                                          */
/* ******************************************************************************/
_SlReturnVal_t _SlDrvDataWriteOp(
    _SlSd_t             Sd,
    _SlCmdCtrl_t  *pCmdCtrl ,
    void                *pTxRxDescBuff ,
    _SlCmdExt_t         *pCmdExt)
{
    _SlReturnVal_t  RetVal = SL_ERROR_BSD_EAGAIN; /*  initiated as SL_EAGAIN for the non blocking mode */
    _u32 allocTxPoolPkts;

    while( 1 )
    {
        /*  Do Flow Control check/update for DataWrite operation */
        SL_DRV_OBJ_LOCK_FOREVER(&g_pCB->FlowContCB.TxLockObj);

        /*  Clear SyncObj for the case it was signaled before TxPoolCnt */
        /*  dropped below '1' (last Data buffer was taken) */
        /* OSI_RET_OK_CHECK( sl_SyncObjClear(&g_pCB->FlowContCB.TxSyncObj) ); */
        SL_DRV_SYNC_OBJ_CLEAR(&g_pCB->FlowContCB.TxSyncObj);

        /*  number of tx pool packet that will be used */
        allocTxPoolPkts = 1 + (pCmdExt->TxPayload1Len-1) / g_pCB->FlowContCB.MinTxPayloadSize; /* MinTxPayloadSize will be updated by Asunc event from NWP */
        /*  we have indication that the last send has failed - socket is no longer valid for operations  */
        if(g_pCB->SocketTXFailure & (1<<(Sd & SL_BSD_SOCKET_ID_MASK)))
        {
            SL_DRV_OBJ_UNLOCK(&g_pCB->FlowContCB.TxLockObj);
            return SL_ERROR_BSD_SOC_ERROR;
        }
        if(g_pCB->FlowContCB.TxPoolCnt <= FLOW_CONT_MIN + allocTxPoolPkts)
        {
            /*  we have indication that this socket is set as blocking and we try to  */
            /*  unblock it - return an error */
            if( g_pCB->SocketNonBlocking & (1<<(Sd & SL_BSD_SOCKET_ID_MASK) ))
            {
#if (defined (SL_PLATFORM_MULTI_THREADED)) && (!defined (SL_PLATFORM_EXTERNAL_SPAWN))
                if (_SlDrvIsSpawnOwnGlobalLock())
                {
                    _SlInternalSpawnWaitForEvent();
                }
#endif
                SL_DRV_OBJ_UNLOCK(&g_pCB->FlowContCB.TxLockObj);
                return RetVal;
            }
            /*  If TxPoolCnt was increased by other thread at this moment, */
            /*  TxSyncObj won't wait here */
#if (defined (SL_PLATFORM_MULTI_THREADED)) && (!defined (SL_PLATFORM_EXTERNAL_SPAWN))
            if (_SlDrvIsSpawnOwnGlobalLock())
            {
                while (TRUE)
                {
                    /* If we are in spawn context, this is an API which was called from event handler,
                    read any async event and check if we got signaled */
                    _SlInternalSpawnWaitForEvent();
                    /* is it mine? */
                    if (0 == sl_SyncObjWait(&g_pCB->FlowContCB.TxSyncObj, SL_OS_NO_WAIT))
                    {
                       break;
                    }
                }
            }
            else
#endif
            {
                SL_DRV_SYNC_OBJ_WAIT_FOREVER(&g_pCB->FlowContCB.TxSyncObj);
            }
        }
        if(g_pCB->FlowContCB.TxPoolCnt > FLOW_CONT_MIN + allocTxPoolPkts )
        {
            break;
        }
        else
        {
            SL_DRV_OBJ_UNLOCK(&g_pCB->FlowContCB.TxLockObj);
        }
    }

    SL_DRV_LOCK_GLOBAL_LOCK_FOREVER(GLOBAL_LOCK_FLAGS_UPDATE_API_IN_PROGRESS);

    /* In case the global was succesffully taken but error in progress
    it means it has been released as part of an error handling and we should abort immediately */
    if (SL_IS_RESTART_REQUIRED)
    {
        SL_DRV_LOCK_GLOBAL_UNLOCK(TRUE);
        return SL_API_ABORTED;
    }

    /* Here we consider the case in which some cmd has been sent to the NWP,
    And its allocated packet has not been freed yet. */
    VERIFY_PROTOCOL(g_pCB->FlowContCB.TxPoolCnt > (FLOW_CONT_MIN + allocTxPoolPkts -1) );
    g_pCB->FlowContCB.TxPoolCnt -= (_u8)allocTxPoolPkts;

    SL_DRV_OBJ_UNLOCK(&g_pCB->FlowContCB.TxLockObj);
    
    SL_TRACE1(DBG_MSG, MSG_312, "\n\r_SlDrvCmdOp: call _SlDrvMsgWrite: %x\n\r", pCmdCtrl->Opcode);

    /* send the message */
    RetVal =  _SlDrvMsgWrite(pCmdCtrl, pCmdExt, pTxRxDescBuff);
    SL_DRV_LOCK_GLOBAL_UNLOCK(TRUE);

    return RetVal;
}

/* ******************************************************************************/
/*  _SlDrvMsgWrite */
/* ******************************************************************************/
static _SlReturnVal_t _SlDrvMsgWrite(_SlCmdCtrl_t  *pCmdCtrl,_SlCmdExt_t  *pCmdExt, _u8 *pTxRxDescBuff)
{
    _u8 sendRxPayload = FALSE;
    _SL_ASSERT_ERROR(NULL != pCmdCtrl, SL_API_ABORTED);

    g_pCB->FunctionParams.pCmdCtrl = pCmdCtrl;
    g_pCB->FunctionParams.pTxRxDescBuff = pTxRxDescBuff;
    g_pCB->FunctionParams.pCmdExt = pCmdExt;
    
    g_pCB->TempProtocolHeader.Opcode   = pCmdCtrl->Opcode;
    g_pCB->TempProtocolHeader.Len   = (_u16)(_SL_PROTOCOL_CALC_LEN(pCmdCtrl, pCmdExt));

    if (pCmdExt && pCmdExt->RxPayloadLen < 0 )
    {
        pCmdExt->RxPayloadLen = pCmdExt->RxPayloadLen * (-1); /* change sign */
        sendRxPayload = TRUE;
        g_pCB->TempProtocolHeader.Len = g_pCB->TempProtocolHeader.Len + pCmdExt->RxPayloadLen;
    }

#ifdef SL_START_WRITE_STAT
    sl_IfStartWriteSequence(g_pCB->FD);
#endif

#ifdef SL_IF_TYPE_UART
    /*  Write long sync pattern */
    NWP_IF_WRITE_CHECK(g_pCB->FD, (_u8 *)&g_H2NSyncPattern.Long, 2*SYNC_PATTERN_LEN);
#else
    /*  Write short sync pattern */
    NWP_IF_WRITE_CHECK(g_pCB->FD, (_u8 *)&g_H2NSyncPattern.Short, SYNC_PATTERN_LEN);
#endif

    /*  Header */
    NWP_IF_WRITE_CHECK(g_pCB->FD, (_u8 *)&g_pCB->TempProtocolHeader, _SL_CMD_HDR_SIZE);

    /*  Descriptors */
    if (pTxRxDescBuff && pCmdCtrl->TxDescLen > 0)
    {
        NWP_IF_WRITE_CHECK(g_pCB->FD, pTxRxDescBuff, 
                           _SL_PROTOCOL_ALIGN_SIZE(pCmdCtrl->TxDescLen));
    }

    /*  A special mode where Rx payload and Rx length are used as Tx as well */
    /*  This mode requires no Rx payload on the response and currently used by fs_Close and sl_Send on */
    /*  transceiver mode */
    if (sendRxPayload == TRUE )
    {
         NWP_IF_WRITE_CHECK(g_pCB->FD, pCmdExt->pRxPayload, 
                           _SL_PROTOCOL_ALIGN_SIZE(pCmdExt->RxPayloadLen));
    }


    /* if the message has some payload */
    if (pCmdExt)
    {
        /*  If the message has payload, it is mandatory that the message's arguments are protocol aligned. */
        /*  Otherwise the aligning of arguments will create a gap between arguments and payload. */
        VERIFY_PROTOCOL(_SL_IS_PROTOCOL_ALIGNED_SIZE(pCmdCtrl->TxDescLen));
    
        /* In case two seperated buffers were supplied we should merge the two buffers*/
        if ((pCmdExt->TxPayload1Len > 0) && (pCmdExt->TxPayload2Len > 0))
        {    
            _u8 BuffInTheMiddle[4]; 
            _u8 FirstPayloadReminder = 0;
            _u8 SecondPayloadOffset = 0;

            FirstPayloadReminder = pCmdExt->TxPayload1Len & 3; /* calulate the first payload reminder */
            
            /* we first write the 4-bytes aligned payload part */
            pCmdExt->TxPayload1Len -= FirstPayloadReminder;
            
            /* writing the first transaction*/
            NWP_IF_WRITE_CHECK(g_pCB->FD, pCmdExt->pTxPayload1, pCmdExt->TxPayload1Len);
            
            /* Only if we the first payload is not aligned we need the intermediate transaction */
            if (FirstPayloadReminder != 0)
            {
                /* here we count how many bytes we need to take from the second buffer */
                SecondPayloadOffset = 4 - FirstPayloadReminder; 

                /* copy the first payload reminder */
                sl_Memcpy(&BuffInTheMiddle[0], pCmdExt->pTxPayload1 + pCmdExt->TxPayload1Len, FirstPayloadReminder);
            
                /* add the beginning of the second payload to complete 4-bytes transaction */
                sl_Memcpy(&BuffInTheMiddle[FirstPayloadReminder], pCmdExt->pTxPayload2, SecondPayloadOffset);
            
                /* write the second transaction of the 4-bytes buffer */
                NWP_IF_WRITE_CHECK(g_pCB->FD, &BuffInTheMiddle[0], 4);
            }

            
            /* if we still has bytes to write in the second buffer  */
            if (pCmdExt->TxPayload2Len > SecondPayloadOffset)
            {
                /* write the third transaction (truncated second payload) */
                NWP_IF_WRITE_CHECK(g_pCB->FD, 
                                   pCmdExt->pTxPayload2 + SecondPayloadOffset,
                                   _SL_PROTOCOL_ALIGN_SIZE(pCmdExt->TxPayload2Len - SecondPayloadOffset));
            }
        
        }
        else if (pCmdExt->TxPayload1Len > 0)
        {
            /* Only 1 payload supplied (Payload1) so just align to 4 bytes and send it */
            NWP_IF_WRITE_CHECK(g_pCB->FD, pCmdExt->pTxPayload1, 
                            _SL_PROTOCOL_ALIGN_SIZE(pCmdExt->TxPayload1Len));
        }
        else if (pCmdExt->TxPayload2Len > 0)
        {
            /* Only 1 payload supplied (Payload2) so just align to 4 bytes and send it */
            NWP_IF_WRITE_CHECK(g_pCB->FD, pCmdExt->pTxPayload2, 
                            _SL_PROTOCOL_ALIGN_SIZE(pCmdExt->TxPayload2Len));
        
        }
    }

    _SL_DBG_CNT_INC(MsgCnt.Write);

#ifdef SL_START_WRITE_STAT
    sl_IfEndWriteSequence(g_pCB->FD);
#endif

    return SL_OS_RET_CODE_OK;
}

/* ******************************************************************************/
/*  _SlDrvMsgRead  */
/* ******************************************************************************/
_SlReturnVal_t _SlDrvMsgRead(_u16* outMsgReadLen, _u8** pOutAsyncBuf)
{
    /*  alignment for small memory models */
    union
    {      
        _u8           TempBuf[_SL_RESP_HDR_SIZE];
        _u32          DummyBuf[2];
    } uBuf;
    _u8               TailBuffer[4];
    _u16              LengthToCopy;
    _u16              AlignedLengthRecv;
    _u8               *pAsyncBuf = NULL;
    _u16              OpCode;
    _u16              RespPayloadLen;
    _u8               sd = SL_MAX_SOCKETS;
    _SlReturnVal_t    RetVal;
    _SlRxMsgClass_e   RxMsgClass;
    int32_t           Count = 0;
    /* Save parameters in global CB */
    g_pCB->FunctionParams.AsyncExt.AsyncEvtHandler = NULL;
    _SlDrvMemZero(&TailBuffer[0], sizeof(TailBuffer));

    if (_SlDrvRxHdrRead((_u8*)(uBuf.TempBuf)) == SL_API_ABORTED)
    {
        SL_DRV_LOCK_GLOBAL_UNLOCK(TRUE);

        _SlDrvHandleFatalError(SL_DEVICE_EVENT_FATAL_SYNC_LOSS, 0, 0);
        return SL_API_ABORTED;
    }

    OpCode = OPCODE(uBuf.TempBuf);
    RespPayloadLen = (_u16)(RSP_PAYLOAD_LEN(uBuf.TempBuf));

    /* Update the NWP status */
    g_pCB->FlowContCB.TxPoolCnt = ((_SlResponseHeader_t *)uBuf.TempBuf)->TxPoolCnt;
    g_pCB->SocketNonBlocking = ((_SlResponseHeader_t *)uBuf.TempBuf)->SocketNonBlocking;
    g_pCB->SocketTXFailure = ((_SlResponseHeader_t *)uBuf.TempBuf)->SocketTXFailure;
    g_pCB->FlowContCB.MinTxPayloadSize = ((_SlResponseHeader_t *)uBuf.TempBuf)->MinMaxPayload;

    SL_SET_DEVICE_STATUS(((_SlResponseHeader_t *)uBuf.TempBuf)->DevStatus);

    if(g_pCB->FlowContCB.TxPoolCnt > FLOW_CONT_MIN)
    {
        sl_SyncObjGetCount(&g_pCB->FlowContCB.TxSyncObj, &Count);
        if (0 == Count)
        {
            SL_DRV_SYNC_OBJ_SIGNAL(&g_pCB->FlowContCB.TxSyncObj);
        }
    }

    /* Find the RX message class and set its Async event handler */
    _SlDrvClassifyRxMsg(OpCode);
    
    RxMsgClass = g_pCB->FunctionParams.AsyncExt.RxMsgClass;

    switch(RxMsgClass)
    {
        case ASYNC_EVT_CLASS:
        {
            VERIFY_PROTOCOL(NULL == pAsyncBuf);

#ifdef SL_MEMORY_MGMT_DYNAMIC
            *pOutAsyncBuf = (_u8*)sl_Malloc(SL_ASYNC_MAX_MSG_LEN);

#else
            *pOutAsyncBuf = g_StatMem.AsyncRespBuf;
#endif
           /* set the local pointer to the allocated one */
            pAsyncBuf = *pOutAsyncBuf;

            MALLOC_OK_CHECK(pAsyncBuf);

            /* clear the async buffer */
            _SlDrvMemZero(pAsyncBuf, (_u16)SL_ASYNC_MAX_MSG_LEN);
            sl_Memcpy(pAsyncBuf, uBuf.TempBuf, _SL_RESP_HDR_SIZE);

            /* add the protocol header length */
            *outMsgReadLen = _SL_RESP_HDR_SIZE;

            if (_SL_PROTOCOL_ALIGN_SIZE(RespPayloadLen) <= SL_ASYNC_MAX_PAYLOAD_LEN)
            {
                AlignedLengthRecv = (_u16)_SL_PROTOCOL_ALIGN_SIZE(RespPayloadLen);
            }
            else
            {
                AlignedLengthRecv = (_u16)_SL_PROTOCOL_ALIGN_SIZE(SL_ASYNC_MAX_PAYLOAD_LEN);
            }

            /* complete the read of the entire message to the async buffer */
            if (RespPayloadLen > 0)
            {
                NWP_IF_READ_CHECK(g_pCB->FD,
                                  pAsyncBuf + _SL_RESP_HDR_SIZE,
                                  AlignedLengthRecv);
                *outMsgReadLen += AlignedLengthRecv;
            }
            /* In case ASYNC RX buffer length is smaller then the received data length, dump the rest */
            if ((_SL_PROTOCOL_ALIGN_SIZE(RespPayloadLen) > SL_ASYNC_MAX_PAYLOAD_LEN))
            {
                AlignedLengthRecv = (_u16)(_SL_PROTOCOL_ALIGN_SIZE(RespPayloadLen) - SL_ASYNC_MAX_PAYLOAD_LEN);
                while (AlignedLengthRecv > 0)
                {
                    NWP_IF_READ_CHECK(g_pCB->FD,TailBuffer,4);
                    AlignedLengthRecv = AlignedLengthRecv - 4;
                }
            }

            SL_DRV_PROTECTION_OBJ_LOCK_FOREVER();

            if (
               (SL_OPCODE_SOCKET_ACCEPTASYNCRESPONSE == OpCode) ||
               (SL_OPCODE_SOCKET_ACCEPTASYNCRESPONSE_V6 == OpCode) ||
               (SL_OPCODE_SOCKET_CONNECTASYNCRESPONSE == OpCode) ||
               (SL_OPCODE_SOCKET_SOCKETCLOSEASYNCEVENT == OpCode)
               )
               {
                   /* go over the active list if exist to find obj waiting for this Async event */
                   sd = ((((SlSocketResponse_t *)(pAsyncBuf + _SL_RESP_HDR_SIZE))->Sd) & SL_BSD_SOCKET_ID_MASK);
               }
            if (SL_OPCODE_SOCKET_SOCKETASYNCEVENT == OpCode)
               {
                   /* Save the socket descriptor which has been waiting for this opcode */
                   sd = ((((SlSocketAsyncEvent_t *)(pAsyncBuf + _SL_RESP_HDR_SIZE))->Sd) & SL_BSD_SOCKET_ID_MASK);
               }

               _SlDrvFindAndSetActiveObj(OpCode, sd);

               SL_DRV_PROTECTION_OBJ_UNLOCK();

               break;
        }

        case RECV_RESP_CLASS:
        {
            _u8   ExpArgSize; /*  Expected size of Recv/Recvfrom arguments */

            switch(OpCode)
            {
                case SL_OPCODE_SOCKET_RECVFROMASYNCRESPONSE:
                    ExpArgSize = (_u8)RECVFROM_IPV4_ARGS_SIZE;
                    break;
                case SL_OPCODE_SOCKET_RECVFROMASYNCRESPONSE_V6:
                    ExpArgSize = (_u8)RECVFROM_IPV6_ARGS_SIZE;
                    break;
                default:
                    /* SL_OPCODE_SOCKET_RECVASYNCRESPONSE: */
                    ExpArgSize = (_u8)RECV_ARGS_SIZE;
            }

            /*  Read first 4 bytes of Recv/Recvfrom response to get SocketId and actual  */
            /*  response data length */
            NWP_IF_READ_CHECK(g_pCB->FD, &uBuf.TempBuf[4], RECV_ARGS_SIZE);

            /*  Validate Socket ID and Received Length value.  */
            VERIFY_PROTOCOL((SD(&uBuf.TempBuf[4])& SL_BSD_SOCKET_ID_MASK) < SL_MAX_SOCKETS);

            SL_DRV_PROTECTION_OBJ_LOCK_FOREVER();

            /* go over the active list if exist to find obj waiting for this Async event */
            RetVal = _SlDrvFindAndSetActiveObj(OpCode, SD(&uBuf.TempBuf[4]) & SL_BSD_SOCKET_ID_MASK);

#if (defined(SL_PLATFORM_MULTI_THREADED) && !defined(slcb_SocketTriggerEventHandler))
            /* This case is for reading the receive response sent by clearing the control socket. */
            if((RetVal == SL_RET_OBJ_NOT_SET) && (SD(&uBuf.TempBuf[4]) == g_pCB->MultiSelectCB.CtrlSockFD))
            {
                _u8 buffer[16];

                sl_Memcpy(&buffer[0], &uBuf.TempBuf[4], RECV_ARGS_SIZE);

               if(ExpArgSize > (_u8)RECV_ARGS_SIZE)
               {
                   NWP_IF_READ_CHECK(g_pCB->FD,
                                     &buffer[RECV_ARGS_SIZE],
                                     ExpArgSize - RECV_ARGS_SIZE);
               }

               /*  Here g_pCB->ObjPool[g_pCB->FunctionParams.AsyncExt.ActionIndex].pData contains requested(expected) Recv/Recvfrom DataSize. */
               /*  Overwrite requested DataSize with actual one. */
               /*  If error is received, this information will be read from arguments. */
               if(ACT_DATA_SIZE(&uBuf.TempBuf[4]) > 0)
               {

                   /*  Read 4 bytes aligned from interface */
                   /*  therefore check the requested length and read only  */
                   /*  4 bytes aligned data. The rest unaligned (if any) will be read */
                   /*  and copied to a TailBuffer  */
                   LengthToCopy = (_u16)(ACT_DATA_SIZE(&uBuf.TempBuf[4]) & (3));
                   AlignedLengthRecv = (_u16)(ACT_DATA_SIZE(&uBuf.TempBuf[4]) & (~3));
                   if( AlignedLengthRecv >= 4)
                   {
                       NWP_IF_READ_CHECK(g_pCB->FD, &buffer[ExpArgSize], AlignedLengthRecv);
                   }
                   /*  copy the unaligned part, if any */
                   if( LengthToCopy > 0)
                   {
                       NWP_IF_READ_CHECK(g_pCB->FD,TailBuffer,4);
                       /*  copy TailBuffer unaligned part (1/2/3 bytes) */
                       sl_Memcpy(&buffer[ExpArgSize + AlignedLengthRecv], TailBuffer, LengthToCopy);
                   }
               }

               SL_DRV_PROTECTION_OBJ_UNLOCK();
            }
            else
#endif
            {
                /* if _SlDrvFindAndSetActiveObj returned an error, release the protection lock, and return. */
                if(RetVal < 0)
                {
                    SL_DRV_PROTECTION_OBJ_UNLOCK();
                    return SL_API_ABORTED;
                }

                /*  Verify data is waited on this socket. The pArgs should have been set by _SlDrvDataReadOp(). */
                VERIFY_SOCKET_CB(NULL !=  ((_SlArgsData_t *)(g_pCB->ObjPool[g_pCB->FunctionParams.AsyncExt.ActionIndex].pData))->pArgs);

                sl_Memcpy( ((_SlArgsData_t *)(g_pCB->ObjPool[g_pCB->FunctionParams.AsyncExt.ActionIndex].pRespArgs))->pArgs, &uBuf.TempBuf[4], RECV_ARGS_SIZE);

                if(ExpArgSize > (_u8)RECV_ARGS_SIZE)
                {
                    NWP_IF_READ_CHECK(g_pCB->FD,
                        ((_SlArgsData_t *)(g_pCB->ObjPool[g_pCB->FunctionParams.AsyncExt.ActionIndex].pRespArgs))->pArgs + RECV_ARGS_SIZE,
                        ExpArgSize - RECV_ARGS_SIZE);
                }

                /*  Here g_pCB->ObjPool[g_pCB->FunctionParams.AsyncExt.ActionIndex].pData contains requested(expected) Recv/Recvfrom DataSize. */
                /*  Overwrite requested DataSize with actual one. */
                /*  If error is received, this information will be read from arguments. */
                if(ACT_DATA_SIZE(&uBuf.TempBuf[4]) > 0)
                {
                    VERIFY_SOCKET_CB(NULL != ((_SlArgsData_t *)(g_pCB->ObjPool[g_pCB->FunctionParams.AsyncExt.ActionIndex].pRespArgs))->pData);

                    /*  Read 4 bytes aligned from interface */
                    /*  therefore check the requested length and read only  */
                    /*  4 bytes aligned data. The rest unaligned (if any) will be read */
                    /*  and copied to a TailBuffer  */
                    LengthToCopy = (_u16)(ACT_DATA_SIZE(&uBuf.TempBuf[4]) & (3));
                    AlignedLengthRecv = (_u16)(ACT_DATA_SIZE(&uBuf.TempBuf[4]) & (~3));
                    if( AlignedLengthRecv >= 4)
                    {
                        NWP_IF_READ_CHECK(g_pCB->FD,((_SlArgsData_t *)(g_pCB->ObjPool[g_pCB->FunctionParams.AsyncExt.ActionIndex].pRespArgs))->pData, AlignedLengthRecv);
                    }
                    /*  copy the unaligned part, if any */
                    if( LengthToCopy > 0)
                    {
                        NWP_IF_READ_CHECK(g_pCB->FD,TailBuffer,4);
                        /*  copy TailBuffer unaligned part (1/2/3 bytes) */
                        sl_Memcpy(((_SlArgsData_t *)(g_pCB->ObjPool[g_pCB->FunctionParams.AsyncExt.ActionIndex].pRespArgs))->pData + AlignedLengthRecv,TailBuffer,LengthToCopy);
                    }
                }
                     SL_DRV_SYNC_OBJ_SIGNAL(&g_pCB->ObjPool[g_pCB->FunctionParams.AsyncExt.ActionIndex].SyncObj);
                     SL_DRV_PROTECTION_OBJ_UNLOCK();
            }

            break;
        }

        case CMD_RESP_CLASS:
        {
            /*  Some commands pass a maximum arguments size. */
            /*  In this case Driver will send extra dummy patterns to NWP if */
            /*  the response message is smaller than maximum. */
            /*  When RxDescLen is not exact, using RxPayloadLen is forbidden! */
            /*  If such case cannot be avoided - parse message here to detect */
            /*  arguments/payload border. */
            NWP_IF_READ_CHECK(g_pCB->FD,
                g_pCB->FunctionParams.pTxRxDescBuff,
                _SL_PROTOCOL_ALIGN_SIZE(g_pCB->FunctionParams.pCmdCtrl->RxDescLen));

            if((NULL != g_pCB->FunctionParams.pCmdExt) && (0 != g_pCB->FunctionParams.pCmdExt->RxPayloadLen))
            {
                /*  Actual size of command's response payload: <msg_payload_len> - <rsp_args_len> */
                _i16    ActDataSize = (_i16)(RSP_PAYLOAD_LEN(uBuf.TempBuf) - g_pCB->FunctionParams.pCmdCtrl->RxDescLen);

                g_pCB->FunctionParams.pCmdExt->ActualRxPayloadLen = ActDataSize;

                /* Check that the space prepared by user for the response data is sufficient. */
                if(ActDataSize <= 0)
                {
                    g_pCB->FunctionParams.pCmdExt->RxPayloadLen = 0;
                }
                else
                {
                    /* In case the user supplied Rx buffer length which is smaller then the received data length, copy according to user length */
                    if (ActDataSize > g_pCB->FunctionParams.pCmdExt->RxPayloadLen)
                    {
                        LengthToCopy = (_u16)(g_pCB->FunctionParams.pCmdExt->RxPayloadLen & (3));
                        AlignedLengthRecv = (_u16)(g_pCB->FunctionParams.pCmdExt->RxPayloadLen & (~3));
                    }
                    else
                    {
                        LengthToCopy = (_u16)(ActDataSize & (3));
                        AlignedLengthRecv = (_u16)(ActDataSize & (~3));
                    }
                    /*  Read 4 bytes aligned from interface */
                    /*  therefore check the requested length and read only  */
                    /*  4 bytes aligned data. The rest unaligned (if any) will be read */
                    /*  and copied to a TailBuffer  */

                    if( AlignedLengthRecv >= 4)
                    {
                        NWP_IF_READ_CHECK(g_pCB->FD,
                            g_pCB->FunctionParams.pCmdExt->pRxPayload,
                            AlignedLengthRecv );

                    }
                    /*  copy the unaligned part, if any */
                    if( LengthToCopy > 0)
                    {
                        NWP_IF_READ_CHECK(g_pCB->FD,TailBuffer,4);
                        /*  copy TailBuffer unaligned part (1/2/3 bytes) */
                        sl_Memcpy(g_pCB->FunctionParams.pCmdExt->pRxPayload + AlignedLengthRecv,
                            TailBuffer,
                            LengthToCopy);
                        ActDataSize = ActDataSize-4;
                    }
                    /* In case the user supplied Rx buffer length which is smaller then the received data length, dump the rest */
                    if (ActDataSize > g_pCB->FunctionParams.pCmdExt->RxPayloadLen)
                    {
                        /* calculate the rest of the data size to dump  */
                        AlignedLengthRecv = (_u16)( (ActDataSize + 3 - g_pCB->FunctionParams.pCmdExt->RxPayloadLen) & (~3) );
                        while( AlignedLengthRecv > 0)
                        {
                            NWP_IF_READ_CHECK(g_pCB->FD,TailBuffer, 4 );
                            AlignedLengthRecv = AlignedLengthRecv - 4;
                        }
                    }
                }
            }

            break;
        }

#if (defined(SL_PLATFORM_MULTI_THREADED) && !defined(slcb_SocketTriggerEventHandler))
    case MULTI_SELECT_RESP_CLASS:
    {
        /* In case we read Select Response from NWP, and we're not waiting for
         * command complete, that means that a select command was send for a select Joiner. */

        _u8 Idx;

        NWP_IF_READ_CHECK(g_pCB->FD,
                          (_u8*)(&g_pCB->MultiSelectCB.SelectCmdResp),
                          _SL_PROTOCOL_ALIGN_SIZE(sizeof(_BasicResponse_t)));

        if(g_pCB->MultiSelectCB.SelectCmdResp.status != SL_RET_CODE_OK)
        {
            /* If a select response returns without Status O.K, this means that
             * something terribly wrong have happen. So we stop all waiting select callers,
             * and return command error. */
            g_pCB->MultiSelectCB.ActiveSelect = FALSE;

            for(Idx = 0 ; Idx < MAX_CONCURRENT_ACTIONS ; Idx++)
            {
                if(g_pCB->MultiSelectCB.SelectEntry[Idx] != NULL)
                {
                    sl_SyncObjSignal(&g_pCB->ObjPool[Idx].SyncObj);
                }
            }
            /* Clean all table entries, and clear the global read/write fds  */
            _SlDrvMemZero(&g_pCB->MultiSelectCB, sizeof(_SlMultiSelectCB_t));
        }

        break;
    }
#endif

    default:
        /*  DUMMY_MSG_CLASS: Flow control message has no payload. */
        break;
    }

    _SL_DBG_CNT_INC(MsgCnt.Read);

    /*  Unmask Interrupt call */
    sl_IfUnMaskIntHdlr();

    return SL_OS_RET_CODE_OK;
}


/* ******************************************************************************/
/*  _SlDrvAsyncEventGenericHandler */
/* ******************************************************************************/
static void _SlDrvAsyncEventGenericHandler(_u8 bInCmdContext, _u8 *pAsyncBuffer)
{
    _u32 SlAsyncEvent = 0;
    _u8  OpcodeFound = FALSE; 
    _u8  i;
    
    _u32* pEventLocation  = NULL; /* This pointer will override the async buffer with the translated event type */
    _SlResponseHeader_t  *pHdr       = (_SlResponseHeader_t *)pAsyncBuffer;


    /* if no async event registered nothing to do..*/
    if (g_pCB->FunctionParams.AsyncExt.AsyncEvtHandler == NULL)
    {
        return;
    }
    
    /* In case we in the middle of the provisioning, filter out 
       all the async events except the provisioning ones  */
    if ( (( SL_IS_PROVISIONING_ACTIVE || SL_IS_PROVISIONING_INITIATED_BY_USER) && !(SL_IS_PROVISIONING_API_ALLOWED)) &&
          (pHdr->GenHeader.Opcode != SL_OPCODE_WLAN_PROVISIONING_STATUS_ASYNC_EVENT) &&
          (pHdr->GenHeader.Opcode != SL_OPCODE_DEVICE_RESET_REQUEST_ASYNC_EVENT) &&
          (pHdr->GenHeader.Opcode != SL_OPCODE_DEVICE_INITCOMPLETE) &&
          (pHdr->GenHeader.Opcode != SL_OPCODE_WLAN_PROVISIONING_PROFILE_ADDED_ASYNC_RESPONSE) &&
          (pHdr->GenHeader.Opcode != SL_OPCODE_NETAPP_REQUEST) )
    {
        return;
    }

    /* Iterate through all the opcode in the table */
    for (i=0; i< (_u8)(sizeof(OpcodeTranslateTable) / sizeof(OpcodeKeyVal_t)); i++)
    {
        if (OpcodeTranslateTable[i].opcode == pHdr->GenHeader.Opcode)
        {
            SlAsyncEvent = OpcodeTranslateTable[i].event;
            OpcodeFound = TRUE;
            break;
        }
    }

    /* No Async event found in the table */
    if (OpcodeFound == FALSE)
    {
        if ((pHdr->GenHeader.Opcode & SL_OPCODE_SILO_MASK) == SL_OPCODE_SILO_DEVICE)
        {
            DeviceEventInfo_t deviceEvent;

            deviceEvent.pAsyncMsgBuff = pAsyncBuffer;
            deviceEvent.bInCmdContext = bInCmdContext;

            g_pCB->FunctionParams.AsyncExt.AsyncEvtHandler(&deviceEvent);
        }
        else
        {
            /* This case handles all the async events handlers of the DEVICE & SOCK Silos which are handled internally.
            For these cases we send the async even buffer as is */
            g_pCB->FunctionParams.AsyncExt.AsyncEvtHandler(pAsyncBuffer);
        }
    }
    else
    {
       /* calculate the event type location to be filled in the async buffer */
       pEventLocation = (_u32*)(pAsyncBuffer + sizeof (_SlResponseHeader_t) - sizeof(SlAsyncEvent));

       /* Override the async buffer (before the data starts ) with our event type  */
       *pEventLocation = SlAsyncEvent;

       /* call the event handler registered by the user with our async buffer which now holds
                the User's event type and its related data */
       g_pCB->FunctionParams.AsyncExt.AsyncEvtHandler(pEventLocation);

    }
}

/* ******************************************************************************/
/*  _SlDrvMsgReadCmdCtx  */
/* ******************************************************************************/
static _SlReturnVal_t _SlDrvMsgReadCmdCtx(_u16 cmdOpcode, _u8 IsLockRequired)
{
    _u32 CmdCmpltTimeout;
    _i16 RetVal=0;
    _u8  *pAsyncBuf = NULL;

    /* the sl_FsOpen/sl_FsProgram APIs may take long time */
    if ((cmdOpcode == SL_OPCODE_NVMEM_FILEOPEN) || (cmdOpcode == SL_OPCODE_NVMEM_NVMEMFSPROGRAMMINGCOMMAND))
    {
        CmdCmpltTimeout = ((_u32)SL_DRIVER_TIMEOUT_LONG * 10);
    }
    else
    {
        /* For any FS command, the timeout will be the long one as the commnad response holds the full response data */
        CmdCmpltTimeout = (SL_OPCODE_SILO_FS & cmdOpcode)? (_u32)(SL_DRIVER_TIMEOUT_LONG) : (_u32)SL_DRIVER_TIMEOUT_SHORT;
    }

    /*  after command response is received and WaitForCmdResp */
    /*  flag is set FALSE, it is necessary to read out all */
    /*  Async messages in Commands context, because ssiDma_IsrHandleSignalFromSlave */
    /*  could have dispatched some Async messages to g_NwpIf.CmdSyncObj */
    /*  after command response but before this response has been processed */
    /*  by spi_singleRead and WaitForCmdResp was set FALSE. */
    while (TRUE == g_pCB->WaitForCmdResp)
    {
        if(_SL_PENDING_RX_MSG(g_pCB))
        {
            _u16 outMsgLen = 0;

            RetVal = _SlDrvMsgRead(&outMsgLen,&pAsyncBuf);

            if (RetVal != SL_OS_RET_CODE_OK)
            {
                g_pCB->WaitForCmdResp = FALSE;

                if ((IsLockRequired) && (RetVal != SL_API_ABORTED))
                {
                    SL_DRV_LOCK_GLOBAL_UNLOCK(TRUE);
                }
                
                return SL_API_ABORTED;
            }
            g_pCB->RxDoneCnt++;

            if (CMD_RESP_CLASS == g_pCB->FunctionParams.AsyncExt.RxMsgClass)
            {
                g_pCB->WaitForCmdResp = FALSE;
                /*  In case CmdResp has been read without  waiting on CmdSyncObj -  that */
                /*  Sync object. That to prevent old signal to be processed. */
                SL_DRV_SYNC_OBJ_CLEAR(&g_pCB->CmdSyncObj);
            }
            else if (ASYNC_EVT_CLASS == g_pCB->FunctionParams.AsyncExt.RxMsgClass)
            {
                
#ifdef SL_PLATFORM_MULTI_THREADED
                /* Do not handle async events in command context */
                /* All async events data will be stored in list and handled in spawn context */
                RetVal = _SlSpawnMsgListInsert(outMsgLen, pAsyncBuf);
                if (SL_RET_CODE_NO_FREE_ASYNC_BUFFERS_ERROR == RetVal)
                {
                     _SlFindAndReleasePendingCmd();
                }

#else
                _SlDrvAsyncEventGenericHandler(TRUE, pAsyncBuf);
#endif
                
#ifdef SL_MEMORY_MGMT_DYNAMIC
                sl_Free(pAsyncBuf);
#else
                pAsyncBuf = NULL;
#endif
            }
#if (defined(SL_PLATFORM_MULTI_THREADED) && !defined(slcb_SocketTriggerEventHandler))
            else if(MULTI_SELECT_RESP_CLASS == g_pCB->FunctionParams.AsyncExt.RxMsgClass)
            {
                sl_SyncObjSignal(&g_pCB->MultiSelectCB.SelectSyncObj);
            }
#endif
        }
        else
        {
             /* CmdSyncObj will be signaled by IRQ */
            RetVal = sl_SyncObjWait(&g_pCB->CmdSyncObj, CmdCmpltTimeout);
            if (RetVal != 0)
            {
                g_pCB->WaitForCmdResp = FALSE;

                if (IsLockRequired)
                {
                    SL_DRV_LOCK_GLOBAL_UNLOCK(TRUE);
                }
                
                /* only if device started handle the fatal error */
                if (SL_IS_DEVICE_STARTED)
                {
                    _SlDrvHandleFatalError(SL_DEVICE_EVENT_FATAL_NO_CMD_ACK, cmdOpcode, (_u32)CmdCmpltTimeout);
                }
                
                return SL_API_ABORTED;
            }
        }
    }

#ifdef SL_PLATFORM_MULTI_THREADED
    if (_SlSpawnMsgListGetCount() > 0)
    {
        /* signal the spawn task to process the pending async events received during the cmd */
        sl_Spawn((_SlSpawnEntryFunc_t)_SlDrvMsgReadSpawnCtx, NULL, SL_SPAWN_FLAG_FROM_CMD_PROCESS);
    }
#endif

    /*  If there are more pending Rx Msgs after CmdResp is received, */
    /*  that means that these are Async, Dummy or Read Data Msgs. */
    /*  Spawn _SlDrvMsgReadSpawnCtx to trigger reading these messages from */
    /*  Temporary context. */
    /*  sl_Spawn is activated, using a different context */
    if (IsLockRequired)
    {
        SL_DRV_LOCK_GLOBAL_UNLOCK(TRUE);
    }
    
    if(_SL_PENDING_RX_MSG(g_pCB))
    {
        sl_Spawn((_SlSpawnEntryFunc_t)_SlDrvMsgReadSpawnCtx, NULL, SL_SPAWN_FLAG_FROM_CMD_CTX);
    }

    return SL_OS_RET_CODE_OK;
}

/* ******************************************************************************/
/*  _SlDrvMsgReadSpawnCtx                                                       */
/* ******************************************************************************/
_SlReturnVal_t _SlDrvMsgReadSpawnCtx(void *pValue)
{
    _SlReturnVal_t RetVal = SL_OS_RET_CODE_OK;
    _u16 outMsgLen = 0;
    _u8  *pAsyncBuf = NULL;

#ifdef SL_POLLING_MODE_USED

    /*  for polling based systems */
    do
    {
        if (GlobalLockObj != NULL)
        {
            RetVal = sl_LockObjLock(&GlobalLockObj, 0);
    
            if (SL_OS_RET_CODE_OK != RetVal )
            {
                if (TRUE == g_pCB->WaitForCmdResp)
                {
                    SL_DRV_SYNC_OBJ_SIGNAL(&g_pCB->CmdSyncObj);
                    return SL_RET_CODE_OK;
                }
            }

        }
        
    }
    while (SL_OS_RET_CODE_OK != RetVal);

#else
    SL_DRV_LOCK_GLOBAL_LOCK_FOREVER(GLOBAL_LOCK_FLAGS_NONE);
#endif

#ifndef SL_PLATFORM_MULTI_THREADED
    /* set the global lock owner (spawn context) */
    _SlDrvSetGlobalLockOwner(GLOBAL_LOCK_CONTEXT_OWNER_SPAWN);
#endif
    /* pValue parameter is currently not in use */
    (void)pValue;

    /*  Messages might have been read by CmdResp context. Therefore after */
    /*  getting LockObj, check again where the Pending RX Msg is still present. */
    if(FALSE == (_SL_PENDING_RX_MSG(g_pCB)))
    {
#ifndef SL_PLATFORM_MULTI_THREADED
        /* clear the global lock owner (spawn context) */
        _SlDrvSetGlobalLockOwner(GLOBAL_LOCK_CONTEXT_OWNER_APP);
#endif
        SL_DRV_LOCK_GLOBAL_UNLOCK(FALSE);
        
        return SL_RET_CODE_OK;
    }
   
    if (SL_IS_DEVICE_STARTED || SL_IS_DEVICE_START_IN_PROGRESS)
    {

        RetVal = _SlDrvMsgRead(&outMsgLen, &pAsyncBuf);

        if (RetVal != SL_OS_RET_CODE_OK)
        {
            if (RetVal != SL_API_ABORTED)
            {
    #ifndef SL_PLATFORM_MULTI_THREADED
                /* clear the global lock owner (spawn context) */
                _SlDrvSetGlobalLockOwner(GLOBAL_LOCK_CONTEXT_OWNER_APP);
    #endif
                SL_DRV_LOCK_GLOBAL_UNLOCK(FALSE);
            }
            return SL_API_ABORTED;
        }

        g_pCB->RxDoneCnt++;

        switch(g_pCB->FunctionParams.AsyncExt.RxMsgClass)
        {
        case ASYNC_EVT_CLASS:
            /*  If got here and protected by LockObj a message is waiting  */
            /*  to be read */
            VERIFY_PROTOCOL(NULL != pAsyncBuf);


            _SlDrvAsyncEventGenericHandler(FALSE, pAsyncBuf);

    #ifdef SL_MEMORY_MGMT_DYNAMIC
            sl_Free(pAsyncBuf);
    #else
            pAsyncBuf = NULL;
    #endif
            break;
        case DUMMY_MSG_CLASS:
        case RECV_RESP_CLASS:
            /* These types are legal in this context. Do nothing */
            break;
        case CMD_RESP_CLASS:
            /* Command response is illegal in this context -  */
            /* One exception exists though: 'Select' response (SL_OPCODE_SOCKET_SELECTRESPONSE) Opcode = 0x1407 */
            break;
    #if (defined(SL_PLATFORM_MULTI_THREADED) && !defined(slcb_SocketTriggerEventHandler))
        case MULTI_SELECT_RESP_CLASS:
            /* If everything's OK, we signal for any other joiners to call 'Select'.*/
            sl_SyncObjSignal(&g_pCB->MultiSelectCB.SelectSyncObj);
            break;
    #endif
        default:
            _SL_ASSERT_ERROR(0, SL_API_ABORTED);
        }
    #ifndef SL_PLATFORM_MULTI_THREADED
        /* clear the global lock owner (spawn context) */
        _SlDrvSetGlobalLockOwner(GLOBAL_LOCK_CONTEXT_OWNER_APP);
    #endif
        SL_DRV_LOCK_GLOBAL_UNLOCK(FALSE);
        return(SL_RET_CODE_OK);
    }
    return(SL_RET_CODE_INTERFACE_CLOSED);
}

/*

#define SL_OPCODE_SILO_DEVICE                           ( 0x0 << SL_OPCODE_SILO_OFFSET )
#define SL_OPCODE_SILO_WLAN                             ( 0x1 << SL_OPCODE_SILO_OFFSET )
#define SL_OPCODE_SILO_SOCKET                           ( 0x2 << SL_OPCODE_SILO_OFFSET )
#define SL_OPCODE_SILO_NETAPP                           ( 0x3 << SL_OPCODE_SILO_OFFSET )
#define SL_OPCODE_SILO_FS                               ( 0x4 << SL_OPCODE_SILO_OFFSET )
#define SL_OPCODE_SILO_NETCFG                           ( 0x5 << SL_OPCODE_SILO_OFFSET )

*/

/* The Lookup table below holds the event handlers to be called according to the incoming
    RX message SILO type */
static const _SlSpawnEntryFunc_t RxMsgClassLUT[] = {
    (_SlSpawnEntryFunc_t)_SlDeviceEventHandler, /* SL_OPCODE_SILO_DEVICE */
#if defined(slcb_WlanEvtHdlr) || defined(EXT_LIB_REGISTERED_WLAN_EVENTS)
    (_SlSpawnEntryFunc_t)_SlDrvHandleWlanEvents,           /* SL_OPCODE_SILO_WLAN */
#else
    NULL,
#endif
#if defined (slcb_SockEvtHdlr) || defined(EXT_LIB_REGISTERED_SOCK_EVENTS)
    (_SlSpawnEntryFunc_t)_SlDrvHandleSockEvents,      /* SL_OPCODE_SILO_SOCKET */
#else
    NULL,
#endif
#if defined(slcb_NetAppEvtHdlr) || defined(EXT_LIB_REGISTERED_NETAPP_EVENTS)
    (_SlSpawnEntryFunc_t)_SlDrvHandleNetAppEvents,    /* SL_OPCODE_SILO_NETAPP */
#else
    NULL,  
#endif
    NULL,                                             /* SL_OPCODE_SILO_FS      */
    NULL,                                             /* SL_OPCODE_SILO_NETCFG  */
    (_SlSpawnEntryFunc_t)_SlNetUtilHandleAsync_Cmd,   /* SL_OPCODE_SILO_NETUTIL */
    NULL
};


/* ******************************************************************************/
/*  _SlDrvClassifyRxMsg */
/* ******************************************************************************/
static _SlReturnVal_t _SlDrvClassifyRxMsg(
    _SlOpcode_t         Opcode)
{
    _SlSpawnEntryFunc_t AsyncEvtHandler = NULL;
    _SlRxMsgClass_e     RxMsgClass  = CMD_RESP_CLASS;
    _u8               Silo;
    

    if (0 == (SL_OPCODE_SYNC & Opcode))
    {   /* Async event has received */
        
        if (SL_OPCODE_DEVICE_DEVICEASYNCDUMMY == Opcode)
        { 
            RxMsgClass = DUMMY_MSG_CLASS;
        }
        else if ( (SL_OPCODE_SOCKET_RECVASYNCRESPONSE == Opcode) || (SL_OPCODE_SOCKET_RECVFROMASYNCRESPONSE == Opcode)
                    || (SL_OPCODE_SOCKET_RECVFROMASYNCRESPONSE_V6 == Opcode)
                ) 
        {
            RxMsgClass = RECV_RESP_CLASS;
        }
        else
        {
            /* This is Async Event class message */
            RxMsgClass = ASYNC_EVT_CLASS;
        
            /* Despite the fact that 4 bits are allocated in the SILO field, we actually have only 6 SILOs
              So we can use the 8 options of SILO in look up table */
            Silo = (_u8)((Opcode >> SL_OPCODE_SILO_OFFSET) & 0x7);

            VERIFY_PROTOCOL(Silo < (_u8)(sizeof(RxMsgClassLUT)/sizeof(_SlSpawnEntryFunc_t)));

            /* Set the SILO's async event handler according to the LUT
               If this specific event requires a direct async event handler, the 
               async event handler will be overwrite according to the action table */
            AsyncEvtHandler = RxMsgClassLUT[Silo];
            
            if ((SL_OPCODE_NETAPP_HTTPGETTOKENVALUE == Opcode) || (SL_OPCODE_NETAPP_HTTPPOSTTOKENVALUE == Opcode) || 
                (SL_OPCODE_NETAPP_REQUEST == Opcode) || (SL_OPCODE_NETAPP_RESPONSE == Opcode) || (SL_OPCODE_NETAPP_SEND == Opcode))
            {
                AsyncEvtHandler = _SlNetAppEventHandler;
            }
            else if (SL_OPCODE_NETAPP_PINGREPORTREQUESTRESPONSE == Opcode)
            {
                AsyncEvtHandler = (_SlSpawnEntryFunc_t)_SlNetAppHandleAsync_PingResponse;
            }
        }
    }
#if (defined(SL_PLATFORM_MULTI_THREADED) && !defined(slcb_SocketTriggerEventHandler))
    else if((Opcode == SL_OPCODE_SOCKET_SELECTRESPONSE) &&
            (g_pCB->FunctionParams.pCmdCtrl->Opcode != SL_OPCODE_SOCKET_SELECT))
    {
        /* Only in case this response came from a 'Select' sent in an Async event, Mark the message as MULTI_SELECT_RESPONSE */
        RxMsgClass = MULTI_SELECT_RESP_CLASS;
    }
#endif
    g_pCB->FunctionParams.AsyncExt.RxMsgClass = RxMsgClass; 
    g_pCB->FunctionParams.AsyncExt.AsyncEvtHandler = AsyncEvtHandler;
         
    return SL_RET_CODE_OK;
}


/* ******************************************************************************/
/*  _SlDrvRxHdrRead  */
/* ******************************************************************************/
static _SlReturnVal_t _SlDrvRxHdrRead(_u8 *pBuf)
{
    _u8        ShiftIdx;  
    _u8        TimeoutState = TIMEOUT_STATE_INIT_VAL;
    _u8        SearchSync = TRUE;
    _u8        SyncPattern[4];
#if (defined(slcb_GetTimestamp))
     _SlTimeoutParams_t      TimeoutInfo={0};
#endif

#ifndef SL_IF_TYPE_UART
    /*  1. Write CNYS pattern to NWP when working in SPI mode only  */
    NWP_IF_WRITE_CHECK(g_pCB->FD, (_u8 *)&g_H2NCnysPattern.Short, SYNC_PATTERN_LEN);
#endif

#if (defined(slcb_GetTimestamp))
    _SlDrvStartMeasureTimeout(&TimeoutInfo, SYNC_PATTERN_TIMEOUT_IN_MSEC);
#endif

    /*  2. Read 8 bytes (protocol aligned) - expected to be the sync pattern */
    NWP_IF_READ_CHECK(g_pCB->FD, &pBuf[0], 8);

    /* read while first 4 bytes are different than last 4 bytes */
    while ( *(_u32 *)&pBuf[0] == *(_u32 *)&pBuf[4])
    {
         NWP_IF_READ_CHECK(g_pCB->FD, &pBuf[4], 4);
#if (defined(slcb_GetTimestamp))
        if (_SlDrvIsTimeoutExpired(&TimeoutInfo))
        {
            return SL_API_ABORTED;
        }
#endif
    }

    /* scan for the sync pattern till found or timeout elapsed (if configured) */
    while (SearchSync && TimeoutState)
    {
        /* scan till we get the real sync pattern */
        for (ShiftIdx =0; ShiftIdx <=4 ; ShiftIdx++)
        {
           /* copy to local variable to ensure starting address which is 4-bytes aligned */
           sl_Memcpy(&SyncPattern[0],  &pBuf[ShiftIdx], 4);

           /* sync pattern found so complete the read to  4 bytes aligned */
           if (N2H_SYNC_PATTERN_MATCH(&SyncPattern[0], g_pCB->TxSeqNum))
           {
                   /* copy the bytes following the sync pattern to the buffer start */
                   sl_Memcpy(&pBuf[0],  &pBuf[ShiftIdx + SYNC_PATTERN_LEN], 4);

                   if (ShiftIdx != 0)
                   {
                         /* read the rest of the bytes (only if wer'e not aligned) (expected to complete the opcode + length fields ) */
                      NWP_IF_READ_CHECK(g_pCB->FD, &pBuf[SYNC_PATTERN_LEN - ShiftIdx], ShiftIdx);
                   }

                  /* here we except to get the opcode + length or false doubled sync..*/
                  SearchSync = FALSE;
                  break;
           }
        }

        if (SearchSync == TRUE)
        {
            /* sync not found move top 4 bytes to bottom */
            *(_u32 *)&pBuf[0] = *(_u32 *)&pBuf[4];

            /* read 4 more bytes to the buffer top */
            NWP_IF_READ_CHECK(g_pCB->FD, &pBuf[4], 4);
        }

 #if (defined (slcb_GetTimestamp))

        /* if we got here after first timeout detection, it means that we gave
            one more chance, and we can now exit the loop with timeout expiry  */
        if (TIMEOUT_ONE_MORE_SHOT == TimeoutState)
        {
            TimeoutState =  TIMEOUT_STATE_EXPIRY;
            break;
        }
        
        /* Timeout occurred. do not break now as we want to give one more chance in case
            the timeout occurred due to some external context switch */
        if (_SlDrvIsTimeoutExpired(&TimeoutInfo))
        {
            TimeoutState = TIMEOUT_ONE_MORE_SHOT;
        }

#endif
    } /* end of while */

#if (defined (slcb_GetTimestamp))
    if (TIMEOUT_STATE_EXPIRY  == TimeoutState)
    {
        return SL_API_ABORTED;
    }
#endif

    /*  6. Scan for Double pattern. */
    while ( N2H_SYNC_PATTERN_MATCH(pBuf, g_pCB->TxSeqNum) )
    {
        _SL_DBG_CNT_INC(Work.DoubleSyncPattern);
        NWP_IF_READ_CHECK(g_pCB->FD, &pBuf[0], SYNC_PATTERN_LEN);
    }
    g_pCB->TxSeqNum++;

    /*  7. Here we've read Generic Header (4 bytes opcode+length).
     * Now Read the Resp Specific header (4 more bytes). */
    NWP_IF_READ_CHECK(g_pCB->FD, &pBuf[SYNC_PATTERN_LEN], _SL_RESP_SPEC_HDR_SIZE);

    return SL_RET_CODE_OK;
}

/* ***************************************************************************** */
/*  _SlDrvBasicCmd */
/* ***************************************************************************** */
typedef union
{
    _BasicResponse_t    Rsp;
}_SlBasicCmdMsg_u;


_SlReturnVal_t _SlDrvBasicCmd(_SlOpcode_t Opcode)
{
    _SlBasicCmdMsg_u       Msg;
    _SlCmdCtrl_t           CmdCtrl;

    _SlDrvMemZero(&Msg, (_u16)sizeof(_SlBasicCmdMsg_u));
    CmdCtrl.Opcode = Opcode;
    CmdCtrl.TxDescLen = 0;
    CmdCtrl.RxDescLen = (_SlArgSize_t)sizeof(_BasicResponse_t);


    VERIFY_RET_OK(_SlDrvCmdOp((_SlCmdCtrl_t *)&CmdCtrl, &Msg, NULL));

    return (_SlReturnVal_t)Msg.Rsp.status;
}

/*****************************************************************************
  _SlDrvCmdSend_noLock 
  Send SL command without waiting for command response 
  This function is unprotected and the caller should make 
  sure global lock is active. Used to send data within async event handler, where the driver is already locked.
*****************************************************************************/
_SlReturnVal_t _SlDrvCmdSend_noLock(
    _SlCmdCtrl_t  *pCmdCtrl ,
    void          *pTxRxDescBuff ,
    _SlCmdExt_t   *pCmdExt)
{
    _SlReturnVal_t RetVal;
    _u8            WaitForCmdRespOriginalVal;

    _SlFunctionParams_t originalFuncParms;

    /* save the current RespWait flag before clearing it */
    WaitForCmdRespOriginalVal = g_pCB->WaitForCmdResp;

    /* save the current command paramaters */
    sl_Memcpy(&originalFuncParms,  &g_pCB->FunctionParams, sizeof(_SlFunctionParams_t));

    g_pCB->WaitForCmdResp = FALSE;
  
    SL_TRACE0(DBG_MSG, MSG_312, "_SlDrvCmdSend_noLock: call _SlDrvMsgWrite");

    /* send the message */
    RetVal = _SlDrvMsgWrite(pCmdCtrl, pCmdExt, pTxRxDescBuff);

    /* restore the original RespWait flag */
    g_pCB->WaitForCmdResp = WaitForCmdRespOriginalVal;

    /* restore the original command paramaters  */
    sl_Memcpy(&g_pCB->FunctionParams, &originalFuncParms, sizeof(_SlFunctionParams_t));

    return RetVal;
}
/*****************************************************************************
  _SlDrvCmdSend_noWait
  Send SL command without waiting for command response
  This function send command form any possiable context, without waiting.
*****************************************************************************/
_SlReturnVal_t _SlDrvCmdSend_noWait(
    _SlCmdCtrl_t  *pCmdCtrl ,
    void          *pTxRxDescBuff ,
    _SlCmdExt_t   *pCmdExt)
{
    _SlReturnVal_t RetVal;

    _SlFunctionParams_t originalFuncParms;

    SL_TRACE1(DBG_MSG, MSG_312, "\n\r_SlDrvCmdSend_noLock: call _SlDrvMsgWrite: %x\n\r", pCmdCtrl->Opcode);

    /* Save the current function parameters */
    sl_Memcpy(&originalFuncParms,  &g_pCB->FunctionParams, sizeof(_SlFunctionParams_t));

    /* send the message */
    RetVal = _SlDrvMsgWrite(pCmdCtrl, pCmdExt, pTxRxDescBuff);

    /* Restore */
    sl_Memcpy(&g_pCB->FunctionParams, &originalFuncParms, sizeof(_SlFunctionParams_t));

    return RetVal;
}

/*****************************************************************************
  _SlDrvCmdSend
  Send SL command without waiting for command response 
*****************************************************************************/
_SlReturnVal_t _SlDrvCmdSend(
    _SlCmdCtrl_t  *pCmdCtrl ,
    void          *pTxRxDescBuff ,
    _SlCmdExt_t   *pCmdExt)
{
    _SlReturnVal_t RetVal;

    _SlDrvObjLockWaitForever(&GlobalLockObj);

    g_pCB->WaitForCmdResp = FALSE;
  
    SL_TRACE1(DBG_MSG, MSG_312, "_SlDrvCmdSend: call _SlDrvMsgWrite:%x", pCmdCtrl->Opcode);

    /* send the message */
    RetVal = _SlDrvMsgWrite(pCmdCtrl, pCmdExt, pTxRxDescBuff);

    _SlDrvObjUnLock(&GlobalLockObj);

    return RetVal;
}


/* ***************************************************************************** */
/*  _SlDrvProtectAsyncRespSetting */
/* ***************************************************************************** */
_SlReturnVal_t _SlDrvProtectAsyncRespSetting(_u8 *pAsyncRsp, _SlActionID_e ActionID, _u8 SocketID)
{
    _i16 ObjIdx;

    /* Use Obj to issue the command, if not available try later */
    ObjIdx = _SlDrvWaitForPoolObj(ActionID, SocketID);

    if (SL_RET_CODE_STOP_IN_PROGRESS == ObjIdx)
    {
        return SL_RET_CODE_STOP_IN_PROGRESS;
    }
    else if (MAX_CONCURRENT_ACTIONS == ObjIdx)
    {
        return MAX_CONCURRENT_ACTIONS;
    }
    else
    {
        SL_DRV_PROTECTION_OBJ_LOCK_FOREVER();
        g_pCB->ObjPool[ObjIdx].pRespArgs = pAsyncRsp;
        SL_DRV_PROTECTION_OBJ_UNLOCK();
    }

    return ObjIdx;
}


/* ***************************************************************************** */
/*  _SlDrvIsSpawnOwnGlobalLock */
/* ***************************************************************************** */
#if (!defined (SL_PLATFORM_EXTERNAL_SPAWN))
_u8 _SlDrvIsSpawnOwnGlobalLock()
{
#ifdef SL_PLATFORM_MULTI_THREADED
    _u32 ThreadId = (_i32)pthread_self();
    return _SlInternalIsItSpawnThread(ThreadId);
#else
    return (gGlobalLockContextOwner == GLOBAL_LOCK_CONTEXT_OWNER_SPAWN);
#endif
}
#endif
/* ***************************************************************************** */
/*  _SlDrvWaitForPoolObj */
/* ***************************************************************************** */
_SlReturnVal_t _SlDrvWaitForPoolObj(_u8 ActionID, _u8 SocketID)
{
    _u8 CurrObjIndex = MAX_CONCURRENT_ACTIONS;

    /* Get free object  */
    SL_DRV_PROTECTION_OBJ_LOCK_FOREVER();
    
    if (MAX_CONCURRENT_ACTIONS > g_pCB->FreePoolIdx)
    {
        /* save the current obj index */
        CurrObjIndex = g_pCB->FreePoolIdx;
        /* set the new free index */
        if (MAX_CONCURRENT_ACTIONS > g_pCB->ObjPool[CurrObjIndex].NextIndex)
        {
            g_pCB->FreePoolIdx = g_pCB->ObjPool[CurrObjIndex].NextIndex;
        }
        else
        {
            /* No further free actions available */
            g_pCB->FreePoolIdx = MAX_CONCURRENT_ACTIONS;
        }
    }
    else
    {
        SL_DRV_PROTECTION_OBJ_UNLOCK();
        return CurrObjIndex;
    }
    g_pCB->ObjPool[CurrObjIndex].ActionID = (_u8)ActionID;
    if (SL_MAX_SOCKETS > SocketID)
    {
        g_pCB->ObjPool[CurrObjIndex].AdditionalData = SocketID;
    }
    /*In case this action is socket related, SocketID bit will be on
    In case SocketID is set to SL_MAX_SOCKETS, the socket is not relevant to the action. In that case ActionID bit will be on */
    while ( ( (SL_MAX_SOCKETS > SocketID) && (g_pCB->ActiveActionsBitmap & (1<<SocketID)) ) || 
            ( (g_pCB->ActiveActionsBitmap & ( MULTI_SELECT_MASK & (1<<ActionID))) && (SL_MAX_SOCKETS == SocketID) ) )
    {
        /* If we are in spawn context, this is an API which was called from event handler,
        return error since there is no option to block the spawn context on the pending sync obj */
#if (defined (SL_PLATFORM_MULTI_THREADED)) && (!defined (SL_PLATFORM_EXTERNAL_SPAWN))
        if (_SlDrvIsSpawnOwnGlobalLock())
        {
            g_pCB->ObjPool[CurrObjIndex].ActionID = 0;
            g_pCB->ObjPool[CurrObjIndex].AdditionalData = SL_MAX_SOCKETS;
            g_pCB->FreePoolIdx = CurrObjIndex;
            SL_DRV_PROTECTION_OBJ_UNLOCK();
            return MAX_CONCURRENT_ACTIONS;
        }
#endif
        /* action in progress - move to pending list */
        g_pCB->ObjPool[CurrObjIndex].NextIndex = g_pCB->PendingPoolIdx;
        g_pCB->PendingPoolIdx = CurrObjIndex;
        SL_DRV_PROTECTION_OBJ_UNLOCK();
        
        /* wait for action to be free */
        (void)_SlDrvSyncObjWaitForever(&g_pCB->ObjPool[CurrObjIndex].SyncObj);
        if (SL_IS_DEVICE_STOP_IN_PROGRESS)
        {
            OSI_RET_OK_CHECK(sl_SyncObjDelete(&g_pCB->ObjPool[CurrObjIndex].SyncObj));
            SL_DRV_PROTECTION_OBJ_LOCK_FOREVER();
            g_pCB->NumOfDeletedSyncObj++;
            SL_DRV_PROTECTION_OBJ_UNLOCK();
            return SL_RET_CODE_STOP_IN_PROGRESS;
        }

        /* set params and move to active (remove from pending list at _SlDrvReleasePoolObj) */
        SL_DRV_PROTECTION_OBJ_LOCK_FOREVER();
    }

    /* mark as active. Set socket as active if action is on socket, otherwise mark action as active */
    if (SL_MAX_SOCKETS > SocketID)
    {
        g_pCB->ActiveActionsBitmap |= (1<<SocketID);
    }
    else
    {
        g_pCB->ActiveActionsBitmap |= (1<<ActionID);
    }
    /* move to active list  */
    g_pCB->ObjPool[CurrObjIndex].NextIndex = g_pCB->ActivePoolIdx;
    g_pCB->ActivePoolIdx = CurrObjIndex;    
    
    /* unlock */
    SL_DRV_PROTECTION_OBJ_UNLOCK();
    
    /* Increment the API in progress counter as this routine is called for every 
       API, which will be waiting for async event to be released */
    _SlDrvUpdateApiInProgress(API_IN_PROGRESS_UPDATE_INCREMENT);

    return CurrObjIndex;
}

/* ******************************************************************************/
/*  _SlDrvReleasePoolObj */
/* ******************************************************************************/
_SlReturnVal_t _SlDrvReleasePoolObj(_u8 ObjIdx)
{
    _u8 PendingIndex;

    /* Delete sync obj in case stop in progress and return */
    if (SL_IS_DEVICE_STOP_IN_PROGRESS)
    {
        OSI_RET_OK_CHECK(sl_SyncObjDelete(&g_pCB->ObjPool[ObjIdx].SyncObj));
        SL_DRV_PROTECTION_OBJ_LOCK_FOREVER();
        g_pCB->NumOfDeletedSyncObj++;
        SL_DRV_PROTECTION_OBJ_UNLOCK();
        return SL_RET_CODE_OK;
    }

    SL_DRV_PROTECTION_OBJ_LOCK_FOREVER();

    /* go over the pending list and release other pending action if needed */
    PendingIndex = g_pCB->PendingPoolIdx;
        
    while(MAX_CONCURRENT_ACTIONS > PendingIndex)
    {
        /* In case this action is socket related, SocketID is in use, otherwise will be set to SL_MAX_SOCKETS */
        if ( (g_pCB->ObjPool[PendingIndex].ActionID == g_pCB->ObjPool[ObjIdx].ActionID) && 
            ( (SL_MAX_SOCKETS == (g_pCB->ObjPool[PendingIndex].AdditionalData & SL_BSD_SOCKET_ID_MASK)) || 
            ((SL_MAX_SOCKETS > (g_pCB->ObjPool[ObjIdx].AdditionalData & SL_BSD_SOCKET_ID_MASK)) && ( (g_pCB->ObjPool[PendingIndex].AdditionalData & SL_BSD_SOCKET_ID_MASK) == (g_pCB->ObjPool[ObjIdx].AdditionalData & SL_BSD_SOCKET_ID_MASK) ))) )
        {
            /* remove from pending list */
            _SlDrvRemoveFromList(&g_pCB->PendingPoolIdx, PendingIndex);
             SL_DRV_SYNC_OBJ_SIGNAL(&g_pCB->ObjPool[PendingIndex].SyncObj);
             break;
        }
        PendingIndex = g_pCB->ObjPool[PendingIndex].NextIndex;
    }

    if (SL_MAX_SOCKETS > (g_pCB->ObjPool[ObjIdx].AdditionalData & SL_BSD_SOCKET_ID_MASK))
    {
    /* unset socketID  */
        g_pCB->ActiveActionsBitmap &= ~(1<<(g_pCB->ObjPool[ObjIdx].AdditionalData & SL_BSD_SOCKET_ID_MASK));
    }
    else
    {
    /* unset actionID  */
        g_pCB->ActiveActionsBitmap &= ~(1<<g_pCB->ObjPool[ObjIdx].ActionID);
    }

    /* delete old data */
    g_pCB->ObjPool[ObjIdx].pRespArgs = NULL;
    g_pCB->ObjPool[ObjIdx].ActionID = 0;
    g_pCB->ObjPool[ObjIdx].AdditionalData = SL_MAX_SOCKETS;

    /* remove from active list */
    _SlDrvRemoveFromList(&g_pCB->ActivePoolIdx, ObjIdx);

    /* move to free list */
    g_pCB->ObjPool[ObjIdx].NextIndex = g_pCB->FreePoolIdx;
    g_pCB->FreePoolIdx = ObjIdx;

    SL_DRV_PROTECTION_OBJ_UNLOCK();

    /* Here we decrement the API in progrees counter as we just released the pool object,
       which is held till the API is finished (async event received) */
    _SlDrvUpdateApiInProgress(API_IN_PROGRESS_UPDATE_DECREMENT);
    return SL_RET_CODE_OK;
}

/* ******************************************************************************/
/*  _SlDrvReleaseAllActivePendingPoolObj */
/* ******************************************************************************/
_SlReturnVal_t _SlDrvReleaseAllActivePendingPoolObj()
{     
    _u8 ActiveIndex;

    SL_DRV_PROTECTION_OBJ_LOCK_FOREVER();

    /* go over the active list and release each action with error */
    ActiveIndex = g_pCB->ActivePoolIdx;

    while (MAX_CONCURRENT_ACTIONS > ActiveIndex)
    {
        /* Set error in case sync objects release due to stop device command */
        if (g_pCB->ObjPool[ActiveIndex].ActionID == NETUTIL_CMD_ID)
        {
            ((_SlNetUtilCmdData_t *)(g_pCB->ObjPool[ActiveIndex].pRespArgs))->Status = SL_RET_CODE_STOP_IN_PROGRESS;
        }
        else if (g_pCB->ObjPool[ActiveIndex].ActionID == RECV_ID)
        {
            ((SlSocketResponse_t *)((_SlArgsData_t *)(g_pCB->ObjPool[ActiveIndex].pRespArgs))->pArgs)->StatusOrLen = SL_RET_CODE_STOP_IN_PROGRESS;
        }
        /* First 2 bytes of all async response holds the status except with NETUTIL_CMD_ID and RECV_ID */
        else
        {
            ((SlSocketResponse_t *)(g_pCB->ObjPool[ActiveIndex].pRespArgs))->StatusOrLen = SL_RET_CODE_STOP_IN_PROGRESS;
        }
        /* Signal the pool obj*/
        SL_DRV_SYNC_OBJ_SIGNAL(&g_pCB->ObjPool[ActiveIndex].SyncObj);
        ActiveIndex = g_pCB->ObjPool[ActiveIndex].NextIndex;
    }

    /* go over the pending list and release each action */
    ActiveIndex = g_pCB->PendingPoolIdx;

    while (MAX_CONCURRENT_ACTIONS > ActiveIndex)
    {
        /* Signal the pool obj*/
        SL_DRV_SYNC_OBJ_SIGNAL(&g_pCB->ObjPool[ActiveIndex].SyncObj);
        ActiveIndex = g_pCB->ObjPool[ActiveIndex].NextIndex;
    }

    /* Delete only unoccupied objects from the Free list, other obj (pending and active)
    will be deleted from the relevant context */
    ActiveIndex = g_pCB->FreePoolIdx;
    while(MAX_CONCURRENT_ACTIONS > ActiveIndex)
    {
        OSI_RET_OK_CHECK(sl_SyncObjDelete(&g_pCB->ObjPool[ActiveIndex].SyncObj));
        g_pCB->NumOfDeletedSyncObj++;
        ActiveIndex = g_pCB->ObjPool[ActiveIndex].NextIndex;
    }
    /* In case trigger select in progress, delete the sync obj */
#if defined(slcb_SocketTriggerEventHandler)
    if (MAX_CONCURRENT_ACTIONS != g_pCB->SocketTriggerSelect.Info.ObjPoolIdx)
    {
        OSI_RET_OK_CHECK(sl_SyncObjDelete(&g_pCB->ObjPool[g_pCB->SocketTriggerSelect.Info.ObjPoolIdx].SyncObj));
        g_pCB->NumOfDeletedSyncObj++;
    }
#endif
    SL_DRV_PROTECTION_OBJ_UNLOCK();
    return SL_RET_CODE_OK;
}

/* ******************************************************************************/
/* _SlDrvRemoveFromList  */
/* ******************************************************************************/
static void _SlDrvRemoveFromList(_u8 *ListIndex, _u8 ItemIndex)
{
    _u8 Idx;
        
    if (MAX_CONCURRENT_ACTIONS == g_pCB->ObjPool[*ListIndex].NextIndex)
    {
        *ListIndex = MAX_CONCURRENT_ACTIONS;
    }
    /* need to remove the first item in the list and therefore update the global which holds this index */
    else if (*ListIndex == ItemIndex)
    {
        *ListIndex = g_pCB->ObjPool[ItemIndex].NextIndex;
    }
    else
    {
        Idx = *ListIndex;
      
        while(MAX_CONCURRENT_ACTIONS > Idx)
        {
            /* remove from list */
            if (g_pCB->ObjPool[Idx].NextIndex == ItemIndex)
            {
                g_pCB->ObjPool[Idx].NextIndex = g_pCB->ObjPool[ItemIndex].NextIndex;
                break;
            }

            Idx = g_pCB->ObjPool[Idx].NextIndex;
        }
    }
}


/* ******************************************************************************/
/*  _SlDrvFindAndSetActiveObj                                                     */
/* ******************************************************************************/
static _SlReturnVal_t _SlDrvFindAndSetActiveObj(_SlOpcode_t  Opcode, _u8 Sd)
{
    _u8 ActiveIndex;

    ActiveIndex = g_pCB->ActivePoolIdx;
    /* go over the active list if exist to find obj waiting for this Async event */

    while (MAX_CONCURRENT_ACTIONS > ActiveIndex)
    {
        /* unset the Ipv4\IPv6 bit in the opcode if family bit was set  */
        if (g_pCB->ObjPool[ActiveIndex].AdditionalData & SL_NETAPP_FAMILY_MASK)
        {
            Opcode &= ~SL_OPCODE_IPV6;
        }

        if ((g_pCB->ObjPool[ActiveIndex].ActionID == RECV_ID) && (Sd == g_pCB->ObjPool[ActiveIndex].AdditionalData) && 
                        ( (SL_OPCODE_SOCKET_RECVASYNCRESPONSE == Opcode) || (SL_OPCODE_SOCKET_RECVFROMASYNCRESPONSE == Opcode)
                        || (SL_OPCODE_SOCKET_RECVFROMASYNCRESPONSE_V6 == Opcode))
               )
        {
            g_pCB->FunctionParams.AsyncExt.ActionIndex = ActiveIndex;
            return SL_RET_CODE_OK;
        }
        /* In case this action is socket related, SocketID is in use, otherwise will be set to SL_MAX_SOCKETS */
        if ( (_SlActionLookupTable[ g_pCB->ObjPool[ActiveIndex].ActionID - MAX_SOCKET_ENUM_IDX].ActionAsyncOpcode == Opcode) && 
            ( ((Sd == (g_pCB->ObjPool[ActiveIndex].AdditionalData & SL_BSD_SOCKET_ID_MASK) ) && (SL_MAX_SOCKETS > Sd)) || (SL_MAX_SOCKETS == (g_pCB->ObjPool[ActiveIndex].AdditionalData & SL_BSD_SOCKET_ID_MASK)) ) )
        {
            /* set handler */
            g_pCB->FunctionParams.AsyncExt.AsyncEvtHandler = _SlActionLookupTable[ g_pCB->ObjPool[ActiveIndex].ActionID - MAX_SOCKET_ENUM_IDX].AsyncEventHandler;
            g_pCB->FunctionParams.AsyncExt.ActionIndex = ActiveIndex;
            return SL_RET_CODE_OK;
        }
        ActiveIndex = g_pCB->ObjPool[ActiveIndex].NextIndex;
    }

    return SL_RET_OBJ_NOT_SET;
}

#if defined(slcb_NetAppHttpServerHdlr) || defined(EXT_LIB_REGISTERED_HTTP_SERVER_EVENTS)
void _SlDrvDispatchHttpServerEvents(SlNetAppHttpServerEvent_t *slHttpServerEvent, SlNetAppHttpServerResponse_t *slHttpServerResponse)
{
    _SlDrvHandleHttpServerEvents (slHttpServerEvent, slHttpServerResponse);
}
#endif

#if defined(slcb_NetAppRequestHdlr) || defined(EXT_LIB_REGISTERED_NETAPP_REQUEST_EVENTS)
void _SlDrvDispatchNetAppRequestEvents(SlNetAppRequest_t *slNetAppRequestEvent, SlNetAppResponse_t *slNetAppResponse)
{
    _SlDrvHandleNetAppRequestEvents (slNetAppRequestEvent, slNetAppResponse);
}
#endif


/* Wrappers for the object functions */
_SlReturnVal_t _SlDrvSyncObjSignal(_SlSyncObj_t *pSyncObj)
{
    OSI_RET_OK_CHECK(sl_SyncObjSignal(pSyncObj));
    return SL_OS_RET_CODE_OK;
}

_SlReturnVal_t _SlDrvObjLockWaitForever(_SlLockObj_t *pLockObj)
{
    OSI_RET_OK_CHECK(sl_LockObjLock(pLockObj, SL_OS_WAIT_FOREVER));
    return SL_OS_RET_CODE_OK;
}

_SlReturnVal_t _SlDrvProtectionObjLockWaitForever(void)
{
    OSI_RET_OK_CHECK(sl_LockObjLock(&g_pCB->ProtectionLockObj, SL_OS_WAIT_FOREVER));

    return SL_OS_RET_CODE_OK;
}

_SlReturnVal_t _SlDrvObjUnLock(_SlLockObj_t *pLockObj)
{
    OSI_RET_OK_CHECK(sl_LockObjUnlock(pLockObj));

    return SL_OS_RET_CODE_OK;
}

_SlReturnVal_t _SlDrvProtectionObjUnLock(void)
{
    OSI_RET_OK_CHECK(sl_LockObjUnlock(&g_pCB->ProtectionLockObj));
    return SL_OS_RET_CODE_OK;
}

_SlReturnVal_t _SlDrvObjGlobalLockWaitForever(_u32 Flags)
{
    _SlReturnVal_t ret;
    _u16 Opcode;
    _u16 Silo;
    _u8 UpdateApiInProgress = (Flags & GLOBAL_LOCK_FLAGS_UPDATE_API_IN_PROGRESS);
    _u16 IsProvStopApi = (Flags & GLOBAL_LOCK_FLAGS_PROVISIONING_STOP_API);

    if (SL_IS_RESTART_REQUIRED)
    {
         return SL_API_ABORTED;
    }

    gGlobalLockCntRequested++;

    ret = sl_LockObjLock(&GlobalLockObj, SL_OS_WAIT_FOREVER);

    /*  start/stop device is in progress so return right away */
    if (SL_IS_DEVICE_START_IN_PROGRESS || SL_IS_DEVICE_STOP_IN_PROGRESS || SL_IS_PROVISIONING_IN_PROGRESS)
    {
        return ret;
    }

    /* after the lock acquired check if API is allowed */
    if (0 == ret)
    {
        
        Opcode = (Flags >> 16);
        Silo = Opcode & ((0xF << SL_OPCODE_SILO_OFFSET)); 

        /* After acquiring the lock, check if there is stop in progress */
        if (Opcode != SL_OPCODE_DEVICE_STOP_COMMAND)
        {
            _i16 Status = _SlDrvDriverIsApiAllowed(Silo); 

            if (Status) 
            {
                sl_LockObjUnlock(&GlobalLockObj);
                return Status;
            }
        }
    }
    
    /* if lock was successfully taken and increment of the API in progress is required */
    if ((0 == ret) && (UpdateApiInProgress))
    {
        if (!SL_IS_PROVISIONING_ACTIVE || SL_IS_PROVISIONING_API_ALLOWED)
        {
            /* Increment the API in progress counter */
            _SlDrvUpdateApiInProgress(API_IN_PROGRESS_UPDATE_INCREMENT);
        }
        /* if we are in provisioning than don't abort the stop provisioning cmd.. */
        else if (FALSE == IsProvStopApi )
        {
            /*  Provisioning is active so release the lock immediately as
                we do not want to allow more APIs to run. */
            SL_DRV_LOCK_GLOBAL_UNLOCK(TRUE);
            return SL_RET_CODE_PROVISIONING_IN_PROGRESS;
        }
    }

    return ret;
}
_SlReturnVal_t _SlDrvGlobalObjUnLock(_u8 bDecrementApiInProgress)
{
    gGlobalLockCntReleased++;

    OSI_RET_OK_CHECK(sl_LockObjUnlock(&GlobalLockObj));

    if (bDecrementApiInProgress)
    {
        _SlDrvUpdateApiInProgress(API_IN_PROGRESS_UPDATE_DECREMENT);
    }

    return SL_OS_RET_CODE_OK;
}

void _SlDrvMemZero(void* Addr, _u16 size)
{
    sl_Memset(Addr, 0, size);
}


void _SlDrvResetCmdExt(_SlCmdExt_t* pCmdExt)
{
    _SlDrvMemZero(pCmdExt, (_u16)sizeof (_SlCmdExt_t));
}



_SlReturnVal_t  _SlDrvSyncObjWaitForever(_SlSyncObj_t *pSyncObj)
{    
    _SlReturnVal_t RetVal = sl_SyncObjWait(pSyncObj, SL_OS_WAIT_FOREVER);
    
    /*  if the wait is finished and we detect that restart is required (we in the middle of error handling),
          than we should abort immediately from the current API command execution
    */
    if (SL_IS_RESTART_REQUIRED)
    {
        return SL_API_ABORTED;
    }

    return RetVal;
}


#if (defined(slcb_GetTimestamp))

void _SlDrvStartMeasureTimeout(_SlTimeoutParams_t *pTimeoutInfo, _u32 TimeoutInMsec)
{
    _SlDrvMemZero(pTimeoutInfo, sizeof (_SlTimeoutParams_t));

    pTimeoutInfo->Total10MSecUnits = TimeoutInMsec / 10;
    pTimeoutInfo->TSPrev = slcb_GetTimestamp();
}

_u8 _SlDrvIsTimeoutExpired(_SlTimeoutParams_t *pTimeoutInfo)
{
    _u32 TSCount;

    pTimeoutInfo->TSCurr = slcb_GetTimestamp();

    if (pTimeoutInfo->TSCurr >= pTimeoutInfo->TSPrev)
    {
        pTimeoutInfo->DeltaTicks = pTimeoutInfo->TSCurr - pTimeoutInfo->TSPrev;
    }
    else
    {
        pTimeoutInfo->DeltaTicks =  (SL_TIMESTAMP_MAX_VALUE - pTimeoutInfo->TSPrev) + pTimeoutInfo->TSCurr;
    }

    TSCount = pTimeoutInfo->DeltaTicksReminder + pTimeoutInfo->DeltaTicks;


    if (TSCount > SL_TIMESTAMP_TICKS_IN_10_MILLISECONDS)
    {
        pTimeoutInfo->Total10MSecUnits -= (TSCount / SL_TIMESTAMP_TICKS_IN_10_MILLISECONDS);
        pTimeoutInfo->DeltaTicksReminder = TSCount % SL_TIMESTAMP_TICKS_IN_10_MILLISECONDS;

        if (pTimeoutInfo->Total10MSecUnits > 0)
        {
            pTimeoutInfo->TSPrev =  pTimeoutInfo->TSCurr;
        }
        else
        {
            return TRUE;
        }
    }

    return FALSE;
}

#endif

void _SlDrvHandleFatalError(_u32 errorId, _u32 info1, _u32 info2)
{ 
    _u8 i;
    SlDeviceFatal_t  FatalEvent;

    _SlDrvMemZero(&FatalEvent, sizeof(FatalEvent));

    if (SL_IS_RESTART_REQUIRED)
    {
        return;
    }
        
    /* set the restart flag */
    SL_SET_RESTART_REQUIRED;
    
     /* Upon the deletion of the mutex, all thread waiting on this
     mutex will return immediately with an error (i.e. MUTEX_DELETED status) */
     (void)sl_LockObjDelete(&GlobalLockObj);
     
     /* Mark the global lock as deleted */
     SL_UNSET_GLOBAL_LOCK_INIT;

    /* signal all waiting sync objects */
    for (i=0; i< MAX_CONCURRENT_ACTIONS; i++)
    {
        SL_DRV_SYNC_OBJ_SIGNAL(&g_pCB->ObjPool[i].SyncObj);
    }

    /* prepare the event and notify the user app/ext libraries */
    FatalEvent.Id = errorId;

    switch (errorId)
    {
        case SL_DEVICE_EVENT_FATAL_DEVICE_ABORT:
        {
            /* set the Abort Type */
            FatalEvent.Data.DeviceAssert.Code = info1;   
            
            /* set the Abort Data */
            FatalEvent.Data.DeviceAssert.Value = info2;  
        }
        break;
        
        case SL_DEVICE_EVENT_FATAL_NO_CMD_ACK:
        {
            /* set the command opcode */
            FatalEvent.Data.NoCmdAck.Code = info1; 
        }
        break;

        case SL_DEVICE_EVENT_FATAL_CMD_TIMEOUT:
        {
            /* set the expected async event opcode */
            FatalEvent.Data.CmdTimeout.Code = info1; 
        }
        break;

        case SL_DEVICE_EVENT_FATAL_SYNC_LOSS:
        case SL_DEVICE_EVENT_FATAL_DRIVER_ABORT:
            /* No Info to transport */
            break;

    }

#if defined(slcb_DeviceFatalErrorEvtHdlr) || defined (EXT_LIB_REGISTERED_FATAL_ERROR_EVENTS)
    /* call the registered fatal error handlers */
    _SlDrvHandleFatalErrorEvents(&FatalEvent);
#endif
}

_SlReturnVal_t  _SlDrvSyncObjWaitTimeout(_SlSyncObj_t *pSyncObj, _u32 timeoutVal, _u32 asyncEventOpcode)
{
    _SlReturnVal_t ret = sl_SyncObjWait(pSyncObj, timeoutVal);

    /* if timeout occured...*/
    if (ret)
    {
        _SlDrvHandleFatalError(SL_DEVICE_EVENT_FATAL_CMD_TIMEOUT, asyncEventOpcode, timeoutVal);
        return SL_API_ABORTED;
    }
    else if (SL_IS_RESTART_REQUIRED)
    {
        return SL_API_ABORTED;
    }

    return SL_RET_CODE_OK;
}


static void _SlDrvUpdateApiInProgress(_i8 Value)
{
    SL_DRV_PROTECTION_OBJ_LOCK_FOREVER();
    
    g_pCB->ApiInProgressCnt  += Value;

    SL_DRV_PROTECTION_OBJ_UNLOCK();
}

_i8 _SlDrvIsApiInProgress(void)
{
    if (g_pCB != NULL)
    {
        return (g_pCB->ApiInProgressCnt > 0);
    }
    
    return TRUE;
}


#ifdef slcb_GetTimestamp

void _SlDrvSleep(_u16 DurationInMsec)
{
    _SlTimeoutParams_t      TimeoutInfo={0};

    _SlDrvStartMeasureTimeout(&TimeoutInfo, DurationInMsec);

    while(!_SlDrvIsTimeoutExpired(&TimeoutInfo));
}
#endif

#ifndef SL_PLATFORM_MULTI_THREADED
void _SlDrvSetGlobalLockOwner(_u8 Owner)
{
    gGlobalLockContextOwner = Owner;
}
#endif


_SlReturnVal_t _SlDrvWaitForInternalAsyncEvent(_u8 ObjIdx , _u32 Timeout, _SlOpcode_t Opcode)
{

#if (defined(SL_PLATFORM_EXTERNAL_SPAWN) || !defined(SL_PLATFORM_MULTI_THREADED))
    SL_DRV_SYNC_OBJ_WAIT_FOREVER(&g_pCB->ObjPool[ObjIdx].SyncObj);
    return SL_OS_RET_CODE_OK;
#else
    _SlTimeoutParams_t SlTimeoutInfo = { 0 };
    if (_SlDrvIsSpawnOwnGlobalLock())
    {
#if (defined(slcb_GetTimestamp))
        _SlDrvStartMeasureTimeout(&SlTimeoutInfo, Timeout);
        while (!Timeout || !_SlDrvIsTimeoutExpired(&SlTimeoutInfo))
#endif
        {
            /* If we are in spawn context, this is an API which was called from event handler,
            read any async event and check if we got signaled */
            _SlInternalSpawnWaitForEvent();
            /* is it mine? */
            if (0 == sl_SyncObjWait(&g_pCB->ObjPool[ObjIdx].SyncObj, SL_OS_NO_WAIT))
            {
                return SL_OS_RET_CODE_OK;
            }
        }
        /* if timeout occured...*/
        _SlDrvHandleFatalError(SL_DEVICE_EVENT_FATAL_CMD_TIMEOUT, Opcode, Timeout);
        return SL_API_ABORTED;
    }
    else
    {
        if (Timeout)
        {
            SL_DRV_SYNC_OBJ_WAIT_TIMEOUT(&g_pCB->ObjPool[ObjIdx].SyncObj, Timeout, Opcode);
        }
        else
        {
            SL_DRV_SYNC_OBJ_WAIT_FOREVER(&g_pCB->ObjPool[ObjIdx].SyncObj);
        }
        return SL_OS_RET_CODE_OK;
    }
    
#endif
}

#if (defined(SL_PLATFORM_MULTI_THREADED) && !defined(SL_MEMORY_MGMT_DYNAMIC))
_SlAsyncRespBuf_t* _SlGetStatSpawnListItem(_u16 AsyncEventLen)
{
    _u8 Idx = 0;
    /* Find free buffer from the pool */
    while (Idx < SL_MAX_ASYNC_BUFFERS)
    {
        if (0xFF == g_StatMem.AsyncBufPool[Idx].ActionIndex)
        {
            /* copy buffer */
            return &g_StatMem.AsyncBufPool[Idx];
        }
        Idx++;
    }
    return NULL;
}
#endif

#if defined(SL_PLATFORM_MULTI_THREADED)
_SlReturnVal_t _SlSpawnMsgListInsert(_u16 AsyncEventLen, _u8 *pAsyncBuf)
{
    _SlReturnVal_t RetVal = SL_OS_RET_CODE_OK;

    /* protect the item insertion */
    SL_DRV_PROTECTION_OBJ_LOCK_FOREVER();

#ifdef SL_MEMORY_MGMT_DYNAMIC
    _SlSpawnMsgItem_t* pCurr = NULL;
    _SlSpawnMsgItem_t* pItem;

    pItem = (_SlSpawnMsgItem_t*)sl_Malloc(sizeof(_SlSpawnMsgItem_t));
    /* now allocate the buffer itself */
    pItem->Buffer = (void*)sl_Malloc(AsyncEventLen);
    pItem->next = NULL;
    /* if list is empty point to the allocated one */
    if (g_pCB->spawnMsgList == NULL)
    {
        g_pCB->spawnMsgList = pItem;
    }
    else
    {
        pCurr = g_pCB->spawnMsgList;
        /* go to end of list */
        while (pCurr->next != NULL)
        {
            pCurr = pCurr->next;
        }
        /* we point to last item in list - add the new one  */
        pCurr->next = pItem;
    }
#else
    _SlAsyncRespBuf_t* pItem = (_SlAsyncRespBuf_t*)_SlGetStatSpawnListItem(AsyncEventLen);
#endif
    if ((NULL != pItem) && (NULL != pItem->Buffer))
    {
        /* save the action idx */
        pItem->ActionIndex = g_pCB->FunctionParams.AsyncExt.ActionIndex;
        /* save the corresponding AsyncHndlr (if registered) */
        pItem->AsyncHndlr = g_pCB->FunctionParams.AsyncExt.AsyncEvtHandler;
        /* copy the async event that we read to the buffer */
        sl_Memcpy(pItem->Buffer, pAsyncBuf, AsyncEventLen);
    }
    else
    {
            RetVal = SL_RET_CODE_NO_FREE_ASYNC_BUFFERS_ERROR;
    }
        SL_DRV_PROTECTION_OBJ_UNLOCK();
        return RetVal;
}

_SlReturnVal_t _SlSpawnMsgListProcess()
{
    
#ifdef SL_MEMORY_MGMT_DYNAMIC
    _SlSpawnMsgItem_t* pHead = g_pCB->spawnMsgList;
    _SlSpawnMsgItem_t* pCurr = pHead;
    _SlSpawnMsgItem_t* pLast = pHead;

    while (pCurr != NULL)
    {
        /* lock during action */
        SL_DRV_LOCK_GLOBAL_LOCK_FOREVER(GLOBAL_LOCK_FLAGS_NONE);
        /* load the async event params */
        g_pCB->FunctionParams.AsyncExt.ActionIndex = pCurr->ActionIndex;
        g_pCB->FunctionParams.AsyncExt.AsyncEvtHandler = pCurr->AsyncHndlr;

        pLast = pCurr;
        pCurr = pCurr->next;

        /* move the list head to point to the next item (or null) */
        g_pCB->spawnMsgList = pCurr;
        /* Handle async event: here we are in spawn context, after context
        * switch from command context. */
        _SlDrvAsyncEventGenericHandler(FALSE, pLast->Buffer);

        /* free the copied buffer inside the item  */
        sl_Free(pLast->Buffer);

        /* free the spawn msg item */
        sl_Free(pLast);

        SL_DRV_LOCK_GLOBAL_UNLOCK(FALSE);
    }

#else
    _u8 i;

    for (i = 0; i < SL_MAX_ASYNC_BUFFERS; i++)
    {
        if (0xFF != g_StatMem.AsyncBufPool[i].ActionIndex)
        {
            /* lock during action */
        
            SL_DRV_LOCK_GLOBAL_LOCK_FOREVER(GLOBAL_LOCK_FLAGS_NONE);

            /* load the async event params */
            g_pCB->FunctionParams.AsyncExt.ActionIndex = g_StatMem.AsyncBufPool[i].ActionIndex;
            g_pCB->FunctionParams.AsyncExt.AsyncEvtHandler = g_StatMem.AsyncBufPool[i].AsyncHndlr;

            /* Handle async event: here we are in spawn context, after context
            * switch from command context. */
            _SlDrvAsyncEventGenericHandler(FALSE, (unsigned char *)&(g_StatMem.AsyncBufPool[i].Buffer));

            SL_DRV_LOCK_GLOBAL_UNLOCK(FALSE);
	    
            SL_DRV_PROTECTION_OBJ_LOCK_FOREVER();
            g_StatMem.AsyncBufPool[i].ActionIndex = 0xFF;
            SL_DRV_PROTECTION_OBJ_UNLOCK();
        }
    }
#endif
    return SL_OS_RET_CODE_OK;
}

_u16 _SlSpawnMsgListGetCount()
{
    _u16 NumOfItems = 0;
#ifdef SL_MEMORY_MGMT_DYNAMIC
    _SlSpawnMsgItem_t* pCurr = g_pCB->spawnMsgList;

    /* protect the item insertion  */
    SL_DRV_PROTECTION_OBJ_LOCK_FOREVER();
    while (pCurr != NULL)
    {
        NumOfItems++;

        pCurr = pCurr->next;
    }
    SL_DRV_PROTECTION_OBJ_UNLOCK();

#else
    _u8 i;
    /* protect counting parameters  */
    SL_DRV_PROTECTION_OBJ_LOCK_FOREVER();

    for (i = 0; i < SL_MAX_ASYNC_BUFFERS; i++)
    {
        if (0xFF != g_StatMem.AsyncBufPool[i].ActionIndex)
        {
            NumOfItems++;
        }
    }
    SL_DRV_PROTECTION_OBJ_UNLOCK();
#endif
    return NumOfItems;
}


void _SlFindAndReleasePendingCmd()
{
    /* In case there is no free buffer to store the async event until context switch release the command and return specific error */
    ((SlSocketResponse_t *)(g_pCB->ObjPool[g_pCB->FunctionParams.AsyncExt.ActionIndex].pRespArgs))->StatusOrLen = SL_RET_CODE_NO_FREE_ASYNC_BUFFERS_ERROR;
    /* signal pending cmd */
    SL_DRV_SYNC_OBJ_SIGNAL(&g_pCB->ObjPool[g_pCB->FunctionParams.AsyncExt.ActionIndex].SyncObj);
}
#endif
