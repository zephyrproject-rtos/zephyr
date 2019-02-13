/*
 * Copyright (c) 2017-2018, Texas Instruments Incorporated
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

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include <ti/net/slnetsock.h>
#include <ti/net/slnetutils.h>
#include <ti/net/slnetif.h>
#include <ti/net/slneterr.h>

/* POSIX Header files */
#include <pthread.h>

/*****************************************************************************/
/* Macro declarations                                                        */
/*****************************************************************************/
#define SLNETSOCK_NORMALIZE_NEEDED 0
#if SLNETSOCK_NORMALIZE_NEEDED
    #define SLNETSOCK_NORMALIZE_RET_VAL(retVal,err) ((retVal < 0)?(retVal = err):(retVal))
#else
    #define SLNETSOCK_NORMALIZE_RET_VAL(retVal,err)
#endif

#define SLNETSOCK_IPV4_ADDR_LEN           (4)
#define SLNETSOCK_IPV6_ADDR_LEN           (16)

#define SLNETSOCK_SIZEOF_ONE_SDSETBITMAP_SLOT_IN_BITS (sizeof(uint32_t)*8)

#define SLNETSOCK_LOCK()   pthread_mutex_lock(&VirtualSocketMutex) // forever
#define SLNETSOCK_UNLOCK() pthread_mutex_unlock(&VirtualSocketMutex)

#define ENABLE_DEFAULT_QUERY_FLAGS()    (SLNETSOCK_CREATE_IF_STATE_ENABLE | SLNETSOCK_CREATE_IF_STATUS_CONNECTED | SLNETSOCK_CREATE_ALLOW_PARTIAL_MATCH)
#define GET_QUERY_FLAGS(flags)          (flags & (SLNETSOCK_CREATE_IF_STATE_ENABLE | SLNETSOCK_CREATE_IF_STATUS_CONNECTED | SLNETSOCK_CREATE_ALLOW_PARTIAL_MATCH))

/* Macro which merge the 8bit security flags to the upper bits of the 32 bit
   input flags                                                               */
#define MERGE_SEC_INTO_INPUT_FLAGS(inputFlags, secFlags)  (inputFlags |= (secFlags << 24))


/*****************************************************************************/
/* Structure/Enum declarations                                               */
/*****************************************************************************/


/* Socket Endpoint */
typedef struct SlNetSock_VirtualSocket_t
{
    int16_t    realSd;
    uint8_t    sdFlags;
    void      *sdContext;
    SlNetIf_t *netIf;
} SlNetSock_VirtualSocket_t;

/* Structure which holds the realSd and the virtualSd indexes */
typedef struct SlNetSock_RealToVirtualIndexes_t
{
    int16_t    realSd;
    int16_t    virtualSd;
    struct SlNetSock_RealToVirtualIndexes_t *next;
} SlNetSock_RealToVirtualIndexes_t;

/*****************************************************************************/
/* Global declarations                                                       */
/*****************************************************************************/

static SlNetSock_VirtualSocket_t *VirtualSockets[SLNETSOCK_MAX_CONCURRENT_SOCKETS];

/* Variable That is true when the SlNetSock layer is initialized (Mutex
   created and VirtualSockets array initialized) and false when the layer
   isn't initialized.                                                        */
static uint8_t SlNetSock_Initialized = false;

/* Lock Object to secure access to the Virtual Sockets array                 */
static pthread_mutex_t VirtualSocketMutex;


/*****************************************************************************/
/* Function prototypes                                                       */
/*****************************************************************************/

static int32_t SlNetSock_getVirtualSdConf(int16_t virtualSdIndex, int16_t *realSd, uint8_t *sdFlags, void **sdContext, SlNetIf_t **netIf);
static int32_t SlNetSock_AllocVirtualSocket(int16_t *virtualSdIndex, SlNetSock_VirtualSocket_t **newSocketNode);
static int32_t SlNetSock_freeVirtualSocket(int16_t virtualSdIndex);

//*****************************************************************************
//
// SlNetSock_getVirtualSdConf - This function search and returns the
//                              configuration of virtual socket.
//
//*****************************************************************************
static int32_t SlNetSock_getVirtualSdConf(int16_t virtualSdIndex, int16_t *realSd, uint8_t *sdFlags, void **sdContext, SlNetIf_t **netIf)
{
    int32_t retVal = SLNETERR_RET_CODE_OK;

    /* Check if the SlNetSock layer initialized, means that the mutex exists */
    if (false == SlNetSock_Initialized)
    {
        return SLNETERR_RET_CODE_MUTEX_CREATION_FAILED;
    }

    SLNETSOCK_LOCK();

    /* Check if the input is valid and if real socket descriptor exists      */
    if ( (virtualSdIndex >= SLNETSOCK_MAX_CONCURRENT_SOCKETS) || (virtualSdIndex < 0) || (NULL == netIf) )
    {
        retVal = SLNETERR_RET_CODE_INVALID_INPUT;
    }
    else if (NULL == VirtualSockets[virtualSdIndex])
    {
        /* Socket was not found, return error code                           */
        retVal = SLNETERR_RET_CODE_COULDNT_FIND_RESOURCE;
    }
    else
    {
        /* Socket found, copy and return its content                         */
        *netIf  = VirtualSockets[virtualSdIndex]->netIf;
        *realSd = VirtualSockets[virtualSdIndex]->realSd;

        /* If sdContext pointer supplied, copy into it the sdContext of the
           socket                                                            */
        if ( NULL != sdContext )
        {
            *sdContext = VirtualSockets[virtualSdIndex]->sdContext;
        }

        /* If sdFlags pointer supplied, copy into it the sdFlags of the
           socket                                                            */
        if ( NULL != sdFlags )
        {
            *sdFlags = VirtualSockets[virtualSdIndex]->sdFlags;
        }

        /* Check if the interface of the socket is declared                  */
        if ( (NULL == (*netIf)) || (NULL == (*netIf)->ifConf) )
        {
            /* Interface was not found or config list is missing,
               return error code                                             */
            retVal = SLNETERR_RET_CODE_SOCKET_CREATION_IN_PROGRESS;
        }
    }

    SLNETSOCK_UNLOCK();

    return retVal;
}

//*****************************************************************************
//
// SlNetSock_AllocVirtualSocket - Search for free space in the VirtualSockets
//                                array and allocate a socket in this location
//
//*****************************************************************************
static int32_t SlNetSock_AllocVirtualSocket(int16_t *virtualSdIndex, SlNetSock_VirtualSocket_t **newSocketNode)
{
    uint16_t arrayIndex = 0;
    int32_t  retVal     = SLNETERR_RET_CODE_NO_FREE_SPACE;

    /* Check if the SlNetSock layer initialized, means that the mutex exists */
    if (false == SlNetSock_Initialized)
    {
        return SLNETERR_RET_CODE_MUTEX_CREATION_FAILED;
    }

    SLNETSOCK_LOCK();

    /* Search for free space in the VirtualSockets array                     */
    while ( arrayIndex < SLNETSOCK_MAX_CONCURRENT_SOCKETS )
    {
        /* Check if the arrayIndex in VirtualSockets is free                 */
        if ( NULL == VirtualSockets[arrayIndex] )
        {
            /* Allocate memory for new socket node for the socket list       */
            *newSocketNode = (SlNetSock_VirtualSocket_t *)calloc(1, sizeof(SlNetSock_VirtualSocket_t));

            /* Check if the memory allocated successfully                    */
            if (NULL == *newSocketNode)
            {
                /* Allocation failed, return error code                      */
                retVal = SLNETERR_RET_CODE_MALLOC_ERROR;
                break;
            }
            else
            {
                /* Location free, return the Index and function success      */
                *virtualSdIndex = arrayIndex;

                VirtualSockets[arrayIndex] = *newSocketNode;

                retVal = SLNETERR_RET_CODE_OK;
                break;
            }
        }
        else
        {
            /* Location isn't free, continue to next location                */
            arrayIndex++;
        }
    }

    SLNETSOCK_UNLOCK();

    return retVal;
}

//*****************************************************************************
//
// SlNetSock_freeVirtualSocket - free allocated socket and initialize the array
//                               in the virtual socket location
//
//*****************************************************************************
static int32_t SlNetSock_freeVirtualSocket(int16_t virtualSdIndex)
{
    int32_t retVal = SLNETERR_RET_CODE_OK;

    /* Check if the SlNetSock layer initialized, means that the mutex exists */
    if (false == SlNetSock_Initialized)
    {
        return SLNETERR_RET_CODE_MUTEX_CREATION_FAILED;
    }

    SLNETSOCK_LOCK();

    /* Check if the input is valid and if real socket descriptor exists      */
    if ( (virtualSdIndex >= SLNETSOCK_MAX_CONCURRENT_SOCKETS) || (virtualSdIndex < 0) )
    {
        retVal = SLNETERR_RET_CODE_INVALID_INPUT;
    }
    else if (NULL == VirtualSockets[virtualSdIndex])
    {
        /* Socket was not found, return error code                           */
        retVal = SLNETERR_RET_CODE_COULDNT_FIND_RESOURCE;
    }
    else
    {
        /* Free Socket Context allocated memory                              */
        if (NULL != VirtualSockets[virtualSdIndex]->sdContext)
        {
            free((void *)VirtualSockets[virtualSdIndex]->sdContext);
        }

        /* Free Socket Node allocated memory and delete it from the
           VirtualSockets array                                              */
        free((void *)VirtualSockets[virtualSdIndex]);

        VirtualSockets[virtualSdIndex] = NULL;
    }

    SLNETSOCK_UNLOCK();

    return retVal;
}

//*****************************************************************************
//
// SlNetSock_init - init the SlNetSock module
//
//*****************************************************************************
int32_t SlNetSock_init(int32_t flags)
{
    int16_t Index = SLNETSOCK_MAX_CONCURRENT_SOCKETS;
    int32_t retVal;

    /* If the SlNetSock layer isn't initialized, initialize it               */
    if (false == SlNetSock_Initialized)
    {
        /* Setup the mutex operations */
        retVal = pthread_mutex_init(&VirtualSocketMutex, (const pthread_mutexattr_t *)NULL);
        if (0 != retVal)
        {
            return SLNETERR_RET_CODE_MUTEX_CREATION_FAILED;
        }
        else
        {
            /* Initialize the VirtualSockets array                           */
            while (Index--)
            {
                VirtualSockets[Index] = NULL;
            }
            SlNetSock_Initialized = true;
        }
    }
    return SLNETERR_RET_CODE_OK;
}


//*****************************************************************************
//
// SlNetSock_create - Create an endpoint for communication
//
//*****************************************************************************
int16_t SlNetSock_create(int16_t domain, int16_t type, int16_t protocol, uint32_t ifBitmap, int16_t flags)
{
    SlNetIf_t                 *netIf;
    SlNetSock_VirtualSocket_t *sdNode;
    int16_t                    socketIndex;
    int16_t                    createdSd;
    int16_t                    queryFlags;
    int32_t                    retVal;


    /* if flags is zero, enable the default bits                             */
    if (0 == flags)
    {
        queryFlags = ENABLE_DEFAULT_QUERY_FLAGS();
    }
    else
    {
        queryFlags = GET_QUERY_FLAGS(flags);
    }

    /* Search for free place in the array */
    retVal = SlNetSock_AllocVirtualSocket(&socketIndex, &sdNode);

    /* Before creating a socket, check if there is a free place in the array */
    if ( retVal < SLNETERR_RET_CODE_OK )
    {
        /* There isn't a free space in the array, return error code          */
        return retVal;
    }

    if (protocol == 0) {
        switch (type) {
            case SLNETSOCK_SOCK_STREAM:
                protocol = SLNETSOCK_PROTO_TCP;
                break;
            case SLNETSOCK_SOCK_DGRAM:
                protocol = SLNETSOCK_PROTO_UDP;
                break;
            case SLNETSOCK_SOCK_RAW:
            default:
                /* Keep protocol as is for other types */
                break;
        }
    }

    /* When ifBitmap is 0, that means automatic selection of all interfaces
       is required, enable all bits in ifBitmap                              */
    if (0 == ifBitmap)
    {
        ifBitmap = ~ifBitmap;
    }

    /* This loop tries to create a socket on the required interface with the
       required queryFlags.
       When multiple interfaces, in addition to the queryFlags it will try
       to create the socket on the interface with the highest priority       */
    while ( ifBitmap > 0 )
    {
        /* Search for the highest priority interface according to the
           ifBitmap and the queryFlags                                       */
        netIf = SlNetIf_queryIf(ifBitmap, queryFlags);

        /* Check if the function returned NULL or the requested interface
           exists                                                            */
        if (NULL == netIf)
        {
            /* Free the captured VirtualSockets location                     */
            SlNetSock_freeVirtualSocket(socketIndex);

            /* Interface doesn't exists, save error code                     */
            return retVal;
        }
        else
        {

            /* Disable the ifID bit from the ifBitmap after finding the
               netIf                                                         */
            ifBitmap &= ~(netIf->ifID);

            /* Interface exists, try to create new socket                    */
            createdSd = (netIf->ifConf)->sockCreate(netIf->ifContext, domain, type, protocol, &(sdNode->sdContext));

            /* Check createdSd for error codes                               */
            if (createdSd < 0)
            {
                /* sockCreate failed, continue to the next ifID              */
                retVal = createdSd;
            }
            else
            {
                /* Real socket created, fill the allocated socket node       */
                sdNode->realSd    = createdSd;
                sdNode->netIf     = netIf;

                /* Socket created, allocated and connected to the
                   VirtualSockets array, return VirtualSockets index         */
                return socketIndex;
            }
        }
    }

    /* Free the captured VirtualSockets location                             */
    SlNetSock_freeVirtualSocket(socketIndex);

    /* There isn't a free space in the array or socket couldn't be opened,
       return error code                                                     */
    return retVal;
}


//*****************************************************************************
//
// SlNetSock_close - Gracefully close socket
//
//*****************************************************************************
int32_t SlNetSock_close(int16_t sd)
{
    int32_t    retVal = SLNETERR_RET_CODE_OK;
    int16_t    realSd;
    SlNetIf_t *netIf;
    void      *sdContext;

    /* Check if the sd input exists and return it                            */
    retVal = SlNetSock_getVirtualSdConf(sd, &realSd, NULL, &sdContext, &netIf);

    /* Check if sd found                                                     */
    if (SLNETERR_RET_CODE_OK != retVal)
    {
        /* Validation failed, return error code                              */
        return retVal;
    }

    /* Function exists in the interface of the socket descriptor, dispatch
       the Close command                                                     */
    retVal = (netIf->ifConf)->sockClose(realSd, sdContext);
    SLNETSOCK_NORMALIZE_RET_VAL(retVal,SLNETSOCK_ERR_SOCKCLOSE_FAILED);

    if (retVal == SLNETERR_RET_CODE_OK)
    {
        /* When freeing the virtual socket, it will free allocated memory
           of the sdContext and of the socket node, if other threads will
           try to use this socket or the retrieved data of the socket the
           stack needs to return an error                                    */
        SlNetSock_freeVirtualSocket(sd);
    }
    return retVal;


}


//*****************************************************************************
//
// SlNetSock_shutdown - Shutting down parts of a full-duplex connection
//
//*****************************************************************************
int32_t SlNetSock_shutdown(int16_t sd, int16_t how)
{
    int32_t    retVal = SLNETERR_RET_CODE_OK;
    int16_t    realSd;
    SlNetIf_t *netIf;
    void      *sdContext;

    /* Check if the sd input exists and return it                            */
    retVal = SlNetSock_getVirtualSdConf(sd, &realSd, NULL, &sdContext, &netIf);

    /* Check if sd found or if the non mandatory function exists             */
    if (SLNETERR_RET_CODE_OK != retVal)
    {
        return retVal;
    }
    if (NULL == (netIf->ifConf)->sockShutdown)
    {
        /* Non mandatory function doesn't exists, return error code          */
        return SLNETERR_RET_CODE_DOESNT_SUPPORT_NON_MANDATORY_FXN;
    }

    /* Function exists in the interface of the socket descriptor, dispatch
       the Shutdown command                                                  */
    retVal = (netIf->ifConf)->sockShutdown(realSd, sdContext, how);
    SLNETSOCK_NORMALIZE_RET_VAL(retVal,SLNETSOCK_ERR_SOCKSHUTDOWN_FAILED);

    return retVal;
}


//*****************************************************************************
//
// SlNetSock_accept - Accept a connection on a socket
//
//*****************************************************************************
int16_t SlNetSock_accept(int16_t sd, SlNetSock_Addr_t *addr, SlNetSocklen_t *addrlen)
{
    SlNetSock_VirtualSocket_t *allocSdNode;
    void                      *sdContext;
    int16_t                    realSd;
    int16_t                    socketIndex;
    int32_t                    retVal = SLNETERR_RET_CODE_OK;

    /* Search for free place in the array */
    retVal = SlNetSock_AllocVirtualSocket(&socketIndex, &allocSdNode);

    /* Before creating a socket, check if there is a free place in the array */
    if ( retVal < SLNETERR_RET_CODE_OK )
    {
        /* There isn't a free space in the array, return error code          */
        return retVal;
    }

    /* Check if the sd input exists and return it                            */
    retVal = SlNetSock_getVirtualSdConf(sd, &realSd, &(allocSdNode->sdFlags), &sdContext, &(allocSdNode->netIf));

    /* Check if sd found or if the non mandatory function exists             */
    if (SLNETERR_RET_CODE_OK != retVal)
    {
        return retVal;
    }
    if (NULL == ((allocSdNode->netIf)->ifConf)->sockAccept)
    {
        /* Free the captured VirtualSockets location                         */
        SlNetSock_freeVirtualSocket(socketIndex);

        /* Validation failed, return error code                              */
        return SLNETERR_RET_CODE_INVALID_INPUT;
    }

    /* Function exists in the interface of the socket descriptor, dispatch
       the Accept command                                                    */
    retVal = ((allocSdNode->netIf)->ifConf)->sockAccept(realSd, sdContext, addr, addrlen, allocSdNode->sdFlags, &(allocSdNode->sdContext));

    /* Check retVal for error codes                                          */
    if (retVal < SLNETERR_RET_CODE_OK)
    {
        /* Free the captured VirtualSockets location                         */
        SlNetSock_freeVirtualSocket(socketIndex);

        /* sockAccept failed, return error code                              */
        return retVal;
    }
    else
    {
        /* Real socket created, fill the allocated socket node               */
        allocSdNode->realSd = retVal;

        /* Socket created, allocated and connected to the
           VirtualSockets array, return VirtualSockets index                 */
        return socketIndex;
    }

}


//*****************************************************************************
//
// SlNetSock_bind - Assign a name to a socket
//
//*****************************************************************************
int32_t SlNetSock_bind(int16_t sd, const SlNetSock_Addr_t *addr, int16_t addrlen)
{
    int32_t    retVal = SLNETERR_RET_CODE_OK;
    int16_t    realSd;
    SlNetIf_t *netIf;
    void      *sdContext;

    /* Check if the sd input exists and return it                            */
    retVal = SlNetSock_getVirtualSdConf(sd, &realSd, NULL, &sdContext, &netIf);

    /* Check if sd found or if the non mandatory function exists             */
    if (SLNETERR_RET_CODE_OK != retVal)
    {
        return retVal;
    }
    if (NULL == (netIf->ifConf)->sockBind)
    {
        /* Non mandatory function doesn't exists, return error code          */
        return SLNETERR_RET_CODE_DOESNT_SUPPORT_NON_MANDATORY_FXN;
    }

    /* Function exists in the interface of the socket descriptor, dispatch
       the Bind command                                                      */
    retVal = (netIf->ifConf)->sockBind(realSd, sdContext, addr, addrlen);
    SLNETSOCK_NORMALIZE_RET_VAL(retVal,SLNETSOCK_ERR_SOCKBIND_FAILED);

    return retVal;
}


//*****************************************************************************
//
// SlNetSock_listen - Listen for connections on a socket
//
//*****************************************************************************
int32_t SlNetSock_listen(int16_t sd, int16_t backlog)
{
    int32_t    retVal = SLNETERR_RET_CODE_OK;
    int16_t    realSd;
    SlNetIf_t *netIf;
    void      *sdContext;

    /* Check if the sd input exists and return it                            */
    retVal = SlNetSock_getVirtualSdConf(sd, &realSd, NULL, &sdContext, &netIf);

    /* Check if sd found  or if the non mandatory function exists            */
    if (SLNETERR_RET_CODE_OK != retVal)
    {
        return retVal;
    }
    if (NULL == (netIf->ifConf)->sockListen)
    {
        /* Non mandatory function doesn't exists, return error code          */
        return SLNETERR_RET_CODE_DOESNT_SUPPORT_NON_MANDATORY_FXN;
    }

    /* Function exists in the interface of the socket descriptor, dispatch
       the Listen command                                                    */
    retVal = (netIf->ifConf)->sockListen(realSd, sdContext, backlog);
    SLNETSOCK_NORMALIZE_RET_VAL(retVal,SLNETSOCK_ERR_SOCKLISTEN_FAILED);

    return retVal;
}


//*****************************************************************************
//
// SlNetSock_connect - Initiate a connection on a socket
//
//*****************************************************************************
int32_t SlNetSock_connect(int16_t sd, const SlNetSock_Addr_t *addr, SlNetSocklen_t addrlen)
{
    int32_t    retVal = SLNETERR_RET_CODE_OK;
    int16_t    realSd;
    uint8_t    sdFlags;
    SlNetIf_t *netIf;
    void      *sdContext;

    /* Check if the sd input exists and return it                            */
    retVal = SlNetSock_getVirtualSdConf(sd, &realSd, &sdFlags, &sdContext, &netIf);

    /* Check if sd found or if the non mandatory function exists             */
    if (SLNETERR_RET_CODE_OK != retVal)
    {
        return retVal;
    }
    if (NULL == (netIf->ifConf)->sockConnect)
    {
        /* Non mandatory function doesn't exists, return error code          */
        return SLNETERR_RET_CODE_DOESNT_SUPPORT_NON_MANDATORY_FXN;
    }

    /* Function exists in the interface of the socket descriptor, dispatch
       the Connect command                                                   */
    retVal = (netIf->ifConf)->sockConnect(realSd, sdContext, addr, addrlen, sdFlags);
    SLNETSOCK_NORMALIZE_RET_VAL(retVal,SLNETSOCK_ERR_SOCKCONNECT_FAILED);

    return retVal;
}


//*****************************************************************************
//
// SlNetSock_connectUrl - Initiate a connection on a socket by URL
//
//*****************************************************************************
int32_t SlNetSock_connectUrl(int16_t sd, const char *url)
{
    uint32_t            addr[4];
    uint16_t            ipAddrLen;
    SlNetSock_AddrIn_t  localAddr; //address of the server to connect to
    SlNetSocklen_t      localAddrSize;
    int16_t             realSd;
    uint8_t             sdFlags;
    SlNetIf_t          *netIf;
    void               *sdContext;
    int32_t             retVal = SLNETERR_RET_CODE_OK;

    /* Check if the sd input exists and return it                            */
    retVal = SlNetSock_getVirtualSdConf(sd, &realSd, &sdFlags, &sdContext, &netIf);

    /* Check if sd found or if the non mandatory function exists             */
    if (SLNETERR_RET_CODE_OK != retVal)
    {
        return retVal;
    }
    if ( (NULL == (netIf->ifConf)->sockConnect) || (NULL == (netIf->ifConf)->utilGetHostByName) )
    {
        /* Non mandatory function doesn't exists, return error code          */
        return SLNETERR_RET_CODE_DOESNT_SUPPORT_NON_MANDATORY_FXN;
    }

    /* Query DNS for IPv4 address.                                           */
    retVal = (netIf->ifConf)->utilGetHostByName(netIf->ifContext, (char *)url, strlen(url), addr, &ipAddrLen, SLNETSOCK_AF_INET);
    SLNETSOCK_NORMALIZE_RET_VAL(retVal,SLNETUTIL_ERR_UTILGETHOSTBYNAME_FAILED);
    if(retVal < 0)
    {
        /* If call fails, try again for IPv6.                                */
        retVal = (netIf->ifConf)->utilGetHostByName(netIf->ifContext, (char *)url, strlen(url), addr, &ipAddrLen, SLNETSOCK_AF_INET6);
        SLNETSOCK_NORMALIZE_RET_VAL(retVal,SLNETUTIL_ERR_UTILGETHOSTBYNAME_FAILED);
        if(retVal < 0)
        {
            /* if the request failed twice, return error code.               */
            return retVal;
        }
        else
        {
            /* fill the answer fields with IPv6 parameters                   */
            localAddr.sin_family = SLNETSOCK_AF_INET6;
            localAddrSize = sizeof(SlNetSock_AddrIn6_t);
        }
    }
    else
    {
        /* fill the answer fields with IPv4 parameters                       */
        localAddr.sin_family = SLNETSOCK_AF_INET;
        localAddrSize = sizeof(SlNetSock_AddrIn_t);

        /* convert the IPv4 address from host byte order to network byte
           order                                                             */
        localAddr.sin_addr.s_addr = SlNetUtil_htonl(addr[0]);
    }


    /* Function exists in the interface of the socket descriptor, dispatch
       the Connect command                                                   */
    retVal = (netIf->ifConf)->sockConnect(realSd, sdContext, (const SlNetSock_Addr_t *)&localAddr, localAddrSize, sdFlags);
    SLNETSOCK_NORMALIZE_RET_VAL(retVal,SLNETSOCK_ERR_SOCKCONNECT_FAILED);

    return retVal;
}


//*****************************************************************************
//
// SlNetSock_getPeerName - Return address info about the remote side of the
//                         connection
//
//*****************************************************************************
int32_t SlNetSock_getPeerName(int16_t sd, SlNetSock_Addr_t *addr, SlNetSocklen_t *addrlen)
{
    int32_t    retVal = SLNETERR_RET_CODE_OK;
    int16_t    realSd;
    SlNetIf_t *netIf;
    void      *sdContext;

    /* Check if the sd input exists and return it                            */
    retVal = SlNetSock_getVirtualSdConf(sd, &realSd, NULL, &sdContext, &netIf);

    /* Check if sd found or if the non mandatory function exists             */
    if (SLNETERR_RET_CODE_OK != retVal)
    {
        return retVal;
    }
    if (NULL == (netIf->ifConf)->sockGetPeerName)
    {
        /* Non mandatory function doesn't exists, return error code          */
        return SLNETERR_RET_CODE_DOESNT_SUPPORT_NON_MANDATORY_FXN;
    }

    /* Function exists in the interface of the socket descriptor, dispatch
       the GetPeerName command                                               */
    retVal = (netIf->ifConf)->sockGetPeerName(realSd, sdContext, addr, addrlen);
    SLNETSOCK_NORMALIZE_RET_VAL(retVal,SLNETSOCK_ERR_SOCKGETPEERNAME_FAILED);

    return retVal;
}


//*****************************************************************************
//
// SlNetSock_getSockName - Returns the local address info of the socket
//                         descriptor
//
//*****************************************************************************
int32_t SlNetSock_getSockName(int16_t sd, SlNetSock_Addr_t *addr, SlNetSocklen_t *addrlen)
{
    int32_t    retVal = SLNETERR_RET_CODE_OK;
    int16_t    realSd;
    SlNetIf_t *netIf;
    void      *sdContext;

    /* Check if the sd input exists and return it                            */
    retVal = SlNetSock_getVirtualSdConf(sd, &realSd, NULL, &sdContext, &netIf);

    /* Check if sd found or if the non mandatory function exists             */
    if (SLNETERR_RET_CODE_OK != retVal)
    {
        return retVal;
    }
    if (NULL == (netIf->ifConf)->sockGetLocalName)
    {
        /* Non mandatory function doesn't exists, return error code          */
        return SLNETERR_RET_CODE_DOESNT_SUPPORT_NON_MANDATORY_FXN;
    }

    /* Function exists in the interface of the socket descriptor, dispatch
       the GetLocalName command                                              */
    retVal = (netIf->ifConf)->sockGetLocalName(realSd, sdContext, addr, addrlen);
    SLNETSOCK_NORMALIZE_RET_VAL(retVal,SLNETSOCK_ERR_SOCKGETLOCALNAME_FAILED);

    return retVal;
}


//*****************************************************************************
//
// SlNetSock_select - Monitor socket activity
//
//*****************************************************************************
int32_t SlNetSock_select(int16_t nsds, SlNetSock_SdSet_t *readsds, SlNetSock_SdSet_t *writesds, SlNetSock_SdSet_t *exceptsds, SlNetSock_Timeval_t *timeout)
{
    int32_t            retVal     = SLNETERR_RET_CODE_OK;
    int32_t            sdIndex    = 0;
    int16_t            realSd;
    int16_t            ifNsds     = 0;
    bool               skipToNext = true;
    SlNetIf_t         *firstIfID  = NULL;
    SlNetIf_t         *netIf;
    void              *sdContext;
    SlNetSock_SdSet_t  ifReadsds;
    SlNetSock_SdSet_t  ifWritesds;
    SlNetSock_SdSet_t  ifExceptsds;
    SlNetSock_RealToVirtualIndexes_t *tempNode        = NULL;
    SlNetSock_RealToVirtualIndexes_t *realSdToVirtual = NULL;

    /* Initialize sds parameters                                             */
    SlNetSock_sdsClrAll(&ifReadsds);
    SlNetSock_sdsClrAll(&ifWritesds);
    SlNetSock_sdsClrAll(&ifExceptsds);

    /* Run over all possible sd indexes                                      */
    while ( sdIndex < nsds )
    {
        /* get interface ID from the socket identifier                       */
        retVal = SlNetSock_getVirtualSdConf(sdIndex, &realSd, NULL, &sdContext, &netIf);

        /* Check if sd found                                                 */
        if (SLNETERR_RET_CODE_OK == retVal)
        {
            /* Check if sdIndex is set in read/write/except virtual sd sets,
               if so, set the real sd set and set skipToNext to false
               for further use                                               */
            if (SlNetSock_sdsIsSet(sdIndex, readsds) == 1)
            {
                SlNetSock_sdsSet(realSd, &ifReadsds);
                skipToNext = false;
            }
            if (SlNetSock_sdsIsSet(sdIndex, writesds) == 1)
            {
                SlNetSock_sdsSet(realSd, &ifWritesds);
                skipToNext = false;
            }
            if (SlNetSock_sdsIsSet(sdIndex, exceptsds) == 1)
            {
                SlNetSock_sdsSet(realSd, &ifExceptsds);
                skipToNext = false;
            }

            if (false == skipToNext)
            {

                /* Create a node which stores the relation between the virtual
                   sd index and the real sd index and connect it to the list */
                tempNode = (SlNetSock_RealToVirtualIndexes_t *)malloc(sizeof(SlNetSock_RealToVirtualIndexes_t));

                /* Check if the malloc function failed                       */
                if (NULL == tempNode)
                {
                    firstIfID = NULL;
                    retVal = SLNETERR_RET_CODE_MALLOC_ERROR;
                    break;
                }

                tempNode->realSd    = realSd;
                tempNode->virtualSd = sdIndex;
                tempNode->next      = realSdToVirtual;
                realSdToVirtual     = tempNode;

                /* Check if the stored interface ID is different from the
                   interface ID of the socket                                */
                if (netIf != firstIfID)
                {
                    /* Check if the stored interface ID is still initialized */
                    if (NULL == firstIfID)
                    {
                        /* Store the interface ID                            */
                        firstIfID = netIf;
                    }
                    else
                    {
                        /* Different interface ID from the stored interface
                           ID, that means more than one interface supplied
                           in the read sd set                                */
                        firstIfID = NULL;
                        break;
                    }
                }
                if (ifNsds <= realSd)
                {
                    ifNsds = realSd + 1;
                }
                skipToNext = true;
            }
        }

        /* Continue to next sd index                                         */
        sdIndex++;
    }

    /* Check if non mandatory function exists                                */
    if ( (NULL != firstIfID) && (NULL != (firstIfID->ifConf)->sockSelect) )
    {
        /* Function exists in the interface of the socket descriptor,
           dispatch the Select command                                       */
        retVal = (firstIfID->ifConf)->sockSelect(firstIfID->ifContext, ifNsds, &ifReadsds, &ifWritesds, &ifExceptsds, timeout);
        SLNETSOCK_NORMALIZE_RET_VAL(retVal,SLNETSOCK_ERR_SOCKSELECT_FAILED);

        /* Clear all virtual sd sets before setting the sockets that are set */
        SlNetSock_sdsClrAll(readsds);
        SlNetSock_sdsClrAll(writesds);
        SlNetSock_sdsClrAll(exceptsds);

        /* check if the sockselect returned positive value, this value
           represents how many socket descriptors are set                    */
        if (retVal > 0)
        {
            /* Run over all the socket descriptors in the list and check if
               the sockSelect function set them, if so, set the virtual
               socket descriptors sets                                       */
            tempNode = realSdToVirtual;
            while ( NULL != tempNode )
            {
                if (SlNetSock_sdsIsSet(tempNode->realSd, &ifReadsds) == 1)
                {
                    SlNetSock_sdsSet(tempNode->virtualSd, readsds);
                }
                if (SlNetSock_sdsIsSet(tempNode->realSd, &ifWritesds) == 1)
                {
                    SlNetSock_sdsSet(tempNode->virtualSd, writesds);
                }
                if (SlNetSock_sdsIsSet(tempNode->realSd, &ifExceptsds) == 1)
                {
                    SlNetSock_sdsSet(tempNode->virtualSd, exceptsds);
                }
                tempNode = tempNode->next;
            }
        }
    }
    else
    {
        if ( SLNETERR_RET_CODE_MALLOC_ERROR != retVal )
        {
            /* Validation failed, return error code                              */
            retVal = SLNETERR_RET_CODE_INVALID_INPUT;
        }
    }

    /* List isn't needed anymore, free it from the head of the list          */
    while (NULL != realSdToVirtual)
    {
        tempNode = realSdToVirtual->next;
        free(realSdToVirtual);
        realSdToVirtual = tempNode;
    }

    return retVal;
}


//*****************************************************************************
//
// SlNetSock_sdsSet - SlNetSock_select's SlNetSock_SdSet_t SET function
//
//*****************************************************************************
int32_t SlNetSock_sdsSet(int16_t sd, SlNetSock_SdSet_t *sdset)
{
    int sdArrayIndex;

    /* Validation check                                                      */
    if ( (NULL == sdset) || (sd >= SLNETSOCK_MAX_CONCURRENT_SOCKETS) )
    {
        /* Validation failed, return error code                              */
        return SLNETERR_RET_CODE_INVALID_INPUT;
    }

    /* Check in which sdset index the input socket exists                    */
    sdArrayIndex = (sd / SLNETSOCK_SIZEOF_ONE_SDSETBITMAP_SLOT_IN_BITS);

    /* Set the socket in the sd set                                          */
    sdset->sdSetBitmap[sdArrayIndex] |= ( 1 << (sd % SLNETSOCK_SIZEOF_ONE_SDSETBITMAP_SLOT_IN_BITS) );

    return SLNETERR_RET_CODE_OK;

}


//*****************************************************************************
//
// SlNetSock_sdsClr - SlNetSock_select's SlNetSock_SdSet_t CLR function
//
//*****************************************************************************
int32_t SlNetSock_sdsClr(int16_t sd, SlNetSock_SdSet_t *sdset)
{
    int sdArrayIndex;

    /* Validation check                                                      */
    if ( (NULL == sdset) || (sd >= SLNETSOCK_MAX_CONCURRENT_SOCKETS) )
    {
        /* Validation failed, return error code                              */
        return SLNETERR_RET_CODE_INVALID_INPUT;
    }
    /* Check in which sdset index the input socket exists                    */
    sdArrayIndex = (sd / SLNETSOCK_SIZEOF_ONE_SDSETBITMAP_SLOT_IN_BITS);

    /* Set the socket in the sd set                                          */
    sdset->sdSetBitmap[sdArrayIndex] &= ~( 1 << (sd % SLNETSOCK_SIZEOF_ONE_SDSETBITMAP_SLOT_IN_BITS) );

    return SLNETERR_RET_CODE_OK;
}


//*****************************************************************************
//
// SlNetSock_sdsClrAll - SlNetSock_select's SlNetSock_SdSet_t ZERO function
//
//*****************************************************************************
int32_t SlNetSock_sdsClrAll(SlNetSock_SdSet_t *sdset)
{
    int sdArrayIndex;

    /* Validation check                                                      */
    if (NULL == sdset)
    {
        /* Validation failed, return error code                              */
        return SLNETERR_RET_CODE_INVALID_INPUT;
    }

    /* Check the size of the sdArrayIndex                                    */
    sdArrayIndex = (((sizeof(sdset)*8)-1) / SLNETSOCK_SIZEOF_ONE_SDSETBITMAP_SLOT_IN_BITS);

    while (sdArrayIndex >= 0)
    {
        /* Set to 0 the sd set                                               */
        sdset->sdSetBitmap[sdArrayIndex] = 0;
        sdArrayIndex --;
    }

    return SLNETERR_RET_CODE_OK;
}



//*****************************************************************************
//
// SlNetSock_sdsIsSet - SlNetSock_select's SlNetSock_SdSet_t ISSET function
//
//*****************************************************************************
int32_t SlNetSock_sdsIsSet(int16_t sd, SlNetSock_SdSet_t *sdset)
{
    int sdArrayIndex;

    /* Validation check                                                      */
    if ( (NULL == sdset) || (sd >= SLNETSOCK_MAX_CONCURRENT_SOCKETS) )
    {
        /* Validation failed, return error code                              */
        return SLNETERR_RET_CODE_INVALID_INPUT;
    }

    /* Check in which sdset index the input socket exists                    */
    sdArrayIndex = (sd / SLNETSOCK_SIZEOF_ONE_SDSETBITMAP_SLOT_IN_BITS);

    /* Check if the sd is set in the sdSetBitmap                             */
    if ( (sdset->sdSetBitmap[sdArrayIndex]) & (1 << (sd % SLNETSOCK_SIZEOF_ONE_SDSETBITMAP_SLOT_IN_BITS)) )
    {
        /* Bit is set, return 1                                              */
        return 1;
    }
    else
    {
        /* Bit is not set, return 0                                          */
        return 0;
    }
}


//*****************************************************************************
//
// SlNetSock_setOpt - Set socket options
//
//*****************************************************************************
int32_t SlNetSock_setOpt(int16_t sd, int16_t level, int16_t optname, void *optval, SlNetSocklen_t optlen)
{
    int32_t    retVal = SLNETERR_RET_CODE_OK;
    int16_t    realSd;
    SlNetIf_t *netIf;
    void      *sdContext;

    /* Check if the sd input exists and return it                            */
    retVal = SlNetSock_getVirtualSdConf(sd, &realSd, NULL, &sdContext, &netIf);

    /* Check if sd found or if the non mandatory function exists             */
    if (SLNETERR_RET_CODE_OK != retVal)
    {
        return retVal;
    }
    if (NULL == (netIf->ifConf)->sockSetOpt)
    {
        /* Non mandatory function doesn't exists, return error code          */
        return SLNETERR_RET_CODE_DOESNT_SUPPORT_NON_MANDATORY_FXN;
    }

    /* Function exists in the interface of the socket descriptor, dispatch
       the SetOpt command                                                    */
    retVal = (netIf->ifConf)->sockSetOpt(realSd, sdContext, level, optname, optval, optlen);
    SLNETSOCK_NORMALIZE_RET_VAL(retVal,SLNETSOCK_ERR_SOCKSETOPT_FAILED);

    return retVal;
}


//*****************************************************************************
//
// SlNetSock_getOpt - Get socket options
//
//*****************************************************************************
int32_t SlNetSock_getOpt(int16_t sd, int16_t level, int16_t optname, void *optval, SlNetSocklen_t *optlen)
{
    int32_t    retVal = SLNETERR_RET_CODE_OK;
    int16_t    realSd;
    SlNetIf_t *netIf;
    void      *sdContext;

    /* Check if the sd input exists and return it                            */
    retVal = SlNetSock_getVirtualSdConf(sd, &realSd, NULL, &sdContext, &netIf);

    /* Check if sd found or if the non mandatory function exists             */
    if (SLNETERR_RET_CODE_OK != retVal)
    {
        return retVal;
    }
    if (NULL == (netIf->ifConf)->sockGetOpt)
    {
        /* Non mandatory function doesn't exists, return error code          */
        return SLNETERR_RET_CODE_DOESNT_SUPPORT_NON_MANDATORY_FXN;
    }

    /* Function exists in the interface of the socket descriptor, dispatch
       the GetOpt command                                                    */
    retVal = (netIf->ifConf)->sockGetOpt(realSd, sdContext, level, optname, optval, optlen);
    SLNETSOCK_NORMALIZE_RET_VAL(retVal,SLNETSOCK_ERR_SOCKGETOPT_FAILED);

    return retVal;
}


//*****************************************************************************
//
// SlNetSock_recv - Read data from TCP socket
//
//*****************************************************************************
int32_t SlNetSock_recv(int16_t sd, void *buf, uint32_t len, uint32_t flags)
{
    int32_t    retVal = SLNETERR_RET_CODE_OK;
    int16_t    realSd;
    uint8_t    sdFlags;
    SlNetIf_t *netIf;
    void      *sdContext;

    /* Check if the sd input exists and return it                            */
    retVal = SlNetSock_getVirtualSdConf(sd, &realSd, &sdFlags, &sdContext, &netIf);

    /* Check if sd found or if the non mandatory function exists             */
    if (SLNETERR_RET_CODE_OK != retVal)
    {
        return retVal;
    }
    if (NULL == (netIf->ifConf)->sockRecv)
    {
        /* Non mandatory function doesn't exists, return error code          */
        return SLNETERR_RET_CODE_DOESNT_SUPPORT_NON_MANDATORY_FXN;
    }
    if ((flags & 0xff000000) != 0)
    {
        /* invalid user flags                                                */
        return SLNETERR_BSD_EOPNOTSUPP;
    }

    /* Macro which merge the 8bit security flags to the upper bits of the
       32bit input flags                                                     */
    MERGE_SEC_INTO_INPUT_FLAGS(flags, sdFlags);

    /* Function exists in the interface of the socket descriptor, dispatch
       the Recv command                                                      */
    retVal = (netIf->ifConf)->sockRecv(realSd, sdContext, buf, len, flags);
    SLNETSOCK_NORMALIZE_RET_VAL(retVal,SLNETSOCK_ERR_SOCKRECV_FAILED);

    return retVal;
}


//*****************************************************************************
//
// SlNetSock_recvFrom - Read data from socket
//
//*****************************************************************************
int32_t SlNetSock_recvFrom(int16_t sd, void *buf, uint32_t len, uint32_t flags, SlNetSock_Addr_t *from, SlNetSocklen_t *fromlen)
{
    int32_t    retVal = SLNETERR_RET_CODE_OK;
    int16_t    realSd;
    uint8_t    sdFlags;
    SlNetIf_t *netIf;
    void      *sdContext;

    /* Check if the sd input exists and return it                            */
    retVal = SlNetSock_getVirtualSdConf(sd, &realSd, &sdFlags, &sdContext, &netIf);

    /* Check if sd found                                                     */
    if (SLNETERR_RET_CODE_OK != retVal)
    {
        /* Validation failed, return error code                              */
        return SLNETERR_RET_CODE_INVALID_INPUT;
    }
    if ((flags & 0xff000000) != 0)
    {
        /* invalid user flags                                                */
        return SLNETERR_BSD_EOPNOTSUPP;
    }

    /* Macro which merge the 8bit security flags to the upper bits of the
       32bit input flags                                                     */
    MERGE_SEC_INTO_INPUT_FLAGS(flags, sdFlags);

    /* Function exists in the interface of the socket descriptor, dispatch
       the RecvFrom command                                                  */
    retVal = (netIf->ifConf)->sockRecvFrom(realSd, sdContext, buf, len, flags, from, fromlen);
    SLNETSOCK_NORMALIZE_RET_VAL(retVal,SLNETSOCK_ERR_SOCKRECVFROM_FAILED);

    return retVal;
}


//*****************************************************************************
//
// SlNetSock_send - Write data to TCP socket
//
//*****************************************************************************
int32_t SlNetSock_send(int16_t sd, const void *buf, uint32_t len, uint32_t flags)
{
    int32_t    retVal = SLNETERR_RET_CODE_OK;
    int16_t    realSd;
    uint8_t    sdFlags;
    SlNetIf_t *netIf;
    void      *sdContext;

    /* Check if the sd input exists and return it                            */
    retVal = SlNetSock_getVirtualSdConf(sd, &realSd, &sdFlags, &sdContext, &netIf);

    /* Check if sd found or if the non mandatory function exists             */
    if (SLNETERR_RET_CODE_OK != retVal)
    {
        return retVal;
    }
    if (NULL == (netIf->ifConf)->sockSend)
    {
        /* Non mandatory function doesn't exists, return error code          */
        return SLNETERR_RET_CODE_DOESNT_SUPPORT_NON_MANDATORY_FXN;
    }
    if ((flags & 0xff000000) != 0)
    {
        /* invalid user flags                                                */
        return SLNETERR_BSD_EOPNOTSUPP;
    }

    /* Macro which merge the 8bit security flags to the upper bits of the
       32bit input flags                                                     */
    MERGE_SEC_INTO_INPUT_FLAGS(flags, sdFlags);

    /* Function exists in the interface of the socket descriptor, dispatch
       the Send command                                                      */
    retVal = (netIf->ifConf)->sockSend(realSd, sdContext, buf, len, flags);
    SLNETSOCK_NORMALIZE_RET_VAL(retVal,SLNETSOCK_ERR_SOCKSEND_FAILED);

    return retVal;
}


//*****************************************************************************
//
// SlNetSock_sendTo - Write data to socket
//
//*****************************************************************************
int32_t SlNetSock_sendTo(int16_t sd, const void *buf, uint32_t len, uint32_t flags, const SlNetSock_Addr_t *to, SlNetSocklen_t tolen)
{
    int32_t    retVal = SLNETERR_RET_CODE_OK;
    int16_t    realSd;
    uint8_t    sdFlags;
    SlNetIf_t *netIf;
    void      *sdContext;

    /* Check if the sd input exists and return it                            */
    retVal = SlNetSock_getVirtualSdConf(sd, &realSd, &sdFlags, &sdContext, &netIf);

    /* Check if sd found                                                     */
    if (SLNETERR_RET_CODE_OK != retVal)
    {
        /* Validation failed, return error code                              */
        return SLNETERR_RET_CODE_INVALID_INPUT;
    }
    if ((flags & 0xff000000) != 0)
    {
        /* invalid user flags                                                */
        return SLNETERR_BSD_EOPNOTSUPP;
    }

    /* Macro which merge the 8bit security flags to the upper bits of the
       32bit input flags                                                     */
    MERGE_SEC_INTO_INPUT_FLAGS(flags, sdFlags);

    /* Function exists in the interface of the socket descriptor, dispatch
       the SendTo command                                                    */
    retVal = (netIf->ifConf)->sockSendTo(realSd, sdContext, buf, len, flags, to, tolen);
    SLNETSOCK_NORMALIZE_RET_VAL(retVal,SLNETSOCK_ERR_SOCKSENDTO_FAILED);

    return retVal;
}


//*****************************************************************************
//
// SlNetSock_getIfID - Get interface ID from socket descriptor (sd)
//
//*****************************************************************************
int32_t SlNetSock_getIfID(uint16_t sd)
{
    int32_t    retVal = SLNETERR_RET_CODE_OK;
    int16_t    realSd;
    SlNetIf_t *netIf;

    /* Check if the sd input exists and return it                            */
    retVal = SlNetSock_getVirtualSdConf(sd, &realSd, NULL, NULL, &netIf);

    /* Check if sd found                                                     */
    if (SLNETERR_RET_CODE_OK != retVal)
    {
        /* Validation failed, return error code                              */
        return SLNETERR_RET_CODE_INVALID_INPUT;
    }

    /* Return interface identifier                                           */
    return netIf->ifID;
}


//*****************************************************************************
//
// SlNetSock_secAttribCreate - Creates a security attributes object
//
//*****************************************************************************
SlNetSockSecAttrib_t *SlNetSock_secAttribCreate(void)
{
    SlNetSockSecAttrib_t *secAttribHandler;

    /* Allocate and initialize dynamic memory for security attribute handler */
    secAttribHandler = (SlNetSockSecAttrib_t *)calloc(1, sizeof(SlNetSockSecAttrib_t));

    /* Check if the calloc function failed                                   */
    if (NULL == secAttribHandler)
    {
        /* Function failed, return error code                                */
        return NULL;
    }

    return (secAttribHandler);
}


//*****************************************************************************
//
// SlNetSock_secAttribDelete - Deletes a security attributes object
//
//*****************************************************************************
int32_t SlNetSock_secAttribDelete(SlNetSockSecAttrib_t *secAttrib)
{
    SlNetSock_SecAttribNode_t *nextSecAttrib;
    SlNetSock_SecAttribNode_t *tempSecAttrib;

    /* Check if the input doesn't exist                                      */
    if (NULL == secAttrib)
    {
        /* Function failed, return error code                                */
        return SLNETERR_RET_CODE_INVALID_INPUT;
    }
    else
    {
        nextSecAttrib = (SlNetSock_SecAttribNode_t *)*secAttrib;
        tempSecAttrib = (SlNetSock_SecAttribNode_t *)*secAttrib;
    }

    /* Free all SecAttrib list nodes                                         */
    while (NULL != nextSecAttrib)
    {
        nextSecAttrib = tempSecAttrib->next;
        free((void *)tempSecAttrib);
        tempSecAttrib = nextSecAttrib;
    }

    free(secAttrib);

    return SLNETERR_RET_CODE_OK;
}


//*****************************************************************************
//
// SlNetSock_secAttribSet - used to set a security attribute of a security
//                          attributes object
//
//*****************************************************************************
int32_t SlNetSock_secAttribSet(SlNetSockSecAttrib_t *secAttrib, SlNetSockSecAttrib_e attribName, void *val, uint16_t len)
{
    SlNetSock_SecAttribNode_t *secAttribObj;

    /* Check if the inputs doesn't exists or not valid                       */
    if ( (NULL == secAttrib) || (0 == len) || (NULL == val) )
    {
        /* Function failed, return error code                                */
        return SLNETERR_RET_CODE_INVALID_INPUT;
    }

    /* Allocate dynamic memory for security attribute handler                */
    secAttribObj = (SlNetSock_SecAttribNode_t *)malloc(sizeof(SlNetSock_SecAttribNode_t));

    /* Check if the malloc function failed                                   */
    if (NULL == secAttribObj)
    {
        /* Function failed, return error code                                */
        return SLNETERR_RET_CODE_MALLOC_ERROR;
    }

    /* Set the inputs in the allocated security attribute handler            */
    secAttribObj->attribName    = attribName;
    secAttribObj->attribBuff    = val;
    secAttribObj->attribBuffLen = len;
    secAttribObj->next          = *secAttrib;

    /* Connect the security attribute to the secAttrib list                  */
    *secAttrib = secAttribObj;



    return SLNETERR_RET_CODE_OK;
}


//*****************************************************************************
//
// SlNetSock_startSec - Start a security session on an opened socket
//
//*****************************************************************************
int32_t SlNetSock_startSec(int16_t sd, SlNetSockSecAttrib_t *secAttrib, uint8_t flags)
{
    int32_t    retVal = SLNETERR_RET_CODE_OK;
    int16_t    realSd;
    uint8_t    sdFlags;
    SlNetIf_t *netIf;
    void      *sdContext;

    /* Check if the sd input exists and return it                            */
    retVal = SlNetSock_getVirtualSdConf(sd, &realSd, &sdFlags, &sdContext, &netIf);

    /* Check if sd found or if the non mandatory function exists             */
    if (SLNETERR_RET_CODE_OK != retVal)
    {
        return retVal;
    }
    if (NULL == (netIf->ifConf)->sockstartSec)
    {
        /* Non mandatory function doesn't exists, return error code          */
        return SLNETERR_RET_CODE_DOESNT_SUPPORT_NON_MANDATORY_FXN;
    }
    /* StartSec function called, set bit                                     */
    sdFlags |= flags;
    /* Function exists in the interface of the socket descriptor, dispatch
       the startSec command                                                  */
    retVal = (netIf->ifConf)->sockstartSec(realSd, sdContext, secAttrib, flags);
    SLNETSOCK_NORMALIZE_RET_VAL(retVal,SLNETSOCK_ERR_SOCKSTARTSEC_FAILED);

    return retVal;
}
