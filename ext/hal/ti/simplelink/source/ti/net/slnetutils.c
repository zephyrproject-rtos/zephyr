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

#include <unistd.h>  /* needed? */
#include <string.h>
#include <stdlib.h>

#include <ti/net/slnetutils.h>
#include <ti/net/slnetif.h>
#include <ti/net/slneterr.h>

/*****************************************************************************/
/* Macro declarations                                                        */
/*****************************************************************************/

#define SLNETUTIL_NORMALIZE_NEEDED 0
#if SLNETUTIL_NORMALIZE_NEEDED
    #define SLNETUTIL_NORMALIZE_RET_VAL(retVal,err) ((retVal < 0)?(retVal = err):(retVal))
#else
    #define SLNETUTIL_NORMALIZE_RET_VAL(retVal,err)
#endif

/* Size needed for getaddrinfo mem allocation */
#define SLNETUTIL_ADDRINFO_ALLOCSZ ((sizeof(SlNetUtil_addrInfo_t)) + \
        (sizeof(SlNetSock_AddrIn6_t)))

/* Cap number of mem allocations in case of large number of results from DNS */
#define SLNETUTIL_ADDRINFO_MAX_DNS_NODES 10

#define SLNETUTIL_DNSBUFSIZE ((SLNETUTIL_ADDRINFO_MAX_DNS_NODES) * \
        (sizeof(uint32_t)))

#define LL_PREFIX 0xFE80

/*****************************************************************************/
/* Structure/Enum declarations                                               */
/*****************************************************************************/


/*****************************************************************************/
/* Function prototypes                                                       */
/*****************************************************************************/

static int32_t SlNetUtil_UTOA(uint16_t value, char * string, uint16_t base);
static int32_t SlNetUtil_bin2StrIpV4(SlNetSock_InAddr_t *binaryAddr, char *strAddr, uint16_t strAddrLen);
static int32_t SlNetUtil_bin2StrIpV6(SlNetSock_In6Addr_t *binaryAddr, char *strAddr, uint16_t strAddrLen);
static int32_t SlNetUtil_strTok(char **string, char *returnstring, const char *delimiter);
static int32_t SlNetUtil_str2BinIpV4(char *strAddr, SlNetSock_InAddr_t *binaryAddr);
static int32_t SlNetUtil_str2BinIpV6(char *strAddr, SlNetSock_In6Addr_t *binaryAddr);

/* Local SlNetUtil_getAddrInfo utility functions */
static SlNetUtil_addrInfo_t *setAddrInfo(uint16_t ifID, SlNetSock_Addr_t *addr,
        int family, const char *service, const SlNetUtil_addrInfo_t *hints);

static SlNetUtil_addrInfo_t *createAddrInfo(uint16_t ifID,
        SlNetSock_Addr_t *addr, int family, const char *service, int flags,
        int socktype, int protocol);

static int mergeLists(SlNetUtil_addrInfo_t **curList,
        SlNetUtil_addrInfo_t **newList);

//*****************************************************************************
//
// SlNetUtil_init - Initialize the slnetutil module
//
//*****************************************************************************
int32_t SlNetUtil_init(int32_t flags)
{
    return 0;
}


//*****************************************************************************
//  SlNetUtil_gaiStrErr
//*****************************************************************************

/*
 * Error codes for gai_strerror
 * The order of this array MUST match the numbering of the SLNETUTIL_EAI_CODE
 * defines in <ti/net/slneterr.h>
 */
static const char *strErrorMsgs[] =
{
    "Temporary failure in name resolution",         /* SLNETUTIL_EAI_AGAIN */
    "Bad value for ai_flags",                       /* SLNETUTIL_EAI_BADFLAGS */
    "Non-recoverable failure in name resolution",   /* SLNETUTIL_EAI_FAIL */
    "ai_family not supported",                      /* SLNETUTIL_EAI_FAMILY */
    "Memory allocation failure",                    /* SLNETUTIL_EAI_MEMORY */
    "Node or service not known, "                   /* SLNETUTIL_EAI_NONAME */
    "or both node and service are NULL",
    "Service (port number) not supported "          /* SLNETUTIL_EAI_SERVICE */
    "for ai_socktype",
    "ai_socktype not supported",                    /* SLNETUTIL_EAI_SOCKTYPE */
    "System error",                                 /* SLNETUTIL_EAI_SYSTEM */
    "An argument buffer overflowed",                /* SLNETUTIL_EAI_OVERFLOW */
    "Address family for node not supported"         /* SLNETUTIL_EAI_ADDRFAMILY */
};

const char *SlNetUtil_gaiStrErr(int32_t errorCode)
{
    /*
     * Error codes are negative starting with -3121 so
     * ~(errorCode - SLNETUTIL_EAI_BASE) takes the one's compliment of the code
     * minus the base of the error codes to convert the code into a positive
     * value that matches the StrerrorMsgs array index.
     */
    int msgsIndex = ~(errorCode - SLNETUTIL_EAI_BASE);
    int msgsRange = sizeof(strErrorMsgs) / sizeof(strErrorMsgs[0]);

    if ((msgsIndex < msgsRange) && (msgsIndex >= 0))
    {
        return strErrorMsgs[msgsIndex];
    }
    else
    {
        return "Unknown error";
    }
}

//*****************************************************************************
//
// SlNetUtil_getHostByName - Obtain the IP Address of machine on network, by
//                           machine name
//
//*****************************************************************************
int32_t SlNetUtil_getHostByName(uint32_t ifBitmap, char *name, const uint16_t nameLen, uint32_t *ipAddr, uint16_t *ipAddrLen, const uint8_t family)
{
    uint16_t origIpAddrLen;
    SlNetIf_t *netIf;
    int32_t    retVal;

    /* When ifBitmap is 0, that means automatic selection of all interfaces
       is required, enable all bits in ifBitmap                              */
    if (0 == ifBitmap)
    {
        ifBitmap = ~ifBitmap;
    }

    /*
     * Save original value of ipAddrLen. If DNS resolution fails, ipAddrLen
     * will be overwritten with zero, which is problematic for next iteration
     * of the while loop.
     */
    origIpAddrLen = *ipAddrLen;

    /* This loop tries to run get host by name on the required interface
       only if it in state enable and connected status
       When multiple interfaces, in addition to the enable and connected it
       will try to run the function on the interface from the highest
       priority until an answer will return or until no interfaces left      */
    do
    {
        /* Search for the highest priority interface according to the
           ifBitmap and the queryFlags                                       */
        netIf = SlNetIf_queryIf(ifBitmap, SLNETIF_QUERY_IF_STATE_BIT | SLNETIF_QUERY_IF_CONNECTION_STATUS_BIT);

        /* Check if the function returned NULL or the requested interface
           exists                                                            */
        if ( (NULL == netIf) || ( NULL == (netIf->ifConf)->utilGetHostByName) )
        {
            /* Interface doesn't exists, return error code                   */
            return SLNETERR_RET_CODE_INVALID_INPUT;
        }
        else
        {
            /* Disable the ifID bit from the ifBitmap after finding the netIf*/
            ifBitmap &= ~(netIf->ifID);

            /* Interface exists, return interface IP address                 */
            retVal = (netIf->ifConf)->utilGetHostByName(netIf->ifContext, name, nameLen, ipAddr, ipAddrLen, family);
            SLNETUTIL_NORMALIZE_RET_VAL(retVal, SLNETUTIL_ERR_UTILGETHOSTBYNAME_FAILED);

            /* Check retVal for error codes                                  */
            if (retVal < SLNETERR_RET_CODE_OK)
            {
                /*
                 * utilGetHostByName failed. Restore the size of the ipAddr
                 * array and continue to the next ifID
                 */
                *ipAddrLen = origIpAddrLen;
                continue;
            }
            else
            {
                /* Return success                                            */
                return netIf->ifID;
            }
        }
    }while ( ifBitmap > 0 );
    /* Interface doesn't exists, return error code                   */
    return SLNETERR_RET_CODE_INVALID_INPUT;
}

//*****************************************************************************
// SlNetUtil_getAddrInfo
//*****************************************************************************
int32_t SlNetUtil_getAddrInfo(uint16_t ifID, const char *node,
        const char *service, const struct SlNetUtil_addrInfo_t *hints,
        struct SlNetUtil_addrInfo_t **res)
{
    int i;
    int retval = 0;
    int resolved = 0;
    int numNodes = 0;
    char *buffer = NULL;
    char *currAddr = NULL;
    uint16_t numIpAddrs;
    int32_t selectedIfId;
    int32_t ipv4DnsError = 0;
    int32_t ipv6DnsError = 0;

    SlNetUtil_addrInfo_t *ai = NULL;
    SlNetUtil_addrInfo_t *aiTmp = NULL;
    SlNetSock_AddrIn_t sin;
    SlNetSock_AddrIn6_t sin6;

    /* check args passed in for errors */
    if (!node && !service) {
        /* Error: node and service args cannot both be NULL */
        return (SLNETUTIL_EAI_NONAME);
    }

    if (!service) {
        service = "0";
    }

    if (!res) {
        /* Error: res cannot be NULL */
        return (SLNETUTIL_EAI_NONAME);
    }

    if (!hints) {
        /* User passed NULL hints. Create a hints for them with all 0 values */
        static const SlNetUtil_addrInfo_t defHints = {
            0,                   /* ai_flags */
            SLNETSOCK_AF_UNSPEC, /* ai_family */
            0,                   /* ai_socktype */
            0,                   /* ai_protocol */
            0,                   /* ai_addrlen */
            NULL,                /* ai_addr */
            NULL,                /* ai_canonname */
            NULL                 /* ai_next */
        };
        hints = &defHints;
    }
    else {
        /* Check the user's hints for invalid settings */
        if (hints->ai_socktype != SLNETSOCK_SOCK_STREAM &&
                hints->ai_socktype != SLNETSOCK_SOCK_DGRAM &&
                hints->ai_socktype != 0) {
            /* Error: invalid or unknown socktype */
            return (SLNETUTIL_EAI_SOCKTYPE);
        }
        else if (hints->ai_protocol != SLNETSOCK_PROTO_TCP &&
                hints->ai_protocol != SLNETSOCK_PROTO_UDP &&
                hints->ai_protocol != 0) {
            /* Error: invalid or unknown protocol */
            return (SLNETUTIL_EAI_SOCKTYPE);
        }
        else if ((hints->ai_family != SLNETSOCK_AF_INET) &&
                (hints->ai_family != SLNETSOCK_AF_INET6) &&
                (hints->ai_family != SLNETSOCK_AF_UNSPEC)) {
            /* Error: invalid or unknown family */
            return (SLNETUTIL_EAI_FAMILY);
        }
    }

    if (node) {
        /*
         * Client case. User needs an address structure to call connect() with.
         *
         * Determine what caller has passed to us for 'node'. Should be either:
         *     - an IPv4 address
         *     - an IPv6 address
         *     - or a hostname
         */

        /* Test if 'node' is an IPv4 address */
        retval = SlNetUtil_inetAton(node, &(sin.sin_addr));
        if (retval) {
            /* Ensure address family matches */
            if (hints->ai_family != SLNETSOCK_AF_INET &&
                    hints->ai_family != SLNETSOCK_AF_UNSPEC) {
                return (SLNETUTIL_EAI_ADDRFAMILY);
            }

            /*
             * Create addrinfo struct(s) containing this IPv4 address. If ai
             * is NULL, this will be caught at end of getaddrinfo
             *
             * Pass zero for IF ID. This is a don't care for the case of node
             * being set to an IPv4 address
             */
            ai = setAddrInfo(0, (SlNetSock_Addr_t *)&sin, SLNETSOCK_AF_INET,
                    service, hints);
        }
        else {
            /* 'node' is either an IPv6 address or a hostname (or invalid) */

            /* Test if 'node' is an IPv6 address */
            retval = SlNetUtil_inetPton(SLNETSOCK_AF_INET6, node,
                    &(sin6.sin6_addr));
            if (retval > 0) {
                /* Ensure address family matches */
                if (hints->ai_family != SLNETSOCK_AF_INET6 &&
                        hints->ai_family != SLNETSOCK_AF_UNSPEC) {
                    return (SLNETUTIL_EAI_ADDRFAMILY);
                }

                /*
                 * If we were given a link local address and corresponding IF,
                 * pass the IF number through. It must be used for the scope ID
                 */
                if ((SlNetUtil_ntohs(sin6.sin6_addr._S6_un._S6_u16[0]) ==
                        LL_PREFIX) && ifID != 0) {
                     selectedIfId = ifID;
                }
                else {
                    /*
                     * Set scope ID to zero for these cases:
                     *     - Link local addr and ifID == 0:
                     *         (caller responsibe for setting scope ID)
                     *
                     *     - Non-local addr with ifID == 0:
                     *     - Non-local addr with ifID == 1:
                     *         scope ID not used and should be set to 0
                     */
                    selectedIfId = 0;
                }

                /*
                 * Create addrinfo struct(s) containing this IPv6 address. If
                 * ai is NULL, this will be caught at end of getaddrinfo
                 */
                ai = setAddrInfo(selectedIfId, (SlNetSock_Addr_t *)&sin6,
                        SLNETSOCK_AF_INET6, service, hints);
            }
            else {
                /* Test if 'node' is a host name. Use DNS to resolve it. */

                /*
                 * Per RFC 2553, if node is not a valid numeric address string
                 * and AI_NUMERICHOST is set, return error (and prevent call to
                 * DNS).
                 */
                if (hints->ai_flags & SLNETUTIL_AI_NUMERICHOST) {
                    return (SLNETUTIL_EAI_NONAME);
                }

                buffer = malloc(SLNETUTIL_DNSBUFSIZE);
                if (!buffer) {
                    /* Error: couldn't alloc DNS buffer */
                    return (SLNETUTIL_EAI_MEMORY);
                }

                /* IPv4 DNS lookup */
                if (hints->ai_family == SLNETSOCK_AF_INET ||
                        hints->ai_family == SLNETSOCK_AF_UNSPEC) {
                    /*
                     * Set the size of the buffer to the number of 32-bit IPv4
                     * addresses this buffer can hold
                     */
                    numIpAddrs = SLNETUTIL_DNSBUFSIZE / sizeof(uint32_t);

                    selectedIfId = SlNetUtil_getHostByName(ifID, (char *)node,
                            strlen(node), (uint32_t *)buffer, &numIpAddrs,
                            SLNETSOCK_AF_INET);

                    if (selectedIfId > 0) {

                        /*
                         * Process the results returned by DNS. Upon success,
                         * numIpAddrs contains the number of IP addresses stored
                         * into the buffer
                         */
                        resolved = 1;
                        currAddr = buffer;
                        for (i = 0; i < numIpAddrs &&
                                numNodes < SLNETUTIL_ADDRINFO_MAX_DNS_NODES;
                                i++) {
                            sin.sin_addr.s_addr =
                                    SlNetUtil_htonl(*((uint32_t *)currAddr));
                            /*
                             * Create addrinfo struct(s) containing this IPv4
                             * address. This can return a list with multiple
                             * nodes, depending on hints provided. Empty lists
                             * are handled before returning.
                             *
                             * Pass zero for IF ID. This is a don't care for
                             * the case of node being set to an IPv4 address.
                             */
                            aiTmp = setAddrInfo(0, (SlNetSock_Addr_t *)&sin,
                                    SLNETSOCK_AF_INET, service, hints);

                            if (aiTmp) {
                                /*
                                 * Merge the results into the main list
                                 * for each loop iteration:
                                 */
                                numNodes += mergeLists(&ai, &aiTmp);
                            }

                            /* move to the next IPv4 address */
                            currAddr += sizeof(uint32_t);
                        }
                    }
                    else {
                        /* save the IPv4 error code */
                        ipv4DnsError = selectedIfId;
                    }
                }

                /* IPv6 DNS lookup */
                if (hints->ai_family == SLNETSOCK_AF_INET6 ||
                        hints->ai_family == SLNETSOCK_AF_UNSPEC) {
                    /*
                     * Set the size of the buffer to the number of 128-bit IPv6
                     * addresses this buffer can hold
                     */
                    numIpAddrs =
                            SLNETUTIL_DNSBUFSIZE / sizeof(SlNetSock_In6Addr_t);

                    selectedIfId = SlNetUtil_getHostByName(ifID, (char *)node,
                            strlen(node), (uint32_t *)buffer, &numIpAddrs,
                            SLNETSOCK_AF_INET6);

                    if (selectedIfId > 0) {

                        /*
                         * Process the results returned by DNS. Upon success,
                         * numIpAddrs contains the number of IP addresses stored
                         * into the buffer
                         */
                        resolved = 1;
                        currAddr = buffer;
                        for (i = 0; i < numIpAddrs &&
                                numNodes < SLNETUTIL_ADDRINFO_MAX_DNS_NODES;
                                i++) {

                            /* Copy the IPv6 address out of the buffer */
                            memcpy(&(sin6.sin6_addr), currAddr,
                                    sizeof(SlNetSock_In6Addr_t));

                            /*
                             * Is this address non-local? If so, IF ID is a
                             * don't care
                             */
                            if (sin6.sin6_addr._S6_un._S6_u16[0] != LL_PREFIX) {
                                selectedIfId = 0;
                            }

                            /* Change byte ordering to net byte order */
                            sin6.sin6_addr._S6_un._S6_u16[0] = SlNetUtil_htons(
                                    sin6.sin6_addr._S6_un._S6_u16[0]);
                            sin6.sin6_addr._S6_un._S6_u16[1] = SlNetUtil_htons(
                                    sin6.sin6_addr._S6_un._S6_u16[1]);
                            sin6.sin6_addr._S6_un._S6_u16[2] = SlNetUtil_htons(
                                    sin6.sin6_addr._S6_un._S6_u16[2]);
                            sin6.sin6_addr._S6_un._S6_u16[3] = SlNetUtil_htons(
                                    sin6.sin6_addr._S6_un._S6_u16[3]);
                            sin6.sin6_addr._S6_un._S6_u16[4] = SlNetUtil_htons(
                                    sin6.sin6_addr._S6_un._S6_u16[4]);
                            sin6.sin6_addr._S6_un._S6_u16[5] = SlNetUtil_htons(
                                    sin6.sin6_addr._S6_un._S6_u16[5]);
                            sin6.sin6_addr._S6_un._S6_u16[6] = SlNetUtil_htons(
                                    sin6.sin6_addr._S6_un._S6_u16[6]);
                            sin6.sin6_addr._S6_un._S6_u16[7] = SlNetUtil_htons(
                                    sin6.sin6_addr._S6_un._S6_u16[7]);

                            /*
                             * Create addrinfo struct(s) containing this IPv6
                             * address. This can return a list with multiple
                             * nodes, depending on hints provided. Empty lists
                             * are handled before returning.
                             *
                             * Pass down the appropriate IF number or zero,
                             * depending on whether this address is
                             * local or not
                             */

                            aiTmp = setAddrInfo(selectedIfId,
                                    (SlNetSock_Addr_t *)&sin6,
                                    SLNETSOCK_AF_INET6, service, hints);

                            if (aiTmp) {
                                /*
                                 * Merge the results into the main list
                                 * for each loop iteration:
                                 */
                                numNodes += mergeLists(&ai, &aiTmp);
                            }

                            /* move to the next IPv6 address */
                            currAddr += sizeof(SlNetSock_In6Addr_t);
                        }
                    }
                    else {
                        /* save the IPv6 error code */
                        ipv6DnsError = selectedIfId;
                    }
                }

                free(buffer);

                if (!resolved) {
                    /*
                     * Error: couldn't resolve host name
                     * Translate the SlNetSock error code to a GAI error code.
                     * Give the IPv4 error precedence:
                     */
                    retval = (ipv4DnsError != 0) ? ipv4DnsError : ipv6DnsError;

                    switch (retval) {
                        case SLNETERR_NET_APP_DNS_ALLOC_ERROR:
                            retval = SLNETUTIL_EAI_MEMORY;
                            break;
                        case SLNETERR_NET_APP_DNS_INVALID_FAMILY_TYPE:
                            retval = SLNETUTIL_EAI_FAMILY;
                            break;
                        case SLNETERR_NET_APP_DNS_IPV6_REQ_BUT_IPV6_DISABLED:
                            retval = SLNETUTIL_EAI_SERVICE;
                            break;
                        case SLNETERR_NET_APP_DNS_PARAM_ERROR:
                        case SLNETERR_NET_APP_DNS_QUERY_FAILED:
                        default:
                            retval = SLNETUTIL_EAI_FAIL;
                            break;
                    }
                    return (retval);
                }
            }
        }
    }
    else {
        /* Server case. User needs an address structure to call bind() with. */
        if (hints->ai_family == SLNETSOCK_AF_INET ||
                hints->ai_family == SLNETSOCK_AF_UNSPEC) {
            if (hints->ai_flags & SLNETUTIL_AI_PASSIVE) {
                /* Per RFC 2553, accept connections on any IF */
                sin.sin_addr.s_addr = SlNetUtil_htonl(SLNETSOCK_INADDR_ANY);
            }
            else {
                /* Per RFC 2553, accept connections on loopback IF */
                retval = SlNetUtil_inetPton(SLNETSOCK_AF_INET, "127.0.0.1",
                        &(sin.sin_addr.s_addr));
                if (retval <= 0) {
                    return (SLNETUTIL_EAI_SYSTEM);
                }
            }

            /*
             * Create addrinfo struct(s) containing this IPv4 address. If ai
             * is NULL, this will be caught at end of getaddrinfo
             *
             * Pass zero for IF ID. This is a don't care for
             * the case of setting up a server socket.
             */
            ai = setAddrInfo(0, (SlNetSock_Addr_t *)&sin,
                    SLNETSOCK_AF_INET, service, hints);
        }

        if (hints->ai_family == SLNETSOCK_AF_INET6 ||
                hints->ai_family == SLNETSOCK_AF_UNSPEC) {
            if (hints->ai_flags & SLNETUTIL_AI_PASSIVE) {
                /*
                 * Per RFC 2553, accept connections on any IF
                 * (The IPv6 unspecified address is all zeroes)
                 */
                /* TODO: use in6addr_any, once available (NS-84) */
                memset(&(sin6.sin6_addr), 0, sizeof(SlNetSock_In6Addr_t));
            }
            else {
                /*
                 * Per RFC 2553, accept connections on loopback IF
                 * (The IPv6 loopback address is a 1 preceded by all zeroes)
                 */
                /* TODO: use in6addr_loopback, once available (NS-84) */
                sin6.sin6_addr._S6_un._S6_u32[0] = 0;
                sin6.sin6_addr._S6_un._S6_u32[1] = 0;
                sin6.sin6_addr._S6_un._S6_u32[2] = 0;
                sin6.sin6_addr._S6_un._S6_u32[3] = SlNetUtil_htonl(1);
            }

            /*
             * Create addrinfo struct(s) containing this IPv6 address. If ai
             * is NULL, this will be caught at end of getaddrinfo
             *
             * Pass zero for IF ID. This is a don't care for
             * the case of setting up a server socket.
             */
            aiTmp = setAddrInfo(0, (SlNetSock_Addr_t *)&sin6,
                    SLNETSOCK_AF_INET6, service, hints);

            if (aiTmp) {
                /*
                 * The current list (ai) may not be empty.  Merge the new
                 * results (aiTmp) into the existing ai to handle this case.
                 */
                mergeLists(&ai, &aiTmp);
            }
        }
    }

    /* Give user our allocated and initialized addrinfo struct(s) */
    *res = ai;

    if (!ai) {
        /* Our list is empty - memory allocations failed */
        return (SLNETUTIL_EAI_MEMORY);
    }

    return (0);
}

//*****************************************************************************
// SlNetUtil_freeAddrInfo
//*****************************************************************************
void SlNetUtil_freeAddrInfo(struct SlNetUtil_addrInfo_t *res)
{
    SlNetUtil_addrInfo_t *aiTmp;

    /* Delete all nodes in linked list */
    while (res) {
        aiTmp = res->ai_next;
        free((void *)res);
        res = aiTmp;
    }
}

//*****************************************************************************
// setAddrInfo
// Intermediate step to handle permutations of ai_socktype and ai_protocol
// hints fields, passed by the user.  If both socktype and protocol are 0, must
// create a results struct for each socktype and protocol.
// Returns an empty list (NULL) or a list with one or more nodes.
//*****************************************************************************
static SlNetUtil_addrInfo_t *setAddrInfo(uint16_t ifID, SlNetSock_Addr_t *addr,
        int family, const char *service,
        const SlNetUtil_addrInfo_t *hints)
{
    SlNetUtil_addrInfo_t *ai = NULL;
    SlNetUtil_addrInfo_t *aiTmp = NULL;

    if ((hints->ai_socktype == 0 && hints->ai_protocol == 0) ||
        (hints->ai_socktype == 0 && hints->ai_protocol == SLNETSOCK_PROTO_UDP)
        || (hints->ai_socktype == SLNETSOCK_SOCK_DGRAM &&
        hints->ai_protocol == 0) || (hints->ai_socktype == SLNETSOCK_SOCK_DGRAM
        && hints->ai_protocol == SLNETSOCK_PROTO_UDP)) {

        ai = createAddrInfo(ifID, addr, family, service, hints->ai_flags,
                SLNETSOCK_SOCK_DGRAM, SLNETSOCK_PROTO_UDP);
    }

    if ((hints->ai_socktype == 0 && hints->ai_protocol == 0) ||
        (hints->ai_socktype == 0 && hints->ai_protocol == SLNETSOCK_PROTO_TCP)
        || (hints->ai_socktype == SLNETSOCK_SOCK_STREAM &&
        hints->ai_protocol == 0) || (hints->ai_socktype == SLNETSOCK_SOCK_STREAM
        && hints->ai_protocol == SLNETSOCK_PROTO_TCP)) {

        aiTmp = createAddrInfo(ifID, addr, family, service, hints->ai_flags,
                SLNETSOCK_SOCK_STREAM, SLNETSOCK_PROTO_TCP);

        if (aiTmp) {
            /* Insert into front of list (assume UDP node was added above) */
            aiTmp->ai_next = ai;
            ai = aiTmp;
        }
    }

    return (ai);
}

//*****************************************************************************
// createAddrInfo
// Create new address info structure. Returns a single node.
//*****************************************************************************
static SlNetUtil_addrInfo_t *createAddrInfo(uint16_t ifID,
        SlNetSock_Addr_t *addr, int family, const char *service, int flags,
        int socktype, int protocol)
{
    SlNetUtil_addrInfo_t *ai = NULL;

    /*
     * Allocate memory for the addrinfo struct, which we must fill out and
     * return to the caller.  This struct also has a pointer to a generic socket
     * address struct, which will point to either struct sockaddr_in, or
     * struct sockaddr_in6, depending.  Need to allocate enough space to hold
     * that struct, too.
     */
    ai = (SlNetUtil_addrInfo_t *)calloc(1, SLNETUTIL_ADDRINFO_ALLOCSZ);
    if (!ai) {
        /* Error: memory allocation failed */
        return (NULL);
    }

    ai->ai_flags = flags;
    ai->ai_socktype = socktype;
    ai->ai_protocol = protocol;
    ai->ai_canonname = NULL;
    ai->ai_next = NULL;

    /* Store socket addr struct after the addrinfo struct in our memory block */
    ai->ai_addr = (SlNetSock_Addr_t *)(ai + 1);

    if (family == SLNETSOCK_AF_INET) {
        /* Fill in structure for IPv4 */
        ai->ai_family = SLNETSOCK_AF_INET;
        ai->ai_addrlen = sizeof(SlNetSock_AddrIn_t);

        /* Write values to addrinfo's socket struct as an sockaddr_in struct */
        ((SlNetSock_AddrIn_t *)ai->ai_addr)->sin_family = SLNETSOCK_AF_INET;

        ((SlNetSock_AddrIn_t *)ai->ai_addr)->sin_port =
                SlNetUtil_htons(atoi(service));

        ((SlNetSock_AddrIn_t *)ai->ai_addr)->sin_addr =
                ((SlNetSock_AddrIn_t *)addr)->sin_addr;
    }
    else {
        /* Fill in structure for IPv6 */
        ai->ai_family = SLNETSOCK_AF_INET6;
        ai->ai_addrlen = sizeof(SlNetSock_AddrIn6_t);

        /* Write values to addrinfo's socket struct as an sockaddr_in6 struct */
        ((SlNetSock_AddrIn6_t *)ai->ai_addr)->sin6_family = SLNETSOCK_AF_INET6;

        ((SlNetSock_AddrIn6_t *)ai->ai_addr)->sin6_port =
                SlNetUtil_htons(atoi(service));

        memcpy(&(((SlNetSock_AddrIn6_t *)ai->ai_addr)->sin6_addr),
                &(((SlNetSock_AddrIn6_t *)addr)->sin6_addr),
                sizeof(SlNetSock_In6Addr_t));

        /* Scope ID should have been determined correctly by the caller */
        ((SlNetSock_AddrIn6_t *)ai->ai_addr)->sin6_scope_id = (uint32_t)ifID;
    }

    return (ai);
}

//*****************************************************************************
// mergeLists
// Combines an existing linked list of addrinfo structs (curList) and a newly
// obtained list (newList) into a single list. If the existing list is empty,
// it will be initialized to the new list.  If both lists are empty, no action
// is taken.
//*****************************************************************************
static int mergeLists(SlNetUtil_addrInfo_t **curList,
        SlNetUtil_addrInfo_t **newList)
{
    int numNodes = 0;
    SlNetUtil_addrInfo_t *tail = NULL;

    /* Check params */
    if (!curList || !newList || !(*newList)) {
        return (numNodes);
    }

    /* Update node count & find end of new list */
    for (tail = *newList; tail != NULL;) {
        numNodes++;
        if (tail->ai_next != NULL) {
            /* Not the tail, keep traversing */
            tail = tail->ai_next;
        }
        else {
            /* Tail found, quit loop */
            break;
        }
    }

    /* Append current list to end of new list */
    tail->ai_next = *curList;
    *curList = *newList;

    return (numNodes);
}

//*****************************************************************************
//
// SlNetUtil_htonl - Reorder the bytes of a 32-bit unsigned value from host
//                   order to network order(Big endian)
//
//*****************************************************************************
uint32_t SlNetUtil_htonl(uint32_t val)
{
    uint32_t i = 1;
    int8_t  *p = (int8_t *)&i;

    /* When the LSB of i stored in the smallest address of *p                */
    if (p[0] == 1) /* little endian */
    {
        /* Swap the places of the value                                      */
        p[0] = ((int8_t *)&val)[3];
        p[1] = ((int8_t *)&val)[2];
        p[2] = ((int8_t *)&val)[1];
        p[3] = ((int8_t *)&val)[0];

        /* return the reordered bytes                                        */
        return i;
    }
    else /* big endian */
    {
        /* return the input without any changes                              */
        return val;
    }
}


//*****************************************************************************
//
// SlNetUtil_ntohl - Reorder the bytes of a 32-bit unsigned value from network
//                   order(Big endian) to host order
//
//*****************************************************************************
uint32_t SlNetUtil_ntohl(uint32_t val)
{
    /* return the reordered bytes                                            */
    return SlNetUtil_htonl(val);
}


//*****************************************************************************
//
// SlNetUtil_htons - Reorder the bytes of a 16-bit unsigned value from host
//                   order to network order(Big endian)
//
//*****************************************************************************
uint16_t SlNetUtil_htons(uint16_t val)
{
    int16_t i = 1;
    int8_t *p = (int8_t *)&i;

    /* When the LSB of i stored in the smallest address of *p                */
    if (p[0] == 1) /* little endian */
    {
        /* Swap the places of the value                                      */
        p[0] = ((int8_t *)&val)[1];
        p[1] = ((int8_t *)&val)[0];

        /* return the reordered bytes                                        */
        return (uint16_t)i;
    }
    else /* big endian */
    {
        /* return the input without any changes                              */
        return val;
    }
}


//*****************************************************************************
//
// SlNetUtil_ntohs - Reorder the bytes of a 16-bit unsigned value from network
//                   order(Big endian) to host order
//
//*****************************************************************************
uint16_t SlNetUtil_ntohs(uint16_t val)
{
    /* return the reordered bytes                                            */
    return SlNetUtil_htons(val);
}

//*****************************************************************************
//
// SlNetUtil_UTOA - converts unsigned 16 bits binary number to string with
//                  maximum of 4 characters + 1 NULL terminated
//
//*****************************************************************************
static int32_t SlNetUtil_UTOA(uint16_t value, char * string, uint16_t base)
{
    uint16_t Index         = 4;
    char     tempString[5] = { 0 };
    char *   ptempString   = tempString;
    char *   pString       = string;

    /* Check if the inputs valid                                             */
    if ( (NULL == string) || ((base < 2 ) && (base > 16 )) )
    {
        return SLNETERR_RET_CODE_INVALID_INPUT;
    }

    /* If value is zero, that means that the returned string needs to be zero*/
    if (0 == value)
    {
        *ptempString = '0';
        ptempString++;
        Index--;
    }

    /* Run until all value digits are 0 or until Index get to 0              */
    for (; (value && (Index > 0)); Index--, value /= base)
    {
        *ptempString = "0123456789abcdef"[value % base];
        ptempString++;
    }
    /* Invalid value input                                                   */
    if (0 != value)
    {
        return SLNETERR_RET_CODE_INVALID_INPUT;
    }

    /* Reverse the string and initialize temporary array                     */
    while (Index < 4)
    {
        *(pString++) = *(--ptempString);
        *ptempString = '\0';
        *pString     = '\0';
        Index++;
    }

    return 0;
}

//*****************************************************************************
//
// SlNetUtil_bin2StrIpV4 - converts IPv4 address in binary representation
//                         (network byte order) to IP address in string
//                         representation
//
//*****************************************************************************
static int32_t SlNetUtil_bin2StrIpV4(SlNetSock_InAddr_t *binaryAddr, char *strAddr, uint16_t strAddrLen)
{
    uint8_t  tempOctet;
    uint32_t tempBinAddr;
    int32_t  octetIndex      = 0;
    char     tempStrOctet[4] = { 0 };

    /* Check if the strAddr buffer is at least in the minimum required size  */
    if (strAddrLen < SLNETSOCK_INET_ADDRSTRLEN)
    {
        /* Return error code                                                 */
        return SLNETERR_RET_CODE_INVALID_INPUT;
    }

    /* initialize strAddr to an empty string (so we can strcat() later)      */
    strAddr[0] = '\0';

    /* Copy the address value for further use                                */
    memcpy(&tempBinAddr, binaryAddr, sizeof(SlNetSock_InAddr_t));

    /* Run over all octets (in network byte order), starting with the
       most significant octet and ending with the least significant octet    */
    while ( octetIndex <= 3 )
    {
        /* Save octet on tempOctet for further usage.
           When converting from binary representation to string
           representation, the MSO of the binary number is the first char
           of the string, so it needs to copied to the first location of
           the array                                                         */
        tempOctet = ((int8_t *)&tempBinAddr)[octetIndex];

        /* Initialize the octet for validation after copying the value       */
        ((int8_t *)&tempBinAddr)[octetIndex] = 0;

        /* Convert tempOctet to string                                       */
        SlNetUtil_UTOA(tempOctet, tempStrOctet, 10);

        /* Appends the tempStrOctet to strAddr                               */
        strcat(strAddr, tempStrOctet);

        /* Appends the "." to strAddr for the first 3 octets                 */
        if ( octetIndex < 3)
        {
            strcat(strAddr, ".");
        }

        /* Move to the next octet                                            */
        octetIndex ++;

    }

    /* Check if the address had only 4 octets, this was done by initializing
       each octet that was copied and than checking if the number equal to 0 */
    if ( 0 == tempBinAddr )
    {
        /* Return success                                                    */
        return SLNETERR_RET_CODE_OK;
    }
    else
    {
        /* Return error code                                                 */
        return SLNETERR_RET_CODE_INVALID_INPUT;
    }
}


//*****************************************************************************
//
// SlNetUtil_bin2StrIpV6 - converts IPv6 address in binary representation to
//                         IP address in string representation
//
//*****************************************************************************
static int32_t SlNetUtil_bin2StrIpV6(SlNetSock_In6Addr_t *binaryAddr, char *strAddr, uint16_t strAddrLen)
{
    uint16_t tempHextet;
    int32_t  hextetIndex      = 0;
    uint8_t  tempBinAddr[16]  = { 0 };
    char     tempStrHextet[5] = { 0 };

    /* Check if the strAddr buffer is at least in the minimum required size  */
    if (strAddrLen < SLNETSOCK_INET6_ADDRSTRLEN)
    {
        /* Return error code                                                 */
        return SLNETERR_RET_CODE_INVALID_INPUT;
    }

    /* initialize strAddr to an empty string (so we can strcat() later)      */
    strAddr[0] = '\0';

    /* Copy the address value for further use                                */
    memcpy(tempBinAddr, binaryAddr, sizeof(SlNetSock_In6Addr_t));

    /* Run over all octets, from the latest hextet (the most significant
       hextet) until the first one (the least significant hextet)            */
    while (hextetIndex < 8)
    {
        /* Save hextet on tempHextet for further usage.
           When converting from binary representation to string
           representation, the most significant hextet of the binary number
           is the first char of the string, so it needs to copied to the
           first location of the array                                       */
        tempHextet = (tempBinAddr[hextetIndex * 2] << 8) |
            (tempBinAddr[(hextetIndex * 2) + 1]);

        /* Convert tempHextet to string                                      */
        SlNetUtil_UTOA(tempHextet, tempStrHextet, 16);

        /* Appends the tempStrHextet to strAddr                              */
        strcat(strAddr, tempStrHextet);

        /* Appends the ":" after each hextet (without the last one)          */
        if (hextetIndex < 7)
        {
            strcat(strAddr, ":");
        }

        /* Move to the next hextet                                           */
        hextetIndex++;

    }

    /* Return success                                                    */
    return SLNETERR_RET_CODE_OK;
}


//*****************************************************************************
//
// SlNetUtil_inetNtop - converts IP address in binary representation to IP
//                      address in string representation
//
//*****************************************************************************
const char *SlNetUtil_inetNtop(int16_t addrFamily, const void *binaryAddr, char *strAddr, SlNetSocklen_t strAddrLen)
{
    int32_t retVal;

    /* Switch according to the address family                                */
    switch(addrFamily)
    {
        case SLNETSOCK_AF_INET:
            /* Convert from IPv4 string to numeric/binary representation     */
            retVal = SlNetUtil_bin2StrIpV4((SlNetSock_InAddr_t *)binaryAddr, strAddr, strAddrLen);

        break;
        case SLNETSOCK_AF_INET6:
            /* Convert from IPv6 string to numeric/binary representation     */
            retVal = SlNetUtil_bin2StrIpV6((SlNetSock_In6Addr_t *)binaryAddr, strAddr, strAddrLen);

        break;
        default:
            /* wrong address family - function error, return NULL error      */
            return NULL;
    }

    /* Check if conversion was successful                                    */
    if (retVal != SLNETERR_RET_CODE_OK)
    {
        /* Conversion failed, return NULL as error code                      */
        return NULL;
    }
    /* Conversion success - return strAddr for success                       */
    return strAddr;
}

//*****************************************************************************
//
// SlNetUtil_strTok - Split a string up into tokens
//
//*****************************************************************************
static int32_t SlNetUtil_strTok(char **string, char *returnstring, const char *delimiter)
{
    char * retStr;

    retStr = returnstring;

    while ( (**string !='\0') && (**string != *delimiter) )
    {
        *retStr = **string;
        retStr++;
        (*string)++;
    }
    if (**string !='\0')
    {
        (*string)++;
    }
    *retStr = '\0';

    return SLNETERR_RET_CODE_OK;
}

//*****************************************************************************
//
// SlNetUtil_str2BinIpV4 - converts IPv4 address in string representation to
//                         IP address in binary representation
//
//*****************************************************************************
static int32_t SlNetUtil_str2BinIpV4(char *strAddr, SlNetSock_InAddr_t *binaryAddr)
{
    uint32_t  decNumber;
    char      token[4];
    int32_t   retVal;
    int32_t   ipOctet     = 0;
    uint32_t  ipv4Address = 0;
    char     *modifiedStr = strAddr;

    /* split strAddr into tokens separated by "."                            */
    retVal = SlNetUtil_strTok(&modifiedStr, token, ".");
    if (SLNETERR_RET_CODE_OK != retVal)
    {
        return retVal;
    }

    /* run 4 times as IPv4 contain of four octets and separated by periods   */
    while(ipOctet < 4)
    {
        /* Check Whether IP is valid */
        if(token != NULL)
        {
            /* Parses the token strAddr, interpreting its content as an integral
               number of the specified base 10                               */
            decNumber = (int)strtoul(token, 0, 10);

            /* Check if the octet holds valid number between the range 0-255 */
            if (decNumber < 256)
            {
                /* manually place each byte in network order */
                ((int8_t *)&ipv4Address)[ipOctet] = (uint8_t)decNumber;

                /* split strAddr into tokens separated by "."                */
                SlNetUtil_strTok(&modifiedStr, token, ".");
                ipOctet++;
            }
            else
            {
                return SLNETERR_RET_CODE_INVALID_INPUT;
            }
        }
        else
        {
            return SLNETERR_RET_CODE_INVALID_INPUT;
        }
    }

    /* Copy the temporary variable to the input variable                     */
    memcpy(binaryAddr, &ipv4Address, sizeof(SlNetSock_InAddr_t));

    return SLNETERR_RET_CODE_OK;
}


//*****************************************************************************
//
// SlNetUtil_str2BinIpV6 - converts IPv6 address in string representation to
//                         IP address in binary representation
//
//*****************************************************************************
static int32_t SlNetUtil_str2BinIpV6(char *strAddr, SlNetSock_In6Addr_t *binaryAddr)
{

    int32_t   octetIndex      = 0;
    int32_t   octetTailIndex;
    uint8_t  *pLocalStr;
    uint8_t   tmp[16];
    int32_t   zeroCompressPos = -1;
    uint16_t  value           = 0;
    uint8_t   asciiCharacter  = 0;

    /* Copy the first address of the string                                  */
    pLocalStr = (uint8_t *)strAddr;

    /* Initialize tmp parameter                                              */
    memset(tmp, 0, sizeof(tmp));

    /* Check if the IP starts with "::"                                      */
    if(*pLocalStr==':')
    {
        /* If the IP starts with ":", check if it doesn't have the second ":"
           If so, return an error                                            */
        if(*++pLocalStr!=':')
        {
            return SLNETERR_RET_CODE_INVALID_INPUT;
        }
    }

    /* run over the remaining two octets                                     */
    while(*pLocalStr && (octetIndex < 16))
    {
        /* Check if the ASCII character is a number between "0" to "9"       */
        if(*pLocalStr >= '0' && *pLocalStr <= '9')
        {
            /* Each ASCII character can be max 4 bits, shift the number
              4 bits and copy the new converted number                       */
            value = (value << 4) | (*pLocalStr - '0');

            /* Set the flag for ASCII character                              */
            asciiCharacter = 1;
        }
        /* Check if the ASCII character is a hex character between "a" to "f"*/
        else if(*pLocalStr >= 'a' && *pLocalStr <= 'f')
        {
            /* Each ASCII character can be max 4 bits, shift the number
              4 bits and copy the new converted number                       */
            value = (value << 4) | ((*pLocalStr - 'a') + 10);

            /* Set the flag for ASCII character                              */
            asciiCharacter = 1;
        }
        /* Check if the ASCII character is a hex character between "A" to "F"*/
        else if(*pLocalStr >= 'A' && *pLocalStr <= 'F')
        {
            /* Each ASCII character can be max 4 bits, shift the number
              4 bits and copy the new converted number                       */
            value = (value << 4) | ((*pLocalStr - 'A') + 10);

            /* Set the flag for ASCII character                              */
            asciiCharacter = 1;
        }
        /* Check if the hextet (two octets) finished with ":" and still a
           part of the IP                                                    */
        else if((*pLocalStr == ':') && (octetIndex < 14))
        {
            /* Check if the hextet contain ASCII character                   */
            if(asciiCharacter)
            {
                /* ASCII character exists, store the converted number in tmp
                   and reset the value and ascii character parameters        */
                tmp[octetIndex++] = (value >> 8) & 0xFF;
                tmp[octetIndex++] = (value) & 0xFF;
                asciiCharacter = 0;
                value = 0;
            }
            else
            {
                /* ASCII character doesn't exists, compressed hextet found   */
                if(zeroCompressPos < 0)
                {
                    /* first compressed hextet found, sore the octet Index   */
                    zeroCompressPos = octetIndex;
                }
                else
                {
                    /* Second compressed hextet found, return error code     */
                    return SLNETERR_RET_CODE_INVALID_INPUT;
                }
            }
        }
        /* Continue to the next ASCII character                              */
        pLocalStr++;
    }

    /* if more than 15 octets found, return error code                       */
    if(octetIndex > 15)
    {
        return SLNETERR_RET_CODE_INVALID_INPUT;
    }
    /* if less than 14 octets found, and without any compress hextet,
       return error code                                                     */
    else if(asciiCharacter && (zeroCompressPos < 0) && (octetIndex < 14))
    {
        return SLNETERR_RET_CODE_INVALID_INPUT;
    }
    /* if all octets found, but still found compressed hextet,
       return error code                                                     */
    else if((zeroCompressPos >= 0) && octetIndex >= 14)
    {
        return SLNETERR_RET_CODE_INVALID_INPUT;
    }

    /* copy the last available hextet to the tmp array                       */
    if((asciiCharacter) && (octetIndex <= 14))
    {
        /* Store the converted number in tmp and reset the value and
           ascii character parameters                                        */
        tmp[octetIndex++] = (value >> 8) & 0xFF;
        tmp[octetIndex++] = (value) & 0xFF;
        asciiCharacter = 0;
        value = 0;
    }

    /* compressed position found, add zeros in the compressed sections       */
    if(zeroCompressPos >= 0)
    {
        /* compressed position found, add zeros in the compressed sections   */
        octetIndex--;
        octetTailIndex = 15;
        /* Move the converted octets from the position they are located on
           to the end of the array and add zero instead */
        while(octetIndex >= zeroCompressPos)
        {
            /* Check if the indexes are still in range                       */
            if ((octetTailIndex >= 0) && (octetIndex >= 0))
            {
                /* Move all the octets after the zero compress position to
                   the end of the array                                      */
                tmp[octetTailIndex] = tmp[octetIndex];
                tmp[octetIndex] = 0;
                octetTailIndex--;
                octetIndex--;
            }
        }
    }

    /* Copy the temporary variable to the input variable                     */
    memcpy(binaryAddr, tmp, sizeof(tmp));

    return SLNETERR_RET_CODE_OK;

}

//*****************************************************************************
//
// SlNetUtil_inetAton - Converts a string to a network address structure
//
//*****************************************************************************
int SlNetUtil_inetAton(const char *str, SlNetSock_InAddr_t *addr)
{
    uint32_t val[4];
    uint32_t base;
    int sect;
    char c;

    sect = -1;
    while (*str) {
        /* New section */
        sect++;

        /* Get the base for this number */
        base = 10;
        if (*str == '0')
        {
            if (*(str + 1) == 'x' || *(str + 1) == 'X') {
                base = 16;
                str += 2;
            }
            else {
                base = 8;
                str++;
            }
        }

        /* Now decode this number */
        val[sect] = 0;
        for (;;) {
            c = *str++;

            if ((c >= '0' && c <= '9')) {
                val[sect] = (val[sect] * base) + (c - '0');
            }
            else if (base == 16 && (c >= 'A' && c <= 'F')) {
                val[sect] = (val[sect] * 16) + (c - 'A') + 10;
            }
            else if (base == 16 && (c >= 'a' && c <= 'f')) {
                val[sect] = (val[sect] * 16) + (c - 'a') + 10;
            }
            else if (c == '.') {
                /* validate value */
                if(val[sect] > 255) {
                    return (0);
                }

                /*
                 * Once we have four sections, quit.
                 * We want to accept: "1.2.3.4.in-addr.arpa"
                 */
                if (sect == 3) {
                    goto done;
                }

                /* Break this section */
                break;
            }
            else if (!c) {
                goto done;
            }
            else if (c != ' ') {
                return (0);
            }
        }
    }

done:
    /* What we do changes based on the number of sections */
    switch (sect) {
        case 0:
            addr->s_addr = val[0];
            break;
        case 1:
            if (val[1] > 0xffffff) {
                return (0);
            }
            addr->s_addr = val[0] << 24;
            addr->s_addr += val[1];
            break;
        case 2:
            if (val[2] > 0xffff) {
                return (0);
            }
            addr->s_addr = val[0] << 24;
            addr->s_addr += (val[1] << 16);
            addr->s_addr += val[2];
            break;
        case 3:
            if (val[3] > 0xff) {
                return (0);
            }
            addr->s_addr = val[0] << 24;
            addr->s_addr += (val[1] << 16);
            addr->s_addr += (val[2] << 8);
            addr->s_addr += val[3];
            break;
        default:
            return (0);
    }

    addr->s_addr = SlNetUtil_htonl(addr->s_addr);
    return (1);
}

//*****************************************************************************
//
// SlNetUtil_inetPton - converts IP address in string representation to IP
//                      address in binary representation
//
//*****************************************************************************
int32_t SlNetUtil_inetPton(int16_t addrFamily, const char *strAddr, void *binaryAddr)
{
    int32_t retVal;

    /* Switch according to the address family                                */
    switch(addrFamily)
    {
        case SLNETSOCK_AF_INET:
            /* Convert from IPv4 string to numeric/binary representation     */
            retVal = SlNetUtil_str2BinIpV4((char *)strAddr, (SlNetSock_InAddr_t *)binaryAddr);
            break;

        case SLNETSOCK_AF_INET6:
            /* Convert from IPv6 string to numeric/binary representation     */
            retVal = SlNetUtil_str2BinIpV6((char *)strAddr, (SlNetSock_In6Addr_t *)binaryAddr);
            break;

        default:
            /* wrong address family - function error, return -1 error        */
            return SLNETERR_RET_CODE_INVALID_INPUT;
    }

    /* Check if conversion was successful                                    */
    if (retVal != SLNETERR_RET_CODE_OK)
    {
        /* Conversion failed, that means the input wasn't a
           valid IP address, return 0 as error code                          */
        return 0;
    }
    /* Conversion success - return 1 for success                             */
    return 1;
}
