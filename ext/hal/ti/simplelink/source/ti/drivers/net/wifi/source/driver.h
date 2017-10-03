/*
 * driver.h - CC31xx/CC32xx Host Driver Implementation
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
 
#ifndef __DRIVER_INT_H__
#define __DRIVER_INT_H__

#ifdef    __cplusplus
extern "C" {
#endif

#include <ti/drivers/net/wifi/source/protocol.h>

#define TIMEOUT_STATE_EXPIRY      (0)
#define TIMEOUT_ONE_MORE_SHOT     (1)
#define TIMEOUT_STATE_INIT_VAL    (2)

/* Timeouts for the sync objects  */
#ifndef SL_DRIVER_TIMEOUT_SHORT
#define SL_DRIVER_TIMEOUT_SHORT        (10000) /* msec units */
#endif
#ifndef SL_DRIVER_TIMEOUT_LONG
#define SL_DRIVER_TIMEOUT_LONG         (65535) /* msec units */
#endif

#define INIT_COMPLETE_TIMEOUT          SL_DRIVER_TIMEOUT_LONG
#define STOP_DEVICE_TIMEOUT            SL_DRIVER_TIMEOUT_LONG

#ifndef SYNC_PATTERN_TIMEOUT_IN_MSEC
#define SYNC_PATTERN_TIMEOUT_IN_MSEC   (50) /* the sync patttern timeout in milliseconds units */
#endif
/*****************************************************************************/
/* Macro declarations                                                        */
/*****************************************************************************/

#ifndef CPU_FREQ_IN_MHZ
 #define CPU_FREQ_IN_MHZ        (200)
#endif
#define USEC_DELAY              (50)

#define SL_DRV_PROTECTION_OBJ_UNLOCK()       (void)_SlDrvProtectionObjUnLock();
#define SL_DRV_PROTECTION_OBJ_LOCK_FOREVER() (void)_SlDrvProtectionObjLockWaitForever();
#define SL_DRV_OBJ_UNLOCK(pObj)              (void)_SlDrvObjUnLock(pObj);
#define SL_DRV_OBJ_LOCK_FOREVER(pObj)        (void)_SlDrvObjLockWaitForever(pObj);
#define SL_DRV_SYNC_OBJ_SIGNAL(pObj)         (void)_SlDrvSyncObjSignal(pObj);
#define SL_DRV_SYNC_OBJ_CLEAR(pObj)          (void)sl_SyncObjWait(pObj,SL_OS_NO_WAIT);


#ifdef SL_TINY
#define SL_DRV_SYNC_OBJ_WAIT_FOREVER(SyncObj)						(void)_SlDrvSyncObjWaitForever(SyncObj);
#define SL_DRV_LOCK_GLOBAL_LOCK_FOREVER(Flags)	                    (void)_SlDrvObjGlobalLockWaitForever(Flags);
#define SL_DRV_LOCK_GLOBAL_UNLOCK(bDecrementApiInProgress)			(void)_SlDrvGlobalObjUnLock(bDecrementApiInProgress);
#else
#define SL_DRV_SYNC_OBJ_WAIT_FOREVER(SyncObj) { \
if (SL_API_ABORTED == _SlDrvSyncObjWaitForever(SyncObj)) \
{ \
	return SL_API_ABORTED; \
} \
}
#define SL_DRV_SYNC_OBJ_WAIT_TIMEOUT(SyncObj, timeoutVal, opcode) { \
if (SL_API_ABORTED == _SlDrvSyncObjWaitTimeout(SyncObj, timeoutVal, opcode)) \
{ \
	return SL_API_ABORTED; \
} \
}
#define SL_DRV_LOCK_GLOBAL_LOCK_FOREVER(Flags) { \
_SlReturnVal_t retVal;                     \
                                           \
retVal = _SlDrvObjGlobalLockWaitForever(Flags); \
if (retVal)         \
{                   \
	return retVal;  \
}                   \
}				   

#define SL_DRV_LOCK_GLOBAL_UNLOCK(bDecrementApiInProgress) { \
_SlReturnVal_t retVal;                \
                                      \
retVal = _SlDrvGlobalObjUnLock(bDecrementApiInProgress);    \
if (retVal)         \
{                   \
    return retVal;  \
}                   \
}
#endif

#define SL_IS_RESTART_REQUIRED              (g_SlDeviceStatus & _SL_DRV_STATUS_BIT_RESTART_REQUIRED) /* bit 8 indicates restart is required due to fatal error */
#define SL_IS_DEVICE_STARTED              (g_SlDeviceStatus & _SL_DRV_STATUS_BIT_DEVICE_STARTED) /* bit 9 indicates device is started */
#define SL_IS_DEVICE_LOCKED               (g_SlDeviceStatus & _SL_DEV_STATUS_BIT_LOCKED) /* bits 0-7 devStatus from NWP, bit 2 = device locked */
#define SL_IS_PROVISIONING_ACTIVE			 (!!(g_SlDeviceStatus & _SL_DEV_STATUS_BIT_PROVISIONING_ACTIVE)) /* bits 0-7 devStatus from NWP, bit 3 = provisioning active */
#define SL_IS_PROVISIONING_INITIATED_BY_USER (!!(g_SlDeviceStatus & _SL_DEV_STATUS_BIT_PROVISIONING_USER_INITIATED)) /* bits 0-7 devStatus from NWP, bit 4 = provisioning initiated by the user */
#define SL_IS_PROVISIONING_API_ALLOWED       (!!(g_SlDeviceStatus & _SL_DEV_STATUS_BIT_PROVISIONING_ENABLE_API))
#define SL_IS_DEVICE_STOP_IN_PROGRESS		 (!!(g_SlDeviceStatus & _SL_DRV_STATUS_BIT_STOP_IN_PROGRESS))
#define SL_IS_DEVICE_START_IN_PROGRESS		 (!!(g_SlDeviceStatus & _SL_DRV_STATUS_BIT_START_IN_PROGRESS))

#define SL_IS_PROVISIONING_IN_PROGRESS	     (!!(g_SlDeviceStatus & ( _SL_DEV_STATUS_BIT_PROVISIONING_USER_INITIATED | _SL_DEV_STATUS_BIT_PROVISIONING_ACTIVE)))
/* Check the following conditions:
	1. Device started
	2. Restart device is not required
	3. Provisioning is active
	4. Provisioning was already initiated by the user 
	5. Device is not locked
*/
#define SL_IS_COMMAND_ALLOWED				 ((g_SlDeviceStatus & (_SL_DRV_STATUS_BIT_DEVICE_STARTED |              \
																   _SL_DRV_STATUS_BIT_RESTART_REQUIRED |            \
																   _SL_DEV_STATUS_BIT_PROVISIONING_ACTIVE |         \
																   _SL_DEV_STATUS_BIT_PROVISIONING_USER_INITIATED | \
																   _SL_DRV_STATUS_BIT_STOP_IN_PROGRESS            | \
																   _SL_DEV_STATUS_BIT_LOCKED)) == 0x200) 

#define SL_SET_RESTART_REQUIRED          (g_SlDeviceStatus |= _SL_DRV_STATUS_BIT_RESTART_REQUIRED) /* bit 8 indicates restart is required due to fatal error */
#define SL_UNSET_RESTART_REQUIRED        (g_SlDeviceStatus &= (~_SL_DRV_STATUS_BIT_RESTART_REQUIRED)) /* bit 8 indicates restart is required due to fatal error */
#define SL_SET_DEVICE_STARTED            (g_SlDeviceStatus |= _SL_DRV_STATUS_BIT_DEVICE_STARTED) /* bit 9 indicates device is started */
#define SL_UNSET_DEVICE_STARTED          (g_SlDeviceStatus &= (~_SL_DRV_STATUS_BIT_DEVICE_STARTED)) /* bit 9 indicates device is started */

#define SL_SET_DEVICE_STOP_IN_PROGRESS    (g_SlDeviceStatus |= _SL_DRV_STATUS_BIT_STOP_IN_PROGRESS)    /* bit 10 indicates there is stop in progress */
#define SL_UNSET_DEVICE_STOP_IN_PROGRESS  (g_SlDeviceStatus &= (~_SL_DRV_STATUS_BIT_STOP_IN_PROGRESS)) /* bit 10 indicates there is stop in progress */

/* Start in progress */
#define SL_SET_DEVICE_START_IN_PROGRESS    (g_SlDeviceStatus |= _SL_DRV_STATUS_BIT_START_IN_PROGRESS)    /* bit 11 indicates there is start in progress */
#define SL_UNSET_DEVICE_START_IN_PROGRESS  (g_SlDeviceStatus &= (~_SL_DRV_STATUS_BIT_START_IN_PROGRESS)) /* bit 11 indicates there is start in progress */


#define SL_SET_DEVICE_STATUS(x)          (g_SlDeviceStatus = ((g_SlDeviceStatus & 0xFF00) |  (_u16)x) ) /* bits 0-7 devStatus from NWP */

#define _SL_PENDING_RX_MSG(pDriverCB)   (RxIrqCnt != (pDriverCB)->RxDoneCnt)

/*****************************************************************************/
/* Structure/Enum declarations                                               */
/*****************************************************************************/

typedef struct
{
	_u32  TSPrev;
	_u32  TSCurr;
	_u32  DeltaTicks;
	_u32  DeltaTicksReminder;
	_i32  Total10MSecUnits;
} _SlTimeoutParams_t;

typedef struct
{
	_u8 *pAsyncMsgBuff;
	_u8  bInCmdContext;
} DeviceEventInfo_t;

typedef struct
{
    _SlOpcode_t      Opcode;
    _SlArgSize_t     TxDescLen;
    _SlArgSize_t     RxDescLen;
}_SlCmdCtrl_t;

typedef struct
{
    _u16  TxPayload1Len;
	_u16  TxPayload2Len;
    _i16  RxPayloadLen;
	_i16  ActualRxPayloadLen;
    _u8   *pTxPayload1;
	_u8   *pTxPayload2;
    _u8   *pRxPayload;
}_SlCmdExt_t;

typedef struct _SlArgsData_t
{
    _u8	 *pArgs;
	_u8    *pData;
} _SlArgsData_t;

typedef struct _SlPoolObj_t
{
    _SlSyncObj_t	  SyncObj;
	 _u8              *pRespArgs;
	_u8			      ActionID; 
	_u8			      AdditionalData; /* use for socketID and one bit which indicate supprt IPV6 or not (1=support, 0 otherwise) */
    _u8				  NextIndex;  

} _SlPoolObj_t;

typedef enum
{
	SOCKET_0,
	SOCKET_1,
	SOCKET_2,
	SOCKET_3,
	SOCKET_4,
	SOCKET_5,
	SOCKET_6,
	SOCKET_7,
	SOCKET_8,
	SOCKET_9,
	SOCKET_10,
	SOCKET_11,
	SOCKET_12,
	SOCKET_13,
	SOCKET_14,
	SOCKET_15,
	MAX_SOCKET_ENUM_IDX,
#ifndef SL_TINY
    ACCEPT_ID = MAX_SOCKET_ENUM_IDX,
    CONNECT_ID,
#else
    CONNECT_ID = MAX_SOCKET_ENUM_IDX,
#endif
#ifndef SL_TINY    
	SELECT_ID,
#endif
	GETHOSYBYNAME_ID,
#ifndef SL_TINY    
	GETHOSYBYSERVICE_ID,
	PING_ID,
    NETAPP_RECEIVE_ID,
#endif	
    START_STOP_ID,
	NETUTIL_CMD_ID,
	CLOSE_ID,
	/**********/
	RECV_ID /* Please note!! this member must be the last in this action enum */
}_SlActionID_e;

typedef struct _SlActionLookup_t
{
    _u8                     ActionID;
    _u16                    ActionAsyncOpcode;
    _SlSpawnEntryFunc_t     AsyncEventHandler; 

} _SlActionLookup_t;

typedef struct
{
    _u8             TxPoolCnt;
    _u16            MinTxPayloadSize;
    _SlLockObj_t    TxLockObj;
    _SlSyncObj_t    TxSyncObj;
}_SlFlowContCB_t;

typedef enum
{
    RECV_RESP_CLASS,
    CMD_RESP_CLASS,
    ASYNC_EVT_CLASS,
#if (defined(SL_PLATFORM_MULTI_THREADED) && !defined(slcb_SocketTriggerEventHandler))
    MULTI_SELECT_RESP_CLASS,
#endif
    DUMMY_MSG_CLASS
}_SlRxMsgClass_e;

typedef struct
{
    _u8                     *pAsyncBuf;         /* place to write pointer to buffer with CmdResp's Header + Arguments */
    _u8                     ActionIndex; 
    _SlSpawnEntryFunc_t     AsyncEvtHandler;    /* place to write pointer to AsyncEvent handler (calc-ed by Opcode)   */
    _SlRxMsgClass_e         RxMsgClass;         /* type of Rx message                                                 */
} AsyncExt_t;

typedef _u8 _SlSd_t;

typedef struct
{
	_SlCmdCtrl_t         *pCmdCtrl;
	_u8                  *pTxRxDescBuff;
	_SlCmdExt_t          *pCmdExt;
	AsyncExt_t           AsyncExt;
}_SlFunctionParams_t;

#if (defined(SL_PLATFORM_MULTI_THREADED) && !defined(slcb_SocketTriggerEventHandler))

typedef struct SlSelectEntry_t
{
    SlSelectAsyncResponse_t     Response;
    _u32                        TimeStamp;
    _u16                        readlist;
    _u16                        writelist;
    _u8                         ObjIdx;
}_SlSelectEntry_t;

typedef struct _SlMultiSelectCB_t
{
    _u16                    readsds;
    _u16                    writesds;
    _u16                    CtrlSockFD;
    _u8                     ActiveSelect;
    _u8                     ActiveWaiters;
    _BasicResponse_t        SelectCmdResp;
    _SlSyncObj_t            SelectSyncObj;
    _SlLockObj_t            SelectLockObj;
    _SlSelectEntry_t*       SelectEntry[MAX_CONCURRENT_ACTIONS];
}_SlMultiSelectCB_t;

#else

typedef enum
{
	SOCK_TRIGGER_READY,
	SOCK_TRIGGER_WAITING_FOR_RESP,
	SOCK_TRIGGER_RESP_RECEIVED
} _SlSockTriggerState_e;

typedef struct
{
	_SlSockTriggerState_e State;
	_u8 ObjPoolIdx;
} _SlSockTriggerData_t;

typedef struct
{
	_SlSockTriggerData_t    Info;
	SlSelectAsyncResponse_t Resp;
} _SlSockTriggerSelect_t;

#endif

#ifdef   SL_INC_INTERNAL_ERRNO

#define  SL_DRVER_ERRNO_FLAGS_UNREAD    (1)

typedef struct
{
#ifdef 	SL_PLATFORM_MULTI_THREADED
  _i32 Id;
#endif
  _i32       Errno;
#ifdef 	SL_PLATFORM_MULTI_THREADED
   _u8       Index;
   _u8       Flags;
#endif

}_SlDrvErrno_t;
#endif

typedef struct
{
    _SlFd_t                 FD;
    _SlCommandHeader_t      TempProtocolHeader;
    P_INIT_CALLBACK         pInitCallback;

    _SlPoolObj_t            ObjPool[MAX_CONCURRENT_ACTIONS];
    _u8                     FreePoolIdx;
    _u8                     PendingPoolIdx;
    _u8                     ActivePoolIdx;
    _u32                    ActiveActionsBitmap;
    _SlLockObj_t            ProtectionLockObj;

    _SlSyncObj_t            CmdSyncObj;  
    _u8                     WaitForCmdResp;
    _SlFlowContCB_t         FlowContCB;
    _u8                     TxSeqNum;
    _u8                     RxDoneCnt;
    _u16                    SocketNonBlocking;   
    _u16                    SocketTXFailure;
    /* for stack reduction the parameters are globals */
    _SlFunctionParams_t     FunctionParams;

    _u8                     ActionIndex;
	_i8                     ApiInProgressCnt;  /* Counts how many APIs are in progress */

#if (defined(SL_PLATFORM_MULTI_THREADED) && !defined(slcb_SocketTriggerEventHandler))
	/* Multiple Select Control block */
	_SlMultiSelectCB_t      MultiSelectCB;
#endif

#if defined(slcb_SocketTriggerEventHandler)
	/* Trigger mode control block */
	_SlSockTriggerSelect_t  SocketTriggerSelect;
#endif

#ifdef SL_INC_INTERNAL_ERRNO
    _SlDrvErrno_t Errno[MAX_CONCURRENT_ACTIONS];
#ifdef 	SL_PLATFORM_MULTI_THREADED
    _SlLockObj_t  ErrnoLockObj;
    _u8           ErrnoIndex;
#endif
#endif

}_SlDriverCb_t;

extern _volatile _u8    RxIrqCnt;

extern _SlLockObj_t		GlobalLockObj;
extern _u16				g_SlDeviceStatus;

extern _SlDriverCb_t* g_pCB;
extern P_SL_DEV_PING_CALLBACK  pPingCallBackFunc;

/*****************************************************************************/
/* Function prototypes                                                       */
/*****************************************************************************/
extern _SlReturnVal_t _SlDrvDriverCBInit(void);
extern _SlReturnVal_t _SlDrvDriverCBDeinit(void);
extern _SlReturnVal_t _SlDrvRxIrqHandler(void *pValue);
extern _SlReturnVal_t _SlDrvCmdOp(_SlCmdCtrl_t *pCmdCtrl , void* pTxRxDescBuff , _SlCmdExt_t* pCmdExt);
extern _SlReturnVal_t _SlDrvCmdSend_noLock(_SlCmdCtrl_t *pCmdCtrl , void* pTxRxDescBuff , _SlCmdExt_t* pCmdExt);
extern _SlReturnVal_t _SlDrvCmdSend_noWait(_SlCmdCtrl_t *pCmdCtrl , void* pTxRxDescBuff , _SlCmdExt_t* pCmdExt);
extern _SlReturnVal_t _SlDrvCmdSend(_SlCmdCtrl_t  *pCmdCtrl , void *pTxRxDescBuff , _SlCmdExt_t *pCmdExt);
extern _SlReturnVal_t _SlDrvDataReadOp(_SlSd_t Sd, _SlCmdCtrl_t *pCmdCtrl , void* pTxRxDescBuff , _SlCmdExt_t* pCmdExt);
extern _SlReturnVal_t _SlDrvDataWriteOp(_SlSd_t Sd, _SlCmdCtrl_t *pCmdCtrl , void* pTxRxDescBuff , _SlCmdExt_t* pCmdExt);
extern _SlReturnVal_t _SlDeviceHandleAsync_InitComplete(void *pVoidBuf);
extern _SlReturnVal_t _SlSocketHandleAsync_Connect(void *pVoidBuf);
extern _SlReturnVal_t _SlSocketHandleAsync_Close(void *pVoidBuf);
extern _SlReturnVal_t _SlDrvGlobalObjUnLock(_u8 bDecrementApiInProgress);
extern _SlReturnVal_t _SlDrvDriverIsApiAllowed(_u16 Silo);
extern _SlReturnVal_t _SlDrvMsgReadSpawnCtx(void *pValue);


#ifndef SL_TINY
extern _i16  _SlDrvBasicCmd(_SlOpcode_t Opcode);
extern _SlReturnVal_t _SlSocketHandleAsync_Accept(void *pVoidBuf);
extern _SlReturnVal_t _SlNetAppHandleAsync_DnsGetHostByService(void *pVoidBuf);
extern _SlReturnVal_t _SlSocketHandleAsync_Select(void *pVoidBuf);

#ifdef slcb_GetTimestamp
extern void _SlDrvStartMeasureTimeout(_SlTimeoutParams_t *pTimeoutInfo, _u32 TimeoutInMsec);
extern _u8 _SlDrvIsTimeoutExpired(_SlTimeoutParams_t *pTimeoutInfo);
extern void _SlDrvSleep(_u16 DurationInMsec);
#endif

#endif


extern _SlReturnVal_t _SlNetAppHandleAsync_DnsGetHostByName(void *pVoidBuf);
extern _SlReturnVal_t _SlNetAppHandleAsync_DnsGetHostByAddr(void *pVoidBuf);
extern _SlReturnVal_t _SlNetAppHandleAsync_PingResponse(void *pVoidBuf);
extern _SlReturnVal_t _SlNetAppEventHandler(void* pArgs);

#if defined(slcb_NetAppHttpServerHdlr) || defined(EXT_LIB_REGISTERED_HTTP_SERVER_EVENTS)
extern void _SlDrvDispatchHttpServerEvents(SlNetAppHttpServerEvent_t *slHttpServerEvent, SlNetAppHttpServerResponse_t *slHttpServerResponse);
#endif

#if defined(slcb_NetAppRequestHdlr) || defined(EXT_LIB_REGISTERED_NETAPP_REQUEST_EVENTS)
extern void _SlDrvDispatchNetAppRequestEvents(SlNetAppRequest_t *slNetAppRequestEvent, SlNetAppResponse_t *slNetAppResponse);
#endif

extern void _SlDeviceHandleAsync_Stop(void *pVoidBuf);
extern void _SlNetUtilHandleAsync_Cmd(void *pVoidBuf);
extern _u8  _SlDrvWaitForPoolObj(_u8 ActionID, _u8 SocketID);
extern void _SlDrvReleasePoolObj(_u8 pObj);
extern _u16 _SlDrvAlignSize(_u16 msgLen); 
extern _u8  _SlDrvProtectAsyncRespSetting(_u8 *pAsyncRsp, _SlActionID_e ActionID, _u8 SocketID);
extern void _SlNetAppHandleAsync_NetAppReceive(void *pVoidBuf);


extern _SlReturnVal_t _SlDeviceEventHandler(void* pEventInfo);
extern _SlReturnVal_t  _SlDrvSyncObjWaitForever(_SlSyncObj_t *pSyncObj);
extern _SlReturnVal_t  _SlDrvObjLockWaitForever(_SlLockObj_t *pLockObj);
extern _SlReturnVal_t  _SlDrvSyncObjWaitTimeout(_SlSyncObj_t *pSyncObj,
		                                _u32 timeoutVal,
		                                _u32 asyncEventOpcode);

extern _SlReturnVal_t  _SlDrvSyncObjSignal(_SlSyncObj_t *pSyncObj);
extern _SlReturnVal_t  _SlDrvObjLock(_SlLockObj_t *pLockObj, _SlTime_t Timeout);
extern _SlReturnVal_t  _SlDrvProtectionObjLockWaitForever(void);
extern _SlReturnVal_t  _SlDrvObjUnLock(_SlLockObj_t *pLockObj);
extern _SlReturnVal_t  _SlDrvProtectionObjUnLock(void);

extern void  _SlDrvMemZero(void* Addr, _u16 size);
extern void  _SlDrvResetCmdExt(_SlCmdExt_t* pCmdExt);

extern _i8 _SlDrvIsApiInProgress(void);
extern void _SlDrvHandleResetRequest(const void* pIfHdl, _i8*  pDevName);

#ifndef SL_TINY
extern void _SlDrvHandleFatalError(_u32 errorId, _u32 info1, _u32 info2);
extern void _SlDrvHandleAssert(void);

#endif

_i32 _SlDrvSetErrno(_i32 Errno);
_i32* _SlDrvallocateErrno(_i32 Errno);

#ifndef SL_INC_INTERNAL_ERRNO
extern int slcb_SetErrno(int Errno);
#endif

#define _SL_PROTOCOL_ALIGN_SIZE(msgLen)             (((msgLen)+3) & (~3))
#define _SL_IS_PROTOCOL_ALIGNED_SIZE(msgLen)        (!((msgLen) & 3))


#define _SL_PROTOCOL_CALC_LEN(pCmdCtrl,pCmdExt)     ((pCmdExt) ? \
                                                     (_SL_PROTOCOL_ALIGN_SIZE(pCmdCtrl->TxDescLen) + _SL_PROTOCOL_ALIGN_SIZE(pCmdExt->TxPayload1Len + pCmdExt->TxPayload2Len)) : \
                                                     (_SL_PROTOCOL_ALIGN_SIZE(pCmdCtrl->TxDescLen)))
                                                     
#ifdef  __cplusplus
}
#endif /* __cplusplus */
                                                     
#endif /* __DRIVER_INT_H__ */
