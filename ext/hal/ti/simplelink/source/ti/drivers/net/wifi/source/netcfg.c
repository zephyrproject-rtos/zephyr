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

/*****************************************************************************/
/* sl_NetCfgSet */
/*****************************************************************************/
typedef union
{
    SlNetCfgSetGet_t    Cmd;
    _BasicResponse_t   Rsp;
}_SlNetCfgMsgSet_u;

#if _SL_INCLUDE_FUNC(sl_NetCfgSet)

static const _SlCmdCtrl_t _SlNetCfgSetCmdCtrl =
{
    SL_OPCODE_DEVICE_NETCFG_SET_COMMAND,
    (_SlArgSize_t)sizeof(SlNetCfgSetGet_t),
    (_SlArgSize_t)sizeof(_BasicResponse_t)
};

_i16 sl_NetCfgSet(const _u16 ConfigId,const _u16 ConfigOpt,const _u16 ConfigLen,const _u8 *pValues)
{
    _SlNetCfgMsgSet_u         Msg;
    _SlCmdExt_t               CmdExt;

    /* verify that this api is allowed. if not allowed then
    ignore the API execution and return immediately with an error */
    VERIFY_API_ALLOWED(SL_OPCODE_SILO_NETCFG);

    _SlDrvResetCmdExt(&CmdExt);
    CmdExt.TxPayload1Len = (ConfigLen+3) & (~3);
    CmdExt.pTxPayload1 = (_u8 *)pValues;


    Msg.Cmd.ConfigId    = ConfigId;
    Msg.Cmd.ConfigLen   = ConfigLen;
    Msg.Cmd.ConfigOpt   = ConfigOpt;

    VERIFY_RET_OK(_SlDrvCmdOp((_SlCmdCtrl_t *)&_SlNetCfgSetCmdCtrl, &Msg, &CmdExt));

    return (_i16)Msg.Rsp.status;
}
#endif


/*****************************************************************************/
/* sl_NetCfgGet */
/*****************************************************************************/
typedef union
{
    SlNetCfgSetGet_t	    Cmd;
    SlNetCfgSetGet_t	    Rsp;
}_SlNetCfgMsgGet_u;

#if _SL_INCLUDE_FUNC(sl_NetCfgGet)

static const _SlCmdCtrl_t _SlNetCfgGetCmdCtrl =
{
    SL_OPCODE_DEVICE_NETCFG_GET_COMMAND,
    (_SlArgSize_t)sizeof(SlNetCfgSetGet_t),
    (_SlArgSize_t)sizeof(SlNetCfgSetGet_t)
};

_i16 sl_NetCfgGet(const _u16 ConfigId, _u16 *pConfigOpt,_u16 *pConfigLen, _u8 *pValues)
{
    _SlNetCfgMsgGet_u         Msg;
    _SlCmdExt_t               CmdExt;

    /* verify that this api is allowed. if not allowed then
    ignore the API execution and return immediately with an error */
    VERIFY_API_ALLOWED(SL_OPCODE_SILO_NETCFG);

    if (*pConfigLen == 0)
    {
        return SL_EZEROLEN;
    }

    _SlDrvResetCmdExt(&CmdExt);
    CmdExt.RxPayloadLen = (_i16)(*pConfigLen);
    CmdExt.pRxPayload = (_u8 *)pValues;

	_SlDrvMemZero((void*) &Msg, sizeof(Msg));

    Msg.Cmd.ConfigLen    = *pConfigLen;
    Msg.Cmd.ConfigId     = ConfigId;

    if( pConfigOpt )
    {
        Msg.Cmd.ConfigOpt   = (_u16)*pConfigOpt;
    }

    VERIFY_RET_OK(_SlDrvCmdOp((_SlCmdCtrl_t *)&_SlNetCfgGetCmdCtrl, &Msg, &CmdExt));

    if( pConfigOpt )
    {
        *pConfigOpt = (_u8)Msg.Rsp.ConfigOpt;
    }
    if (CmdExt.RxPayloadLen < CmdExt.ActualRxPayloadLen)
    {
         *pConfigLen = (_u8)CmdExt.RxPayloadLen;

         return SL_ESMALLBUF;
    }
    else
    {
        *pConfigLen = (_u8)CmdExt.ActualRxPayloadLen;
    }

    return Msg.Rsp.Status;
}
#endif
