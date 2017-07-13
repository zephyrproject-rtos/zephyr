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
