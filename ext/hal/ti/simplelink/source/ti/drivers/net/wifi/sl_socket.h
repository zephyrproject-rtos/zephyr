/*
 * sl_socket.h - CC31xx/CC32xx Host Driver Implementation
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

#ifndef __SL_SOCKET_H__
#define __SL_SOCKET_H__


#ifdef    __cplusplus
extern "C" {
#endif

/*!
    \defgroup Socket 
    \short Controls standard client/server sockets programming options and capabilities

*/
/*!

    \addtogroup Socket
    @{

*/

/*****************************************************************************/
/* Macro declarations                                                        */
/*****************************************************************************/
#undef SL_FD_SETSIZE
#define SL_FD_SETSIZE                          SL_MAX_SOCKETS         /* Number of sockets to select on - same is max sockets!               */
#define SL_BSD_SOCKET_ID_MASK                  (0x1F)                 /* Index using the LBS 4 bits for socket id 0-7 */

/* Define some BSD protocol constants.  */
#define SL_SOCK_STREAM                         (1)                       /* TCP Socket                                                          */
#define SL_SOCK_DGRAM                          (2)                       /* UDP Socket                                                          */
#define SL_SOCK_RAW                            (3)                       /* Raw socket                                                          */
#define SL_IPPROTO_TCP                         (6)                       /* TCP Raw Socket                                                      */
#define SL_IPPROTO_UDP                         (17)                      /* UDP Raw Socket                                                      */
#define SL_IPPROTO_RAW                         (255)                     /* Raw Socket                                                          */
#define SL_SEC_SOCKET                          (100)                     /* Secured Socket Layer (SSL,TLS)                                      */

/* Address families.  */
#define     SL_AF_INET                         (2)                       /* IPv4 socket (UDP, TCP, etc)                                          */
#define     SL_AF_INET6                        (3)                       /* IPv6 socket (UDP, TCP, etc)                                          */
#define     SL_AF_RF                           (6)                       /* data include RF parameter, All layer by user (Wifi could be disconnected) */ 
#define     SL_AF_PACKET                       (17)
/* Protocol families, same as address families.  */
#define     SL_PF_INET                         AF_INET
#define     SL_PF_INET6                        AF_INET6
#define     SL_INADDR_ANY                      (0)                       /*  bind any address  */
#define     SL_IN6ADDR_ANY                     (0)


/* Max payload size by protocol */
#define SL_SOCKET_PAYLOAD_TYPE_MASK            (0xF0)  /*4 bits type, 4 bits sockets id */
#define SL_SOCKET_PAYLOAD_TYPE_RAW_TRANCEIVER  (0x80)  /* 1536 bytes */

  
/* SL_SOCKET_EVENT_CLASS_BSD user events */               
#define    SL_SOCKET_TX_FAILED_EVENT              (1) 
#define    SL_SOCKET_ASYNC_EVENT                  (2)


/* SL_SOCKET_EVENT_CLASS_BSD user trigger events */               
#define    SL_SOCKET_TRIGGER_EVENT_SELECT         (1) 

#define SL_SOL_SOCKET          (1)   /* Define the socket option category. */
#define SL_IPPROTO_IP          (2)   /* Define the IP option category.     */
#define SL_SOL_PHY_OPT         (3)   /* Define the PHY option category.    */

#define SL_SO_RCVBUF           (8)   /* Setting TCP receive buffer size */
#define SL_SO_KEEPALIVE        (9)   /* Connections are kept alive with periodic messages */
#define SL_SO_LINGER           (13)  /* Socket lingers on close pending remaining send/receive packets. */
#define SL_SO_RCVTIMEO         (20)  /* Enable receive timeout */
#define SL_SO_NONBLOCKING      (24)  /* Enable . disable nonblocking mode  */
#define SL_SO_SECMETHOD        (25)  /* security metohd */
#define SL_SO_SECURE_MASK      (26)  /* security mask */
#define SL_SO_SECURE_FILES     (27)  /* security files */
#define SL_SO_CHANGE_CHANNEL   (28)  /* This option is available only when transceiver started */
#define SL_SO_SECURE_FILES_PRIVATE_KEY_FILE_NAME (30) /* This option used to configue secure file */
#define SL_SO_SECURE_FILES_CERTIFICATE_FILE_NAME (31) /* This option used to configue secure file */
#define SL_SO_SECURE_FILES_CA_FILE_NAME          (32) /* This option used to configue secure file */
#define SL_SO_SECURE_FILES_PEER_CERT_OR_DH_KEY_FILE_NAME      (33) /* This option used to configue secure file - in server mode DH params file, and in client mode peer cert for domain verification */
#define SL_SO_STARTTLS                           (35)  /* initiate STARTTLS on non secure socket */
#define SL_SO_SSL_CONNECTION_PARAMS              (36)  /* retrieve by getsockopt the connection params of the current SSL connection in to SlSockSSLConnectionParams_t*/
#define SL_SO_KEEPALIVETIME                      (37)  /* keepalive time out  */
#define SL_SO_SECURE_DISABLE_CERTIFICATE_STORE   (38)  /* disable certificate store */
#define SL_SO_RX_NO_IP_BOUNDARY                  (39)  /* connectionless socket disable rx boundary */
#define SL_SO_SECURE_ALPN       (40) /* set the ALPN bitmap list */
#define SL_SO_SECURE_EXT_CLIENT_CHLNG_RESP (41) /*set external challange for client certificate */
#define SL_SO_SECURE_DOMAIN_NAME_VERIFICATION (42) /* set a domain name for verification */

#define SL_IP_MULTICAST_IF      (60) /* Specify outgoing multicast interface */
#define SL_IP_MULTICAST_TTL     (61) /* Specify the TTL value to use for outgoing multicast packet. */
#define SL_IP_ADD_MEMBERSHIP    (65) /* Join IPv4 multicast membership */
#define SL_IP_DROP_MEMBERSHIP   (66) /* Leave IPv4 multicast membership */
#define SL_IP_HDRINCL           (67) /* Raw socket IPv4 header included. */
#define SL_IP_RAW_RX_NO_HEADER  (68) /* Proprietary socket option that does not includeIPv4/IPv6 header (and extension headers) on received raw sockets*/
#define SL_IP_RAW_IPV6_HDRINCL  (69) /* Transmitted buffer over IPv6 socket contains IPv6 header. */
#define SL_IPV6_ADD_MEMBERSHIP  (70) /* Join IPv6 multicast membership */
#define SL_IPV6_DROP_MEMBERSHIP (71) /* Leave IPv6 multicast membership */
#define SL_IPV6_MULTICAST_HOPS  (72) /* Specify the hops value to use for outgoing multicast packet. */

#define SL_SO_PHY_RATE              (100)   /* WLAN Transmit rate */
#define SL_SO_PHY_TX_POWER          (101)   /* TX Power level */  
#define SL_SO_PHY_NUM_FRAMES_TO_TX  (102)   /* Number of frames to transmit */
#define SL_SO_PHY_PREAMBLE          (103)   /* Preamble for transmission */
#define SL_SO_PHY_TX_INHIBIT_THRESHOLD (104)   /* TX Inhibit Threshold (CCA) */
#define SL_SO_PHY_TX_TIMEOUT           (105)   /* TX timeout for Transceiver frames (lifetime) in miliseconds (max value is 100ms) */
#define SL_SO_PHY_ALLOW_ACKS           (106)   /* Enable sending ACKs in transceiver mode */

typedef enum
{
  SL_TX_INHIBIT_THRESHOLD_MIN = 1,
  SL_TX_INHIBIT_THRESHOLD_LOW = 2,
  SL_TX_INHIBIT_THRESHOLD_DEFAULT = 3,
  SL_TX_INHIBIT_THRESHOLD_MED = 4,
  SL_TX_INHIBIT_THRESHOLD_HIGH = 5,
  SL_TX_INHIBIT_THRESHOLD_MAX = 6
} SlTxInhibitThreshold_e;

#define SL_SO_SEC_METHOD_SSLV3                             (0)  /* security metohd SSL v3*/
#define SL_SO_SEC_METHOD_TLSV1                             (1)  /* security metohd TLS v1*/
#define SL_SO_SEC_METHOD_TLSV1_1                           (2)  /* security metohd TLS v1_1*/
#define SL_SO_SEC_METHOD_TLSV1_2                           (3)  /* security metohd TLS v1_2*/
#define SL_SO_SEC_METHOD_SSLv3_TLSV1_2                     (4)  /* use highest possible version from SSLv3 - TLS 1.2*/
#define SL_SO_SEC_METHOD_DLSV1                             (5)  /* security metohd DTL v1  */

#define SL_SEC_MASK_SSL_RSA_WITH_RC4_128_SHA                         (1 << 0)
#define SL_SEC_MASK_SSL_RSA_WITH_RC4_128_MD5                         (1 << 1)
#define SL_SEC_MASK_TLS_RSA_WITH_AES_256_CBC_SHA                     (1 << 2)
#define SL_SEC_MASK_TLS_DHE_RSA_WITH_AES_256_CBC_SHA                 (1 << 3)
#define SL_SEC_MASK_TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA               (1 << 4)
#define SL_SEC_MASK_TLS_ECDHE_RSA_WITH_RC4_128_SHA                   (1 << 5)
#define SL_SEC_MASK_TLS_RSA_WITH_AES_128_CBC_SHA256                  (1 << 6)
#define SL_SEC_MASK_TLS_RSA_WITH_AES_256_CBC_SHA256                  (1 << 7)
#define SL_SEC_MASK_TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA256            (1 << 8)
#define SL_SEC_MASK_TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA256          (1 << 9)
#define SL_SEC_MASK_TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA             (1 << 10)
#define SL_SEC_MASK_TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA             (1 << 11)
#define SL_SEC_MASK_TLS_RSA_WITH_AES_128_GCM_SHA256                  (1 << 12)
#define SL_SEC_MASK_TLS_RSA_WITH_AES_256_GCM_SHA384                  (1 << 13)
#define SL_SEC_MASK_TLS_DHE_RSA_WITH_AES_128_GCM_SHA256              (1 << 14)
#define SL_SEC_MASK_TLS_DHE_RSA_WITH_AES_256_GCM_SHA384              (1 << 15)
#define SL_SEC_MASK_TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256            (1 << 16)
#define SL_SEC_MASK_TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384            (1 << 17)
#define SL_SEC_MASK_TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256          (1 << 18)
#define SL_SEC_MASK_TLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384          (1 << 19)
#define SL_SEC_MASK_TLS_ECDHE_ECDSA_WITH_CHACHA20_POLY1305_SHA256    (1 << 20)
#define SL_SEC_MASK_TLS_ECDHE_RSA_WITH_CHACHA20_POLY1305_SHA256      (1 << 21)
#define SL_SEC_MASK_TLS_DHE_RSA_WITH_CHACHA20_POLY1305_SHA256        (1 << 22)

#define SL_SEC_MASK_SECURE_DEFAULT                         ((SL_SEC_MASK_TLS_DHE_RSA_WITH_CHACHA20_POLY1305_SHA256  <<  1)  -  1)

#define SL_SECURE_ALPN_H1               (1 << 0)
#define SL_SECURE_ALPN_H2               (1 << 1)
#define SL_SECURE_ALPN_H2C              (1 << 2)
#define SL_SECURE_ALPN_H2_14            (1 << 3)
#define SL_SECURE_ALPN_H2_16            (1 << 4)
#define SL_SECURE_ALPN_FULL_LIST        ((SL_SECURE_ALPN_H2_16 << 1 ) - 1)


#define SL_MSG_DONTWAIT                                   (0x00000008)  /* Nonblocking IO */

/* AP DHCP Server - IP Release reason code */
#define SL_IP_LEASE_PEER_RELEASE     (0)
#define SL_IP_LEASE_PEER_DECLINE     (1)
#define SL_IP_LEASE_EXPIRED          (2)

/* possible types when receiving SL_SOCKET_ASYNC_EVENT*/
#define SL_SSL_ACCEPT                                        (0) /* accept failed due to ssl issue ( tcp pass) */
#define SL_RX_FRAGMENTATION_TOO_BIG                          (1) /* connection less mode, rx packet fragmentation > 16K, packet is being released */
#define SL_OTHER_SIDE_CLOSE_SSL_DATA_NOT_ENCRYPTED           (2) /* remote side down from secure to unsecure */
#define SL_SSL_NOTIFICATION_CONNECTED_SECURED                (3) /* STARTTLS success */
#define SL_SSL_NOTIFICATION_HANDSHAKE_FAILED                 (4) /* STARTTLS handshake faild */
#define SL_SSL_NOTIFICATION_WRONG_ROOT_CA                    (5) /* Root CA configured is wrong, the name is in SocketAsyncEvent.EventData.extraInfo */
#define SL_SOCKET_ASYNC_EVENT_SSL_NOTIFICATION_WRONG_ROOT_CA (5)
#define SL_MAX_ISSUER_AND_SUBJECT_NAME_LEN                   (16)

/*****************************************************************************/
/* Structure/Enum declarations                                               */
/*****************************************************************************/

/* Internet address   */
typedef struct SlInAddr_t
{
#ifndef s_addr
    _u32           s_addr;             /* Internet address 32 bits */
#else
    union S_un {
       struct { _u8 s_b1,s_b2,s_b3,s_b4; } S_un_b;
       struct { _u16 s_w1,s_w2; } S_un_w;
        _u32 S_addr;
    } S_un;
#endif
}SlInAddr_t;

/* IpV6 or Ipv6 EUI64 */
typedef struct SlIn6Addr_t
{
    union 
    {
        _u8   _S6_u8[16];
        _u32  _S6_u32[4];
    } _S6_un;
}SlIn6Addr_t;


/* sockopt */
typedef struct 
{
    _u32 KeepaliveEnabled; /* 0 = disabled;1 = enabled; default = 1*/
}SlSockKeepalive_t;

typedef struct 
{
    _u32 ReuseaddrEnabled; /* 0 = disabled; 1 = enabled; default = 1*/
}SlSockReuseaddr_t;

typedef struct 
{
    _i32 RxIpNoBoundaryEnabled;  /* 0 = keep IP boundary; 1 = don`t keep ip boundary; default = 0; */
} SlSockRxNoIpBoundary_t;


typedef struct 
{
    _u32 WinSize;          /* receive window size for tcp sockets  */
}SlSockWinsize_t;

typedef struct 
{
    _u32 NonBlockingEnabled;/* 0 = disabled;1 = enabled;default = 1*/
}SlSockNonblocking_t;

typedef struct
{
    _u8   Sd;
    _u8   Type;
    _i16  Val;
    _i8   pExtraInfo[128];
} SlSocketAsyncEvent_t;

typedef struct
{
   _i16        Status;
    _u8        Sd;
    _u8        Padding;
} SlSockTxFailEventData_t;


typedef union
{
    SlSockTxFailEventData_t   SockTxFailData;
    SlSocketAsyncEvent_t      SockAsyncData;
} SlSockEventData_u;


typedef struct
{
   _u32                    Event;
    SlSockEventData_u      SocketAsyncEvent;
} SlSockEvent_t;

typedef struct
{
   _u32         Event;
   _u32         EventData;
} SlSockTriggerEvent_t;


typedef struct
{
    _u32    SecureALPN;
} SlSockSecureALPN_t;

typedef struct
{
    _u32    SecureMask;
} SlSockSecureMask_t;

typedef struct
{
    _u8     SecureMethod;
} SlSockSecureMethod_t;

typedef struct
{
    _u16 SubjectNameXoredSha1;
    _u16 IssuerNameXoredSha1;
    _i8  FromDate[8];
    _i8  ToDate[8];
    _i8  SubjectName[SL_MAX_ISSUER_AND_SUBJECT_NAME_LEN];
    _i8  IssuerName[SL_MAX_ISSUER_AND_SUBJECT_NAME_LEN];
    _i8  SubjectNameLen;
    _i8  IssuerNameLen; 
    _i8  Padding[2];
} SlSockSSLCertInfo_t;


typedef struct
{
    _u32     SecureVersion;              /* what version of SSL decided in the handshake    */
    _u32     SecureCipherSuit;           /* what Cipher Index was decided in the handshake  */
    _u32     SecureIsPeerValidated;      /* was the other peer verified         */
    _u32     SecureALPNChosenProtocol;    /* bit indicate one of the protocol defined above
                                         SL_SECURE_ALPN_H1   
                                         SL_SECURE_ALPN_H2   
                                         SL_SECURE_ALPN_H2C  
                                         SL_SECURE_ALPN_H2_14
                                         SL_SECURE_ALPN_H2_16
                                         */
    SlSockSSLCertInfo_t SecurePeerCertinfo;
} SlSockSSLConnectionParams_t;

typedef enum
{
  SL_BSD_SECURED_PRIVATE_KEY_IDX = 0,
  SL_BSD_SECURED_CERTIFICATE_IDX,
  SL_BSD_SECURED_CA_IDX,
  SL_BSD_SECURED_DH_IDX
}SlSockSecureSocketFilesIndex_e;

typedef struct 
{
    SlInAddr_t   imr_multiaddr;     /* The IPv4 multicast address to join */
    SlInAddr_t   imr_interface;     /* The interface to use for this group */
}SlSockIpMreq_t;

typedef struct{
    SlIn6Addr_t ipv6mr_multiaddr; /* IPv6 multicast address of group */
    _u32        ipv6mr_interface; /*should be 0 to choose the default multicast interface*/
}SlSockIpV6Mreq_t;

typedef struct 
{
    _u32 l_onoff;                            /* 0 = disabled; 1 = enabled; default = 0;*/
    _u32 l_linger;                           /* linger time in seconds; default = 0;*/
}SlSocklinger_t;

/* sockopt */
typedef _i32   SlTime_t;
typedef _i32   SlSuseconds_t;

typedef struct SlTimeval_t
{
    SlTime_t          tv_sec;             /* Seconds      */
    SlSuseconds_t     tv_usec;            /* Microseconds */
}SlTimeval_t;

typedef _u16 SlSocklen_t;

/* IpV4 socket address */
typedef struct SlSockAddr_t
{
    _u16          sa_family;     /* Address family (e.g. , AF_INET)     */
    _u8           sa_data[14];  /* Protocol- specific address information*/
}SlSockAddr_t;


typedef struct SlSockAddrIn6_t
{
    _u16           sin6_family;                 /* AF_INET6 || AF_INET6_EUI_48*/
    _u16           sin6_port;                   /* Transport layer port.  */
    _u32           sin6_flowinfo;               /* IPv6 flow information. */
    SlIn6Addr_t    sin6_addr;                   /* IPv6 address. */
    _u32           sin6_scope_id;               /* set of interfaces for a scope. */
}SlSockAddrIn6_t;

/* Socket address, Internet style. */

typedef struct SlSockAddrIn_t
{
    _u16              sin_family;         /* Internet Protocol (AF_INET).                    */
    _u16              sin_port;           /* Address port (16 bits).                         */
    SlInAddr_t        sin_addr;           /* Internet address (32 bits).                     */
    _i8               sin_zero[8];        /* Not used.                                       */
}SlSockAddrIn_t;


typedef struct
{
    _u8  SecureFiles[4];
}SlSockSecureFiles_t;


typedef struct SlFdSet_t                    /* The select socket array manager */
{ 
   _u32        fd_array[(SL_FD_SETSIZE + (_u8)31)/(_u8)32]; /* Bit map of SOCKET Descriptors */
} SlFdSet_t;

typedef struct
{
    _u8   Rate;               /* Recevied Rate  */
    _u8   Channel;            /* The received channel*/
    _i8   Rssi;               /* The computed RSSI value in db of current frame */
    _u8   Padding;            /* pad to align to 32 bits */
    _u32  Timestamp;          /* Timestamp in microseconds,     */
}SlTransceiverRxOverHead_t;



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
                                   - SL_AF_INET for network protocol IPv4
                                   - SL_AF_INET6 for network protocol IPv6
                                   - SL_AF_RF for starting transceiver mode. Notes: 
                                        - sending and receiving any packet overriding 802.11 header
                                        - for optimized power consumption the socket will be started in TX 
                                            only mode until receive command is activated

    \param[in] Type              specifies the communication semantic, one of:
                                   - SL_SOCK_STREAM (reliable stream-oriented service or Stream Sockets)
                                   - SL_SOCK_DGRAM (datagram service or Datagram Sockets)
                                   - SL_SOCK_RAW (raw protocols atop the network layer)
                                   - when used with AF_RF:
                                      - SL_SOCK_DGRAM - L2 socket
                                      - SL_SOCK_RAW - L1 socket - bypass WLAN CCA (Clear Channel Assessment)

    \param[in] Protocol         specifies a particular transport to be used with 
                                the socket. \n
                                The most common are 
                                    - SL_IPPROTO_TCP
                                    - SL_IPPROTO_UDP 
                                The value 0 may be used to select a default 
                                protocol from the selected domain and type
 
    \return                     On success, socket handle that is used for consequent socket operations. \n
                                A successful return code should be a positive number (int16)\n
                                On error, a negative (int16) value will be returned specifying the error code.
                                   - SL_EAFNOSUPPORT  - illegal domain parameter
                                   - SL_EPROTOTYPE  - illegal type parameter
                                   - SL_EACCES   - permission denied
                                   - SL_ENSOCK  - exceeded maximal number of socket 
                                   - SL_ENOMEM  - memory allocation error
                                   - SL_EINVAL  - error in socket configuration
                                   - SL_EPROTONOSUPPORT  - illegal protocol parameter
                                   - SL_EOPNOTSUPP  - illegal combination of protocol and type parameters
 
    \sa                         sl_Close
    \note                       belongs to \ref basic_api
    \warning
*/
#if _SL_INCLUDE_FUNC(sl_Socket)
_i16 sl_Socket(_i16 Domain, _i16 Type, _i16 Protocol);
#endif

/*!
    \brief Gracefully close socket

    This function causes the system to release resources allocated to a socket.  \n
    In case of TCP, the connection is terminated.

    \param[in] sd               Socket handle (received in sl_Socket)

    \return                     Zero on success, or negative error code on failure                               

    \sa                         sl_Socket
    \note                       belongs to \ref ext_api
    \warning
*/
#if _SL_INCLUDE_FUNC(sl_Close)
_i16 sl_Close(_i16 sd);
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
    sl_Socket(), bound to a local address with sl_Bind(), and is 
    listening for connections after a sl_Listen(). The argument \b 
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
                                On a non-blocking accept a possible negative value is SL_EAGAIN.\n
                                On failure, negative error code.\n
                                SL_POOL_IS_EMPTY may be return in case there are no resources in the system
                                 In this case try again later or increase MAX_CONCURRENT_ACTIONS
    
    \sa                         sl_Socket  sl_Bind  sl_Listen
    \note                       Belongs to \ref server_side
    \warning
*/
#if _SL_INCLUDE_FUNC(sl_Accept)
_i16 sl_Accept(_i16 sd, SlSockAddr_t *addr, SlSocklen_t *addrlen);
#endif

/*!
    \brief Assign a name to a socket
 
    This function gives the socket the local address addr.
    addr is addrlen bytes long. Traditionally, this is called
    When a socket is created with socket, it exists in a name
    space (address family) but has no name assigned.
    It is necessary to assign a local address before a SOCK_STREAM
    socket may receive connections.
 
    \param[in] sd                Socket descriptor (handle)
    \param[in] addr              Specifies the destination 
                                addrs\n sockaddr:\n - code for
                                the address format. On this
                                version only SL_AF_INET is
                                supported.\n - socket address,
                                the length depends on the code
                                format
    \param[in] addrlen          Contains the size of the structure pointed to by addr
 
    \return                     Zero on success, or negative error code on failure    
 
    \sa                         sl_Socket  sl_Accept sl_Listen
    \note                       belongs to \ref basic_api
    \warning
*/
#if _SL_INCLUDE_FUNC(sl_Bind)
_i16 sl_Bind(_i16 sd, const SlSockAddr_t *addr, _i16 addrlen);
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
                                
 
    \return                     Zero on success, or negative error code on failure 
 
    \sa                         sl_Socket  sl_Accept  sl_Bind
    \note                       Belongs to \ref server_side
    \warning
*/
#if _SL_INCLUDE_FUNC(sl_Listen)
_i16 sl_Listen(_i16 sd, _i16 backlog);
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
                                On a non-blocking connect a possible negative value is SL_EALREADY.
                                On failure, negative value.\n
                                SL_POOL_IS_EMPTY may be return in case there are no resources in the system
                                  In this case try again later or increase MAX_CONCURRENT_ACTIONS
 
    \sa                         sl_Socket
    \note                       belongs to \ref client_side
    \warning
*/
#if _SL_INCLUDE_FUNC(sl_Connect)
_i16 sl_Connect(_i16 sd, const SlSockAddr_t *addr, _i16 addrlen);
#endif

/*!
    \brief Monitor socket activity
   
    Select allow a program to monitor multiple file descriptors,
    waiting until one or more of the file descriptors become 
    "ready" for some class of I/O operation.
    If trigger mode is enabled the active fdset is the one that was retrieved in the first triggered call.
    To enable the trigger mode, an handler must be statically registered as slcb_SocketTriggerEventHandler in user.h
   
   
    \param[in]  nfds        The highest-numbered file descriptor in any of the
                            three sets, plus 1.
    \param[out] readsds     Socket descriptors list for read monitoring and accept monitoring
    \param[out] writesds    Socket descriptors list for connect monitoring only, write monitoring is not supported
    \param[out] exceptsds   Socket descriptors list for exception monitoring, not supported.
    \param[in]  timeout     Is an upper bound on the amount of time elapsed
                            before select() returns. Null or above 0xffff seconds means 
                            infinity timeout. The minimum timeout is 10 milliseconds,
                            less than 10 milliseconds will be set automatically to 10 milliseconds. 
                            Max microseconds supported is 0xfffc00.
                            In trigger mode the timout fields must be set to zero.
   
    \return                 On success, select()  returns the number of
                            file descriptors contained in the three returned
                            descriptor sets (that is, the total number of bits that
                            are set in readfds, writefds, exceptfds) which may be
                            zero if the timeout expires before anything interesting
                            happens.\n On error, a negative value is returned.
                            readsds - return the sockets on which read request will
                            return without delay with valid data.\n
                            writesds - return the sockets on which write request 
                            will return without delay.\n
                            exceptsds - return the sockets closed recently. \n
                            SL_POOL_IS_EMPTY may be return in case there are no resources in the system
                            In this case try again later or increase MAX_CONCURRENT_ACTIONS
 
    \sa     sl_Socket
    \note   If the timeout value set to less than 10ms it will automatically set 
            to 10ms to prevent overload of the system\n
            Belongs to \ref basic_api
            
            Several threads can call sl_Select at the same time.\b
            Calling this API while the same command is called from another thread, may result
                in one of the following scenarios:
            1. The command will be executed alongside other select callers (success).
            2. The command will wait (internal) until the previous sl_select finish, and then be executed.
            3. There are not enough resources and SL_POOL_IS_EMPTY error will return. 
            In this case, MAX_CONCURRENT_ACTIONS can be increased (result in memory increase) or try
            again later to issue the command.

            In case all the user sockets are open, sl_Select will exhibit the behaviour mentioned in (2)
            This is due to the fact sl_select supports multiple callers by utilizing one user socket internally.
            User who wish to ensure multiple select calls at any given time, must reserve one socket out of the 16 given.
   
    \warning
            multiple select calls aren't supported when trigger mode is active. The two are mutually exclusive.
*/
#if _SL_INCLUDE_FUNC(sl_Select)
_i16 sl_Select(_i16 nfds, SlFdSet_t *readsds, SlFdSet_t *writesds, SlFdSet_t *exceptsds, struct SlTimeval_t *timeout);
#endif

/*!
    \cond DOXYGEN_IGNORE
*/

/*!
    \brief Select's SlFdSet_t SET function
   
    Sets current socket descriptor on SlFdSet_t container
*/
void SL_SOCKET_FD_SET(_i16 fd, SlFdSet_t *fdset);

/*!
    \brief Select's SlFdSet_t CLR function
   
    Clears current socket descriptor on SlFdSet_t container
*/
void SL_SOCKET_FD_CLR(_i16 fd, SlFdSet_t *fdset);


/*!
    \brief Select's SlFdSet_t ISSET function
   
    Checks if current socket descriptor is set (TRUE/FALSE)

    \return            Returns TRUE if set, FALSE if unset

*/
_i16  SL_SOCKET_FD_ISSET(_i16 fd, SlFdSet_t *fdset);

/*!
    \brief Select's SlFdSet_t ZERO function
   
    Clears all socket descriptors from SlFdSet_t
*/
void SL_SOCKET_FD_ZERO(SlFdSet_t *fdset);

/*!
    \endcond
*/


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
                                - <b>SL_SOL_SOCKET</b>   Socket level configurations (L4, transport layer)
                                - <b>SL_IPPROTO_IP</b>   IP level configurations (L3, network layer)
                                - <b>SL_SOL_PHY_OPT</b>  Link level configurations (L2, link layer)
    \param[in] optname          Defines the option name to interrogate
                                - <b>SL_SOL_SOCKET</b>
                                - <b>SL_SO_KEEPALIVE</b>  \n
                                                 Enable/Disable periodic keep alive.
                                                 Keeps TCP connections active by enabling the periodic transmission of messages \n
                                                 Timeout is 5 minutes.\n
                                                 Default: Enabled \n
                                                 This options takes SlSockKeepalive_t struct as parameter
                                  - <b>SL_SO_KEEPALIVETIME</b>  \n
                                                 Set keep alive timeout.
                                                 Value is in seconds \n
                                                 Default: 5 minutes \n
                                  - <b>SL_SO_RX_NO_IP_BOUNDARY</b>  \n
                                                 Enable/Disable rx ip boundary.
                                                 In connectionless socket (udp/raw), unread data is dropped (when recvfrom len parameter < data size), Enable this option in order to read the left data on the next recvfrom iteration 
                                                 Default: Disabled, IP boundary kept,  \n
                                                 This options takes SlSockRxNoIpBoundary_t struct as parameter                                               
                                  - <b>SL_SO_RCVTIMEO</b>  \n
                                                 Sets the timeout value that specifies the maximum amount of time an input function waits until it completes. \n
                                                 Default: No timeout \n
                                                 This options takes SlTimeval_t struct as parameter
                                  - <b>SL_SO_RCVBUF</b>  \n
                                                 Sets tcp max recv window size. \n
                                                 This options takes SlSockWinsize_t struct as parameter
                                  - <b>SL_SO_NONBLOCKING</b> \n
                                                 Sets socket to non-blocking operation Impacts: connect, accept, send, sendto, recv and recvfrom. \n
                                                 Default: Blocking.
                                                 This options takes SlSockNonblocking_t struct as parameter
                                  - <b>SL_SO_SECMETHOD</b> \n
                                                 Sets method to tcp secured socket (SL_SEC_SOCKET) \n
                                                 Default: SL_SO_SEC_METHOD_SSLv3_TLSV1_2 \n
                                                 This options takes SlSockSecureMethod_t struct as parameter
                                  - <b>SL_SO_SECURE_MASK</b> \n
                                                 Sets specific cipher to tcp secured socket (SL_SEC_SOCKET) \n
                                                 Default: "Best" cipher suitable to method \n
                                                 This options takes SlSockSecureMask_t struct as parameter
                                  - <b>SL_SO_SECURE_FILES_CA_FILE_NAME</b> \n
                                                 Map secured socket to CA file by name \n
                                                 This options takes <b>_u8</b> buffer as parameter 
                                  - <b>SL_SO_SECURE_FILES_PRIVATE_KEY_FILE_NAME</b> \n
                                                 Map secured socket to private key by name \n
                                                 This options takes <b>_u8</b> buffer as parameter 
                                  - <b>SL_SO_SECURE_FILES_CERTIFICATE_FILE_NAME</b> \n
                                                 Map secured socket to certificate file by name \n
                                                 This options takes <b>_u8</b> buffer as parameter 
                                  - <b>SL_SO_SECURE_FILES_DH_KEY_FILE_NAME</b> \n
                                                 Map secured socket to Diffie Hellman file by name \n
                                                 This options takes <b>_u8</b> buffer as parameter 
                                  - <b>SL_SO_CHANGE_CHANNEL</b> \n
                                                 Sets channel in transceiver mode.
                                                 This options takes <b>_u32</b> as channel number parameter
                                  - <b>SL_SO_SECURE_ALPN</b> \n
                                                 Sets the ALPN list. the parameter is a bit map consist of or of the following values -
                                                 SL_SECURE_ALPN_H1       
                                                 SL_SECURE_ALPN_H2       
                                                 SL_SECURE_ALPN_H2C      
                                                 SL_SECURE_ALPN_H2_14    
                                                 SL_SECURE_ALPN_H2_16    
                                                 SL_SECURE_ALPN_FULL_LIST
                                - <b>SL_IPPROTO_IP</b> 
                                  - <b>SL_IP_MULTICAST_TTL</b> \n
                                                 Set the time-to-live value of outgoing multicast packets for this socket. \n
                                                 This options takes <b>_u8</b> as parameter 
                                  - <b>SL_IP_ADD_MEMBERSHIP</b> \n
                                                 UDP socket, Join a multicast group. \n
                                                 This options takes SlSockIpMreq_t struct as parameter
                                  - <b>SL_IP_DROP_MEMBERSHIP</b> \n
                                                 UDP socket, Leave a multicast group \n
                                                 This options takes SlSockIpMreq_t struct as parameter
                                  - <b>SL_IP_RAW_RX_NO_HEADER</b> \n                 
                                                 Raw socket remove IP header from received data. \n
                                                 Default: data includes ip header \n
                                                 This options takes <b>_u32</b> as parameter
                                  - <b>SL_IP_HDRINCL</b> \n
                                                 RAW socket only, the IPv4 layer generates an IP header when sending a packet unless \n
                                                 the IP_HDRINCL socket option is enabled on the socket.    \n
                                                 When it is enabled, the packet must contain an IP header. \n
                                                 Default: disabled, IPv4 header generated by Network Stack \n
                                                 This options takes <b>_u32</b> as parameter
                                  - <b>SL_IP_RAW_IPV6_HDRINCL</b> (inactive) \n
                                                 RAW socket only, the IPv6 layer generates an IP header when sending a packet unless \n
                                                 the IP_HDRINCL socket option is enabled on the socket. When it is enabled, the packet must contain an IP header \n
                                                 Default: disabled, IPv4 header generated by Network Stack \n
                                                 This options takes <b>_u32</b> as parameter
                                - <b>SL_SOL_PHY_OPT</b>
                                  - <b>SL_SO_PHY_RATE</b> \n
                                                 RAW socket, set WLAN PHY transmit rate \n
                                                 The values are based on SlWlanRateIndex_e    \n
                                                 This options takes <b>_u32</b> as parameter
                                  - <b>SL_SO_PHY_TX_POWER</b> \n
                                                 RAW socket, set WLAN PHY TX power \n
                                                 Valid rage is 1-15 \n
                                                 This options takes <b>_u32</b> as parameter
                                  - <b>SL_SO_PHY_NUM_FRAMES_TO_TX</b> \n
                                                 RAW socket, set number of frames to transmit in transceiver mode.
                                                 Default: 1 packet
                                                 This options takes <b>_u32</b> as parameter
                                  - <b>SL_SO_PHY_PREAMBLE</b> \n  
                                                 RAW socket, set WLAN PHY preamble for Long/Short\n
                                                 This options takes <b>_u32</b> as parameter      
                                  - <b>SL_SO_PHY_TX_INHIBIT_THRESHOLD</b> \n  
                                                 RAW socket, set WLAN Tx – Set CCA threshold. \n
                                                 The values are based on SlTxInhibitThreshold_e    \n
                                                 This options takes <b>_u32</b> as parameter 
                                  - <b>SL_SO_PHY_TX_TIMEOUT</b> \n  
                                                 RAW socket, set WLAN Tx – changes the TX timeout (lifetime) of transceiver frames. \n   
                                                 Value in Ms, maximum value is 10ms    \n
                                                 This options takes <b>_u32</b> as parameter 
                                  - <b>SL_SO_PHY_ALLOW_ACKS </b> \n  
                                                 RAW socket, set WLAN Tx – Enable\Disable sending ACKs in transceiver mode \n  
                                                 0 = disabled / 1 = enabled    \n
                                                 This options takes <b>_u32</b> as parameter 
                                  - <b>SL_SO_LINGER</b> \n
                                                 Socket lingers on close pending remaining send/receive packetst\n
                                  - <b>SL_SO_SECURE_EXT_CLIENT_CHLNG_RESP</b> \n
                                                 Set with no parameter to indicate that the client uses external signature using netapp requesrt.\n
                                                 needs netapp request handler\n
                                  - <b>SL_SO_SECURE_DOMAIN_NAME_VERIFICATION </b>\n
                                                 Set a domain name, to check in ssl client connection.
            
  
    \param[in] optval           Specifies a value for the option
    \param[in] optlen           Specifies the length of the 
        option value
 
    \return                     Zero on success, or negative error code on failure
    
    \par Persistent                 
                All params are <b>Non- Persistent</b> 
    \sa     sl_getsockopt
    \note   Belongs to \ref basic_api  
    \warning
    \par   Examples

    - SL_SO_KEEPALIVE (disable Keepalive):
    \code
        SlSockKeepalive_t enableOption;
        enableOption.KeepaliveEnabled = 0;
        sl_SetSockOpt(SockID,SL_SOL_SOCKET,SL_SO_KEEPALIVE, (_u8 *)&enableOption,sizeof(enableOption));  
    \endcode
    <br>

    - SL_SO_KEEPALIVETIME (Set Keepalive timeout):
    \code   
        _i16 Status;
        _u32 TimeOut = 120;
        sl_SetSockOpt(Sd, SL_SOL_SOCKET, SL_SO_KEEPALIVETIME,( _u8*) &TimeOut, sizeof(TimeOut));
    \endcode
    <br>

    - SL_SO_RX_NO_IP_BOUNDARY (disable boundary):
    \code
        SlSockRxNoIpBoundary_t enableOption;
        enableOption.RxIpNoBoundaryEnabled = 1;
        sl_SetSockOpt(SockID,SL_SOL_SOCKET,SL_SO_RX_NO_IP_BOUNDARY, (_u8 *)&enableOption,sizeof(enableOption));  
    \endcode
    <br>

    - SL_SO_RCVTIMEO:
    \code
        struct SlTimeval_t timeVal;
        timeVal.tv_sec =  1;             // Seconds
        timeVal.tv_usec = 0;             // Microseconds. 10000 microseconds resolution
        sl_SetSockOpt(SockID,SL_SOL_SOCKET,SL_SO_RCVTIMEO, (_u8 *)&timeVal, sizeof(timeVal));    // Enable receive timeout 
    \endcode
    <br>

    - SL_SO_RCVBUF:
    \code
           SlSockWinsize_t size;
           size.Winsize = 3000;  // bytes
           sl_SetSockOpt(SockID,SL_SOL_SOCKET,SL_SO_RCVBUF, (_u8 *)&size, sizeof(size));
    \endcode
    <br>

    - SL_SO_NONBLOCKING:
    \code
    
           SlSockNonblocking_t enableOption;
           enableOption.NonblockingEnabled = 1;
           sl_SetSockOpt(SockID,SL_SOL_SOCKET,SL_SO_NONBLOCKING, (_u8 *)&enableOption,sizeof(enableOption)); // Enable/disable nonblocking mode
    \endcode
    <br>

    - SL_SO_SECMETHOD:
    \code
           SlSockSecureMethod_t method;
           method.SecureMethod = SL_SO_SEC_METHOD_SSLV3;                                 // security method we want to use
           SockID = sl_Socket(SL_AF_INET,SL_SOCK_STREAM, SL_SEC_SOCKET);
           sl_SetSockOpt(SockID, SL_SOL_SOCKET, SL_SO_SECMETHOD, (_u8 *)&method, sizeof(method));
    \endcode
    <br>

    - SL_SO_SECURE_MASK:
    \code 
           SlSockSecureMask_t cipher;
           cipher.SecureMask = SL_SEC_MASK_SSL_RSA_WITH_RC4_128_SHA;                   // cipher type
           SockID = sl_Socket(SL_AF_INET,SL_SOCK_STREAM, SL_SEC_SOCKET);
           sl_SetSockOpt(SockID, SL_SOL_SOCKET, SL_SO_SECURE_MASK,(_u8 *)&cipher, sizeof(cipher));
    \endcode
    <br>

    - SL_SO_SECURE_FILES_CA_FILE_NAME:
    \code               
        sl_SetSockOpt(SockID,SL_SOL_SOCKET,SL_SO_SECURE_FILES_CA_FILE_NAME,"exuifaxCaCert.der",strlen("exuifaxCaCert.der"));
    \endcode
    <br>

    - SL_SO_SECURE_FILES_PRIVATE_KEY_FILE_NAME;
    \code           
        sl_SetSockOpt(SockID,SL_SOL_SOCKET,SL_SO_SECURE_FILES_PRIVATE_KEY_FILE_NAME,"myPrivateKey.der",strlen("myPrivateKey.der"));
    \endcode
    <br>

    - SL_SO_SECURE_FILES_CERTIFICATE_FILE_NAME:
     \code
        sl_SetSockOpt(SockID,SL_SOL_SOCKET,SL_SO_SECURE_FILES_CERTIFICATE_FILE_NAME,"myCertificate.der",strlen("myCertificate.der"));
     \endcode
     <br>

    - SL_SO_SECURE_FILES_DH_KEY_FILE_NAME:
     \code
        sl_SetSockOpt(SockID,SL_SOL_SOCKET,SL_SO_SECURE_FILES_DH_KEY_FILE_NAME,"myDHinServerMode.der",strlen("myDHinServerMode.der"));
     \endcode
     <br>

    - SL_IP_MULTICAST_TTL:
     \code
           _u8 ttl = 20;
           sl_SetSockOpt(SockID, SL_IPPROTO_IP, SL_IP_MULTICAST_TTL, &ttl, sizeof(ttl));
     \endcode
     <br>

    - SL_IP_ADD_MEMBERSHIP:
     \code
           SlSockIpMreq_t mreq;
           sl_SetSockOpt(SockID, SL_IPPROTO_IP, SL_IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));
    \endcode
    <br>

    - SL_IP_DROP_MEMBERSHIP:
    \code
        SlSockIpMreq_t mreq;
        sl_SetSockOpt(SockID, SL_IPPROTO_IP, SL_IP_DROP_MEMBERSHIP, &mreq, sizeof(mreq));
    \endcode
    <br>

    - SL_SO_CHANGE_CHANNEL:
    \code
        _u32 newChannel = 6; // range is 1-13
        sl_SetSockOpt(SockID, SL_SOL_SOCKET, SL_SO_CHANGE_CHANNEL, &newChannel, sizeof(newChannel));  
    \endcode
    <br>

    - SL_SO_SECURE_ALPN:
    \code
            SlSockSecureALPN_t alpn;
            alpn.SecureALPN = SL_SECURE_ALPN_H2 | SL_SECURE_ALPN_H2_14;
            sl_SetSockOpt(SockID, SL_SOL_SOCKET, SL_SO_SECURE_ALPN, &alpn, sizeof(SlSockSecureALPN_t));  
    \endcode
    <br>

    -   SL_IP_RAW_RX_NO_HEADER:
    \code
        _u32 header = 1;  // remove ip header
        sl_SetSockOpt(SockID, SL_IPPROTO_IP, SL_IP_RAW_RX_NO_HEADER, &header, sizeof(header));
    \endcode
    <br>

    - SL_IP_HDRINCL:
    \code
        _u32 header = 1;
        sl_SetSockOpt(SockID, SL_IPPROTO_IP, SL_IP_HDRINCL, &header, sizeof(header));
    \endcode
    <br>

    - SL_IP_RAW_IPV6_HDRINCL:
    \code
        _u32 header = 1;
        sl_SetSockOpt(SockID, SL_IPPROTO_IP, SL_IP_RAW_IPV6_HDRINCL, &header, sizeof(header));
    \endcode
    <br>

    -   SL_SO_PHY_RATE:
    \code
        _u32 rate = 6; // see wlan.h SlWlanRateIndex_e for values
        sl_SetSockOpt(SockID, SL_SOL_PHY_OPT, SL_SO_PHY_RATE, &rate, sizeof(rate));  
    \endcode
    <br>

    - SL_SO_PHY_TX_POWER:
    \code
        _u32 txpower = 1; // valid range is 1-15
        sl_SetSockOpt(SockID, SL_SOL_PHY_OPT, SL_SO_PHY_TX_POWER, &txpower, sizeof(txpower));
    \endcode
    <br>

    - SL_SO_PHY_NUM_FRAMES_TO_TX:
    \code
        _u32 numframes = 1;
        sl_SetSockOpt(SockID, SL_SOL_PHY_OPT, SL_SO_PHY_NUM_FRAMES_TO_TX, &numframes, sizeof(numframes));
    \endcode
    <br>

    - SL_SO_PHY_PREAMBLE:
    \code
        _u32 preamble = 1;
        sl_SetSockOpt(SockID, SL_SOL_PHY_OPT, SL_SO_PHY_PREAMBLE, &preamble, sizeof(preamble));
    \endcode
    <br>

    - SL_SO_PHY_TX_INHIBIT_THRESHOLD:
    \code
        _u32 thrshld = SL_TX_INHIBIT_THRESHOLD_MED;
        sl_SetSockOpt(SockID, SL_SOL_PHY_OPT, SL_SO_PHY_TX_INHIBIT_THRESHOLD , &thrshld, sizeof(thrshld));
    \endcode
    <br>

    - SL_SO_PHY_TX_TIMEOUT:
    \code
        _u32 timeout = 50;
        sl_SetSockOpt(SockID, SL_SOL_PHY_OPT, SL_SO_PHY_TX_TIMEOUT  , &timeout, sizeof(timeout));
    \endcode
    <br>
    
    - SL_SO_PHY_ALLOW_ACKS:
    \code
        _u32 acks = 1; // 0 = disabled / 1 = enabled
        sl_SetSockOpt(SockID, SL_SOL_PHY_OPT, SL_SO_PHY_ALLOW_ACKS, &acks, sizeof(acks));
    \endcode
    <br>
    
    - SL_SO_LINGER:
    \code
        SlSocklinger_t linger;
        linger.l_onoff = 1;
        linger.l_linger = 10;
        sl_SetSockOpt(SockID, SL_SOL_SOCKET, SL_SO_LINGER, &linger, sizeof(linger));
    \endcode
    <br>

    - SL_SO_SECURE_EXT_CLIENT_CHLNG_RESP:
    \code
        int dummy;
        sl_SetSockOpt(SockID, SL_SOL_SOCKET, SL_SO_SECURE_EXT_CLIENT_CHLNG_RESP, &dummy, sizeof(dummy));
    \endcode
    <br>

    - SL_SO_SECURE_DOMAIN_NAME_VERIFICATION:
    \code
        sl_SetSockOpt(SockID,SL_SOL_SOCKET,SL_SO_SECURE_DOMAIN_NAME_VERIFICATION,"www.google.co.il",strlen("www.google.co.il"));
    \endcode
              
*/
#if _SL_INCLUDE_FUNC(sl_SetSockOpt)
_i16 sl_SetSockOpt(_i16 sd, _i16 level, _i16 optname, const void *optval, SlSocklen_t optlen);
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
    
    \return                     Zero on success, or negative error code on failure
    \sa     sl_SetSockOpt
    \note   See sl_SetSockOpt
            Belongs to \ref ext_api
    \warning
*/
#if _SL_INCLUDE_FUNC(sl_GetSockOpt)
_i16 sl_GetSockOpt(_i16 sd, _i16 level, _i16 optname, void *optval, SlSocklen_t *optlen);
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
                                or a negative value if an error occurred.\n
                                Using a non-blocking recv a possible negative value is SL_EAGAIN.\n
                                SL_POOL_IS_EMPTY may be return in case there are no resources in the system
                                 In this case try again later or increase MAX_CONCURRENT_ACTIONS
    
    \sa     sl_RecvFrom
    \note                       Belongs to \ref recv_api
    \warning
    \par        Examples

    - Receiving data using TCP socket:
    \code    
        SlSockAddrIn_t  Addr;
        SlSockAddrIn_t  LocalAddr;
        _i16 AddrSize = sizeof(SlSockAddrIn_t);
        _i16 SockID, newSockID;
        _i16 Status;
        _i8 Buf[RECV_BUF_LEN];

        LocalAddr.sin_family = SL_AF_INET;
        LocalAddr.sin_port = sl_Htons(5001);
        LocalAddr.sin_addr.s_addr = 0;

        Addr.sin_family = SL_AF_INET;
        Addr.sin_port = sl_Htons(5001);
        Addr.sin_addr.s_addr = sl_Htonl(SL_IPV4_VAL(10,1,1,200));

        SockID = sl_Socket(SL_AF_INET,SL_SOCK_STREAM, 0);
        Status = sl_Bind(SockID, (SlSockAddr_t *)&LocalAddr, AddrSize);
        Status = sl_Listen(SockID, 0);
        newSockID = sl_Accept(SockID, (SlSockAddr_t*)&Addr, (SlSocklen_t*) &AddrSize);
        Status = sl_Recv(newSockID, Buf, 1460, 0);
    \endcode
    <br>

    - Rx transceiver mode using a raw socket:
    \code      
        _i8 buffer[1536];
        _i16 sd;
        _u16 size;
        SlTransceiverRxOverHead_t *transHeader;
        sd = sl_Socket(SL_AF_RF,SL_SOCK_RAW,11); // channel 11
        while(1)
        {
            size = sl_Recv(sd,buffer,1536,0);
            transHeader = (SlTransceiverRxOverHead_t *)buffer;
            printf("RSSI is %d frame type is 0x%x size %d\n",transHeader->rssi,buffer[sizeof(SlTransceiverRxOverHead_t)],size);
        }
    \endcode
*/
#if _SL_INCLUDE_FUNC(sl_Recv)
_i16 sl_Recv(_i16 sd, void *buf, _i16 len, _i16 flags);
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
                                or a negative value if an error occurred.\n
                                Using a non-blocking recv a possible negative value is SL_EAGAIN.
                                SL_RET_CODE_INVALID_INPUT (-2) will be returned if fromlen has incorrect length. \n
                                SL_POOL_IS_EMPTY may be return in case there are no resources in the system
                                 In this case try again later or increase MAX_CONCURRENT_ACTIONS
    
    \sa     sl_Recv
    \note                       Belongs to \ref recv_api
    \warning
    \par        Example

    - Receiving data:
    \code
        SlSockAddrIn_t  Addr;
        SlSockAddrIn_t  LocalAddr;
        _i16 AddrSize = sizeof(SlSockAddrIn_t);
        _i16 SockID;
        _i16 Status;
        _i8 Buf[RECV_BUF_LEN];

        LocalAddr.sin_family = SL_AF_INET;
        LocalAddr.sin_port = sl_Htons(5001);
        LocalAddr.sin_addr.s_addr = 0;

        SockID = sl_Socket(SL_AF_INET,SL_SOCK_DGRAM, 0);
        Status = sl_Bind(SockID, (SlSockAddr_t *)&LocalAddr, AddrSize);
        Status = sl_RecvFrom(SockID, Buf, 1472, 0, (SlSockAddr_t *)&Addr, (SlSocklen_t*)&AddrSize);

    \endcode
*/
#if _SL_INCLUDE_FUNC(sl_RecvFrom)
_i16 sl_RecvFrom(_i16 sd, void *buf, _i16 len, _i16 flags, SlSockAddr_t *from, SlSocklen_t *fromlen);
#endif

/*!
    \brief Write data to TCP socket
    
    This function is used to transmit a message to another socket.
    Returns immediately after sending data to device.
    In case of TCP failure an async event SL_SOCKET_TX_FAILED_EVENT is going to
    be received.\n
    In case of a RAW socket (transceiver mode), extra 4 bytes should be reserved at the end of the 
    frame data buffer for WLAN FCS 
     
    \param[in] sd               Socket handle
    \param[in] buf              Points to a buffer containing 
                                the message to be sent
    \param[in] len              Message size in bytes. Range: 1-1460 bytes
    \param[in] flags            Specifies the type of message 
                                transmission. On this version, this parameter is not
                                supported for TCP.
                                For transceiver mode, the SL_WLAN_RAW_RF_TX_PARAMS macro can be used to determine
                                transmission parameters (channel,rate,tx_power,preamble)
    
    
    \return                     Zero on success, or negative error code on failure
    
    \sa     sl_SendTo 
    \note                       Belongs to \ref send_api
    \warning   
    \par        Example

    - Sending data:
    \code
        SlSockAddrIn_t  Addr;
        _i16 AddrSize = sizeof(SlSockAddrIn_t);
        _i16 SockID;
        _i16 Status;
        _i8 Buf[SEND_BUF_LEN];

        Addr.sin_family = SL_AF_INET;
        Addr.sin_port = sl_Htons(5001);
        Addr.sin_addr.s_addr = sl_Htonl(SL_IPV4_VAL(10,1,1,200));

        SockID = sl_Socket(SL_AF_INET,SL_SOCK_STREAM, 0);
        Status = sl_Connect(SockID, (SlSockAddr_t *)&Addr, AddrSize);
        Status = sl_Send(SockID, Buf, 1460, 0 );
    \endcode
 */ 
#if _SL_INCLUDE_FUNC(sl_Send )
_i16 sl_Send(_i16 sd, const void *buf, _i16 len, _i16 flags);
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
    
    \return                     Zero on success, or negative error code on failure
    
    \sa     sl_Send
    \note                       Belongs to \ref send_api
    \warning
    \par        Example

    - Sending data:
    \code
        SlSockAddrIn_t  Addr;
        _i16 AddrSize = sizeof(SlSockAddrIn_t);
        _i16 SockID;
        _i16 Status;
        _i8 Buf[SEND_BUF_LEN];

        Addr.sin_family = SL_AF_INET;
        Addr.sin_port = sl_Htons(5001);
        Addr.sin_addr.s_addr = sl_Htonl(SL_IPV4_VAL(10,1,1,200));

        SockID = sl_Socket(SL_AF_INET,SL_SOCK_DGRAM, 0);
        Status = sl_SendTo(SockID, Buf, 1472, 0, (SlSockAddr_t *)&Addr, AddrSize);
    \endcode
*/
#if _SL_INCLUDE_FUNC(sl_SendTo)
_i16 sl_SendTo(_i16 sd, const void *buf, _i16 len, _i16 flags, const SlSockAddr_t *to, SlSocklen_t tolen);
#endif

/*!
    \brief Reorder the bytes of a 32-bit unsigned value
    
    This function is used to Reorder the bytes of a 32-bit unsigned value from processor order to network order.
     
    \param[in] val              Variable to reorder 
    
    \return                     Return the reorder variable, 
    
    \sa     sl_SendTo  sl_Bind  sl_Connect  sl_RecvFrom  sl_Accept
    \note                       Belongs to \ref send_api
    \warning   
*/
#if _SL_INCLUDE_FUNC(sl_Htonl )
_u32 sl_Htonl( _u32 val );

#define sl_Ntohl sl_Htonl  /* Reorder the bytes of a 16-bit unsigned value from network order to processor orde. */
#endif

/*!
    \brief Reorder the bytes of a 16-bit unsigned value
    
    This function is used to Reorder the bytes of a 16-bit unsigned value from processor order to network order.
     
    \param[in] val              Variable to reorder 
    
    \return                     Return the reorder variable, 
    
    \sa     sl_SendTo  sl_Bind    sl_Connect  sl_RecvFrom  sl_Accept
    \note                       Belongs to \ref send_api
    \warning   
*/
#if _SL_INCLUDE_FUNC(sl_Htons )
_u16 sl_Htons( _u16 val );

#define sl_Ntohs sl_Htons   /* Reorder the bytes of a 16-bit unsigned value from network order to processor orde. */
#endif


/*!

 Close the Doxygen group.
 @}

 */


#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif /* __SOCKET_H__ */


