/*
 * socket.h - CC31xx/CC32xx Host Driver Implementation
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
#include <ti/drivers/net/wifi/simplelink.h>
#include <ti/drivers/net/wifi/source/driver.h>

#ifndef __SOCKET_H__
#define __SOCKET_H__

#ifdef    __cplusplus
extern "C" {
#endif

#if defined(ti_sysbios_posix_sys_types__include) || defined(os_freertos_posix_sys_types__include) || defined(__GNUC__)
/* This condition is temporary until a single header guard would be applied to core SDK */
#include <sys/types.h>
#else
typedef long int ssize_t;
#endif

/*!
    \defgroup BSD_Socket
    \short Controls standard client/server sockets programming options and capabilities

*/
/*!

    \addtogroup BSD_Socket
    @{
*/

/* Socket structures */
#define socklen_t                           SlSocklen_t
#define sockaddr                            SlSockAddr_t

/* Socket opts */
#define SOCK_STREAM                         SL_SOCK_STREAM
#define SOCK_DGRAM                          SL_SOCK_DGRAM
#define SOCK_RAW                            SL_SOCK_RAW
#define AF_INET                             SL_AF_INET
#define AF_INET6                            SL_AF_INET6
#define AF_RF                               SL_AF_RF
#define AF_PACKET                           SL_AF_PACKET
#define PF_INET                             SL_PF_INET
#define PF_INET6                            SL_PF_INET6
#define SOL_SOCKET                          SL_SOL_SOCKET
#define SO_KEEPALIVE                        SL_SO_KEEPALIVE
#define SO_KEEPALIVETIME                    SL_SO_KEEPALIVETIME
#define SO_RX_NO_IP_BOUNDARY                SL_SO_RX_NO_IP_BOUNDARY
#define SO_RCVTIMEO                         SL_SO_RCVTIMEO
#define SO_RCVBUF                           SL_SO_RCVBUF
#define SO_NONBLOCKING                      SL_SO_NONBLOCKING
#define MSG_DONTWAIT                        SL_MSG_DONTWAIT
#define SO_BROADCAST                        (200)                   /* Unsupported: this is only a placeholder to not break BSD code. */
#define SO_REUSEADDR                        (201)                   /* Unsupported: this is only a placeholder to not break BSD code. */
#define SO_SNDBUF                           (202)                   /* Unsupported: this is only a placeholder to not break BSD code. */
#define TCP_NODELAY                         (203)                   /* Trivially supported: this option closes Nagel Algo. for TCP connections,
                                                                       which CC31xx \ CC32XX always close. */

/*****************************************************************************/
/* Function prototypes                                                       */
/*****************************************************************************/
/*!

    \brief Create an endpoint for communication
 
    The socket function creates a new socket of a certain socket type, identified 
    by an integer number, and allocates system resources to it.\n
    This function is called by the application layer to obtain a socket handle.
 
    \param[in] Domain           Specifies the protocol family of the created socket.
                                For example:
                                   - AF_INET for network protocol IPv4
                                   - AF_INET6 for network protocol IPv6                             

    \param[in] Type              specifies the communication semantic, one of:
                                   - SOCK_STREAM (reliable stream-oriented service or Stream Sockets)
                                   - SOCK_DGRAM (datagram service or Datagram Sockets)
                                   - SOCK_RAW (raw protocols atop the network layer)
                                   
    \param[in] Protocol         specifies a particular transport to be used with 
                                the socket. \n
                                The most common are 
                                    - IPPROTO_TCP
                                    - IPPROTO_UDP 
                                The value 0 may be used to select a default 
                                protocol from the selected domain and type
 
    \return                     On success, socket handle that is used for consequent socket operations. \n
                                A successful return code should be a positive number (int16)\n
                                On error, a negative (int16) value will be returned specifying the error code.
                                   - EAFNOSUPPORT  - illegal domain parameter
                                   - EPROTOTYPE  - illegal type parameter
                                   - EACCES   - permission denied
                                   - ENSOCK  - exceeded maximal number of socket 
                                   - ENOMEM  - memory allocation error
                                   - EINVAL  - error in socket configuration
                                   - EPROTONOSUPPORT  - illegal protocol parameter
                                   - EOPNOTSUPP  - illegal combination of protocol and type parameters
 
    \sa                         close
    \note                       belongs to \ref basic_api
    \warning
*/
#if _SL_INCLUDE_FUNC(sl_Socket)
int socket(int Domain, int Type, int Protocol);
#endif

/*!
    \brief Gracefully close socket

    This function causes the system to release resources allocated to a socket.  \n
    In case of TCP, the connection is terminated.

    \param[in] sd               Socket handle (received in socket)

    \return                     Zero on success, or negative error code on failure                               

    \sa                         socket
    \note                       belongs to \ref ext_api
    \warning
*/
#if _SL_INCLUDE_FUNC(sl_Close)
static inline int close(int sd)
{
    int RetVal = (int)sl_Close((_i16)sd);
    return _SlDrvSetErrno(RetVal);
}
#endif

/*!
    \brief Accept a connection on a socket
    
    This function is used with connection-based socket types (SOCK_STREAM).\n
    It extracts the first connection request on the queue of pending 
    connections, creates a new connected socket, and returns a new file 
    descriptor referring to that socket.\n
    The newly created socket is not in the listening state. The 
    original socket sd is unaffected by this call. \n
    The argument sd is a socket that has been created with 
    socket(), bound to a local address with bind(), and is 
    listening for connections after a listen(). The argument \b 
    \e addr is a pointer to a sockaddr structure. This structure 
    is filled in with the address of the peer socket, as known to 
    the communications layer. The exact format of the address 
    returned addr is determined by the socket's address family. \n
    The \b \e addrlen argument is a value-result argument: it 
    should initially contain the size of the structure pointed to 
    by addr, on return it will contain the actual length (in 
    bytes) of the address returned.
    
    \param[in] sd               Socket descriptor (handle)
    \param[out] addr            The argument addr is a pointer 
                                to a sockaddr structure. This
                                structure is filled in with the
                                address of the peer socket, as
                                known to the communications
                                layer. The exact format of the
                                address returned addr is
                                determined by the socket's
                                address\n
                                sockaddr:\n - code for the
                                address format. On this version
                                only AF_INET is supported.\n -
                                socket address, the length
                                depends on the code format
    \param[out] addrlen         The addrlen argument is a value-result 
                                argument: it should initially contain the
                                size of the structure pointed to by addr
    
    \return                     On success, a socket handle.\n
                                On a non-blocking accept a possible retrun is -1 and errno is set to EAGAIN.\n
                                On failure, errno is set and -1 is returned.\n
                                ENOMEM may be return in case there are no resources in the system
                                In this case try again later or increase MAX_CONCURRENT_ACTIONS.
    
    \sa                         socket  bind  listen
    \note                       Belongs to \ref server_side
    \warning
*/
#if _SL_INCLUDE_FUNC(sl_Accept)
int accept(int sd, sockaddr *addr, socklen_t *addrlen);
#endif

/*!
    \brief Assign a name to a socket
 
    This function gives the socket the local address addr.
    addr is addrlen bytes long. Traditionally, this is called
    When a socket is created with socket, it exists in a name
    space (address family) but has no name assigned.
    It is necessary to assign a local address before a SOCK_STREAM
    socket may receive connections.
 
    \param[in] sd               Socket descriptor (handle)
    \param[in] addr             Specifies the destination 
                                addrs\n sockaddr:\n - code for
                                the address format. On this
                                version only AF_INET is
                                supported.\n - socket address,
                                the length depends on the code
                                format.
    \param[in] addrlen          Contains the size of the structure pointed to by addr
 
    \return                     Zero on success, or -1 on failure and sets errno to the corresponding BDS error code.   
 
    \sa                         socket  accept listen
    \note                       belongs to \ref basic_api
    \warning
*/
#if _SL_INCLUDE_FUNC(sl_Bind)
int bind(int sd, const sockaddr *addr, socklen_t addrlen);
#endif

/*!
    \brief Listen for connections on a socket
 
    The willingness to accept incoming connections and a queue
    limit for incoming connections are specified with listen(),
    and then the connections are accepted with accept.
    The listen() call applies only to sockets of type SOCK_STREAM
    The backlog parameter defines the maximum length the queue of
    pending connections may grow to. 
 
    \param[in] sd               Socket descriptor (handle)
    \param[in] backlog          Specifies the listen queue depth. 
                                
 
    \return                     Zero on success, or -1 on failure and sets errno to the corresponding BDS error code.
 
    \sa                         socket  accept  bind
    \note                       Belongs to \ref server_side
    \warning
*/
#if _SL_INCLUDE_FUNC(sl_Listen)
int listen(int sd, int backlog);
#endif

/*!
    \brief Initiate a connection on a socket 
   
    Function connects the socket referred to by the socket 
    descriptor sd, to the address specified by addr. The addrlen 
    argument specifies the size of addr. The format of the 
    address in addr is determined by the address space of the 
    socket. If it is of type SOCK_DGRAM, this call specifies the 
    peer with which the socket is to be associated; this address 
    is that to which datagrams are to be sent, and the only 
    address from which datagrams are to be received.  If the 
    socket is of type SOCK_STREAM, this call attempts to make a 
    connection to another socket. The other socket is specified 
    by address, which is an address in the communications space 
    of the socket. 
   
   
    \param[in] sd               Socket descriptor (handle)
    \param[in] addr             Specifies the destination addr\n
                                sockaddr:\n - code for the
                                address format. On this version
                                only AF_INET is supported.\n -
                                socket address, the length
                                depends on the code format
   
    \param[in] addrlen          Contains the size of the structure pointed 
                                to by addr
 
    \return                     On success, a socket handle.\n
                                On a non-blocking connect a possible negative value is EALREADY.
                                On failure, -1 is returned and sets errno to the corresponding BDS error code.\n
                                ENOMEM may be return in case there are no resources in the system
                                In this case try again later or increase MAX_CONCURRENT_ACTIONS
 
    \sa                         sl_Socket
    \note                       belongs to \ref client_side
    \warning
*/
#if _SL_INCLUDE_FUNC(sl_Connect)
int connect(int sd, const sockaddr *addr, socklen_t addrlen);
#endif

/*!
    \brief Set socket options-
 
    This function manipulate the options associated with a socket.\n
    Options may exist at multiple protocol levels; they are always
    present at the uppermost socket level.\n
 
    When manipulating socket options the level at which the option resides
    and the name of the option must be specified.  To manipulate options at
    the socket level, level is specified as SOL_SOCKET.  To manipulate
    options at any other level the protocol number of the appropriate proto-
    col controlling the option is supplied.  For example, to indicate that an
    option is to be interpreted by the TCP protocol, level should be set to
    the protocol number of TCP; \n
 
    The parameters optval and optlen are used to access optval - 
    ues for setsockopt().  For getsockopt() they identify a 
    buffer in which the value for the requested option(s) are to 
    be returned.  For getsockopt(), optlen is a value-result 
    parameter, initially containing the size of the buffer 
    pointed to by option_value, and modified on return to 
    indicate the actual size of the value returned.  If no option 
    value is to be supplied or returned, option_value may be 
    NULL.
   
    \param[in] sd               Socket handle
    \param[in] level            Defines the protocol level for this option
                                - <b>SOL_SOCKET</b>   Socket level configurations (L4, transport layer)
                                - <b>IPPROTO_IP</b>   IP level configurations (L3, network layer)                             
    \param[in] optname          Defines the option name to interrogate
                                - <b>SOL_SOCKET</b>
                                - <b>SO_KEEPALIVE</b>  \n
                                                 Enable/Disable periodic keep alive.
                                                 Keeps TCP connections active by enabling the periodic transmission of messages \n
                                                 Timeout is 5 minutes.\n
                                                 Default: Enabled \n
                                                 This options takes SlSockKeepalive_t struct as parameter
                                  - <b>SO_KEEPALIVETIME</b>  \n
                                                 Set keep alive timeout.
                                                 Value is in seconds \n
                                                 Default: 5 minutes \n
                                  - <b>SO_RX_NO_IP_BOUNDARY</b>  \n
                                                 Enable/Disable rx ip boundary.
                                                 In connectionless socket (udp/raw), unread data is dropped (when recvfrom len parameter < data size), Enable this option in order to read the left data on the next recvfrom iteration 
                                                 Default: Disabled, IP boundary kept,  \n
                                                 This options takes SlSockRxNoIpBoundary_t struct as parameter                                               
                                  - <b>SO_RCVTIMEO</b>  \n
                                                 Sets the timeout value that specifies the maximum amount of time an input function waits until it completes. \n
                                                 Default: No timeout \n
                                                 This options takes SlTimeval_t struct as parameter
                                  - <b>SO_RCVBUF</b>  \n
                                                 Sets tcp max recv window size. \n
                                                 This options takes SlSockWinsize_t struct as parameter
                                  - <b>SO_NONBLOCKING</b> \n
                                                 Sets socket to non-blocking operation Impacts: connect, accept, send, sendto, recv and recvfrom. \n
                                                 Default: Blocking.
                                                 This options takes SlSockNonblocking_t struct as parameter
                                - <b>IPPROTO_IP</b> 
                                  - <b>IP_MULTICAST_TTL</b> \n
                                                 Set the time-to-live value of outgoing multicast packets for this socket. \n
                                                 This options takes <b>_u8</b> as parameter 
                                  - <b>IP_ADD_MEMBERSHIP</b> \n
                                                 UDP socket, Join a multicast group. \n
                                                 This options takes SlSockIpMreq_t struct as parameter
                                  - <b>IP_DROP_MEMBERSHIP</b> \n
                                                 UDP socket, Leave a multicast group \n
                                                 This options takes SlSockIpMreq_t struct as parameter                            
                                  - <b>SO_LINGER</b> \n
                                                 Socket lingers on close pending remaining send/receive packetst\n
            
 
    \param[in] optval           Specifies a value for the option
    \param[in] optlen           Specifies the length of the 
        option value
 
    \return                     Zero on success, or -1 on failure and sets errno to the corresponding BDS error code.
    
    \par Persistent                 
            All params are <b>Non- Persistent</b> 
    \sa     getsockopt
    \note   Belongs to \ref basic_api  
    \warning
            
*/
#if _SL_INCLUDE_FUNC(sl_SetSockOpt)
int setsockopt(int sd, int level, int optname, const void *optval, socklen_t optlen);
#endif


/*!
    \brief Get socket options
    
    This function manipulate the options associated with a socket.
    Options may exist at multiple protocol levels; they are always
    present at the uppermost socket level.\n
    
    When manipulating socket options the level at which the option resides
    and the name of the option must be specified.  To manipulate options at
    the socket level, level is specified as SOL_SOCKET.  To manipulate
    options at any other level the protocol number of the appropriate proto-
    col controlling the option is supplied.  For example, to indicate that an
    option is to be interpreted by the TCP protocol, level should be set to
    the protocol number of TCP; \n
    
    The parameters optval and optlen are used to access optval - 
    ues for setsockopt().  For getsockopt() they identify a 
    buffer in which the value for the requested option(s) are to 
    be returned.  For getsockopt(), optlen is a value-result 
    parameter, initially containing the size of the buffer 
    pointed to by option_value, and modified on return to 
    indicate the actual size of the value returned.  If no option 
    value is to be supplied or returned, option_value may be 
    NULL. 
    
    
    \param[in]  sd              Socket handle
    \param[in]  level           Defines the protocol level for this option
    \param[in]  optname         defines the option name to interrogate
    \param[out] optval          Specifies a value for the option
    \param[out] optlen          Specifies the length of the 
                                option value
    
    \return                     Zero on success, or -1 on failure and sets errno to the corresponding BDS error code.
    \sa     setsockopt
            Belongs to \ref ext_api
    \warning
*/
#if _SL_INCLUDE_FUNC(sl_GetSockOpt)
int getsockopt(int sd, int level, int optname, void *optval, socklen_t *optlen);
#endif

/*!
    \brief Read data from TCP socket
     
    Function receives a message from a connection-mode socket
     
    \param[in]  sd              Socket handle
    \param[out] buf             Points to the buffer where the 
                                message should be stored.
    \param[in]  len             Specifies the length in bytes of 
                                the buffer pointed to by the buffer argument. 
                                Range: 1-16000 bytes
    \param[in]  flags           Specifies the type of message 
                                reception. On this version, this parameter is not
                                supported.
    
    \return                     Return the number of bytes received, 
                                or a -1 if an error occurred. Errno is set accordingly.\n
                                Using a non-blocking recv a possible errno value is EAGAIN.\n
                                errno may be set to ENOMEM in case there are no resources in the system
                                In this case try again later or increase MAX_CONCURRENT_ACTIONS
    
    \sa     sl_RecvFrom
    \note                       Belongs to \ref recv_api
    \warning
    \par        Examples

    - Receiving data using TCP socket:
    \code    
        sockaddr_in  Addr;
        sockaddr_in  LocalAddr;
        int AddrSize = sizeof(socklen_t);
        int SockID, newSockID;
        int Status;
        char Buf[RECV_BUF_LEN];

        LocalAddr.sin_family = AF_INET;
        LocalAddr.sin_port = htons(5001);
        LocalAddr.sin_addr.s_addr = 0;

        Addr.sin_family = AF_INET;
        Addr.sin_port = htons(5001);
        Addr.sin_addr.s_addr = htonl(SL_IPV4_VAL(10,1,1,200));

        SockID = socket(AF_INET, SOCK_STREAM, 0);
        Status = bind(SockID, (sockaddr *)&LocalAddr, AddrSize);
        Status = listen(SockID, 0);
        newSockID = accept(SockID, (sockaddr *)&Addr, (socklen_t*) &AddrSize);
        Status = recv(newSockID, Buf, 1460, 0);
        
    \endcode
    <br>
  
*/
#if _SL_INCLUDE_FUNC(sl_Recv)
ssize_t recv(int sd, void *pBuf, size_t Len, int flags);
#endif

/*!
    \brief Read data from socket
    
    Function receives a message from a connection-mode or
    connectionless-mode socket
    
    \param[in]  sd              Socket handle 
    \param[out] buf             Points to the buffer where the message should be stored.
    \param[in]  len             Specifies the length in bytes of the buffer pointed to by the buffer argument.
                                Range: 1-16000 bytes
    \param[in]  flags           Specifies the type of message
                                reception. On this version, this parameter is not
                                supported.
    \param[in]  from            Pointer to an address structure 
                                indicating the source
                                address.\n sockaddr:\n - code
                                for the address format. On this
                                version only AF_INET is
                                supported.\n - socket address,
                                the length depends on the code
                                format
    \param[in]  fromlen         Source address structure
                                size. This parameter MUST be set to the size of the structure pointed to by addr.
    
    
    \return                     Return the number of bytes received, 
                                or a -1 if an error occurred. Errno is set accordingly.\n
                                Using a non-blocking recv a possible errno value is EAGAIN.\n
                                errno will be set to EINVAL if fromlen has incorrect length. \n
                                errno may be set to ENOMEM in case there are no resources in the system
                                In this case try again later or increase MAX_CONCURRENT_ACTIONS
                                
    
    \sa     recv
    \note                       Belongs to \ref recv_api
    \warning
    \par        Example

    - Receiving data:
    \code    
        sockaddr_in  Addr;
        sockaddr_in  LocalAddr;
        int AddrSize = sizeof(socklen_t);
        int SockID, newSockID;
        int Status;
        char Buf[RECV_BUF_LEN];

        LocalAddr.sin_family = AF_INET;
        LocalAddr.sin_port = htons(5001);
        LocalAddr.sin_addr.s_addr = 0;

        SockID = socket(AF_INET, SOCK_STREAM, 0);
        Status = bind(SockID, (sockaddr *)&LocalAddr, AddrSize);
        Status = recvfrom(SockID, Buf, 1472, 0, (sockaddr_in *)&Addr, (socklen_t*)&AddrSize); 

    \endcode
*/
#if _SL_INCLUDE_FUNC(sl_RecvFrom)
ssize_t recvfrom(int sd, void *buf, _i16 Len, _i16 flags, sockaddr *from, socklen_t *fromlen);
#endif

/*!
    \brief Write data to TCP socket
    
    This function is used to transmit a message to another socket.
    Returns immediately after sending data to device.
    In case of TCP failure an async event SL_SOCKET_TX_FAILED_EVENT is going to
    be received.\n
     
    \param[in] sd               Socket handle
    \param[in] buf              Points to a buffer containing 
                                the message to be sent
    \param[in] len              Message size in bytes. Range: 1-1460 bytes
    \param[in] flags            Specifies the type of message 
                                transmission. On this version, this parameter is not
                                supported for TCP.
    
    
    \return                     Zero on success, or -1 on failure and sets errno to the corresponding BDS error code.
    
    \sa     sl_SendTo 
    \note                       Belongs to \ref send_api
    \warning   
    \par        Example

    - Sending data:
    \code
        sockaddr_in  Addr;
        int AddrSize = sizeof(socklen_t);
        int SockID;
        int Status;
        char Buf[SEND_BUF_LEN];

        Addr.sin_family = AF_INET;
        Addr.sin_port = htons(5001);
        Addr.sin_addr.s_addr = htonl(SL_IPV4_VAL(10,1,1,200));

        SockID = socket(AF_INET, SOCK_STREAM, 0);
        Status = connect(SockID, (sockaddr_in*)&Addr, AddrSize);
        Status = send(SockID, Buf, 1460, 0 );
    \endcode
 */ 
#if _SL_INCLUDE_FUNC(sl_Send)
ssize_t send(int sd, const void *pBuf, _i16 Len, _i16 flags);
#endif

/*!
    \brief Write data to socket
    
    This function is used to transmit a message to another socket
    (connection less socket SOCK_DGRAM,  SOCK_RAW).\n
    Returns immediately after sending data to device.\n
    In case of transmission failure an async event SL_SOCKET_TX_FAILED_EVENT is going to
    be received.
    
    \param[in] sd               Socket handle
    \param[in] buf              Points to a buffer containing 
                                the message to be sent
    \param[in] len              message size in bytes. Range: 1-1460 bytes
    \param[in] flags            Specifies the type of message 
                                transmission. On this version, this parameter is not
                                supported 
    \param[in] to               Pointer to an address structure 
                                indicating the destination
                                address.\n sockaddr:\n - code
                                for the address format. On this
                                version only AF_INET is
                                supported.\n - socket address,
                                the length depends on the code
                                format
    \param[in] tolen            Destination address structure size 
    
    \return                     Zero on success, or -1 on failure and sets errno to the corresponding BDS error code.
    
    \sa     sl_Send
    \note                       Belongs to \ref send_api
    \warning
    \par        Example

    - Sending data:
    \code
        sockaddr_in  Addr;
        int AddrSize = sizeof(socklen_t);
        int SockID;
        int Status;
        char Buf[SEND_BUF_LEN];

        Addr.sin_family = AF_INET;
        Addr.sin_port = htons(5001);
        Addr.sin_addr.s_addr = htonl(SL_IPV4_VAL(10,1,1,200));

        SockID = socket(AF_INET, SOCK_DGRAM, 0);
        Status = sendto(SockID, Buf, 1472, 0, (sockaddr_in *)&Addr, AddrSize);
    \endcode
*/
#if _SL_INCLUDE_FUNC(sl_SendTo)
ssize_t sendto(int sd, const void *pBuf, size_t Len, int flags, const sockaddr *to, socklen_t tolen);
#endif


/*!
    \brief errno - returns error code for last failure of BSD API calling.

     if SL_INC_INTERNAL_ERRNO is enabled, when an error occurs the BSD API returns -1, in order to detect the specific error value,
     user should invoke this function.

    \return  Return number of last error

    \sa      bind  connect  recvfrom  recv  accept  sendto  send
    \warning
    \par        Example

    - Querying errno:
    \code
        Status = recvfrom(SockID, Buf, 1472, 0, (sockaddr *)&Addr, (socklen_t*)&AddrSize);
        while(Status < 0)
        {
            if(errno == EAGAIN)
            {
               Status = recvfrom(SockID, Buf, 1472, 0, (sockaddr *)&Addr, (socklen_t*)&AddrSize);
            }
            else if(errno)
            {
                printf("A socket error occurred..");
                return (close(SockID));
            }
        }
    \endcode
*/
#ifdef SL_INC_INTERNAL_ERRNO
#ifdef errno
#undef errno
#endif
#define errno       *(__errno())
#endif

/*!

 Close the Doxygen group.
 @}

 */

#ifdef  __cplusplus
}
#endif /* __cplusplus */
#endif /* __SOCKET_H__ */
