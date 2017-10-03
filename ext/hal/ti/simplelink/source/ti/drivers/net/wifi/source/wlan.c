/*
 * wlan.c - CC31xx/CC32xx Host Driver Implementation
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
#define MAX_SSID_LEN           (32)
#define MAX_KEY_LEN            (64)
#define MAX_USER_LEN           (64)
#define MAX_ANON_USER_LEN      (64)
#define MAX_SMART_CONFIG_KEY   (16)


/*****************************************************************************
sl_WlanConnect
*****************************************************************************/
typedef struct 
{
    SlWlanConnectEapCommand_t    Args;
    _i8                        Strings[MAX_SSID_LEN + MAX_KEY_LEN + MAX_USER_LEN + MAX_ANON_USER_LEN];
}_WlanConnectCmd_t;

typedef union
{
    _WlanConnectCmd_t   Cmd;
    _BasicResponse_t	Rsp;
}_SlWlanConnectMsg_u;


#if _SL_INCLUDE_FUNC(sl_WlanConnect)
_i16 sl_WlanConnect(const _i8*  pName,const _i16 NameLen,const _u8 *pMacAddr,const SlWlanSecParams_t* pSecParams ,const SlWlanSecParamsExt_t* pSecExtParams)
{
    _SlWlanConnectMsg_u    Msg;
    _SlCmdCtrl_t           CmdCtrl = {0,0,0};

    _SlDrvMemZero(&Msg, (_u16)sizeof(_SlWlanConnectMsg_u));
    
    /* verify that this api is allowed. if not allowed then
    ignore the API execution and return immediately with an error */
    VERIFY_API_ALLOWED(SL_OPCODE_SILO_WLAN);
    CmdCtrl.TxDescLen = 0;/* init */
    CmdCtrl.RxDescLen = (_SlArgSize_t)sizeof(_BasicResponse_t);

    /* verify SSID length */
    VERIFY_PROTOCOL(NameLen <= MAX_SSID_LEN);
    /* verify SSID is not NULL */
    if( NULL == pName )
    {
       return SL_INVALPARAM;
    }
    /* update SSID length */
    Msg.Cmd.Args.Common.SsidLen = (_u8)NameLen;

    /* Profile with no security */
    /* Enterprise security profile */
    if (NULL != pSecExtParams)
    {
        /* Update command opcode */
        CmdCtrl.Opcode = SL_OPCODE_WLAN_WLANCONNECTEAPCOMMAND;
        CmdCtrl.TxDescLen += sizeof(SlWlanConnectEapCommand_t);
        /* copy SSID */
        sl_Memcpy(EAP_SSID_STRING(&Msg), pName, NameLen);
        CmdCtrl.TxDescLen += NameLen;
        /* Copy password if supplied */
        if ((NULL != pSecParams) && (pSecParams->KeyLen > 0))
        {
            /* update security type */
            Msg.Cmd.Args.Common.SecType = pSecParams->Type;
            /* verify key length */
            if (pSecParams->KeyLen > MAX_KEY_LEN)
            {
                return SL_INVALPARAM;
            }
            /* update key length */
            Msg.Cmd.Args.Common.PasswordLen = pSecParams->KeyLen;
            ARG_CHECK_PTR(pSecParams->Key);
            /* copy key		 */
            sl_Memcpy(EAP_PASSWORD_STRING(&Msg), pSecParams->Key, pSecParams->KeyLen);
            CmdCtrl.TxDescLen += pSecParams->KeyLen;
        }
        else
        {
            Msg.Cmd.Args.Common.PasswordLen = 0;
        }

        ARG_CHECK_PTR(pSecExtParams);
        /* Update Eap bitmask */
        Msg.Cmd.Args.EapBitmask = pSecExtParams->EapMethod;
        /* Update Certificate file ID index - currently not supported */
        Msg.Cmd.Args.CertIndex = pSecExtParams->CertIndex;
        /* verify user length */
        if (pSecExtParams->UserLen > MAX_USER_LEN)
        {
            return SL_INVALPARAM;
        }
        Msg.Cmd.Args.UserLen = pSecExtParams->UserLen;
        /* copy user name (identity) */
        if(pSecExtParams->UserLen > 0)
        {
            sl_Memcpy(EAP_USER_STRING(&Msg), pSecExtParams->User, pSecExtParams->UserLen);
            CmdCtrl.TxDescLen += pSecExtParams->UserLen;
        }
        /* verify Anonymous user length  */
        if (pSecExtParams->AnonUserLen > MAX_ANON_USER_LEN)
        {
            return SL_INVALPARAM;
        }
        Msg.Cmd.Args.AnonUserLen = pSecExtParams->AnonUserLen;
        /* copy Anonymous user */
        if(pSecExtParams->AnonUserLen > 0)
        {
            sl_Memcpy(EAP_ANON_USER_STRING(&Msg), pSecExtParams->AnonUser, pSecExtParams->AnonUserLen);
            CmdCtrl.TxDescLen += pSecExtParams->AnonUserLen;
        }

    }

    /* Regular or open security profile */
    else
    {
        /* Update command opcode */
        CmdCtrl.Opcode = SL_OPCODE_WLAN_WLANCONNECTCOMMAND;
        CmdCtrl.TxDescLen += sizeof(SlWlanConnectCommon_t);
        /* copy SSID */
        sl_Memcpy(SSID_STRING(&Msg), pName, NameLen);	
        CmdCtrl.TxDescLen += NameLen;
        /* Copy password if supplied */
        if( NULL != pSecParams )
        {
            /* update security type */
            Msg.Cmd.Args.Common.SecType = pSecParams->Type;
            /* verify key length is valid */
            if (pSecParams->KeyLen > MAX_KEY_LEN)
            {
                return SL_INVALPARAM;
            }
            /* update key length */
            Msg.Cmd.Args.Common.PasswordLen = pSecParams->KeyLen;
            CmdCtrl.TxDescLen += pSecParams->KeyLen;
            /* copy key (could be no key in case of WPS pin) */
            if( NULL != pSecParams->Key )
            {
                sl_Memcpy(PASSWORD_STRING(&Msg), pSecParams->Key, pSecParams->KeyLen);
            }
        }
        /* Profile with no security */
        else
        {
            Msg.Cmd.Args.Common.PasswordLen = 0;
            Msg.Cmd.Args.Common.SecType = SL_WLAN_SEC_TYPE_OPEN;
        }	
    }
    /* If BSSID is not null, copy to buffer, otherwise set to 0 */
    if(NULL != pMacAddr)
    {
        sl_Memcpy(Msg.Cmd.Args.Common.Bssid, pMacAddr, sizeof(Msg.Cmd.Args.Common.Bssid));
    }
    else
    {
        _SlDrvMemZero(Msg.Cmd.Args.Common.Bssid, (_u16)sizeof(Msg.Cmd.Args.Common.Bssid));
    }


    VERIFY_RET_OK ( _SlDrvCmdOp(&CmdCtrl, &Msg, NULL));

    return (_i16)Msg.Rsp.status;
}
#endif

/*******************************************************************************/
/*   sl_Disconnect  */
/* ******************************************************************************/
#if _SL_INCLUDE_FUNC(sl_WlanDisconnect)
_i16 sl_WlanDisconnect(void)
{
    /* verify that this api is allowed. if not allowed then
    ignore the API execution and return immediately with an error */
    VERIFY_API_ALLOWED(SL_OPCODE_SILO_WLAN);

    return _SlDrvBasicCmd(SL_OPCODE_WLAN_WLANDISCONNECTCOMMAND);
}
#endif

/******************************************************************************/
/*  sl_PolicySet  */
/******************************************************************************/
typedef union
{
    SlWlanPolicySetGet_t    Cmd;
    _BasicResponse_t	    Rsp;
}_SlPolicyMsg_u;

#if _SL_INCLUDE_FUNC(sl_WlanPolicySet)

static const _SlCmdCtrl_t _SlPolicySetCmdCtrl =
{
    SL_OPCODE_WLAN_POLICYSETCOMMAND,
    (_SlArgSize_t)sizeof(SlWlanPolicySetGet_t),
    (_SlArgSize_t)sizeof(_BasicResponse_t)
};

_i16 sl_WlanPolicySet(const _u8 Type , const _u8 Policy, _u8 *pVal,const _u8 ValLen)
{
    _SlPolicyMsg_u         Msg;
    _SlCmdExt_t            CmdExt;

    /* verify that this api is allowed. if not allowed then
    ignore the API execution and return immediately with an error */
    VERIFY_API_ALLOWED(SL_OPCODE_SILO_WLAN);

    _SlDrvResetCmdExt(&CmdExt);
    CmdExt.TxPayload1Len = ValLen;
    CmdExt.pTxPayload1 = (_u8 *)pVal;


    Msg.Cmd.PolicyType        = Type;
    Msg.Cmd.PolicyOption      = Policy;
    Msg.Cmd.PolicyOptionLen   = ValLen;

    VERIFY_RET_OK(_SlDrvCmdOp((_SlCmdCtrl_t *)&_SlPolicySetCmdCtrl, &Msg, &CmdExt));

    return (_i16)Msg.Rsp.status;
}
#endif


/******************************************************************************/
/*  sl_PolicyGet  */
/******************************************************************************/
typedef union
{
    SlWlanPolicySetGet_t	    Cmd;
    SlWlanPolicySetGet_t	    Rsp;
}_SlPolicyGetMsg_u;

#if _SL_INCLUDE_FUNC(sl_WlanPolicyGet)

static const _SlCmdCtrl_t _SlPolicyGetCmdCtrl =
{
    SL_OPCODE_WLAN_POLICYGETCOMMAND,
    (_SlArgSize_t)sizeof(SlWlanPolicySetGet_t),
    (_SlArgSize_t)sizeof(SlWlanPolicySetGet_t)
};


_i16 sl_WlanPolicyGet(const _u8 Type ,_u8 *pPolicy,_u8 *pVal,_u8 *pValLen)
{
    _SlPolicyGetMsg_u      Msg;
    _SlCmdExt_t            CmdExt;

    /* verify that this api is allowed. if not allowed then
    ignore the API execution and return immediately with an error */
    VERIFY_API_ALLOWED(SL_OPCODE_SILO_WLAN);

    if (*pValLen == 0)
    {
        return SL_EZEROLEN;
    }

    _SlDrvResetCmdExt(&CmdExt);
    CmdExt.RxPayloadLen = (_i16)(*pValLen);
    CmdExt.pRxPayload = pVal;

    Msg.Cmd.PolicyType = Type;

    VERIFY_RET_OK(_SlDrvCmdOp((_SlCmdCtrl_t *)&_SlPolicyGetCmdCtrl, &Msg, &CmdExt));


    if (CmdExt.RxPayloadLen < CmdExt.ActualRxPayloadLen) 
    {
        *pValLen = Msg.Rsp.PolicyOptionLen;
        return SL_ESMALLBUF;
    }
    else
    {
        /*  no pointer valus, fill the results into _i8 */
        *pValLen = (_u8)CmdExt.ActualRxPayloadLen;
		*pPolicy = Msg.Rsp.PolicyOption;
		 
        if( 0 == CmdExt.ActualRxPayloadLen )
        {
            *pValLen = 1;
        }

    }
    return (_i16)SL_OS_RET_CODE_OK;
}
#endif


/*******************************************************************************/
/*  sl_ProfileAdd  */
/*******************************************************************************/
typedef struct
{
    SlWlanAddGetEapProfile_t	Args;
    _i8                    Strings[MAX_SSID_LEN + MAX_KEY_LEN + MAX_USER_LEN + MAX_ANON_USER_LEN];
}_SlProfileParams_t;

typedef union
{
    _SlProfileParams_t	    Cmd;
    _BasicResponse_t	    Rsp;
}_SlProfileAddMsg_u;



#if _SL_INCLUDE_FUNC(sl_WlanProfileAdd)
_i16 sl_WlanProfileAdd(const _i8*  pName,const  _i16 NameLen,const  _u8 *pMacAddr,const  SlWlanSecParams_t* pSecParams ,const SlWlanSecParamsExt_t* pSecExtParams,const _u32  Priority,const _u32  Options)
{
    _SlProfileAddMsg_u      Msg;
    _SlCmdCtrl_t           CmdCtrl = {0,0,0};
    CmdCtrl.TxDescLen = 0;/* init */
    CmdCtrl.RxDescLen = (_SlArgSize_t)(sizeof(_BasicResponse_t));


    /* Options paramater is currently not in use */
    (void)Options;

    /* verify that this api is allowed. if not allowed then
    ignore the API execution and return immediately with an error */
    VERIFY_API_ALLOWED(SL_OPCODE_SILO_WLAN);

	_SlDrvMemZero(&Msg,sizeof(_SlProfileAddMsg_u));

    /* update priority */
    Msg.Cmd.Args.Common.Priority = (_u8)Priority; 
    /* verify SSID is not NULL */
    if( NULL == pName )
    {
       return SL_INVALPARAM;
    }
    /* verify SSID length */
    VERIFY_PROTOCOL(NameLen <= MAX_SSID_LEN);
    /* update SSID length */
    Msg.Cmd.Args.Common.SsidLen = (_u8)NameLen;


    /* Enterprise security profile */
    if  (NULL != pSecExtParams)
    {
        /* Update command opcode */
        CmdCtrl.Opcode = SL_OPCODE_WLAN_EAP_PROFILEADDCOMMAND;
        CmdCtrl.TxDescLen += sizeof(SlWlanAddGetEapProfile_t);

        /* copy SSID */
        sl_Memcpy(EAP_PROFILE_SSID_STRING(&Msg), pName, NameLen);	
        CmdCtrl.TxDescLen += NameLen;

        /* Copy password if supplied */
        if ((NULL != pSecParams) && (pSecParams->KeyLen > 0))
        {
            /* update security type */
            Msg.Cmd.Args.Common.SecType = (_i8)(pSecParams->Type);

            if( SL_WLAN_SEC_TYPE_WEP == Msg.Cmd.Args.Common.SecType )
            {
                Msg.Cmd.Args.Common.WepKeyId = 0;
            }

            /* verify key length */
            if (pSecParams->KeyLen > MAX_KEY_LEN)
            {
                return SL_INVALPARAM;
            }
            VERIFY_PROTOCOL(pSecParams->KeyLen <= MAX_KEY_LEN);
            /* update key length */
            Msg.Cmd.Args.Common.PasswordLen = pSecParams->KeyLen;	
            CmdCtrl.TxDescLen += pSecParams->KeyLen;
            ARG_CHECK_PTR(pSecParams->Key);
            /* copy key  */
            sl_Memcpy(EAP_PROFILE_PASSWORD_STRING(&Msg), pSecParams->Key, pSecParams->KeyLen);
        }
        else
        {
            Msg.Cmd.Args.Common.PasswordLen = 0;
        }

        ARG_CHECK_PTR(pSecExtParams);
        /* Update Eap bitmask */
        Msg.Cmd.Args.EapBitmask = pSecExtParams->EapMethod;
        /* Update Certificate file ID index - currently not supported */
        Msg.Cmd.Args.CertIndex = pSecExtParams->CertIndex;
        /* verify user length */
        if (pSecExtParams->UserLen > MAX_USER_LEN)
        {
            return SL_INVALPARAM;
        }
        Msg.Cmd.Args.UserLen = pSecExtParams->UserLen;
        /* copy user name (identity) */
        if(pSecExtParams->UserLen > 0)
        {
            sl_Memcpy(EAP_PROFILE_USER_STRING(&Msg), pSecExtParams->User, pSecExtParams->UserLen);
            CmdCtrl.TxDescLen += pSecExtParams->UserLen;
        }

        /* verify Anonymous user length (for tunneled) */
        if (pSecExtParams->AnonUserLen > MAX_ANON_USER_LEN)
        {
            return SL_INVALPARAM;
        }
        Msg.Cmd.Args.AnonUserLen = pSecExtParams->AnonUserLen;

        /* copy Anonymous user */
        if(pSecExtParams->AnonUserLen > 0)
        {
            sl_Memcpy(EAP_PROFILE_ANON_USER_STRING(&Msg), pSecExtParams->AnonUser, pSecExtParams->AnonUserLen);
            CmdCtrl.TxDescLen += pSecExtParams->AnonUserLen;
        }

    }
    /* Regular or open security profile */
    else
    {
        /* Update command opcode */
        CmdCtrl.Opcode = SL_OPCODE_WLAN_PROFILEADDCOMMAND;
        /* update commnad length */
        CmdCtrl.TxDescLen += sizeof(SlWlanAddGetProfile_t);

        if (NULL != pName)
        {
            /* copy SSID */
            sl_Memcpy(PROFILE_SSID_STRING(&Msg), pName, NameLen);
            CmdCtrl.TxDescLen += NameLen;
        }

        /* Copy password if supplied */
        if( NULL != pSecParams )
        {
            /* update security type */
            Msg.Cmd.Args.Common.SecType = (_i8)(pSecParams->Type);

            if( SL_WLAN_SEC_TYPE_WEP == Msg.Cmd.Args.Common.SecType )
            {
                Msg.Cmd.Args.Common.WepKeyId = 0;
            }

            /* verify key length */
            if (pSecParams->KeyLen > MAX_KEY_LEN)
            {
                return SL_INVALPARAM;
            }
            /* update key length */
            Msg.Cmd.Args.Common.PasswordLen = pSecParams->KeyLen;
            CmdCtrl.TxDescLen += pSecParams->KeyLen;
            /* copy key (could be no key in case of WPS pin) */
            if( NULL != pSecParams->Key )
            {
                sl_Memcpy(PROFILE_PASSWORD_STRING(&Msg), pSecParams->Key, pSecParams->KeyLen);
            }
        }
        else
        {
            Msg.Cmd.Args.Common.SecType = SL_WLAN_SEC_TYPE_OPEN;
            Msg.Cmd.Args.Common.PasswordLen = 0;
        }

    }


    /* If BSSID is not null, copy to buffer, otherwise set to 0  */
    if(NULL != pMacAddr)
    {
        sl_Memcpy(Msg.Cmd.Args.Common.Bssid, pMacAddr, sizeof(Msg.Cmd.Args.Common.Bssid));
    }
    else
    {
		_SlDrvMemZero(Msg.Cmd.Args.Common.Bssid, (_u16)sizeof(Msg.Cmd.Args.Common.Bssid));
    }

    VERIFY_RET_OK(_SlDrvCmdOp(&CmdCtrl, &Msg, NULL));

    return (_i16)Msg.Rsp.status;
}
#endif
/*******************************************************************************/
/*   sl_ProfileGet */
/*******************************************************************************/
typedef union
{
    SlWlanProfileDelGetCommand_t Cmd;
    _SlProfileParams_t	        Rsp;
}_SlProfileGetMsg_u;


#if _SL_INCLUDE_FUNC(sl_WlanProfileGet)

static const _SlCmdCtrl_t _SlProfileGetCmdCtrl =
{
    SL_OPCODE_WLAN_PROFILEGETCOMMAND,
    (_SlArgSize_t)sizeof(SlWlanProfileDelGetCommand_t),
    (_SlArgSize_t)sizeof(_SlProfileParams_t)
};

_i16 sl_WlanProfileGet(const _i16 Index,_i8*  pName, _i16 *pNameLen, _u8 *pMacAddr, SlWlanSecParams_t* pSecParams, SlWlanGetSecParamsExt_t* pEntParams, _u32 *pPriority)
{
    _SlProfileGetMsg_u      Msg;
    Msg.Cmd.Index = (_u8)Index;

    /* verify that this api is allowed. if not allowed then
    ignore the API execution and return immediately with an error */
    VERIFY_API_ALLOWED(SL_OPCODE_SILO_WLAN);

    VERIFY_RET_OK(_SlDrvCmdOp((_SlCmdCtrl_t *)&_SlProfileGetCmdCtrl, &Msg, NULL));

    pSecParams->Type = (_u8)(Msg.Rsp.Args.Common.SecType);
	if (Msg.Rsp.Args.Common.SecType >= 0)
	{
		/* since password is not transferred in getprofile, password length should always be zero */
		pSecParams->KeyLen = Msg.Rsp.Args.Common.PasswordLen;
		if (NULL != pEntParams)
		{
		pEntParams->EapMethod = Msg.Rsp.Args.EapBitmask;
			pEntParams->UserLen = Msg.Rsp.Args.UserLen;
			/* copy user name */
			if (pEntParams->UserLen > 0)
			{	 
				sl_Memcpy(pEntParams->User, EAP_PROFILE_USER_STRING(&Msg), pEntParams->UserLen);
			}
			pEntParams->AnonUserLen = Msg.Rsp.Args.AnonUserLen;
			/* copy anonymous user name */
			if (pEntParams->AnonUserLen > 0)
			{
				sl_Memcpy(pEntParams->AnonUser, EAP_PROFILE_ANON_USER_STRING(&Msg), pEntParams->AnonUserLen);
			}
		}

		*pNameLen  = (_i16)(Msg.Rsp.Args.Common.SsidLen);      
		*pPriority = Msg.Rsp.Args.Common.Priority;       

		if (NULL != Msg.Rsp.Args.Common.Bssid)
		{
			sl_Memcpy(pMacAddr, Msg.Rsp.Args.Common.Bssid, sizeof(Msg.Rsp.Args.Common.Bssid));
		}

		sl_Memset(pName, 0, SL_WLAN_SSID_MAX_LENGTH);
		sl_Memcpy(pName, EAP_PROFILE_SSID_STRING(&Msg), *pNameLen);
	}
    return (_i16)Msg.Rsp.Args.Common.SecType;

}
#endif
/*******************************************************************************/
/*   sl_ProfileDel  */
/*******************************************************************************/
typedef union
{
    SlWlanProfileDelGetCommand_t	    Cmd;
    _BasicResponse_t	            Rsp;
}_SlProfileDelMsg_u;


#if _SL_INCLUDE_FUNC(sl_WlanProfileDel)

static const _SlCmdCtrl_t _SlProfileDelCmdCtrl =
{
    SL_OPCODE_WLAN_PROFILEDELCOMMAND,
    (_SlArgSize_t)sizeof(SlWlanProfileDelGetCommand_t),
    (_SlArgSize_t)sizeof(_BasicResponse_t)
};

_i16 sl_WlanProfileDel(const _i16 Index)
{
    _SlProfileDelMsg_u Msg;

    /* verify that this api is allowed. if not allowed then
    ignore the API execution and return immediately with an error */
    VERIFY_API_ALLOWED(SL_OPCODE_SILO_WLAN);

    Msg.Cmd.Index = (_u8)Index;

    VERIFY_RET_OK(_SlDrvCmdOp((_SlCmdCtrl_t *)&_SlProfileDelCmdCtrl, &Msg, NULL));

    return (_i16)Msg.Rsp.status;
}
#endif


/******************************************************************************/
/*  sl_WlanGetNetworkList  */
/******************************************************************************/
typedef union
{
    SlWlanGetNetworkListCommand_t    Cmd;
    _WlanGetNetworkListResponse_t   Rsp;
}_SlWlanGetNetworkListMsg_u;


#if _SL_INCLUDE_FUNC(sl_WlanGetNetworkList)

static const _SlCmdCtrl_t _SlWlanGetNetworkListCtrl =
{
    SL_OPCODE_WLAN_SCANRESULTSGETCOMMAND,
    (_SlArgSize_t)sizeof(SlWlanGetNetworkListCommand_t),
    (_SlArgSize_t)sizeof(_WlanGetNetworkListResponse_t)
};

_i16 sl_WlanGetNetworkList(const _u8 Index,const _u8 Count, SlWlanNetworkEntry_t *pEntries)
{
    _i16 retVal = 0;
    _SlWlanGetNetworkListMsg_u Msg;
    _SlCmdExt_t    CmdExt;

    /* verify that this api is allowed. if not allowed then
    ignore the API execution and return immediately with an error */
    VERIFY_API_ALLOWED(SL_OPCODE_SILO_WLAN);

    if (Count == 0)
    {
        return SL_EZEROLEN;
    }

    _SlDrvResetCmdExt(&CmdExt);
    CmdExt.RxPayloadLen = (_i16)(sizeof(SlWlanNetworkEntry_t)*(Count));
    CmdExt.pRxPayload = (_u8 *)pEntries; 

    Msg.Cmd.Index = Index;
    Msg.Cmd.Count = Count;

    VERIFY_RET_OK(_SlDrvCmdOp((_SlCmdCtrl_t *)&_SlWlanGetNetworkListCtrl, &Msg, &CmdExt));
    retVal = Msg.Rsp.status;

    return (_i16)retVal;
}
#endif





/******************************************************************************/
/*     RX filters message command response structures  */
/******************************************************************************/

/* Set command */
typedef union
{
    SlWlanRxFilterAddCommand_t	          Cmd;
    SlWlanRxFilterAddCommandReponse_t     Rsp;
}_SlWlanRxFilterAddMsg_u;


#if _SL_INCLUDE_FUNC(sl_WlanRxFilterAdd)

static const _SlCmdCtrl_t _SlWlanRxFilterAddtCmdCtrl =
{
    SL_OPCODE_WLAN_WLANRXFILTERADDCOMMAND,
    (_SlArgSize_t)sizeof(SlWlanRxFilterAddCommand_t),
    (_SlArgSize_t)sizeof(SlWlanRxFilterAddCommandReponse_t)
};


/*****************************************************************************
    RX filters
*****************************************************************************/
_i16 sl_WlanRxFilterAdd(	SlWlanRxFilterRuleType_t 				RuleType,
    SlWlanRxFilterFlags_u 					Flags,
    const SlWlanRxFilterRule_u* const 		pRule,
    const SlWlanRxFilterTrigger_t* const 	pTrigger,
    const SlWlanRxFilterAction_t* const 	pAction,
    SlWlanRxFilterID_t*                     pFilterId)
{


    _SlWlanRxFilterAddMsg_u Msg;

    /* verify that this api is allowed. if not allowed then
    ignore the API execution and return immediately with an error */
    VERIFY_API_ALLOWED(SL_OPCODE_SILO_WLAN);

    Msg.Cmd.RuleType = RuleType;
    /* filterId is zero */
    Msg.Cmd.FilterId = 0;
    Msg.Cmd.Flags = Flags;
    sl_Memcpy( &(Msg.Cmd.Rule), pRule, sizeof(SlWlanRxFilterRule_u) );
    sl_Memcpy( &(Msg.Cmd.Trigger), pTrigger, sizeof(SlWlanRxFilterTrigger_t) );
    sl_Memcpy( &(Msg.Cmd.Action), pAction, sizeof(SlWlanRxFilterAction_t) );
    VERIFY_RET_OK(_SlDrvCmdOp((_SlCmdCtrl_t *)&_SlWlanRxFilterAddtCmdCtrl, &Msg, NULL) );
    *pFilterId = Msg.Rsp.FilterId;
    return (_i16)Msg.Rsp.Status;

}
#endif



/*******************************************************************************/
/*             sl_WlanRxStatStart                                              */
/*******************************************************************************/
#if _SL_INCLUDE_FUNC(sl_WlanRxStatStart)
_i16 sl_WlanRxStatStart(void)
{
    /* verify that this api is allowed. if not allowed then
    ignore the API execution and return immediately with an error */
    VERIFY_API_ALLOWED(SL_OPCODE_SILO_WLAN);

    return _SlDrvBasicCmd(SL_OPCODE_WLAN_STARTRXSTATCOMMAND);
}
#endif

#if _SL_INCLUDE_FUNC(sl_WlanRxStatStop)
_i16 sl_WlanRxStatStop(void)
{
    /* verify that this api is allowed. if not allowed then
    ignore the API execution and return immediately with an error */
    VERIFY_API_ALLOWED(SL_OPCODE_SILO_WLAN);

    return _SlDrvBasicCmd(SL_OPCODE_WLAN_STOPRXSTATCOMMAND);
}
#endif

#if _SL_INCLUDE_FUNC(sl_WlanRxStatGet)
_i16 sl_WlanRxStatGet(SlWlanGetRxStatResponse_t *pRxStat,const _u32 Flags)
{
    _SlCmdCtrl_t            CmdCtrl = {SL_OPCODE_WLAN_GETRXSTATCOMMAND, 0, (_SlArgSize_t)sizeof(SlWlanGetRxStatResponse_t)};
   /* Flags paramater is currently not in use */
   (void)Flags;
    
    /* verify that this api is allowed. if not allowed then
    ignore the API execution and return immediately with an error */
    VERIFY_API_ALLOWED(SL_OPCODE_SILO_WLAN);
    
    _SlDrvMemZero(pRxStat, (_u16)sizeof(SlWlanGetRxStatResponse_t));
    VERIFY_RET_OK(_SlDrvCmdOp((_SlCmdCtrl_t *)&CmdCtrl, pRxStat, NULL)); 

    return 0;
}
#endif

/******************************************************************************/
/*   sl_WlanProvisioning                                                      */
/******************************************************************************/


typedef struct
{
    SlWlanSmartConfigParams_t	Args;
    _i8                         Key[MAX_SMART_CONFIG_KEY]; /* public key + groupId1 key + groupId2 key */
}_SlSmartConfigArgs_t;


typedef struct
{
	SlWlanProvisioningParams_t ProvParams;  
	_SlSmartConfigArgs_t       SmartConfigParams;

}_SlProvisioning_t;

typedef union
{
	_SlProvisioning_t	Cmd;
	_BasicResponse_t	Rsp;
}_SlProvisioningStartMsg_u;

#if _SL_INCLUDE_FUNC(sl_WlanProvisioning)

const _SlCmdCtrl_t _SlProvisioningCmdCtrl =
{
	SL_OPCODE_WLAN_PROVISIONING_COMMAND,
	sizeof(_SlProvisioning_t),
	sizeof(_BasicResponse_t)
};

_i16 sl_WlanProvisioning(_u8 ProvisioningCmd, _u8 RequestedRoleAfterSuccess, _u16 InactivityTimeoutSec,  char *pSmartConfigKey, _u32 Flags)
{
	_SlProvisioningStartMsg_u      Msg;


	/* Verify if we can send this command to the NWP
	   We can send only prov. stop command if command is not allowed */
	if ((!SL_IS_COMMAND_ALLOWED) && (!SL_IS_PROVISIONING_ACTIVE)  && (InactivityTimeoutSec != 0))
	{
		/* return with the correct error code  */
		 return _SlDrvDriverIsApiAllowed(SL_OPCODE_SILO_WLAN);
	}

	/* If there is an API in progress and the timeout is not zero (it means the 
	   command is not prov. stop) then abort and return an error code */
	if (_SlDrvIsApiInProgress() && (InactivityTimeoutSec !=0))
	{
		return SL_RET_CODE_API_COMMAND_IN_PROGRESS;
	}


	 _SlDrvMemZero(&Msg, (_u16)sizeof (_SlProvisioningStartMsg_u));

	Msg.Cmd.ProvParams.ProvisioningCmd           = (_u8)ProvisioningCmd;
	Msg.Cmd.ProvParams.RequestedRoleAfterSuccess = (_u8)RequestedRoleAfterSuccess;
	Msg.Cmd.ProvParams.InactivityTimeoutSec      = (_u16)InactivityTimeoutSec;
	Msg.Cmd.ProvParams.Flags                     = Flags;

	 /* Smart Config parameters */
    if  (NULL != pSmartConfigKey)
	{
		Msg.Cmd.SmartConfigParams.Args.GroupIdBitmask = SL_WLAN_SMART_CONFIG_DEFAULT_GROUP;
		Msg.Cmd.SmartConfigParams.Args.Cipher =         SL_WLAN_SMART_CONFIG_DEFAULT_CIPHER;
		Msg.Cmd.SmartConfigParams.Args.PublicKeyLen =   SL_WLAN_SMART_CONFIG_KEY_LENGTH;

		/* copy keys (if exist) after command (one after another) */
		sl_Memcpy(SMART_CONFIG_START_PUBLIC_KEY_STRING(&Msg.Cmd.SmartConfigParams.Args), pSmartConfigKey, SL_WLAN_SMART_CONFIG_KEY_LENGTH);
	}
	VERIFY_RET_OK(_SlDrvCmdOp((_SlCmdCtrl_t *)&_SlProvisioningCmdCtrl , &Msg, NULL));

	return (_i16)Msg.Rsp.status;
}
#endif


/*******************************************************************************/
/*   sl_WlanSetMode  */
/*******************************************************************************/
typedef union
{
    SlWlanSetMode_t		    Cmd;
    _BasicResponse_t	    Rsp;
}_SlwlanSetModeMsg_u;

#if _SL_INCLUDE_FUNC(sl_WlanSetMode)

static const _SlCmdCtrl_t _SlWlanSetModeCmdCtrl =
{
    SL_OPCODE_WLAN_SET_MODE,
    (_SlArgSize_t)sizeof(SlWlanSetMode_t),
    (_SlArgSize_t)sizeof(_BasicResponse_t)
};

/* possible values are: 
WLAN_SET_STA_MODE   =   1
WLAN_SET_AP_MODE    =   2
WLAN_SET_P2P_MODE   =   3  */
_i16 sl_WlanSetMode(const _u8  Mode)
{
    _SlwlanSetModeMsg_u      Msg;

    /* verify that this api is allowed. if not allowed then
    ignore the API execution and return immediately with an error */
    VERIFY_API_ALLOWED(SL_OPCODE_SILO_WLAN);

    Msg.Cmd.Mode  = Mode;

    VERIFY_RET_OK(_SlDrvCmdOp((_SlCmdCtrl_t *)&_SlWlanSetModeCmdCtrl , &Msg, NULL));

    return (_i16)Msg.Rsp.status;
}
#endif




/*******************************************************************************/
/*   sl_WlanSet  */
/* ******************************************************************************/
typedef union
{
    SlWlanCfgSetGet_t	    Cmd;
    _BasicResponse_t	    Rsp;
}_SlWlanCfgSetMsg_u;


#if _SL_INCLUDE_FUNC(sl_WlanSet)

static const _SlCmdCtrl_t _SlWlanCfgSetCmdCtrl =
{
    SL_OPCODE_WLAN_CFG_SET,
    (_SlArgSize_t)sizeof(SlWlanCfgSetGet_t),
    (_SlArgSize_t)sizeof(_BasicResponse_t)
};

_i16 sl_WlanSet(const _u16 ConfigId ,const _u16 ConfigOpt,const _u16 ConfigLen,const  _u8 *pValues)
{
    _SlWlanCfgSetMsg_u         Msg;
    _SlCmdExt_t                CmdExt;

    /* verify that this api is allowed. if not allowed then
    ignore the API execution and return immediately with an error */
    VERIFY_API_ALLOWED(SL_OPCODE_SILO_WLAN);

    _SlDrvResetCmdExt(&CmdExt);
    CmdExt.TxPayload1Len = (_u16)((ConfigLen+3) & (~3));
    CmdExt.pTxPayload1 = (_u8 *)pValues;

    Msg.Cmd.ConfigId    = ConfigId;
    Msg.Cmd.ConfigLen   = ConfigLen;
    Msg.Cmd.ConfigOpt   = ConfigOpt;

    VERIFY_RET_OK(_SlDrvCmdOp((_SlCmdCtrl_t *)&_SlWlanCfgSetCmdCtrl, &Msg, &CmdExt));

    return (_i16)Msg.Rsp.status;
}
#endif


/******************************************************************************/
/*  sl_WlanGet  */
/******************************************************************************/
typedef union
{
    SlWlanCfgSetGet_t	    Cmd;
    SlWlanCfgSetGet_t	    Rsp;
}_SlWlanCfgMsgGet_u;

#if _SL_INCLUDE_FUNC(sl_WlanGet)

static const _SlCmdCtrl_t _SlWlanCfgGetCmdCtrl =
{
    SL_OPCODE_WLAN_CFG_GET,
    (_SlArgSize_t)sizeof(SlWlanCfgSetGet_t),
    (_SlArgSize_t)sizeof(SlWlanCfgSetGet_t)
};

_i16 sl_WlanGet(const _u16 ConfigId, _u16 *pConfigOpt,_u16 *pConfigLen, _u8 *pValues)
{
    _SlWlanCfgMsgGet_u        Msg;
    _SlCmdExt_t               CmdExt;

    /* verify that this api is allowed. if not allowed then
    ignore the API execution and return immediately with an error */
    VERIFY_API_ALLOWED(SL_OPCODE_SILO_WLAN);

    if (*pConfigLen == 0)
    {
        return SL_EZEROLEN;
    }

    _SlDrvResetCmdExt(&CmdExt);
    CmdExt.RxPayloadLen = (_i16)*pConfigLen;
    CmdExt.pRxPayload = (_u8 *)pValues;
    Msg.Cmd.ConfigLen   = *pConfigLen;
    Msg.Cmd.ConfigId    = ConfigId;
    if( pConfigOpt )
    {
        Msg.Cmd.ConfigOpt   = (_u16)*pConfigOpt;
    }
    VERIFY_RET_OK(_SlDrvCmdOp((_SlCmdCtrl_t *)&_SlWlanCfgGetCmdCtrl, &Msg, &CmdExt));

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


    return (_i16)Msg.Rsp.Status;
}
#endif
