/*
 * Copyright (c) 2017, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

 
/*****************************************************************************/
/* Include files                                                             */
/*****************************************************************************/

#include <ti/drivers/net/wifi/slnetifwifi.h>

/*****************************************************************************/
/* Macro declarations                                                        */
/*****************************************************************************/

/* Macro which split the 8bit security flags from the input flags            */
#define SPLIT_SEC_AND_INPUT_FLAGS(inputFlags, secFlags) (secFlags = inputFlags >> 24)

/* Disable the 8bit security flags                                           */
#define SECURITY_FLAGS_IN_32BIT_REPRESENTATION          (0xFF000000)
#define DISABLE_SEC_BITS_FROM_INPUT_FLAGS(inputFlags)   (inputFlags &= ~SECURITY_FLAGS_IN_32BIT_REPRESENTATION)


/*****************************************************************************/
/* Structure/Enum declarations                                               */
/*****************************************************************************/

/*****************************************************************************/
/* Global declarations                                                       */
/*****************************************************************************/

/*!
    SlNetIfConfigWifi structure contains all the function callbacks that are expected to be filled by the relevant network stack interface
    Each interface has different capabilities, so not all the API's must be supported.
    Interface that is not supporting a non-mandatory API are set to NULL 
*/
SlNetIf_Config_t SlNetIfConfigWifi = 
{
    SlNetIfWifi_socket,              // Callback function sockCreate in slnetif module
    SlNetIfWifi_close,               // Callback function sockClose in slnetif module
    NULL,                            // Callback function sockShutdown in slnetif module
    SlNetIfWifi_accept,              // Callback function sockAccept in slnetif module
    SlNetIfWifi_bind,                // Callback function sockBind in slnetif module
    SlNetIfWifi_listen,              // Callback function sockListen in slnetif module
    SlNetIfWifi_connect,             // Callback function sockConnect in slnetif module
    NULL,                            // Callback function sockGetPeerName in slnetif module
    NULL,                            // Callback function sockGetLocalName in slnetif module
    SlNetIfWifi_select,              // Callback function sockSelect in slnetif module
    SlNetIfWifi_setSockOpt,          // Callback function sockSetOpt in slnetif module
    SlNetIfWifi_getSockOpt,          // Callback function sockGetOpt in slnetif module
    SlNetIfWifi_recv,                // Callback function sockRecv in slnetif module
    SlNetIfWifi_recvFrom,            // Callback function sockRecvFrom in slnetif module
    SlNetIfWifi_send,                // Callback function sockSend in slnetif module
    SlNetIfWifi_sendTo,              // Callback function sockSendTo in slnetif module
    SlNetIfWifi_sockstartSec,        // Callback function sockstartSec in slnetif module
    SlNetIfWifi_getHostByName,       // Callback function utilGetHostByName in slnetif module
    SlNetIfWifi_getIPAddr,           // Callback function ifGetIPAddr in slnetif module
    SlNetIfWifi_getConnectionStatus, // Callback function ifGetConnectionStatus in slnetif module
    SlNetIfWifi_loadSecObj,          // Callback function ifLoadSecObj in slnetif module
    NULL                             // Callback function ifCreateContext in slnetif module
};

static const int16_t StartSecOptName[10] = 
{
    SL_SO_SECURE_FILES_PRIVATE_KEY_FILE_NAME, 
    SL_SO_SECURE_FILES_CERTIFICATE_FILE_NAME,
    SL_SO_SECURE_FILES_CA_FILE_NAME,
    SL_SO_SECURE_FILES_PEER_CERT_OR_DH_KEY_FILE_NAME,
    SL_SO_SECMETHOD,
    SL_SO_SECURE_MASK,
    SL_SO_SECURE_ALPN,
    SL_SO_SECURE_EXT_CLIENT_CHLNG_RESP,
    SL_SO_SECURE_DOMAIN_NAME_VERIFICATION,
    SL_SO_SECURE_DISABLE_CERTIFICATE_STORE
};

static const int16_t socketType[8] = 
{
    SL_SOCK_STREAM, 
    SL_SOCK_DGRAM,
    SL_SOCK_RAW,
    SLNETSOCK_SOCK_RX_MTR,
    SL_SOCK_DGRAM,
    SL_SOCK_RAW,
    SLNETSOCK_SOCK_BRIDGE,
    SLNETSOCK_SOCK_ROUTER,
};

/*****************************************************************************/
/* Function prototypes                                                       */
/*****************************************************************************/

//*****************************************************************************
//
// SlNetIfWifi_socket - Create an endpoint for communication
//
//*****************************************************************************
int16_t SlNetIfWifi_socket(void *ifContext, int16_t Domain, int16_t Type, int16_t Protocol, void **sdContext)
{
    /* Create socket and return the return value of the function             */
    int16_t mappedSocketType = socketType[Type - 1];
    return (sl_Socket(Domain, mappedSocketType, Protocol));
}


//*****************************************************************************
//
// SlNetIfWifi_close - Gracefully close socket
//
//*****************************************************************************
int32_t SlNetIfWifi_close(int16_t sd, void *sdContext)
{
    /* Close socket and return the return value of the function                */
    return sl_Close(sd);
}


//*****************************************************************************
//
// SlNetIfWifi_accept - Accept a connection on a socket
//
//*****************************************************************************
int16_t SlNetIfWifi_accept(int16_t sd, void *sdContext, SlNetSock_Addr_t *addr, SlNetSocklen_t *addrlen, uint8_t flags, void **acceptedSdContext)
{
    return sl_Accept(sd, (SlSockAddr_t *)addr, addrlen);
}


//*****************************************************************************
//
// SlNetIfWifi_bind - Assign a name to a socket
//
//*****************************************************************************
int32_t SlNetIfWifi_bind(int16_t sd, void *sdContext, const SlNetSock_Addr_t *addr, int16_t addrlen)
{
    return sl_Bind(sd, (const SlSockAddr_t *)addr, addrlen);
}


//*****************************************************************************
//
// SlNetIfWifi_listen - Listen for connections on a socket
//
//*****************************************************************************
int32_t SlNetIfWifi_listen(int16_t sd, void *sdContext, int16_t backlog)
{
    return sl_Listen(sd, backlog);
}


//*****************************************************************************
//
// SlNetIfWifi_connect - Initiate a connection on a socket 
//
//*****************************************************************************
int32_t SlNetIfWifi_connect(int16_t sd, void *sdContext, const SlNetSock_Addr_t *addr, SlNetSocklen_t addrlen, uint8_t flags)
{
    return sl_Connect(sd, (const SlSockAddr_t *)addr, addrlen);
}


//*****************************************************************************
//
// SlNetIfWifi_getSockName - Returns the local address info of the socket 
//                         descriptor
//
//*****************************************************************************
int32_t SlNetIfWifi_getSockName(int16_t sd, void *sdContext, SlNetSock_Addr_t *addr, SlNetSocklen_t *addrlen)
{
// Not implemented in NWP
    return SLNETERR_INVALPARAM;
}


//*****************************************************************************
//
// SlNetIfWifi_select - Monitor socket activity
//
//*****************************************************************************
int32_t SlNetIfWifi_select(void *ifContext, int16_t nfds, SlNetSock_SdSet_t *readsds, SlNetSock_SdSet_t *writesds, SlNetSock_SdSet_t *exceptsds, SlNetSock_Timeval_t *timeout)
{
    SlNetSock_Timeval_t *slNetSockTimeVal;
    SlTimeval_t tv;
    /* Translate from SlNetSock_Timeval_t into SlTimeval_t */
    slNetSockTimeVal = (SlNetSock_Timeval_t *)timeout;
    tv.tv_sec = slNetSockTimeVal->tv_sec;
    tv.tv_usec = slNetSockTimeVal->tv_usec;
    return sl_Select(nfds, (SlFdSet_t *)readsds, (SlFdSet_t *)writesds, (SlFdSet_t *)exceptsds, &tv);
}


//*****************************************************************************
//
// SlNetIfWifi_setSockOpt - Set socket options
//
//*****************************************************************************
int32_t SlNetIfWifi_setSockOpt(int16_t sd, void *sdContext, int16_t level, int16_t optname, void *optval, SlNetSocklen_t optlen)
{
    SlNetSock_Timeval_t *slNetSockTimeVal;
    SlTimeval_t tv;

    switch (level)
    {
        case SLNETSOCK_LVL_SOCKET:
        {
            switch (optname)
            {
                case SLNETSOCK_OPSOCK_RCV_TIMEO:
                {
                    /* Translate from SlNetSock_Timeval_t into SlTimeval_t */
                    slNetSockTimeVal = (SlNetSock_Timeval_t *)optval;
                    tv.tv_sec = slNetSockTimeVal->tv_sec;
                    tv.tv_usec = slNetSockTimeVal->tv_usec;
                    optval = (void *)&tv;
                    optlen = sizeof(SlTimeval_t);
                    break;
                }
                default:
                    /* Pass values into sl_SetSockOpt directly */
                    break;
            }
            break;
        }
        default:
            /* Pass values into sl_SetSockOpt directly */
            break;
    }

    return sl_SetSockOpt(sd, level, optname, optval, optlen);
}


//*****************************************************************************
//
// SlNetIfWifi_getSockOpt - Get socket options
//
//*****************************************************************************
int32_t SlNetIfWifi_getSockOpt(int16_t sd, void *sdContext, int16_t level, int16_t optname, void *optval, SlNetSocklen_t *optlen)
{
    SlSocklen_t len;
    int32_t status = 0;
    SlNetSock_Timeval_t *slNetSockTimeVal;
    SlTimeval_t tv;

    switch (level)
    {
        case SLNETSOCK_LVL_SOCKET:
        {
            switch (optname)
            {
                case SLNETSOCK_OPSOCK_RCV_TIMEO:
                {
                    if (*optlen < sizeof(SlNetSock_Timeval_t))
                    {
                        return (SLNETERR_RET_CODE_INVALID_INPUT);
                    }
                    len = sizeof(SlTimeval_t);
                    status =
                        sl_GetSockOpt(sd, level, optname, (void *)&tv, &len);

                    slNetSockTimeVal = (SlNetSock_Timeval_t *)optval;
                    slNetSockTimeVal->tv_sec = tv.tv_sec;
                    slNetSockTimeVal->tv_usec = tv.tv_usec;
                    *optlen = sizeof(SlNetSock_Timeval_t);
                    break;
                }

                default:
                {
                    /* Pass values into sl_SetSockOpt directly */
                    status = sl_GetSockOpt(sd, level, optname, optval, optlen);
                    break;
                }

            }
            break;
        }
        default:
        {
            /* Pass values into sl_SetSockOpt directly */
            status = sl_GetSockOpt(sd, level, optname, optval, optlen);
            break;
        }
    }

    return (status);
}


//*****************************************************************************
//
// SlNetIfWifi_recv - Read data from TCP socket
//
//*****************************************************************************
int32_t SlNetIfWifi_recv(int16_t sd, void *sdContext, void *buf, uint32_t len, uint32_t flags)
{
    DISABLE_SEC_BITS_FROM_INPUT_FLAGS(flags);
    return sl_Recv(sd, buf, len, flags);
}


//*****************************************************************************
//
// SlNetIfWifi_recvFrom - Read data from socket
//
//*****************************************************************************
int32_t SlNetIfWifi_recvFrom(int16_t sd, void *sdContext, void *buf, uint32_t len, uint32_t flags, SlNetSock_Addr_t *from, SlNetSocklen_t *fromlen)
{
    DISABLE_SEC_BITS_FROM_INPUT_FLAGS(flags);
    return sl_RecvFrom(sd, buf, len, flags, (SlSockAddr_t *)from, fromlen);
}


//*****************************************************************************
//
// SlNetIfWifi_send - Write data to TCP socket
//
//*****************************************************************************
int32_t SlNetIfWifi_send(int16_t sd, void *sdContext, const void *buf, uint32_t len, uint32_t flags)
{
    DISABLE_SEC_BITS_FROM_INPUT_FLAGS(flags);
    return sl_Send(sd, buf, len, flags);
}


//*****************************************************************************
//
// SlNetIfWifi_sendTo - Write data to socket
//
//*****************************************************************************
int32_t SlNetIfWifi_sendTo(int16_t sd, void *sdContext, const void *buf, uint32_t len, uint32_t flags, const SlNetSock_Addr_t *to, SlNetSocklen_t tolen)
{
    DISABLE_SEC_BITS_FROM_INPUT_FLAGS(flags);
    return sl_SendTo(sd, buf, len, flags, (const SlSockAddr_t *)to, tolen);
}


//*****************************************************************************
//
// SlNetIfWifi_sockstartSec - Start a security session on an opened socket
//
//*****************************************************************************
int32_t SlNetIfWifi_sockstartSec(int16_t sd, void *sdContext, SlNetSockSecAttrib_t *secAttrib, uint8_t flags)
{
    SlNetSock_SecAttribNode_t *tempSecAttrib = *secAttrib;
    int32_t                   retVal         = SLNETERR_RET_CODE_OK;

    if ( 0 != (flags & SLNETSOCK_SEC_BIND_CONTEXT_ONLY)  )
    {
        /* run over all attributes and set them                              */
        while (NULL != tempSecAttrib)
        {
            if ( tempSecAttrib->attribName <= SLNETSOCK_SEC_ATTRIB_DISABLE_CERT_STORE)
            {
                retVal = sl_SetSockOpt(sd, SL_SOL_SOCKET, StartSecOptName[tempSecAttrib->attribName], tempSecAttrib->attribBuff, tempSecAttrib->attribBuffLen);
            }
            else
            {
                return SLNETERR_RET_CODE_INVALID_INPUT;
            }
            tempSecAttrib = tempSecAttrib->next;
        }
    }

    if ( 0 != (flags & SLNETSOCK_SEC_START_SECURITY_SESSION_ONLY)  )
    {
        /* Start TLS session                                                 */
        retVal = sl_StartTLS(sd);
    }

    return retVal;
}


//*****************************************************************************
//
// SlNetIfWifi_getHostByName - Obtain the IP Address of machine on network, by 
//                             machine name
//
//*****************************************************************************
int32_t SlNetIfWifi_getHostByName(void *ifContext, char *name, const uint16_t nameLen, uint32_t *ipAddr, uint16_t *ipAddrLen, const uint8_t family)
{
    int32_t  retVal = SLNETERR_RET_CODE_OK;
    
    /* sl_NetAppDnsGetHostByName can receive only one ipAddr variable, so
       only the first slot of the array will be used and the ipAddrLen will
       be updated to 1 when function is successfully                         */
    retVal = sl_NetAppDnsGetHostByName((signed char *)name, nameLen, (_u32 *)ipAddr, family);
    
    if (retVal == SLNETERR_RET_CODE_OK)
    {
        *ipAddrLen = 1;
    }
    
    return retVal;
     
}


//*****************************************************************************
//
// matchModeByRole - Service function used by SlNetIfWifi_getIPAddr for matching SlNetIfAddressType_e to SlNetCfg_e
//
//*****************************************************************************
int32_t matchModeByRole(SlNetIfAddressType_e addrType,
                        SlNetCfg_e *newAddrType      ,
                        uint16_t   *ipAddrLen        )
{
    SlWlanConnStatusParam_t WlanConnectInfo;
    uint16_t Len;
    int32_t retVal;

    switch(addrType)
    {

        case SLNETIF_IPV6_ADDR_LOCAL:
            *newAddrType = SL_NETCFG_IPV6_ADDR_LOCAL;
            *ipAddrLen   = sizeof(SlNetCfgIpV6Args_t);
            retVal = SLNETERR_RET_CODE_OK;
            break;

        case SLNETIF_IPV6_ADDR_GLOBAL:
            *newAddrType = SL_NETCFG_IPV6_ADDR_GLOBAL;
            *ipAddrLen   = sizeof(SlNetCfgIpV6Args_t);
            retVal = SLNETERR_RET_CODE_OK;
            break;

        /* IPv4 or P2P (GO and CL) */
        case SLNETIF_IPV4_ADDR:
            Len = sizeof(SlWlanConnStatusParam_t);
            *ipAddrLen = sizeof(SlNetCfgIpV4Args_t);
            retVal = sl_WlanGet(SL_WLAN_CONNECTION_INFO, NULL , &Len, (uint8_t*)&WlanConnectInfo);
            if(retVal != SLNETERR_RET_CODE_OK)
            {
                return retVal;
            }
            if(WlanConnectInfo.ConnStatus == SL_WLAN_CONNECTED_STA || WlanConnectInfo.ConnStatus == SL_WLAN_CONNECTED_P2PCL)
            {
                *newAddrType = SL_NETCFG_IPV4_STA_ADDR_MODE;
                retVal = SLNETERR_RET_CODE_OK;
            }
            else if(WlanConnectInfo.ConnStatus == SL_WLAN_CONNECTED_P2PGO || WlanConnectInfo.ConnStatus == SL_WLAN_AP_CONNECTED_STATIONS)
            {
                *newAddrType = SL_NETCFG_IPV4_AP_ADDR_MODE;
                retVal = SLNETERR_RET_CODE_OK;
            }
            else
            {
                retVal = SLNETERR_BSD_ENOTCONN;
            }

    }
    return retVal;
}


//*****************************************************************************
//
// SlNetIfWifi_getIPAddr - Get IP Address of specific interface
//
//*****************************************************************************
int32_t SlNetIfWifi_getIPAddr(void *ifContext, SlNetIfAddressType_e addrType, uint16_t *addrConfig, uint32_t *ipAddr)
{
    int32_t    retVal;
    uint16_t   ipAddrLen;
    SlNetCfg_e newAddrType;

    /* Translate the addrType of SlNetSock type to addrType of SlNetSockIfWifi type */
    retVal = matchModeByRole(addrType, &newAddrType, &ipAddrLen);
    if(retVal == SLNETERR_RET_CODE_OK)
    {
        retVal = sl_NetCfgGet(newAddrType, addrConfig, &ipAddrLen, (unsigned char *)ipAddr);
    }
    return retVal;
}


//*****************************************************************************
//
// SlNetIfWifi_getConnectionStatus - Get interface connection status
//
//*****************************************************************************
int32_t SlNetIfWifi_getConnectionStatus(void *ifContext)
{
    SlWlanConnStatusParam_t connectionParams;
    uint16_t                Opt    = 0;
    int32_t                 retVal = 0;
    uint16_t                Size   = 0;

    Size = sizeof(SlWlanConnStatusParam_t);
    memset(&connectionParams, 0, Size);
    
    retVal = sl_WlanGet(SL_WLAN_CONNECTION_INFO, &Opt, &Size, (uint8_t *)&connectionParams);

    /* Check if the function returned an error                               */
    if (retVal < SLNETERR_RET_CODE_OK)
    {
        /* Return error code                                                 */
        return retVal;
    }
    return connectionParams.ConnStatus;
}


//*****************************************************************************
//
// SlNetIfWifi_loadSecObj - Load secured buffer to the network stack
//
//*****************************************************************************
int32_t SlNetIfWifi_loadSecObj(void *ifContext, uint16_t objType, char *objName, int16_t objNameLen, uint8_t *objBuff, int16_t objBuffLen)
{
    int32_t   retVal;    /* negative retVal is an error */
    uint16_t  i;
    uint32_t  Offset            = 0;
    uint32_t  MasterToken       = 0;
    int32_t   OpenFlags         = 0;
    int32_t   DeviceFileHandle  = (-1);
    uint16_t  macAddressLen     = SL_MAC_ADDR_LEN;
    char      *deviceFileName   = objName;
    uint8_t   macAddress[SL_MAC_ADDR_LEN];

    /* Check if the inputs exists                                            */
    if ((NULL == objName) || (NULL == objBuff))
    {
        /* input not valid, return error code                                */
        return SLNETERR_RET_CODE_INVALID_INPUT;
    }
    /* Print device MAC address                                              */
    retVal = sl_NetCfgGet(SL_NETCFG_MAC_ADDRESS_GET, 0, &macAddressLen, &macAddress[0]);

    /* Generating Random MasterPassword but constant per deviceFileName      */
    for (i = 0; i < strlen(deviceFileName); i++)
    {
        MasterToken = ((MasterToken << 8) ^ deviceFileName[i]);
    }

    /* Create a file and write data. The file is secured, without
       signature and with a fail safe commit, with vendor token which is
       a XOR combination between the MAC address of the device and the
       object file name                                                      */
    OpenFlags = SL_FS_CREATE;
    OpenFlags |= SL_FS_OVERWRITE;
    OpenFlags |= SL_FS_CREATE_SECURE;
    OpenFlags |= SL_FS_CREATE_VENDOR_TOKEN;
    OpenFlags |= SL_FS_CREATE_NOSIGNATURE;
    OpenFlags |= SL_FS_CREATE_FAILSAFE;

    /* Create a secure file if not exists and open it for write.             */
    DeviceFileHandle =  sl_FsOpen((unsigned char *)deviceFileName, OpenFlags | SL_FS_CREATE_MAX_SIZE( objBuffLen ), (unsigned long *)&MasterToken);

    /* Check if file created successfully                                    */
    if ( DeviceFileHandle < SLNETERR_RET_CODE_OK )
    {
        return DeviceFileHandle;
    }

    Offset = 0;
    /* Write the buffer to the new file                                      */
    retVal = sl_FsWrite(DeviceFileHandle, Offset, (unsigned char *)objBuff, objBuffLen);

    /* Close the file                                                        */
    retVal = sl_FsClose(DeviceFileHandle, NULL, NULL , 0);

    return retVal;
}


//*****************************************************************************
//
// SlNetIfWifi_CreateContext - Allocate and store interface data
//
//*****************************************************************************
int32_t SlNetIfWifi_CreateContext(uint16_t ifID, const char *ifName, void **context)
{
    return SLNETERR_RET_CODE_OK;
}
