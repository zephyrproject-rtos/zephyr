/*
 * netutil.c - CC31xx/CC32xx Host Driver Implementation
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
/* Internal functions                                                        */
/*****************************************************************************/

/*****************************************************************************/
/* API Functions                                                             */
/*****************************************************************************/


typedef struct
{
	_u8   *pOutputValues;
    _u16  *pOutputLen;
    _i16   Status;
}_SlNetUtilCmdData_t;


/******************************************************************************
sl_UtilsGet
******************************************************************************/

typedef union
{
    SlNetUtilSetGet_t	Cmd;
    SlNetUtilSetGet_t	Rsp;
} SlNetUtilMsgGet_u;

#if _SL_INCLUDE_FUNC(sl_NetUtilGet)

const _SlCmdCtrl_t _SlNetUtilGetCmdCtrl =
{
    SL_OPCODE_NETUTIL_GET,
    sizeof(SlNetUtilSetGet_t),
    sizeof(SlNetUtilSetGet_t)
};

_i16 sl_NetUtilGet(const _u16 Option, const _u32 ObjID, _u8 *pValues, _u16 *pValueLen)
{
    SlNetUtilMsgGet_u       Msg;
    _SlCmdExt_t             CmdExt;

    _SlDrvResetCmdExt(&CmdExt);
    CmdExt.RxPayloadLen			= *pValueLen;
    CmdExt.pRxPayload			= (_u8 *)pValues;

    Msg.Cmd.Option				= Option;
    Msg.Cmd.ObjId				= ObjID;
    Msg.Cmd.ValueLen            = *pValueLen;

    VERIFY_RET_OK(_SlDrvCmdOp((_SlCmdCtrl_t *)&_SlNetUtilGetCmdCtrl, &Msg, &CmdExt));

    if(CmdExt.RxPayloadLen < CmdExt.ActualRxPayloadLen)
    {
        *pValueLen = CmdExt.RxPayloadLen;
        return SL_ESMALLBUF;
    }
    else
    {
        *pValueLen = CmdExt.ActualRxPayloadLen;
    }

    return (_i16)Msg.Rsp.Status;

}
#endif


/***************************************************************************
_SlNetUtilHandleAsync_Cmd - handles NetUtil Cmd response, signalling to 
a waiting object
****************************************************************************/
void _SlNetUtilHandleAsync_Cmd(void *pVoidBuf)
{
	_SlNetUtilCmdData_t *pOutData;
    SlNetUtilCmdRsp_t   *pMsgArgs   = (SlNetUtilCmdRsp_t *)_SL_RESP_ARGS_START(pVoidBuf);

    SL_DRV_PROTECTION_OBJ_LOCK_FOREVER();
	
    VERIFY_SOCKET_CB(NULL != g_pCB->ObjPool[g_pCB->FunctionParams.AsyncExt.ActionIndex].pRespArgs);

    pOutData = (_SlNetUtilCmdData_t*)g_pCB->ObjPool[g_pCB->FunctionParams.AsyncExt.ActionIndex].pRespArgs;

	pOutData->Status = pMsgArgs->Status;

	if(SL_RET_CODE_OK == pMsgArgs->Status)
	{
		if (*(pOutData->pOutputLen) < pMsgArgs->OutputLen)
		{
			pOutData->Status = SL_ESMALLBUF;
		}
		else
		{
			*(pOutData->pOutputLen) = pMsgArgs->OutputLen;

			if(*(pOutData->pOutputLen) > 0)
			{
				/* copy only the data from the global async buffer  */
				sl_Memcpy(pOutData->pOutputValues, (char*)pMsgArgs + sizeof(SlNetUtilCmdRsp_t), *(pOutData->pOutputLen));
			}
		}
	}
    
	_SlDrvSyncObjSignal(&g_pCB->ObjPool[g_pCB->FunctionParams.AsyncExt.ActionIndex].SyncObj);
    _SlDrvProtectionObjUnLock();
    return;
}


/*****************************************************************************
sl_NetUtilCmd
******************************************************************************/
typedef union
{
    SlNetUtilCmd_t		Cmd;
    _BasicResponse_t	Rsp;    
} SlNetUtilCmdMsg_u;

#if _SL_INCLUDE_FUNC(sl_NetUtilCmd)
const _SlCmdCtrl_t _SlNetUtilCmdCtrl =
{
    SL_OPCODE_NETUTIL_COMMAND,
    sizeof(SlNetUtilCmd_t),
    sizeof(_BasicResponse_t)
};


_i16 sl_NetUtilCmd(const _u16 Cmd, const _u8 *pAttrib,  const _u16 AttribLen,
				  const _u8 *pInputValues, const _u16 InputLen,
				  _u8 *pOutputValues, _u16 *pOutputLen)
{
    _i16				RetVal=0;
    SlNetUtilCmdMsg_u   Msg;
    _i16				ObjIdx = MAX_CONCURRENT_ACTIONS;
	_SlCmdExt_t         CmdExt;
	_SlNetUtilCmdData_t OutData;


	/* prepare the Cmd (control structure and data-buffer) */
	Msg.Cmd.Cmd			= Cmd;
	Msg.Cmd.AttribLen	= AttribLen;
	Msg.Cmd.InputLen	= InputLen;
	Msg.Cmd.OutputLen	= *pOutputLen;

	_SlDrvResetCmdExt(&CmdExt);
	_SlDrvMemZero(&OutData, sizeof(_SlNetUtilCmdData_t));

	if(AttribLen > 0)
	{
		CmdExt.pTxPayload1 = (_u8*)pAttrib;
		CmdExt.TxPayload1Len = AttribLen;
	}

	if (InputLen > 0)
	{
		CmdExt.pTxPayload2 = (_u8*)pInputValues;
		CmdExt.TxPayload2Len = InputLen;
	}

	/* Set the pointers to be filled upon the async event reception */
	OutData.pOutputValues = pOutputValues;
	OutData.pOutputLen = pOutputLen;

    ObjIdx = _SlDrvProtectAsyncRespSetting((_u8*)&OutData, NETUTIL_CMD_ID, SL_MAX_SOCKETS);
	if (MAX_CONCURRENT_ACTIONS == ObjIdx)
	{
		return SL_POOL_IS_EMPTY;
	}

	/* send the command */
    VERIFY_RET_OK(_SlDrvCmdOp((_SlCmdCtrl_t *)&_SlNetUtilCmdCtrl, &Msg, &CmdExt));

    if(SL_OS_RET_CODE_OK == (_i16)Msg.Rsp.status)
    {
    	/* after the async event is signaled, the data will be copied to the pOutputValues buffer  */
    	SL_DRV_SYNC_OBJ_WAIT_FOREVER(&g_pCB->ObjPool[ObjIdx].SyncObj);

    	/* the response header status */
    	RetVal = OutData.Status;

    }
	else
	{
		RetVal = Msg.Rsp.status;
	}
    _SlDrvReleasePoolObj((_u8)ObjIdx);

    return RetVal;
}
#endif
