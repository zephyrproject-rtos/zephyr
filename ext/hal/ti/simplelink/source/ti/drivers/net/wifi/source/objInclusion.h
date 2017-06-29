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


#include <ti/drivers/net/wifi/simplelink.h>


#ifndef OBJINCLUSION_H_
#define OBJINCLUSION_H_

#ifdef	__cplusplus
extern "C" {
#endif

/******************************************************************************

 For future use

*******************************************************************************/

#define __inln        	/* if inline functions requiered: #define __inln inline */

#define SL_DEVICE      	/* Device silo is currently always mandatory */



/******************************************************************************

 Qualifiers for package customizations

*******************************************************************************/

#if defined (SL_DEVICE)
#define __dev   1
#else
#define __dev   0
#endif

#if defined (SL_DEVICE) && defined (SL_INC_EXT_API)
#define __dev__ext   1
#else
#define __dev__ext   0
#endif


#if (!defined (SL_PLATFORM_MULTI_THREADED)) || (!defined (SL_PLATFORM_EXTERNAL_SPAWN))
#define __int__spwn     1
#else
#define __int__spwn     0
#endif

#if defined (SL_INC_NET_APP_PKG)
#define __nap    1
#else
#define __nap    0
#endif

#if defined  (SL_INC_NET_APP_PKG) && defined (SL_INC_SOCK_CLIENT_SIDE_API)
#define __nap__clt  1
#else
#define __nap__clt  0
#endif

#if defined  (SL_INC_NET_APP_PKG) && defined (SL_INC_EXT_API)
#define __nap__ext   1
#else
#define __nap__ext   0
#endif

#if defined (SL_INC_NET_CFG_PKG)
#define __ncg        1
#else
#define __ncg        0
#endif

#if defined (SL_INC_NET_CFG_PKG) && defined (SL_INC_EXT_API)
#define __ncg__ext        1
#else
#define __ncg__ext        0
#endif

#if defined (SL_INC_NVMEM_PKG)
#define __nvm       1
#else
#define __nvm       0
#endif

#if defined (SL_INC_NVMEM_EXT_PKG) && defined (SL_INC_EXT_API)
#define __nvm__ext      1
#else
#define __nvm__ext       0
#endif

#if defined (SL_INC_SOCKET_PKG)
#define __sck        1
#else
#define __sck        0
#endif

#if defined  (SL_INC_SOCKET_PKG) && defined (SL_INC_EXT_API)
#define __sck__ext    1
#else
#define __sck__ext    0
#endif

#if defined  (SL_INC_SOCKET_PKG) && defined (SL_INC_SOCK_SERVER_SIDE_API)
#define __sck__srv     1
#else
#define __sck__srv     0
#endif

#if defined  (SL_INC_SOCKET_PKG) && defined (SL_INC_SOCK_CLIENT_SIDE_API)
#define __sck__clt      1
#else
#define __sck__clt      0
#endif

#if defined  (SL_INC_SOCKET_PKG) && defined (SL_INC_SOCK_RECV_API)
#define __sck__rcv     1
#else
#define __sck__rcv     0
#endif

#if defined  (SL_INC_SOCKET_PKG) && defined (SL_INC_SOCK_SEND_API)
#define __sck__snd      1
#else
#define __sck__snd      0
#endif

#if defined (SL_INC_WLAN_PKG)
#define __wln           1
#else
#define __wln           0
#endif

#if defined  (SL_INC_WLAN_PKG) && defined (SL_INC_EXT_API)
#define __wln__ext      1
#else
#define __wln__ext      0
#endif

/* The return 1 is the function need to be included in the output */
#define _SL_INCLUDE_FUNC(Name)		(_SL_INC_##Name)

/* Driver */
#define _SL_INC_sl_NetAppStart          __nap__ext
#define _SL_INC_sl_NetAppStop           __nap__ext

#define _SL_INC_sl_NetAppDnsGetHostByName   __nap__clt


#define _SL_INC_sl_NetAppDnsGetHostByService		__nap__ext
#define _SL_INC_sl_NetAppMDNSRegisterService		__nap__ext
#define _SL_INC_sl_NetAppMDNSUnRegisterService		__nap__ext
#define _SL_INC_sl_NetAppGetServiceList	            __nap__ext


#define _SL_INC_sl_DnsGetHostByAddr     __nap__ext
#define _SL_INC_sl_NetAppPing           __nap__ext
#define _SL_INC_sl_NetAppSet            __nap__ext
#define _SL_INC_sl_NetAppGet            __nap__ext
#define _SL_INC_sl_NetAppRecv           __nap__ext

#define _SL_INC_sl_NetAppSend           __nap__ext

/* FS */
#define _SL_INC_sl_FsOpen            __nvm

#define _SL_INC_sl_FsClose           __nvm

#define _SL_INC_sl_FsRead            __nvm

#define _SL_INC_sl_FsWrite           __nvm

#define _SL_INC_sl_FsGetInfo         __nvm

#define _SL_INC_sl_FsDel             __nvm

#define _SL_INC_sl_FsCtl             __nvm__ext

#define _SL_INC_sl_FsProgram   __nvm__ext

#define _SL_INC_sl_FsGetFileList           __nvm__ext

/* netcfg */
#define _SL_INC_sl_MacAdrrSet           __ncg

#define _SL_INC_sl_MacAdrrGet           __ncg

#define _SL_INC_sl_NetCfgGet          __ncg

#define _SL_INC_sl_NetCfgSet          __ncg


/* socket */
#define _SL_INC_sl_Socket               __sck

#define _SL_INC_sl_Close                __sck

#define _SL_INC_sl_Accept               __sck__srv

#define _SL_INC_sl_Bind                 __sck

#define _SL_INC_sl_Listen               __sck__srv

#define _SL_INC_sl_Connect              __sck__clt

#define _SL_INC_sl_Select               __sck

#define _SL_INC_sl_SetSockOpt           __sck

#define _SL_INC_sl_GetSockOpt           __sck__ext

#define _SL_INC_sl_Recv                 __sck__rcv

#define _SL_INC_sl_RecvFrom             __sck__rcv

#define _SL_INC_sl_Write                __sck__snd

#define _SL_INC_sl_Send                 __sck__snd

#define _SL_INC_sl_SendTo               __sck__snd

#define _SL_INC_sl_Htonl                __sck

#define _SL_INC_sl_Htons                __sck

/* wlan */
#define _SL_INC_sl_WlanConnect          __wln__ext

#define _SL_INC_sl_WlanDisconnect           __wln__ext

#define _SL_INC_sl_WlanProfileAdd           __wln__ext

#define _SL_INC_sl_WlanProfileGet           __wln__ext

#define _SL_INC_sl_WlanProfileDel           __wln__ext

#define _SL_INC_sl_WlanPolicySet            __wln__ext

#define _SL_INC_sl_WlanPolicyGet            __wln__ext

#define _SL_INC_sl_WlanGetNetworkList       __wln__ext

#define _SL_INC_sl_WlanRxFilterAdd      __wln__ext

#define _SL_INC_sl_WlanRxFilterSet   __wln__ext

#define _SL_INC_sl_WlanRxFilterGet   __wln__ext

#define _SL_INC_sl_SmartConfigStart     __wln

#define _SL_INC_sl_SmartConfigOptSet    __wln__ext

#define _SL_INC_sl_WlanProvisioning   __wln

#define _SL_INC_sl_WlanSetMode			 __wln

#define _SL_INC_sl_WlanSet			 __wln

#define _SL_INC_sl_WlanGet			 __wln

#define _SL_INC_sl_SmartConfigOptSet    __wln__ext

#define _SL_INC_sl_SmartConfigOptGet    __wln__ext

#define  _SL_INC_sl_WlanRxStatStart      __wln__ext

#define _SL_INC_sl_WlanRxStatStop       __wln__ext

#define _SL_INC_sl_WlanRxStatGet        __wln__ext


/* device */
#define _SL_INC_sl_Task                 __int__spwn

#define _SL_INC_sl_Start                __dev

#define _SL_INC_sl_Stop                 __dev

#define _SL_INC_sl_StatusGet            __dev

#ifdef SL_IF_TYPE_UART
#define _SL_INC_sl_DeviceUartSetMode	__dev__ext
#endif

#define _SL_INC_sl_DeviceEventMaskGet   __dev__ext

#define _SL_INC_sl_DeviceEventMaskSet   __dev__ext

#define _SL_INC_sl_DeviceGet	        __dev__ext

#define _SL_INC_sl_DeviceSet		__dev__ext

/* netutil */
#define _SL_INC_sl_NetUtilGet			    __dev__ext

#define _SL_INC_sl_NetUtilSet			    __dev__ext

#define _SL_INC_sl_NetUtilCmd			    __dev__ext



#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif /*OBJINCLUSION_H_  */
