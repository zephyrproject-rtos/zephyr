/*
 * netdb.h - CC31xx/CC32xx Host Driver Implementation
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

#ifndef __NETDB_H__
#define __NETDB_H__

#ifdef    __cplusplus
extern "C" {
#endif

#include <ti/drivers/net/wifi/simplelink.h>

#define IPv4_ADDR_LEN           (4)
#define IPv6_ADDR_LEN           (16)

/*!

    \addtogroup BSD_Socket
    @{

*/

typedef struct hostent
{
    char *h_name;               /* Host name */
    char **h_aliases;           /* alias list */
    int    h_addrtype;          /* host address type */
    int    h_length;            /* length of address */
    char **h_addr_list;         /* list of addresses */
#define h_addr  h_addr_list[0]  /* Address, for backward compatibility.  */
}hostent_t;

/*!
    \brief Get host IP by name\n
    Obtain the IP Address of machine on network, by machine name.

    \param[in]  const char *name       Host name


    \return     Struct hostent containing the answer on success or NULL failure.\n

    \sa         sl_NetAppDnsGetHostByName

    \note       Note: The function isn't reentrant!
                It utilize a static hostent struct  which holds the answer for the DNS query.
                Calling this function form several threads may result in invalid answers.
                A user interested in a reentrant function which resolves IP address by name,
                can use the SimpleLink API: 'sl_NetAppDnsGetHostByName'.
                Another option is to protect this call with a lock and copy the returned hostent
                struct to a user buffer, before unlocking.

    \warning    The parameter 'name' is assumed to be allocated by the user, and it's the
                user's Responsibility to maintain it's validity. This field is copied (not deep copied)
                to the struct returned by this function.

    \par  Example

    - Getting host by name:
    \code
    struct sockaddr_in sa;
    struct hostent *host_addr;

    host_addr = gethostbyname("www.cloudprovider.com");

    if(!host_addr)
    {
        sa.sin_family = host_addr->h_addrtype;
        memcpy(&sa.sin_addr.s_addr, host_addr->h_addr_list, host_addr->h_length);
        sa.sin_port = htons(80);
    }

    connect(Socketfd, (struct sockaddr *)&sa, sizeof(sa));

    \endcode
*/
struct hostent* gethostbyname(const char *name);

/*!

 Close the Doxygen group.
 @}

 */
#ifdef  __cplusplus
}
#endif /* __cplusplus */
#endif /* __NETDB_ */
