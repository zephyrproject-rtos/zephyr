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

#ifndef __IN_H__
#define __IN_H__

#ifdef    __cplusplus
extern "C" {
#endif

#include <ti/drivers/net/wifi/simplelink.h>

/*!

    \addtogroup BSD_Socket
    @{
*/

/*
 *  Protocols
 */
#define IPPROTO_IP                          SL_IPPROTO_IP   /* dummy for IP */
#define IPPROTO_TCP                         SL_IPPROTO_TCP  /* tcp */
#define IPPROTO_UDP                         SL_IPPROTO_UDP  /* user datagram protocol */
#define IPPROTO_RAW                         SL_IPPROTO_RAW  /* Raw Socket */

/*
 *  Internet Address integers
 */
#define INADDR_ANY                          SL_INADDR_ANY
#define IN6ADDR_ANY                         SL_IN6ADDR_ANY

/*
 *   Socket address, Internet style.
 */
#define in_addr                             SlInAddr_t
#define sockaddr_in                         SlSockAddrIn_t
#define in6_addr                            SlIn6Addr_t
#define sockaddr_in6                        SlSockAddrIn6_t

/*
 * IP ADDR LEN
 */
#define INET_ADDRSTRLEN                     (4)
#define INET6_ADDRSTRLEN                    (16)

/* Sock options */
#define IP_ADD_MEMBERSHIP                   SL_IP_ADD_MEMBERSHIP
#define IP_DROP_MEMBERSHIP                  SL_IP_DROP_MEMBERSHIP
#define IP_MULTICAST_TTL                    SL_IP_MULTICAST_TTL
#define IP_MULTICAST_IF                     SL_IP_MULTICAST_IF
#define IPV6_MULTICAST_HOPS                 SL_IPV6_MULTICAST_HOPS
#define IPV6_ADD_MEMBERSHIP                 SL_IPV6_ADD_MEMBERSHIP
#define IPV6_DROP_MEMBERSHIP                SL_IPV6_DROP_MEMBERSHIP

/*!

 Close the Doxygen group.
 @}

 */

#ifdef  __cplusplus
}
#endif /* __cplusplus */
#endif /* __IN_H__ */
