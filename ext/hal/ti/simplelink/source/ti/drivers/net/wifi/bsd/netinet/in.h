/*
 * in.h - CC31xx/CC32xx Host Driver Implementation
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
