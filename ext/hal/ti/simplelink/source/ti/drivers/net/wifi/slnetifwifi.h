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
#include <ti/drivers/net/wifi/simplelink.h>
#include <ti/net/slnetsock.h>
#include <ti/net/slnetif.h>
#include <ti/net/slneterr.h>
#include <ti/net/slnetutils.h>

#ifndef __SLNETWIFI_SOCKET_H__
#define __SLNETWIFI_SOCKET_H__


#ifdef    __cplusplus
extern "C" {
#endif

/*!
    \defgroup WiFi Socket Stack
    \short Controls standard client/server sockets programming options and capabilities

*/
/*!

    \addtogroup Socket
    @{

*/

/*****************************************************************************/
/* Macro declarations                                                        */
/*****************************************************************************/

/* prototype ifConf */
extern SlNetIf_Config_t SlNetIfConfigWifi;


/*****************************************************************************/
/* Structure/Enum declarations                                               */
/*****************************************************************************/


/*****************************************************************************/
/* Function prototypes                                                       */
/*****************************************************************************/

/*!

    \brief Create an endpoint for communication

    The SlNetIfWifi_socket function creates a new socket of a certain socket
    type, identified by an integer number, and allocates system resources to
    it.\n
    This function is called by the application layer to obtain a socket descriptor (handle).

    \param[in] ifContext        Stores interface data if CreateContext function
                                supported and implemented.
    \param[in] domain           Specifies the protocol family of the created socket.
                                For example:
                                   - SLNETSOCK_AF_INET for network protocol IPv4
                                   - SLNETSOCK_AF_INET6 for network protocol IPv6
                                   - SLNETSOCK_AF_RF for starting transceiver mode.
                                        Notes:
                                        - sending and receiving any packet overriding 802.11 header
                                        - for optimized power consumption the socket will be started in TX
                                            only mode until receive command is activated
    \param[in] type             Specifies the socket type, which determines the semantics of communication over
                                the socket. The socket types supported by the system are implementation-dependent.
                                Possible socket types include:
                                   - SLNETSOCK_SOCK_STREAM (reliable stream-oriented service or Stream Sockets)
                                   - SLNETSOCK_SOCK_DGRAM  (datagram service or Datagram Sockets)
                                   - SLNETSOCK_SOCK_RAW    (raw protocols atop the network layer)
                                   - when used with AF_RF:
                                      - SLNETSOCK_SOCK_RX_MTR
                                      - SLNETSOCK_SOCK_MAC_WITH_CCA
                                      - SLNETSOCK_SOCK_MAC_WITH_NO_CCA
                                      - SLNETSOCK_SOCK_BRIDGE
                                      - SLNETSOCK_SOCK_ROUTER
    \param[in] protocol         Specifies a particular transport to be used with the socket.\n
                                The most common are
                                    - SLNETSOCK_PROTO_TCP
                                    - SLNETSOCK_PROTO_UDP
                                    - SLNETSOCK_PROTO_RAW
                                    - SLNETSOCK_PROTO_SECURE
                                The value 0 may be used to select a default
                                protocol from the selected domain and type
    \param[in] sdContext        Allocate and store socket data if needed for
                                using in other slnetwifi socket functions

    \return                     On success, socket descriptor (handle) that is used for consequent socket operations. \n
                                A successful return code should be a positive number (int16)\n
                                On error, a negative value will be returned specifying the error code.
                                   - SLNETERR_BSD_EAFNOSUPPORT    - illegal domain parameter
                                   - SLNETERR_BSD_EPROTOTYPE      - illegal type parameter
                                   - SLNETERR_BSD_EACCES          - permission denied
                                   - SLNETERR_BSD_ENSOCK          - exceeded maximal number of socket
                                   - SLNETERR_BSD_ENOMEM          - memory allocation error
                                   - SLNETERR_BSD_EINVAL          - error in socket configuration
                                   - SLNETERR_BSD_EPROTONOSUPPORT - illegal protocol parameter
                                   - SLNETERR_BSD_EOPNOTSUPP      - illegal combination of protocol and type parameters

    \sa                         SlNetIfWifi_socket
    \note
    \warning
*/
int16_t SlNetIfWifi_socket(void *ifContext, int16_t Domain, int16_t Type, int16_t Protocol, void **sdContext);

/*!
    \brief Gracefully close socket

    The SlNetIfWifi_close function causes the system to release resources allocated to a socket.  \n
    In case of TCP, the connection is terminated.

    \param[in] sd               Socket descriptor (handle), received in SlNetIfWifi_socket
    \param[in] sdContext        May store socket data if implemented in the
                                SlNetIfWifi_socket function.

    \return                     Zero on success, or negative error code on failure

    \sa                         SlNetIfWifi_socket
    \note
    \warning
*/
int32_t SlNetIfWifi_close(int16_t sd, void *sdContext);

/*!
    \brief Accept a connection on a socket

    The SlNetIfWifi_accept function is used with connection-based socket types (SOCK_STREAM).\n
    It extracts the first connection request on the queue of pending
    connections, creates a new connected socket, and returns a new file
    descriptor referring to that socket.\n
    The newly created socket is not in the listening state. The
    original socket sd is unaffected by this call. \n
    The argument sd is a socket that has been created with
    SlNetIfWifi_socket(), bound to a local address with SlNetIfWifi_bind(), and is
    listening for connections after a SlNetIfWifi_listen(). \n The argument
     \e <b>addr</b> is a pointer to a sockaddr structure. This structure
    is filled in with the address of the peer socket, as known to
    the communications layer. \n The exact format of the address
    returned addr is determined by the socket's address family. \n
    The \b \e addrlen argument is a value-result argument: it
    should initially contain the size of the structure pointed to
    by addr, on return it will contain the actual length (in
    bytes) of the address returned.

    \param[in]  sd                Socket descriptor (handle)
    \param[in]  sdContext         May store socket data if implemented in the
                                  SlNetIfWifi_socket function.
    \param[out] addr              The argument addr is a pointer
                                  to a sockaddr structure. This
                                  structure is filled in with the
                                  address of the peer socket, as
                                  known to the communications
                                  layer. The exact format of the
                                  address returned addr is
                                  determined by the socket's
                                  address\n
                                  sockaddr:\n - code for the
                                  address format.\n -
                                  socket address, the length
                                  depends on the code format
    \param[out] addrlen           The addrlen argument is a value-result
                                  argument: it should initially contain the
                                  size of the structure pointed to by addr
    \param[in]  flags             Specifies socket descriptor flags. \n
                                  The available flags are:
                                      - SLNETSOCK_SEC_START_SECURITY_SESSION_ONLY
                                      - SLNETSOCK_SEC_BIND_CONTEXT_ONLY
                                  Note: This flags can be used in order to start
                                  security session if needed
    \param[in]  acceptedSdContext Allocate and store data for the new socket
                                  if needed in other to use it in other
                                  slnetwifi socket functions

    \return                       On success, a socket descriptor.\n
                                  On a non-blocking accept a possible negative value is SLNETERR_BSD_EAGAIN.\n
                                  On failure, negative error code.\n
                                  SLNETERR_BSD_ENOMEM may be return in case there are no resources in the system
                                    In this case try again later or increase MAX_CONCURRENT_ACTIONS

    \sa                           SlNetIfWifi_Socket  SlNetIfWifi_Bind  SlNetIfWifi_Listen
    \note
    \warning
*/
int16_t SlNetIfWifi_accept(int16_t sd, void *sdContext, SlNetSock_Addr_t *addr, SlNetSocklen_t *addrlen, uint8_t flags, void **acceptedSdContext);

/*!
    \brief Assign a name to a socket

    This SlNetIfWifi_bind function gives the socket the local address addr.
    addr is addrlen bytes long. \n Traditionally, this is called
    When a socket is created with socket, it exists in a name
    space (address family) but has no name assigned. \n
    It is necessary to assign a local address before a SOCK_STREAM
    socket may receive connections.

    \param[in] sd               Socket descriptor (handle)
    \param[in] sdContext        May store socket data if implemented in the
                                SlNetIfWifi_socket function.
    \param[in] addr             Specifies the destination
                                addrs\n sockaddr:\n - code for
                                the address format.\n - socket address,
                                the length depends on the code
                                format
    \param[in] addrlen          Contains the size of the structure pointed to by addr

    \return                     Zero on success, or negative error code on failure

    \sa                         SlNetIfWifi_Socket  SlNetIfWifi_accept  SlNetIfWifi_Listen
    \note
    \warning
*/
int32_t SlNetIfWifi_bind(int16_t sd, void *sdContext, const SlNetSock_Addr_t *addr, int16_t addrlen);

/*!
    \brief Listen for connections on a socket

    The willingness to accept incoming connections and a queue
    limit for incoming connections are specified with SlNetIfWifi_listen(),
    and then the connections are accepted with SlNetIfWifi_accept(). \n
    The SlNetIfWifi_listen() call applies only to sockets of type SOCK_STREAM
    The backlog parameter defines the maximum length the queue of
    pending connections may grow to.

    \param[in] sd               Socket descriptor (handle)
    \param[in] sdContext        May store socket data if implemented in the
                                SlNetIfWifi_socket function.
    \param[in] backlog          Specifies the listen queue depth.

    \return                     Zero on success, or negative error code on failure

    \sa                         SlNetIfWifi_Socket  SlNetIfWifi_accept  SlNetIfWifi_bind
    \note
    \warning
*/
int32_t SlNetIfWifi_listen(int16_t sd, void *sdContext, int16_t backlog);

/*!
    \brief Initiate a connection on a socket

    Function connects the socket referred to by the socket
    descriptor sd, to the address specified by addr. \n The addrlen
    argument specifies the size of addr. \n The format of the
    address in addr is determined by the address space of the
    socket. \n If it is of type SLNETSOCK_SOCK_DGRAM, this call
    specifies the peer with which the socket is to be associated;
    this address is that to which datagrams are to be sent, and
    the only address from which datagrams are to be received. \n If
    the socket is of type SLNETSOCK_SOCK_STREAM, this call
    attempts to make a connection to another socket. \n The other
    socket is specified by address, which is an address in the
    communications space of the socket.

    \param[in] sd               Socket descriptor (handle)
    \param[in]  sdContext       May store socket data if implemented in the
                                SlNetIfWifi_socket function.
    \param[in] addr             Specifies the destination addr\n
                                sockaddr:\n - code for the
                                address format.\n -
                                socket address, the length
                                depends on the code format
    \param[in] addrlen          Contains the size of the structure pointed
                                to by addr
    \param[in]  flags           Specifies socket descriptor flags. \n
                                The available flags are:
                                    - SLNETSOCK_SEC_START_SECURITY_SESSION_ONLY
                                    - SLNETSOCK_SEC_BIND_CONTEXT_ONLY
                                Note: This flags can be used in order to start
                                security session if needed

    \return                     On success, a socket descriptor (handle).\n
                                On a non-blocking connect a possible negative value is NETSCOK_EALREADY.
                                On failure, negative value.\n
                                NETSCOK_POOL_IS_EMPTY may be return in case there are no resources in the system
                                  In this case try again later or increase MAX_CONCURRENT_ACTIONS

    \sa                         SlNetIfWifi_socket
    \note
    \warning
*/
int32_t SlNetIfWifi_connect(int16_t sd, void *sdContext, const SlNetSock_Addr_t *addr, SlNetSocklen_t addrlen, uint8_t flags);

/*!
    \brief Get local address info by socket descriptor\n
    Returns the local address info of the socket descriptor.

    \param[in]  sd              Socket descriptor (handle)
    \param[in]  sdContext       May store socket data if implemented in the
                                SlNetIfWifi_socket function.
    \param[out] addr            The argument addr is a pointer
                                to a SlNetSock_Addr_t structure. This
                                structure is filled in with the
                                address of the peer socket, as
                                known to the communications
                                layer. The exact format of the
                                address returned addr is
                                determined by the socket's
                                address\n
                                SlNetSock_Addr_t:\n - code for the
                                address format.\n -
                                socket address, the length
                                depends on the code format
    \param[out] addrlen         The addrlen argument is a value-result
                                argument: it should initially contain the
                                size of the structure pointed to by addr

    \return                     Zero on success, or negative on failure.\n


    \sa     SlNetSock_create  SlNetSock_bind
    \note   If the provided buffer is too small the returned address will be
            truncated and the addrlen will contain the actual size of the
            socket address
    \warning
*/
int32_t SlNetIfWifi_getSockName(int16_t sd, void *sdContext, SlNetSock_Addr_t *addr, SlNetSocklen_t *addrlen);

/*!
    \brief Monitor socket activity

    SlNetIfWifi_send allow a program to monitor multiple file descriptors,
    waiting until one or more of the file descriptors become
    "ready" for some class of I/O operation.
    If trigger mode is enabled the active sdset is the one that retrieved in the first triggered call.
    To enable the trigger mode, an handler must be statically registered to the slcb_SocketTriggerEventHandler (user.h)

    \param[in]     ifContext   Stores interface data if CreateContext function
                               supported and implemented.
                               Can be used in all SlNetIf_Config_t functions
    \param[in]     nsds        The highest-numbered file descriptor in any of the
                               three sets, plus 1.
    \param[in,out] readsds     Socket descriptors list for read monitoring and accept monitoring
    \param[in,out] writesds    Socket descriptors list for connect monitoring only, write monitoring is not supported
    \param[in,out] exceptsds   Socket descriptors list for exception monitoring, not supported.
    \param[in]     timeout     Is an upper bound on the amount of time elapsed
                               before SlNetIfWifi_send() returns. Null or above 0xffff seconds means
                               infinity timeout. The minimum timeout is 10 milliseconds,
                               less than 10 milliseconds will be set automatically to 10 milliseconds.
                               Max microseconds supported is 0xfffc00.
                               In trigger mode the timeout fields must be set to zero.

    \return                    On success, SlNetIfWifi_send()  returns the number of
                               file descriptors contained in the three returned
                               descriptor sets (that is, the total number of bits that
                               are set in readsds, writesds, exceptsds) which may be
                               zero if the timeout expires before anything interesting
                               happens.\n On error, a negative value is returned.
                               readsds - return the sockets on which Read request will
                               return without delay with valid data.\n
                               writesds - return the sockets on which Write request
                               will return without delay.\n
                               exceptsds - return the sockets closed recently. \n
                               SLNETERR_BSD_ENOMEM may be return in case there are no resources in the system
                                 In this case try again later or increase MAX_CONCURRENT_ACTIONS

    \sa     SlNetIfWifi_socket
    \note   If the timeout value set to less than 10ms it will automatically set
            to 10ms to prevent overload of the system\n

            Only one SlNetIfWifi_send can be handled at a time. \b
            Calling this API while the same command is called from another thread, may result
                in one of the following scenarios:
            1. The command will wait (internal) until the previous command finish, and then be executed.
            2. There are not enough resources and SLNETERR_BSD_ENOMEM error will return.
            In this case, MAX_CONCURRENT_ACTIONS can be increased (result in memory increase) or try
            again later to issue the command.
            3. In case there is already a triggered SlNetIfWifi_send in progress, the following call will return
            with SLNETSOCK_RET_CODE_SOCKET_SELECT_IN_PROGRESS_ERROR.

    \warning
*/
int32_t SlNetIfWifi_select(void *ifContext, int16_t nfds, SlNetSock_SdSet_t *readsds, SlNetSock_SdSet_t *writesds, SlNetSock_SdSet_t *exceptsds, SlNetSock_Timeval_t *timeout);


/*!
    \brief Set socket options-

    The SlNetIfWifi_setSockOpt function manipulate the options associated with a socket.\n
    Options may exist at multiple protocol levels; they are always
    present at the uppermost socket level.\n

    When manipulating socket options the level at which the option resides
    and the name of the option must be specified.  To manipulate options at
    the socket level, level is specified as SOL_SOCKET.  To manipulate
    options at any other level the protocol number of the appropriate protocol
    controlling the option is supplied.  For example, to indicate that an
    option is to be interpreted by the TCP protocol, level should be set to
    the protocol number of TCP; \n

    The parameters optval and optlen are used to access opt_values
    for SlNetIfWifi_setSockOpt().  For SlNetIfWifi_getSockOpt() they identify a
    buffer in which the value for the requested option(s) are to
    be returned.  For SlNetIfWifi_getSockOpt(), optlen is a value-result
    parameter, initially containing the size of the buffer
    pointed to by option_value, and modified on return to
    indicate the actual size of the value returned.  If no option
    value is to be supplied or returned, option_value may be
    NULL.

    \param[in] sd               Socket descriptor (handle)
    \param[in] sdContext        May store socket data if implemented in the
                                SlNetIfWifi_socket function.
    \param[in] level            Defines the protocol level for this option
                                - <b>SLNETSOCK_LVL_SOCKET</b>   Socket level configurations (L4, transport layer)
                                - <b>SLNETSOCK_LVL_IP</b>   IP level configurations (L3, network layer)
                                - <b>SLNETSOCK_LVL_PHY</b>  Link level configurations (L2, link layer)
    \param[in] optname          Defines the option name to interrogate
                                - <b>SLNETSOCK_LVL_SOCKET</b>
                                  - <b>SLNETSOCK_OPSOCK_RCV_BUF</b>  \n
                                                 Sets tcp max recv window size. \n
                                                 This options takes SlNetSock_Winsize_t struct as parameter
                                  - <b>SLNETSOCK_OPSOCK_RCV_TIMEO</b>  \n
                                                 Sets the timeout value that specifies the maximum amount of time an input function waits until it completes. \n
                                                 Default: No timeout \n
                                                 This options takes SlNetSock_Timeval_t struct as parameter
                                  - <b>SLNETSOCK_OPSOCK_KEEPALIVE</b>  \n
                                                 Enable or Disable periodic keep alive.
                                                 Keeps TCP connections active by enabling the periodic transmission of messages \n
                                                 Timeout is 5 minutes.\n
                                                 Default: Enabled \n
                                                 This options takes SlNetSock_Keepalive_t struct as parameter
                                  - <b>SLNETSOCK_OPSOCK_KEEPALIVE_TIME</b>  \n
                                                 Set keep alive timeout.
                                                 Value is in seconds \n
                                                 Default: 5 minutes \n
                                  - <b>SLNETSOCK_OPSOCK_LINGER</b> \n
                                                 Socket lingers on close pending remaining send/receive packets\n
                                  - <b>SLNETSOCK_OPSOCK_NON_BLOCKING</b> \n
                                                 Sets socket to non-blocking operation Impacts: connect, accept, send, sendto, recv and recvfrom. \n
                                                 Default: Blocking.
                                                 This options takes SlNetSock_Nonblocking_t struct as parameter
                                  - <b>SLNETSOCK_OPSOCK_NON_IP_BOUNDARY</b>  \n
                                                 Enable or Disable rx ip boundary.
                                                 In connectionless socket (udp/raw), unread data is dropped (when SlNetIfWifi_recvfrom len parameter < data size), Enable this option in order to read the left data on the next SlNetIfWifi_recvfrom iteration
                                                 Default: Disabled, IP boundary kept,  \n
                                                 This options takes SlNetSock_NonIpBoundary_t struct as parameter
                                - <b>SLNETSOCK_LVL_IP</b>
                                  - <b>SLNETSOCK_OPIP_MULTICAST_TTL</b> \n
                                                 Set the time-to-live value of outgoing multicast packets for this socket. \n
                                                 This options takes <b>uint8_t</b> as parameter
                                  - <b>SLNETSOCK_OPIP_ADD_MEMBERSHIP</b> \n
                                                 UDP socket, Join a multicast group. \n
                                                 This options takes SlNetSock_IpMreq_t struct as parameter
                                  - <b>SLNETSOCK_OPIP_DROP_MEMBERSHIP</b> \n
                                                 UDP socket, Leave a multicast group \n
                                                 This options takes SlNetSock_IpMreq_t struct as parameter
                                  - <b>SLNETSOCK_OPIP_HDRINCL</b> \n
                                                 RAW socket only, the IPv4 layer generates an IP header when sending a packet unless \n
                                                 the IP_HDRINCL socket option is enabled on the socket.    \n
                                                 When it is enabled, the packet must contain an IP header. \n
                                                 Default: disabled, IPv4 header generated by Network Stack \n
                                                 This options takes <b>uint32_t</b> as parameter
                                  - <b>SLNETSOCK_OPIP_RAW_RX_NO_HEADER</b> \n
                                                 Raw socket remove IP header from received data. \n
                                                 Default: data includes ip header \n
                                                 This options takes <b>uint32_t</b> as parameter
                                  - <b>SLNETSOCK_OPIP_RAW_IPV6_HDRINCL</b> (inactive) \n
                                                 RAW socket only, the IPv6 layer generates an IP header when sending a packet unless \n
                                                 the IP_HDRINCL socket option is enabled on the socket. When it is enabled, the packet must contain an IP header \n
                                                 Default: disabled, IPv4 header generated by Network Stack \n
                                                 This options takes <b>uint32_t</b> as parameter
                                - <b>SLNETSOCK_LVL_PHY</b>
                                  - <b>SLNETSOCK_OPPHY_CHANNEL</b> \n
                                                 Sets channel in transceiver mode.
                                                 This options takes <b>uint32_t</b> as channel number parameter
                                  - <b>SLNETSOCK_OPPHY_RATE</b> \n
                                                 RAW socket, set WLAN PHY transmit rate \n
                                                 The values are based on SlWlanRateIndex_e    \n
                                                 This options takes <b>uint32_t</b> as parameter
                                  - <b>SLNETSOCK_OPPHY_TX_POWER</b> \n
                                                 RAW socket, set WLAN PHY TX power \n
                                                 Valid rage is 1-15 \n
                                                 This options takes <b>uint32_t</b> as parameter
                                  - <b>SLNETSOCK_OPPHY_NUM_FRAMES_TO_TX</b> \n
                                                 RAW socket, set number of frames to transmit in transceiver mode.
                                                 Default: 1 packet
                                                 This options takes <b>uint32_t</b> as parameter
                                  - <b>SLNETSOCK_OPPHY_PREAMBLE</b> \n
                                                 RAW socket, set WLAN PHY preamble for Long/Short\n
                                                 This options takes <b>uint32_t</b> as parameter
                                  - <b>SLNETSOCK_OPPHY_TX_INHIBIT_THRESHOLD</b> \n
                                                 RAW socket, set WLAN Tx - Set CCA threshold. \n
                                                 The values are based on SlNetSockTxInhibitThreshold_e \n
                                                 This options takes <b>uint32_t</b> as parameter
                                  - <b>SLNETSOCK_OPPHY_TX_TIMEOUT</b> \n
                                                 RAW socket, set WLAN Tx - changes the TX timeout (lifetime) of transceiver frames. \n
                                                 Value in Ms, maximum value is 10ms \n
                                                 This options takes <b>uint32_t</b> as parameter
                                  - <b>SLNETSOCK_OPPHY_ALLOW_ACKS </b> \n
                                                 RAW socket, set WLAN Tx - Enable or Disable sending ACKs in transceiver mode \n
                                                 0 = disabled / 1 = enabled \n
                                                 This options takes <b>uint32_t</b> as parameter


    \param[in] optval           Specifies a value for the option
    \param[in] optlen           Specifies the length of the
        option value

    \return                     Zero on success, or negative error code on failure

    \par Persistent
                All params are <b>Non- Persistent</b>
    \sa     SlNetIfWifi_getSockOpt
    \note
    \warning
    \par    Examples

    - SLNETSOCK_OPSOCK_RCV_BUF:
    \code
           SlNetSock_Winsize_t size;
           size.winsize = 3000; // bytes
           SlNetIfWifi_setSockOpt(SockID, sdContext, SLNETSOCK_LVL_SOCKET, SLNETSOCK_OPSOCK_RCV_BUF, (uint8_t *)&size, sizeof(size));
    \endcode
    <br>

    - SLNETSOCK_OPSOCK_RCV_TIMEO:
    \code
        struct SlNetSock_Timeval_t timeVal;
        timeVal.tv_sec =  1; // Seconds
        timeVal.tv_usec = 0; // Microseconds. 10000 microseconds resolution
        SlNetIfWifi_setSockOpt(SockID, sdContext, SLNETSOCK_LVL_SOCKET, SLNETSOCK_OPSOCK_RCV_TIMEO, (uint8_t *)&timeVal, sizeof(timeVal)); // Enable receive timeout
    \endcode
    <br>

    - SLNETSOCK_OPSOCK_KEEPALIVE: //disable Keepalive
    \code
        SlNetSock_Keepalive_t enableOption;
        enableOption.keepaliveEnabled = 0;
        SlNetIfWifi_setSockOpt(SockID, sdContext, SLNETSOCK_LVL_SOCKET, SLNETSOCK_OPSOCK_KEEPALIVE, (uint8_t *)&enableOption, sizeof(enableOption));
    \endcode
    <br>

    - SLNETSOCK_OPSOCK_KEEPALIVE_TIME: //Set Keepalive timeout
    \code
        int16_t Status;
        uint32_t TimeOut = 120;
        SlNetIfWifi_setSockOpt(Sd, sdContext, SLNETSOCK_LVL_SOCKET, SLNETSOCK_OPSOCK_KEEPALIVE_TIME, (uint8_t *)&TimeOut, sizeof(TimeOut));
    \endcode
    <br>

    - SLNETSOCK_OPSOCK_NON_BLOCKING: //Enable or disable nonblocking mode
    \code
           SlNetSock_Nonblocking_t enableOption;
           enableOption.nonBlockingEnabled = 1;
           SlNetIfWifi_setSockOpt(SockID, sdContext, SLNETSOCK_LVL_SOCKET, SLNETSOCK_OPSOCK_NON_BLOCKING, (uint8_t *)&enableOption, sizeof(enableOption));
    \endcode
    <br>

    - SLNETSOCK_OPSOCK_NON_IP_BOUNDARY: //disable boundary
    \code
        SlNetSock_NonIpBoundary_t enableOption;
        enableOption.nonIpBoundaryEnabled = 1;
        SlNetIfWifi_setSockOpt(SockID, sdContext, SLNETSOCK_LVL_SOCKET, SLNETSOCK_OPSOCK_NON_IP_BOUNDARY, (uint8_t *)&enableOption, sizeof(enableOption));
    \endcode
    <br>

    - SLNETSOCK_OPSOCK_LINGER:
    \code
        SlNetSock_linger_t linger;
        linger.l_onoff = 1;
        linger.l_linger = 10;
        SlNetIfWifi_setSockOpt(SockID, sdContext, SLNETSOCK_LVL_SOCKET, SLNETSOCK_OPSOCK_LINGER, &linger, sizeof(linger));
    \endcode
    <br>

    - SLNETSOCK_OPIP_MULTICAST_TTL:
     \code
           uint8_t ttl = 20;
           SlNetIfWifi_setSockOpt(SockID, sdContext, SLNETSOCK_LVL_IP, SLNETSOCK_OPIP_MULTICAST_TTL, &ttl, sizeof(ttl));
     \endcode
     <br>

    - SLNETSOCK_OPIP_ADD_MEMBERSHIP:
     \code
           SlNetSock_IpMreq_t mreq;
           SlNetIfWifi_setSockOpt(SockID, sdContext, SLNETSOCK_LVL_IP, SLNETSOCK_OPIP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));
    \endcode
    <br>

    - SLNETSOCK_OPIP_DROP_MEMBERSHIP:
    \code
        SlNetSock_IpMreq_t mreq;
        SlNetIfWifi_setSockOpt(SockID, sdContext, SLNETSOCK_LVL_IP, SLNETSOCK_OPIP_DROP_MEMBERSHIP, &mreq, sizeof(mreq));
    \endcode
    <br>

    - SLNETSOCK_OPIP_RAW_RX_NO_HEADER:
    \code
        uint32_t header = 1;  // remove ip header
        SlNetIfWifi_setSockOpt(SockID, sdContext, SLNETSOCK_LVL_IP, SLNETSOCK_OPIP_RAW_RX_NO_HEADER, &header, sizeof(header));
    \endcode
    <br>

    - SLNETSOCK_OPIP_HDRINCL:
    \code
        uint32_t header = 1;
        SlNetIfWifi_setSockOpt(SockID, sdContext, SLNETSOCK_LVL_IP, SLNETSOCK_OPIP_HDRINCL, &header, sizeof(header));
    \endcode
    <br>

    - SLNETSOCK_OPIP_RAW_IPV6_HDRINCL:
    \code
        uint32_t header = 1;
        SlNetIfWifi_setSockOpt(SockID, sdContext, SLNETSOCK_LVL_IP, SLNETSOCK_OPIP_RAW_IPV6_HDRINCL, &header, sizeof(header));
    \endcode
    <br>

    - SLNETSOCK_OPPHY_CHANNEL:
    \code
        uint32_t newChannel = 6; // range is 1-13
        SlNetIfWifi_setSockOpt(SockID, sdContext, SLNETSOCK_LVL_SOCKET, SLNETSOCK_OPPHY_CHANNEL, &newChannel, sizeof(newChannel));
    \endcode
    <br>

    - SLNETSOCK_OPPHY_RATE:
    \code
        uint32_t rate = 6; // see wlan.h SlWlanRateIndex_e for values
        SlNetIfWifi_setSockOpt(SockID, sdContext, SLNETSOCK_LVL_PHY, SLNETSOCK_OPPHY_RATE, &rate, sizeof(rate));
    \endcode
    <br>

    - SLNETSOCK_OPPHY_TX_POWER:
    \code
        uint32_t txpower = 1; // valid range is 1-15
        SlNetIfWifi_setSockOpt(SockID, sdContext, SLNETSOCK_LVL_PHY, SLNETSOCK_OPPHY_TX_POWER, &txpower, sizeof(txpower));
    \endcode
    <br>

    - SLNETSOCK_OPPHY_NUM_FRAMES_TO_TX:
    \code
        uint32_t numframes = 1;
        SlNetIfWifi_setSockOpt(SockID, sdContext, SLNETSOCK_LVL_PHY, SLNETSOCK_OPPHY_NUM_FRAMES_TO_TX, &numframes, sizeof(numframes));
    \endcode
    <br>

    - SLNETSOCK_OPPHY_PREAMBLE:
    \code
        uint32_t preamble = 1;
        SlNetIfWifi_setSockOpt(SockID, sdContext, SLNETSOCK_LVL_PHY, SLNETSOCK_OPPHY_PREAMBLE, &preamble, sizeof(preamble));
    \endcode
    <br>

    - SLNETSOCK_OPPHY_TX_INHIBIT_THRESHOLD:
    \code
        uint32_t thrshld = SLNETSOCK_TX_INHIBIT_THRESHOLD_MED;
        SlNetIfWifi_setSockOpt(SockID, sdContext, SLNETSOCK_LVL_PHY, SLNETSOCK_OPPHY_TX_INHIBIT_THRESHOLD , &thrshld, sizeof(thrshld));
    \endcode
    <br>

    - SLNETSOCK_OPPHY_TX_TIMEOUT:
    \code
        uint32_t timeout = 50;
        SlNetIfWifi_setSockOpt(SockID, sdContext, SLNETSOCK_LVL_PHY, SLNETSOCK_OPPHY_TX_TIMEOUT  , &timeout, sizeof(timeout));
    \endcode
    <br>

    - SLNETSOCK_OPPHY_ALLOW_ACKS:
    \code
        uint32_t acks = 1; // 0 = disabled / 1 = enabled
        SlNetIfWifi_setSockOpt(SockID, sdContext, SLNETSOCK_LVL_PHY, SLNETSOCK_OPPHY_ALLOW_ACKS, &acks, sizeof(acks));
    \endcode
    <br>

*/
int32_t SlNetIfWifi_setSockOpt(int16_t sd, void *sdContext, int16_t level, int16_t optname, void *optval, SlNetSocklen_t optlen);

/*!
    \brief Get socket options

    The SlNetIfWifi_getSockOpt function gets the options associated with a socket.
    Options may exist at multiple protocol levels; they are always
    present at the uppermost socket level.\n

    The parameters optval and optlen identify a
    buffer in which the value for the requested option(s) are to
    be returned.  optlen is a value-result
    parameter, initially containing the size of the buffer
    pointed to by option_value, and modified on return to
    indicate the actual size of the value returned.  If no option
    value is to be supplied or returned, option_value may be
    NULL.

    \param[in]  sd              Socket descriptor (handle)
    \param[in]  sdContext       May store socket data if implemented in the
                                SlNetIfWifi_socket function.
    \param[in]  level           Defines the protocol level for this option
    \param[in]  optname         defines the option name to interrogate
    \param[out] optval          Specifies a value for the option
    \param[out] optlen          Specifies the length of the
                                option value

    \return                     Zero on success, or negative error code on failure
    \sa     SlNetIfWifi_setSockOpt
    \note
    \warning
*/
int32_t SlNetIfWifi_getSockOpt(int16_t sd, void *sdContext, int16_t level, int16_t optname, void *optval, SlNetSocklen_t *optlen);

/*!
    \brief Read data from TCP socket

    The SlNetIfWifi_recv function receives a message from a connection-mode socket

    \param[in]  sd              Socket descriptor (handle)
    \param[in]  sdContext       May store socket data if implemented in the
                                SlNetIfWifi_socket function.
    \param[out] buf             Points to the buffer where the
                                message should be stored.
    \param[in]  len             Specifies the length in bytes of
                                the buffer pointed to by the buffer argument.
                                Range: 1-16000 bytes
    \param[in]  flags           Upper 8 bits specifies the security flags
                                Lower 24 bits specifies the type of message
                                reception. On this version, the lower 24 bits are not
                                supported

    \return                     Return the number of bytes received,
                                or a negative value if an error occurred.\n
                                Using a non-blocking recv a possible negative value is SLNETERR_BSD_EAGAIN.\n
                                SLNETERR_BSD_ENOMEM may be return in case there are no resources in the system
                                  In this case try again later or increase MAX_CONCURRENT_ACTIONS

    \sa     SlNetIfWifi_recvFrom
    \note
    \warning
    \par    Examples

    - Receiving data using TCP socket:
    \code
        SlNetSock_AddrIn_t  Addr;
        SlNetSock_AddrIn_t  LocalAddr;
        int16_t AddrSize = sizeof(SlNetSock_AddrIn_t);
        int16_t SockID, newSockID;
        int16_t Status;
        int8_t Buf[RECV_BUF_LEN];

        LocalAddr.sin_family = SLNETSOCK_AF_INET;
        LocalAddr.sin_port = SlNetSock_htons(5001);
        LocalAddr.sin_addr.s_addr = 0;

        Addr.sin_family = SLNETSOCK_AF_INET;
        Addr.sin_port = SlNetSock_htons(5001);
        Addr.sin_addr.s_addr = SlNetSock_htonl(SLNETSOCK_IPV4_VAL(10,1,1,200));

        SockID = SlNetIfWifi_socket(SLNETSOCK_AF_INET, SLNETSOCK_SOCK_STREAM, 0, 0, 0);
        Status = SlNetIfWifi_bind(SockID, (SlNetSock_Addr_t *)&LocalAddr, AddrSize);
        Status = SlNetIfWifi_listen(SockID, 0);
        newSockID = SlNetIfWifi_accept(SockID, (SlNetSock_Addr_t*)&Addr, (SlNetSocklen_t*) &AddrSize);
        Status = SlNetIfWifi_recv(newSockID, Buf, 1460, 0);
    \endcode
    <br>

    - Rx transceiver mode using a raw socket:
    \code
        int8_t buffer[1536];
        int16_t sd;
        uint16_t size;
        SlNetSock_TransceiverRxOverHead_t *transHeader;
        sd = SlNetIfWifi_socket(SLNETSOCK_AF_RF, SLNETSOCK_SOCK_RAW, 11, 0, 0); // channel 11
        while(1)
        {
            size = SlNetIfWifi_recv(sd,buffer,1536,0);
            transHeader = (SlNetSock_TransceiverRxOverHead_t *)buffer;
            printf("RSSI is %d frame type is 0x%x size %d\n",transHeader->rssi,buffer[sizeof(SlNetSock_TransceiverRxOverHead_t)],size);
        }
    \endcode
*/
int32_t SlNetIfWifi_recv(int16_t sd, void *sdContext, void *buf, uint32_t len, uint32_t flags);

/*!
    \brief Read data from socket

    SlNetIfWifi_recvFrom function receives a message from a connection-mode or
    connectionless-mode socket

    \param[in]  sd              Socket descriptor (handle)
    \param[in]  sdContext       May store socket data if implemented in the
                                SlNetIfWifi_socket function.
    \param[out] buf             Points to the buffer where the message should be stored.
    \param[in]  len             Specifies the length in bytes of the buffer pointed to by the buffer argument.
                                Range: 1-16000 bytes
    \param[in]  flags           Upper 8 bits specifies the security flags
                                Lower 24 bits specifies the type of message
                                reception. On this version, the lower 24 bits are not
                                supported
    \param[in]  from            Pointer to an address structure
                                indicating the source
                                address.\n sockaddr:\n - code
                                for the address format.\n - socket address,
                                the length depends on the code
                                format
    \param[in]  fromlen         Source address structure
                                size. This parameter MUST be set to the size of the structure pointed to by addr.


    \return                     Return the number of bytes received,
                                or a negative value if an error occurred.\n
                                Using a non-blocking recv a possible negative value is SLNETERR_BSD_EAGAIN.
                                SLNETSOCK_RET_CODE_INVALID_INPUT (-2) will be returned if fromlen has incorrect length. \n
                                SLNETERR_BSD_ENOMEM may be return in case there are no resources in the system
                                  In this case try again later or increase MAX_CONCURRENT_ACTIONS

    \sa     SlNetIfWifi_recv
    \note
    \warning
    \par        Example

    - Receiving data:
    \code
        SlNetSock_AddrIn_t  Addr;
        SlNetSock_AddrIn_t  LocalAddr;
        int16_t AddrSize = sizeof(SlNetSock_AddrIn_t);
        int16_t SockID;
        int16_t Status;
        int8_t Buf[RECV_BUF_LEN];

        LocalAddr.sin_family = SLNETSOCK_AF_INET;
        LocalAddr.sin_port = SlNetSock_htons(5001);
        LocalAddr.sin_addr.s_addr = 0;

        SockID = SlNetIfWifi_socket(SLNETSOCK_AF_INET, SLNETSOCK_SOCK_DGRAM, 0, 0, 0);
        Status = SlNetIfWifi_bind(SockID, (SlNetSock_Addr_t *)&LocalAddr, AddrSize);
        Status = SlNetIfWifi_recvFrom(SockID, Buf, 1472, 0, (SlNetSock_Addr_t *)&Addr, (SlNetSocklen_t*)&AddrSize);

    \endcode
*/
int32_t SlNetIfWifi_recvFrom(int16_t sd, void *sdContext, void *buf, uint32_t len, uint32_t flags, SlNetSock_Addr_t *from, SlNetSocklen_t *fromlen);

/*!
    \brief Write data to TCP socket

    The SlNetIfWifi_send function is used to transmit a message to another socket.
    Returns immediately after sending data to device.
    In case of TCP failure an async event SLNETSOCK_SOCKET_TX_FAILED_EVENT is going to
    be received.\n
    In case of a RAW socket (transceiver mode), extra 4 bytes should be reserved at the end of the
    frame data buffer for WLAN FCS

    \param[in] sd               Socket descriptor (handle)
    \param[in] sdContext        May store socket data if implemented in the
                                SlNetIfWifi_socket function.
    \param[in] buf              Points to a buffer containing
                                the message to be sent
    \param[in] len              Message size in bytes. Range: 1-1460 bytes
    \param[in] flags            Upper 8 bits specifies the security flags
                                Lower 24 bits specifies the type of message
                                reception. On this version, the lower 24 bits are not
                                supported for TCP.
                                For transceiver mode, the SLNETSOCK_WLAN_RAW_RF_TX_PARAMS macro can be used to determine
                                transmission parameters (channel,rate,tx_power,preamble)

    \return                     Zero on success, or negative error code on failure

    \sa     SlNetIfWifi_sendTo
    \note
    \warning
    \par        Example

    - Sending data:
    \code
        SlNetSock_AddrIn_t  Addr;
        int16_t AddrSize = sizeof(SlNetSock_AddrIn_t);
        int16_t SockID;
        int16_t Status;
        int8_t Buf[SEND_BUF_LEN];

        Addr.sin_family = SLNETSOCK_AF_INET;
        Addr.sin_port = SlNetSock_htons(5001);
        Addr.sin_addr.s_addr = SlNetSock_htonl(SLNETSOCK_IPV4_VAL(10,1,1,200));

        SockID = SlNetIfWifi_socket(SLNETSOCK_AF_INET, SLNETSOCK_SOCK_STREAM, 0, 0, 0);
        Status = SlNetIfWifi_connect(SockID, (SlNetSock_Addr_t *)&Addr, AddrSize);
        Status = SlNetIfWifi_send(SockID, Buf, 1460, 0 );
    \endcode
*/
int32_t SlNetIfWifi_send(int16_t sd, void *sdContext, const void *buf, uint32_t len, uint32_t flags);

/*!
    \brief Write data to socket

    The SlNetIfWifi_sendTo function is used to transmit a message on a connectionless socket
    (connection less socket SLNETSOCK_SOCK_DGRAM,  SLNETSOCK_SOCK_RAW).\n
    Returns immediately after sending data to device.\n
    In case of transmission failure an async event SLNETSOCK_SOCKET_TX_FAILED_EVENT is going to
    be received.

    \param[in] sd               Socket descriptor (handle)
    \param[in] sdContext        May store socket data if implemented in the
                                SlNetIfWifi_socket function.
    \param[in] buf              Points to a buffer containing
                                the message to be sent
    \param[in] len              message size in bytes. Range: 1-1460 bytes
    \param[in] flags            Upper 8 bits specifies the security flags
                                Lower 24 bits specifies the type of message
                                reception. On this version, the lower 24 bits are not
                                supported
    \param[in] to               Pointer to an address structure
                                indicating the destination
                                address.\n sockaddr:\n - code
                                for the address format.\n - socket address,
                                the length depends on the code
                                format
    \param[in] tolen            Destination address structure size

    \return                     Zero on success, or negative error code on failure

    \sa     SlNetIfWifi_send
    \note
    \warning
    \par    Example

    - Sending data:
    \code
        SlNetSock_AddrIn_t  Addr;
        int16_t AddrSize = sizeof(SlNetSock_AddrIn_t);
        int16_t SockID;
        int16_t Status;
        int8_t Buf[SEND_BUF_LEN];

        Addr.sin_family = SLNETSOCK_AF_INET;
        Addr.sin_port = SlNetSock_htons(5001);
        Addr.sin_addr.s_addr = SlNetSock_htonl(SLNETSOCK_IPV4_VAL(10,1,1,200));

        SockID = SlNetIfWifi_socket(SLNETSOCK_AF_INET, SLNETSOCK_SOCK_DGRAM, 0, 0, 0);
        Status = SlNetIfWifi_sendTo(SockID, Buf, 1472, 0, (SlNetSock_Addr_t *)&Addr, AddrSize);
    \endcode
*/
int32_t SlNetIfWifi_sendTo(int16_t sd, void *sdContext, const void *buf, uint32_t len, uint32_t flags, const SlNetSock_Addr_t *to, SlNetSocklen_t tolen);


/*!
    \brief Start a security session on an opened socket

    The SlNetIfWifi_sockstartSec function is used start a security session on
    an opened socket. If the security handle is NULL the session would
    be started with the default security settings.

    \param[in] sd               Socket descriptor (handle)
    \param[in] sdContext        May store socket data if implemented in the
                                SlNetIfWifi_socket function.
    \param[in] secAttrib        Secure attribute handle
    \param[in] flags            Specifies flags. \n
                                The available flags are:
                                    - SLNETSOCK_SEC_START_SECURITY_SESSION_ONLY
                                    - SLNETSOCK_SEC_BIND_CONTEXT_ONLY
                                    - SLNETSOCK_SEC_IS_SERVER

    \return                     Zero on success, or negative error code
                                on failure

    \sa
    \note
    \warning
    \par    Example

    - start security session on an opened socket:
    \code

    \endcode
*/
int32_t SlNetIfWifi_sockstartSec(int16_t sd, void *sdContext, SlNetSockSecAttrib_t *secAttrib, uint8_t flags);


/*!
    \brief Get host IP by name\n
    Obtain the IP Address of machine on network, by machine name.

    \param[in]     ifContext    Stores interface data if CreateContext function
                                supported and implemented.
                                Can be used in all SlNetIf_Config_t functions
    \param[in]     ifBitmap     Specifies the interfaces which the host ip
                                needs to be retrieved from (according to
                                the priority until one of them will return
                                an answer).\n
                                The values of the interface identifiers
                                is defined with the prefix SLNETIF_ID_
                                which defined in slnetif.h
    \param[in]     name         Host name
    \param[in]     nameLen      Name length
    \param[out]    ipAddr       This parameter is filled in with
                                host IP addresses. In case that host name is not
                                resolved, out_ip_addr is zero.
    \param[in,out] ipAddrLen    Holds the size of the ipAddr array, when function 
                                successful, the ipAddrLen parameter will be updated with
                                the number of the IP addresses found.
    \param[in]     family       Protocol family
                                
    \return                     Zero on success, or negative on failure.\n
                                SLNETUTIL_POOL_IS_EMPTY may be return in case
                                there are no resources in the system\n
                                    In this case try again later or increase
                                    MAX_CONCURRENT_ACTIONS
                                Possible DNS error codes:
                                - SLNETUTIL_DNS_QUERY_NO_RESPONSE
                                - SLNETUTIL_DNS_NO_SERVER
                                - SLNETUTIL_DNS_QUERY_FAILED
                                - SLNETUTIL_DNS_MALFORMED_PACKET
                                - SLNETUTIL_DNS_MISMATCHED_RESPONSE

    \sa
    \note   Only one sl_NetAppDnsGetHostByName can be handled at a time.\n
            Calling this API while the same command is called from another
            thread, may result in one of the two scenarios:
            1. The command will wait (internal) until the previous command
               finish, and then be executed.
            2. There are not enough resources and POOL_IS_EMPTY error will
               return.\n
               In this case, MAX_CONCURRENT_ACTIONS can be increased (result
               in memory increase) or try again later to issue the command.
    \warning
            In case an IP address in a string format is set as input, without
            any prefix (e.g. "1.2.3.4") the device will not try to access the
            DNS and it will return the input address on the 'out_ip_addr' field
    \par  Example
    - Getting host by name:
    \code
    uint16_t DestIPListSize = 1;
    uint32_t DestIP[1];
    uint32_t ifID;
    int16_t  SockId;
    SlNetSock_AddrIn_t LocalAddr; //address of the server to connect to
    int32_t LocalAddrSize;

    SlNetIfWifi_getHostByName(0, "www.google.com", strlen("www.google.com"), (uint32_t *)DestIP, &DestIPListSize, SLNETSOCK_PF_INET);

    LocalAddr.sin_family = SLNETSOCK_AF_INET;
    LocalAddr.sin_addr.s_addr = SlNetUtil_htonl(DestIP[0]);
    LocalAddr.sin_port = SlNetUtil_htons(80);
    LocalAddrSize = sizeof(SlNetSock_AddrIn_t);

    SockId = SlNetIfWifi_socket(SLNETSOCK_AF_INET, SLNETSOCK_SOCK_STREAM, ifID, 0);

    if (SockId >= 0)
    {
        status = SlNetIfWifi_connect(SockId, (SlNetSock_Addr_t *) &LocalAddr, LocalAddrSize);
    }
    \endcode
*/
int32_t SlNetIfWifi_getHostByName(void *ifContext, char *name, const uint16_t nameLen, uint32_t *ipAddr, uint16_t *ipAddrLen, const uint8_t family);


/*!
    \brief Get IP Address of specific interface

    The SlNetIfWifi_getIPAddr function retrieve the IP address of a specific
    interface according to the Address Type, IPv4, IPv6 LOCAL
    or IPv6 GLOBAL.\n

    \n

    \param[in]  ifContext     Stores interface data if CreateContext function
                              supported and implemented.
                              Can be used in all SlNetIf_Config_t functions
    \param[in]  ifID          Specifies the interface which its connection
                              state needs to be retrieved.\n
                              The values of the interface identifier is
                              defined with the prefix SLNETIF_ID_ which
                              defined in slnetif.h
    \param[in]  addrType      Address type:
                                          - SLNETIF_IPV4_ADDR
                                          - SLNETIF_IPV6_ADDR_LOCAL
                                          - SLNETIF_IPV6_ADDR_GLOBAL
    \param[out] addrConfig    Address config:
                                          - SLNETIF_ADDR_CFG_UNKNOWN
                                          - SLNETIF_ADDR_CFG_DHCP
                                          - SLNETIF_ADDR_CFG_DHCP_LLA
                                          - SLNETIF_ADDR_CFG_STATIC
                                          - SLNETIF_ADDR_CFG_STATELESS
                                          - SLNETIF_ADDR_CFG_STATEFUL
    \param[out] ipAddr        IP Address according to the Address Type

    \return                   Zero on success, or negative error code on failure

    \sa     SlNetIfAddressType_e
    \note
    \warning
    \par   Examples

    \code
        SlNetSock_In6Addr_t IPAdd;
        uint16_t addressConfig = 0;
        SlNetIfWifi_getIPAddr(SLNETIF_ID_1 ,SLNETIF_IPV6_ADDR_LOCAL ,&addressConfig ,(uint8_t *)ipAddr);
    \endcode
    <br>
*/
int32_t SlNetIfWifi_getIPAddr(void *ifContext, SlNetIfAddressType_e addrType, uint16_t *addrConfig, uint32_t *ipAddr);


/*!
    \brief Get interface connection status

    The SlNetIfWifi_getConnectionStatus function gets the connection status of the
    interface (connected Or disconnected).\n

    \param[in] ifContext   Stores interface data if CreateContext function
                           supported and implemented.
                           Can be used in all SlNetIf_Config_t functions

    \return                Connection status of the interface on success,
                           or negative error code on failure

    \sa
    \note
    \warning
    \par    Examples

    \code
        int16_t connection_status
        connection_status = SlNetIfWifi_getConnectionStatus();
    \endcode
    <br>
*/
int32_t SlNetIfWifi_getConnectionStatus(void *ifContext);


/*!
    \brief Load secured buffer to the network stack

    The SlNetSock_secLoadObj function loads buffer/files into the inputted
    network stack for future usage of the socket SSL/TLS connection.
    This option is relevant for network stacks with file system and also for
    network stacks that lack file system that can store the secured files.

    \param[in] ifContext        Stores interface data if CreateContext function
                                supported and implemented.
                                Can be used in all SlNetIf_Config_t functions
    \param[in] objType          Specifies the security object type which
                                could be one of the following:\n
                                   - SLNETIF_SEC_OBJ_TYPE_RSA_PRIVATE_KEY
                                   - SLNETIF_SEC_OBJ_TYPE_CERTIFICATE
                                   - SLNETIF_SEC_OBJ_TYPE_DH_KEY
    \param[in] objName          Specifies the name/input identifier of the
                                secured buffer loaded
                                for file systems - this can be the file name
                                for plain text buffer loading this can be the
                                name of the object
    \param[in] objNameLen       Specifies the buffer name length to be loaded.\n
    \param[in] objBuff          Specifies the pointer to the secured buffer to
                                be loaded.\n
    \param[in] objBuffLen       Specifies the buffer length to be loaded.\n

    \return                     On success, buffer type handler index to be
                                used when attaching the secured buffer to a
                                socket.\n
                                A successful return code should be a positive
                                number (int16)\n
                                On error, a negative value will be returned
                                specifying the error code.
                                - SLNETERR_STATUS_ERROR - load operation failed

    \sa                         SlNetIfWifi_setSockOpt
    \note
    \warning
*/
int32_t SlNetIfWifi_loadSecObj(void *ifContext, uint16_t objType, char *objName, int16_t objNameLen, uint8_t *objBuff, int16_t objBuffLen);


/*!
    \brief Allocate and store interface data

    The SlNetIfWifi_CreateContext function stores interface related data.\n

    \param[in] ifContext  Allocate and store interface data if needed.
                          Can be used in all slnetwifi interface functions

    \return               Zero on success, or negative error code on failure.

    \sa
    \note
    \warning
    \par    Examples

    \code
        void *ifContext;
        connection_status = SlNetIfWifi_CreateContext(&context);
    \endcode
    <br>
*/
int32_t SlNetIfWifi_CreateContext(uint16_t ifID, const char *ifName, void **ifContext);


/*!

 Close the Doxygen group.
 @}

 */


#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif /* __SOCKET_H__ */


