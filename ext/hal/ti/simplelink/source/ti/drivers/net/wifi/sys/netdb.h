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

#define IPv4_ADDR_LEN           (4)
#define IPv6_ADDR_LEN           (16)

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

    \addtogroup BSD_Socket
    @{

*/
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
    int DestinationIP;
    int AddrSize;
    int SockId;
    sockaddr_t Addr;
    struct hostent DnsEntry;

    DnsEntry = gethostbyname("www.google.com");

    if(!DnsEntry)
    {
        Addr.sin_family = AF_INET;
        Addr.sin_port = htons(80);
        Addr.sin_addr.s_addr = htonl(DestinationIP);
        AddrSize = sizeof(sockaddr_t);
        SockId = socket(AF_INET, SOCK_STREAM, 0);
    }
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
