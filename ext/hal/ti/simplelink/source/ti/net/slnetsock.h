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

 /*!


  \page SlNetSock_overview SlNetSock

  \section intro_sec Introduction

SlNetSock provides a standard BSD API for TCP and UDP transport
layers, and a lower-level SlNetSock API for basic and extended
usage. Supported use cases include:

 - Support of multi interface (WiFi NS, Ethernet NDK)
 - Selecting which interfaces the host will use, one or more.
 - Support of different types of sockets (TCP, TLS, UDP, RAW, RF, etc.)
 - BSD and proprietary errors

 The SlNetSock API's lead to easier portability to microcontrollers,
 without compromising the capabilities and robustness of the final
 application.


 \section modules_sec Module Names
 TI's SlNetSock layer is divided into the following software modules:
     -# \ref SlNetSock  - Controls standard client/server sockets options and capabilities
     -# \ref SlNetIf    - Controls standard stack/interface options and capabilities
     -# \ref SlNetUtils - Provides sockets related commands and configuration
     -# \ref SlNetErr   - Provide BSD and proprietary errors

In addition, SlNetSock provides a standard BSD API, built atop the
SlNet* APIs.  The BSD headers are placed in ti/net/bsd directory,
which users should place on their include path.

Also, there is a light
\subpage porting_guide "SL Interface Porting Guide"
with information available for adding SlNetSock support for other stacks.

  \page porting_guide SL Interface Porting Guide

 \section Introduction

The generic SlNetSock layer sits between the application/service and
the interface stack.  This guide describes the details of adding a network stack into the SlNetSock environment.

The porting steps for adding new interface:
    -# Create slnetifxxx file for the new interface
    -# Select the capabilities set
    -# Adding the interface to your application/service
    -# Add the relevant functions to your application/service
    -# Test your code to validate the correctness of your porting

  \subsection porting_step1   Step 1 - slnetifxxx.c and slnetifxxx.h file for your interface

 - Create slnetifxxx file (replace xxx with your interface/stack
   name).  Likely you will copy from an existing port.

 - Implement the needed API's.

Each interface needs to provide a set of API's to work with the
interface.  Some are mandatory, others are optional (but recommended).

 - Mandatory API's:
  - \ref SlNetIf_Config_t.sockCreate            "sockCreate"
  - \ref SlNetIf_Config_t.sockClose             "sockClose"
  - \ref SlNetIf_Config_t.sockSelect            "sockSelect"
  - \ref SlNetIf_Config_t.sockSetOpt            "sockSetOpt"
  - \ref SlNetIf_Config_t.sockGetOpt            "sockGetOpt"
  - \ref SlNetIf_Config_t.sockRecvFrom          "sockRecvFrom"
  - \ref SlNetIf_Config_t.sockSendTo            "sockSendTo"
  - \ref SlNetIf_Config_t.ifGetIPAddr           "ifGetIPAddr"
  - \ref SlNetIf_Config_t.ifGetConnectionStatus "ifGetConnectionStatus"

 - The non-mandatory API's set:
  - \ref SlNetIf_Config_t.sockShutdown          "sockShutdown"
  - \ref SlNetIf_Config_t.sockAccept            "sockAccept"
  - \ref SlNetIf_Config_t.sockBind              "sockBind"
  - \ref SlNetIf_Config_t.sockListen            "sockListen"
  - \ref SlNetIf_Config_t.sockConnect           "sockConnect"
  - \ref SlNetIf_Config_t.sockGetPeerName       "sockGetPeerName"
  - \ref SlNetIf_Config_t.sockGetLocalName      "sockGetLocalName"
  - \ref SlNetIf_Config_t.sockRecv              "sockRecv"
  - \ref SlNetIf_Config_t.sockSend              "sockSend"
  - \ref SlNetIf_Config_t.sockstartSec          "sockstartSec"
  - \ref SlNetIf_Config_t.utilGetHostByName     "utilGetHostByName"
  - \ref SlNetIf_Config_t.ifLoadSecObj          "ifLoadSecOjb"
  - \ref SlNetIf_Config_t.ifCreateContext       "ifCreateContext"


  \note The list of API's and more data can be found in ::SlNetIf_Config_t structure in SlNetIf module \n \n

 \subsection    porting_step2   Step 2 - Select the capabilities set

 The capabilities prototype should be declared in your slnetifxxx.h and implemented in your slnetifxxx.c

 Each mandatory API's must be set, additional API's can be set or must
 be set to NULL.

 An example config declaration for TI's SimpleLink CC31XX/CC32xx

 \code
 SlNetIfConfig SlNetIfConfigWiFi =
 {
    SlNetIfWifi_socket,              // Callback function sockCreate in slnetif module
    SlNetIfWifi_close,               // Callback function sockClose in slnetif module
    NULL,                            // Callback function sockShutdown in slnetif module
    SlNetIfWifi_accept,              // Callback function sockAccept in slnetif module
    SlNetIfWifi_bind,                // Callback function sockBind in slnetif module
    SlNetIfWifi_listen,              // Callback function sockListen in slnetif module
    SlNetIfWifi_connect,             // Callback function sockConnect in slnetif module
    NULL,                            // Callback function sockGetPeerName in slnetif module
    NULL,                            // Callback function sockGetLocalName in slnetif module
    SlNetIfWifi_select,              // Callback function sockSelect in slnetif module
    SlNetIfWifi_setSockOpt,          // Callback function sockSetOpt in slnetif module
    SlNetIfWifi_getSockOpt,          // Callback function sockGetOpt in slnetif module
    SlNetIfWifi_recv,                // Callback function sockRecv in slnetif module
    SlNetIfWifi_recvFrom,            // Callback function sockRecvFrom in slnetif module
    SlNetIfWifi_send,                // Callback function sockSend in slnetif module
    SlNetIfWifi_sendTo,              // Callback function sockSendTo in slnetif module
    SlNetIfWifi_sockstartSec,        // Callback function sockstartSec in slnetif module
    SlNetIfWifi_getHostByName,       // Callback function utilGetHostByName in slnetif module
    SlNetIfWifi_getIPAddr,           // Callback function ifGetIPAddr in slnetif module
    SlNetIfWifi_getConnectionStatus, // Callback function ifGetConnectionStatus in slnetif module
    SlNetIfWifi_loadSecObj,          // Callback function ifLoadSecObj in slnetif module
    NULL                             // Callback function ifCreateContext in slnetif module
 };
 \endcode

 In the example above the following API's are not supported by the interface,
 and are set to NULL:
 - sockShutdown
 - sockGetPeerName
 - sockGetLocalName
 - utilGetHostByName
 - ifCreateContext

 \subsection    porting_step3   Step 3 - Adding the interface to your application/service

 \b Include the new file in the board header file in the application.

 \subsection    porting_step4   Step 4 - Add the relevant functions to your application/service

  After configuring the capabilities of the interface, Adding the interface to the SlNetSock
  is required.

  Use ::SlNetIf_add in order to add the interface and set his ID, Name, function list and priority.
  Later on you need to use the BSD API's or SlNetSock API's for socket handling.

 \subsection    porting_step5   Step 5 - Test your code to validate the correctness of your porting

 After porting the layer into your setup, validate that your code work as expected

*/

/*****************************************************************************/
/* Include files                                                             */
/*****************************************************************************/

#ifndef __SL_NET_SOCK_H__
#define __SL_NET_SOCK_H__

#include <stdint.h>
#include <sys/time.h>

#ifdef    __cplusplus
extern "C" {
#endif


/*!
    \defgroup SlNetSock SlNetSock group

    \short Controls standard client/server sockets options and capabilities

*/
/*!

    \addtogroup SlNetSock
    @{

*/

/*****************************************************************************/
/* Macro declarations                                                        */
/*****************************************************************************/

#define SLNETSOCK_MAX_CONCURRENT_SOCKETS                                    (32)  /**< Declares the maximum sockets that can be opened */

/* Address families.  */
#define SLNETSOCK_AF_UNSPEC                                                 (0)   /**< Unspecified address family      */
#define SLNETSOCK_AF_INET                                                   (2)   /**< IPv4 socket (UDP, TCP, etc)     */
#define SLNETSOCK_AF_INET6                                                  (3)   /**< IPv6 socket (UDP, TCP, etc)     */
#define SLNETSOCK_AF_RF                                                     (6)   /**< Data include RF parameter, All layer by user (Wifi could be disconnected) */
#define SLNETSOCK_AF_PACKET                                                 (17)  /**< Network bypass                  */

/* Protocol families, same as address families. */
#define SLNETSOCK_PF_UNSPEC                                                 SLNETSOCK_AF_UNSPEC
#define SLNETSOCK_PF_INET                                                   SLNETSOCK_AF_INET
#define SLNETSOCK_PF_INET6                                                  SLNETSOCK_AF_INET6

/* Define argument types specifies the socket type.  */
#define SLNETSOCK_SOCK_STREAM                                               (1)   /**< TCP Socket                      */
#define SLNETSOCK_SOCK_DGRAM                                                (2)   /**< UDP Socket                      */
#define SLNETSOCK_SOCK_RAW                                                  (3)   /**< Raw socket                      */
#define SLNETSOCK_SOCK_RX_MTR                                               (4)   /**< RX Metrics socket               */
#define SLNETSOCK_SOCK_MAC_WITH_CCA                                         (5)
#define SLNETSOCK_SOCK_MAC_WITH_NO_CCA                                      (6)
#define SLNETSOCK_SOCK_BRIDGE                                               (7)
#define SLNETSOCK_SOCK_ROUTER                                               (8)

/* Define some BSD protocol constants.  */
#define SLNETSOCK_PROTO_TCP                                                 (6)   /**< TCP Raw Socket                  */
#define SLNETSOCK_PROTO_UDP                                                 (17)  /**< UDP Raw Socket                  */
#define SLNETSOCK_PROTO_RAW                                                 (255) /**< Raw Socket                      */
#define SLNETSOCK_PROTO_SECURE                                              (100) /**< Secured Socket Layer (SSL,TLS)  */

/* bind any addresses */
#define SLNETSOCK_INADDR_ANY                                                (0)
#define SLNETSOCK_IN6ADDR_ANY                                               (0)


/* socket options */

/* possible values for the level parameter in slNetSock_setOpt / slNetSock_getOpt */
#define SLNETSOCK_LVL_SOCKET                                                (1)   /**< Define the socket option category. */
#define SLNETSOCK_LVL_IP                                                    (2)   /**< Define the IP option category.     */
#define SLNETSOCK_LVL_PHY                                                   (3)   /**< Define the PHY option category.    */

/* possible values for the option parameter in slNetSock_setOpt / slNetSock_getOpt */

/* socket level options (SLNETSOCK_LVL_SOCKET) */
#define SLNETSOCK_OPSOCK_RCV_BUF                                            (8)   /**< Setting TCP receive buffer size (window size) - This options takes SlNetSock_Winsize_t struct as parameter                 */
#define SLNETSOCK_OPSOCK_RCV_TIMEO                                          (20)  /**< Enable receive timeout - This options takes SlNetSock_Timeval_t struct as parameter                                        */
#define SLNETSOCK_OPSOCK_KEEPALIVE                                          (9)   /**< Connections are kept alive with periodic messages - This options takes SlNetSock_Keepalive_t struct as parameter           */
#define SLNETSOCK_OPSOCK_KEEPALIVE_TIME                                     (37)  /**< keepalive time out - This options takes <b>uint32_t</b> as parameter                                                       */
#define SLNETSOCK_OPSOCK_LINGER                                             (13)  /**< Socket lingers on close pending remaining send/receive packets - This options takes SlNetSock_linger_t struct as parameter */
#define SLNETSOCK_OPSOCK_NON_BLOCKING                                       (24)  /**< Enable/disable nonblocking mode - This options takes SlNetSock_Nonblocking_t struct as parameter                           */
#define SLNETSOCK_OPSOCK_NON_IP_BOUNDARY                                    (39)  /**< connectionless socket disable rx boundary - This options takes SlNetSock_NonIpBoundary_t struct as parameter               */
#define SLNETSOCK_OPSOCK_ERROR                                              (58)  /**< Socket level error code                                                                                                    */
#define SLNETSOCK_OPSOCK_SLNETSOCKSD                                        (59)  /**< Used by the BSD layer in order to retrieve the slnetsock sd                                                                */
#define SLNETSOCK_OPSOCK_BROADCAST                                          (200) /**< Enable/disable broadcast signals - This option takes SlNetSock_Broadcast_t struct as parameters                            */

/* IP level options (SLNETSOCK_LVL_IP) */
#define SLNETSOCK_OPIP_MULTICAST_TTL                                        (61)  /**< Specify the TTL value to use for outgoing multicast packet. - This options takes <b>uint8_t</b> as parameter                                                      */
#define SLNETSOCK_OPIP_ADD_MEMBERSHIP                                       (65)  /**< Join IPv4 multicast membership - This options takes SlNetSock_IpMreq_t struct as parameter                                                                        */
#define SLNETSOCK_OPIP_DROP_MEMBERSHIP                                      (66)  /**< Leave IPv4 multicast membership - This options takes SlNetSock_IpMreq_t struct as parameter                                                                       */
#define SLNETSOCK_OPIP_HDRINCL                                              (67)  /**< Raw socket IPv4 header included - This options takes <b>uint32_t</b> as parameter                                                                                 */
#define SLNETSOCK_OPIP_RAW_RX_NO_HEADER                                     (68)  /**< Proprietary socket option that does not includeIPv4/IPv6 header (and extension headers) on received raw sockets - This options takes <b>uint32_t</b> as parameter */
#define SLNETSOCK_OPIP_RAW_IPV6_HDRINCL                                     (69)  /**< Transmitted buffer over IPv6 socket contains IPv6 header - This options takes <b>uint32_t</b> as parameter                                                        */
#define SLNETSOCK_OPIPV6_ADD_MEMBERSHIP                                     (70)  /**< Join IPv6 multicast membership - This options takes SlNetSock_IpV6Mreq_t struct as parameter                                                                      */
#define SLNETSOCK_OPIPV6_DROP_MEMBERSHIP                                    (71)  /**< Leave IPv6 multicast membership - This options takes SlNetSock_IpV6Mreq_t struct as parameter                                                                     */
#define SLNETSOCK_OPIPV6_MULTICAST_HOPS                                     (72)  /**< Specify the hops value to use for outgoing multicast packet.                                                                                                      */

/* PHY level options (SLNETSOCK_LVL_PHY) */
#define SLNETSOCK_OPPHY_CHANNEL                                             (28)  /**< This option is available only when transceiver started - This options takes <b>uint32_t</b> as channel number parameter            */
#define SLNETSOCK_OPPHY_RATE                                                (100) /**< WLAN Transmit rate - This options takes <b>uint32_t</b> as parameter based on SlWlanRateIndex_e                                    */
#define SLNETSOCK_OPPHY_TX_POWER                                            (101) /**< TX Power level - This options takes <b>uint32_t</b> as parameter                                                                   */
#define SLNETSOCK_OPPHY_NUM_FRAMES_TO_TX                                    (102) /**< Number of frames to transmit - This options takes <b>uint32_t</b> as parameter                                                     */
#define SLNETSOCK_OPPHY_PREAMBLE                                            (103) /**< Preamble for transmission - This options takes <b>uint32_t</b> as parameter                                                        */
#define SLNETSOCK_OPPHY_TX_INHIBIT_THRESHOLD                                (104) /**< TX Inhibit Threshold (CCA) - This options takes <b>uint32_t</b> as parameter based on SlNetSockTxInhibitThreshold_e                */
#define SLNETSOCK_OPPHY_TX_TIMEOUT                                          (105) /**< TX timeout for Transceiver frames (lifetime) in miliseconds (max value is 100ms) - This options takes <b>uint32_t</b> as parameter */
#define SLNETSOCK_OPPHY_ALLOW_ACKS                                          (106) /**< Enable sending ACKs in transceiver mode - This options takes <b>uint32_t</b> as parameter                                          */

/* TCP level options (SLNETSOCK_PROTO_TCP) */
#define SLNETSOCK_TCP_NODELAY                                               (203) /**< Disables TCP send delay/coalesce algorithm - This option takes SLNetSock_NoDelay_t struct as a parameter                                                 */
#define SLNETSOCK_TCP_MAXSEG                                                (204) /**< Set the maximum TCP segment size - This option takes SLNetSock_MaxSeg_t struct as a parameter                                                            */
#define SLNETSOCK_TCP_NOPUSH                                                (205) /**< Do not send data just to finish a data block (attempt to coalesce). - This option takes SLNetSock_NoPush_t struct as a parameter                         */
#define SLNETSOCK_TCP_NOOPT                                                 (206) /**< Do not use TCP options. - This option takes SLNetSock_NoOpt_t struct as a parameter                                                                      */
#define SLNETSOCK_TCP_SACKPERMITTED                                         (207) /**< Permit RFC-2018 Selective Acknowledgment(SACK) conformant connection - This option takes SLNetSock_SackPermitted_t struct as a parameter                 */
#define SLNETSOCK_TCP_MAXRTT                                                (208) /**< The maximum TCP Round Trip Time value allowed in the determination of the estimated TCP RTT - This option takes SLNetSock_MaxRtt_t struct as a parameter */

/*!
    \brief The SlNetSockTxInhibitThreshold_e enumerations is used in SLNETSOCK_OPPHY_TX_INHIBIT_THRESHOLD PHY level option
*/
typedef enum
{
    SLNETSOCK_TX_INHIBIT_THRESHOLD_MIN     = 1,
    SLNETSOCK_TX_INHIBIT_THRESHOLD_LOW     = 2,
    SLNETSOCK_TX_INHIBIT_THRESHOLD_DEFAULT = 3,
    SLNETSOCK_TX_INHIBIT_THRESHOLD_MED     = 4,
    SLNETSOCK_TX_INHIBIT_THRESHOLD_HIGH    = 5,
    SLNETSOCK_TX_INHIBIT_THRESHOLD_MAX     = 6
} SlNetSockTxInhibitThreshold_e;

/*!
    \brief  The SlNetSockSecAttrib_e enumerations are used to declare security
            attribute objects in SlNetSock_secAttribSet().

    \sa     SlNetSock_secAttribSet()
*/
typedef enum
{
     SLNETSOCK_SEC_ATTRIB_PRIVATE_KEY               = 0,
     SLNETSOCK_SEC_ATTRIB_LOCAL_CERT                = 1,
     SLNETSOCK_SEC_ATTRIB_PEER_ROOT_CA              = 2,
     SLNETSOCK_SEC_ATTRIB_DH_KEY                    = 3,
     SLNETSOCK_SEC_ATTRIB_METHOD                    = 4,
     SLNETSOCK_SEC_ATTRIB_CIPHERS                   = 5,
     SLNETSOCK_SEC_ATTRIB_ALPN                      = 6,
     SLNETSOCK_SEC_ATTRIB_EXT_CLIENT_CHLNG_RESP     = 7,
     SLNETSOCK_SEC_ATTRIB_DOMAIN_NAME               = 8,

     /*!
            @c SLNETSOCK_SEC_ATTRIB_DISABLE_CERT_STORE is
            currently only supported on CC3x20 devices.

            The certificate store is a file, provided by TI,
            containing a list of known and trusted root CAs by TI.
            For more information, see the CC3x20 documentation.

            The certificate store is used only in client mode. Servers
            use a proprietary root CA to authenticate clients, and
            therefore cannot use the certificate store.

            Using this attribute allows using root CA which isn't a
            part of the provided certificate store.
     */

     SLNETSOCK_SEC_ATTRIB_DISABLE_CERT_STORE = 9
} SlNetSockSecAttrib_e;

/* available values for SLNETSOCK_SEC_ATTRIB_METHOD */
#define SLNETSOCK_SEC_METHOD_SSLV3                                          (0)   /**< security method SSL v3                            */
#define SLNETSOCK_SEC_METHOD_TLSV1                                          (1)   /**< security method TLS v1                            */
#define SLNETSOCK_SEC_METHOD_TLSV1_1                                        (2)   /**< security method TLS v1_1                          */
#define SLNETSOCK_SEC_METHOD_TLSV1_2                                        (3)   /**< security method TLS v1_2                          */
#define SLNETSOCK_SEC_METHOD_SSLv3_TLSV1_2                                  (4)   /**< use highest possible version from SSLv3 - TLS 1.2 */
#define SLNETSOCK_SEC_METHOD_DLSV1                                          (5)   /**< security method DTL v1                            */

/* available values for SLNETSOCK_SEC_ATTRIB_CIPHERS. The value is bitmap! */
#define SLNETSOCK_SEC_CIPHER_SSL_RSA_WITH_RC4_128_SHA                       (1 << 0)
#define SLNETSOCK_SEC_CIPHER_SSL_RSA_WITH_RC4_128_MD5                       (1 << 1)
#define SLNETSOCK_SEC_CIPHER_TLS_RSA_WITH_AES_256_CBC_SHA                   (1 << 2)
#define SLNETSOCK_SEC_CIPHER_TLS_DHE_RSA_WITH_AES_256_CBC_SHA               (1 << 3)
#define SLNETSOCK_SEC_CIPHER_TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA             (1 << 4)
#define SLNETSOCK_SEC_CIPHER_TLS_ECDHE_RSA_WITH_RC4_128_SHA                 (1 << 5)
#define SLNETSOCK_SEC_CIPHER_TLS_RSA_WITH_AES_128_CBC_SHA256                (1 << 6)
#define SLNETSOCK_SEC_CIPHER_TLS_RSA_WITH_AES_256_CBC_SHA256                (1 << 7)
#define SLNETSOCK_SEC_CIPHER_TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA256          (1 << 8)
#define SLNETSOCK_SEC_CIPHER_TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA256        (1 << 9)
#define SLNETSOCK_SEC_CIPHER_TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA           (1 << 10)
#define SLNETSOCK_SEC_CIPHER_TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA           (1 << 11)
#define SLNETSOCK_SEC_CIPHER_TLS_RSA_WITH_AES_128_GCM_SHA256                (1 << 12)
#define SLNETSOCK_SEC_CIPHER_TLS_RSA_WITH_AES_256_GCM_SHA384                (1 << 13)
#define SLNETSOCK_SEC_CIPHER_TLS_DHE_RSA_WITH_AES_128_GCM_SHA256            (1 << 14)
#define SLNETSOCK_SEC_CIPHER_TLS_DHE_RSA_WITH_AES_256_GCM_SHA384            (1 << 15)
#define SLNETSOCK_SEC_CIPHER_TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256          (1 << 16)
#define SLNETSOCK_SEC_CIPHER_TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384          (1 << 17)
#define SLNETSOCK_SEC_CIPHER_TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256        (1 << 18)
#define SLNETSOCK_SEC_CIPHER_TLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384        (1 << 19)
#define SLNETSOCK_SEC_CIPHER_TLS_ECDHE_ECDSA_WITH_CHACHA20_POLY1305_SHA256  (1 << 20)
#define SLNETSOCK_SEC_CIPHER_TLS_ECDHE_RSA_WITH_CHACHA20_POLY1305_SHA256    (1 << 21)
#define SLNETSOCK_SEC_CIPHER_TLS_DHE_RSA_WITH_CHACHA20_POLY1305_SHA256      (1 << 22)
#define SLNETSOCK_SEC_CIPHER_FULL_LIST                                      (0xFFFFFFFF)

/* available values for SLNETSOCK_SEC_ATTRIB_ALPN */
#define SLNETSOCK_SEC_ALPN_H1                                               (1 << 0)
#define SLNETSOCK_SEC_ALPN_H2                                               (1 << 1)
#define SLNETSOCK_SEC_ALPN_H2C                                              (1 << 2)
#define SLNETSOCK_SEC_ALPN_H2_14                                            (1 << 3)
#define SLNETSOCK_SEC_ALPN_H2_16                                            (1 << 4)
#define SLNETSOCK_SEC_ALPN_FULL_LIST                                        ((SLNETSOCK_SEC_ALPN_H2_16 << 1 ) - 1)

/* available values for the flags of the SlNetSock_startSec function */
#define SLNETSOCK_SEC_START_SECURITY_SESSION_ONLY                           (1 << 0) /**< Sends the command that will start the security session for a specific socket descriptor */
#define SLNETSOCK_SEC_BIND_CONTEXT_ONLY                                     (1 << 1) /**< Binds the security context to a specific socket descriptor */
#define SLNETSOCK_SEC_IS_SERVER                                             (1 << 2) /**< Used to define if the socket is client/server socket       */

/* available values for the flags of the SlNetSock_create function */

#define SLNETSOCK_CREATE_IF_STATE_ENABLE                                    (1 << 0) /**< Creation of the socket will be on enabled state      */
#define SLNETSOCK_CREATE_IF_STATUS_CONNECTED                                (1 << 1) /**< Creation of the socket will be on status connected   */
#define SLNETSOCK_CREATE_ALLOW_PARTIAL_MATCH                                (1 << 2) /**< Creation of the socket will be on the interface with
                                                                                        the highest priority if the other flags will fail    */

/* Definitions for shutting down some or all parts of a full duplex connection */
#define SLNETSOCK_SHUT_RD                                                   (0) /**< Further receptions will be disallowed                   */
#define SLNETSOCK_SHUT_WR                                                   (1) /**< Further transmissions will be disallowed                */
#define SLNETSOCK_SHUT_RDWR                                                 (2) /**< Further receptions and transmissions will be disallowed */

/* Length of address string representation  */
#define SLNETSOCK_INET6_ADDRSTRLEN                                          (46)
#define SLNETSOCK_INET_ADDRSTRLEN                                           (16)

/* flags used in send/recv and friends.
 *
 * Note these flags must not exceed 24-bits.  The implementation will
 * OR the 8-bits of security flags into the remaining high 8 bits of
 * 32-bit flag variables.
 */
#define SLNETSOCK_MSG_OOB         (0x0001)
#define SLNETSOCK_MSG_PEEK        (0x0002)
#define SLNETSOCK_MSG_WAITALL     (0x0004)
#define SLNETSOCK_MSG_DONTWAIT    (0x0008)
#define SLNETSOCK_MSG_DONTROUTE   (0x0010)
#define SLNETSOCK_MSG_NOSIGNAL    (0x0020)


/*****************************************************************************/
/* Structure/Enum declarations                                               */
/*****************************************************************************/

/*!
    \brief Internet address
*/
typedef struct SlNetSock_InAddr_t
{
#ifndef s_addr
    uint32_t s_addr;  /* Internet address 32 bits */
#else
/*!
    \brief Different representations for in addr for different hosts.
*/
    union S_un
    {
        uint32_t S_addr;
        struct
        {
            uint8_t s_b1,s_b2,s_b3,s_b4;
        } S_un_b;
        struct
        {
            uint16_t s_w1,s_w2;
        } S_un_w;
    } S_un;
#endif
} SlNetSock_InAddr_t;

/*!
    \brief IpV6 or Ipv6 EUI64
*/
typedef struct SlNetSock_In6Addr_t
{
    union
    {
        uint8_t  _S6_u8[16];
        uint16_t _S6_u16[8];
        uint32_t _S6_u32[4];
    } _S6_un;
} SlNetSock_In6Addr_t;

/*!
    \brief The SlNetSock_NoDelay_t structure is used in #SLNETSOCK_TCP_NODELAY TCP level option
*/
typedef struct SlNetSock_NoDelay_t
{
    uint32_t noDelayEnabled;      /**< 0 = disabled;1 = enabled; default = 0 */
} SlNetSock_NoDelay_t;

/*!
    \brief The SlNetSock_MaxSeg_t structure is used in #SLNETSOCK_TCP_MAXSEG TCP level option
*/
typedef struct SlNetSock_MaxSeg_t
{
    uint32_t maxSeg;               /**< Maximum TCP segment size. Default = 536 */
} SlNetSock_MaxSeg_t;

/*!
    \brief The SlNetSock_NoPush_t structure is used in #SLNETSOCK_TCP_NOPUSH TCP level option
*/
typedef struct SlNetSock_NoPush_t
{
    uint32_t noPushEnabled;      /**< 0 = disabled;1 = enabled; default = 0 */
} SlNetSock_NoPush_t;

/*!
    \brief The SlNetSock_NoOpt_t structure is used in #SLNETSOCK_TCP_NOOPT TCP level option
*/
typedef struct SlNetSock_NoOpt_t
{
    uint32_t noOptEnabled;      /**< 0 = disabled;1 = enabled; default = 0 */
} SlNetSock_NoOpt_t;

/*!
    \brief The SlNetSock_SackPermitted_t structure is used in #SLNETSOCK_TCP_NOPUSH TCP level option
*/
typedef struct SlNetSock_SackPermitted_t
{
    uint32_t sackPermittedEnabled;      /**< 0 = disabled;1 = enabled; default = 0 */
} SlNetSock_SackPermitted_t;

/*!
    \brief The SlNetSock_MaxRtt_t structure is used in #SLNETSOCK_TCP_MAXRTT TCP level option
*/
typedef struct SlNetSock_MaxRtt_t
{
    uint32_t maxRtt;               /**< Maximum TCP Round Trip Time value allowed; Default = 360000000 */
} SlNetSock_MaxRtt_t;

/*!
    \brief The SlNetSock_Keepalive_t structure is used in #SLNETSOCK_OPSOCK_KEEPALIVE socket level option
*/
typedef struct SlNetSock_Keepalive_t
{
    uint32_t keepaliveEnabled;      /**< 0 = disabled;1 = enabled; default = 1 */
} SlNetSock_Keepalive_t;

/*!
    \brief The SlNetSock_NonIpBoundary_t structure is used in #SLNETSOCK_OPSOCK_NON_IP_BOUNDARY socket level option
*/
typedef struct SlNetSock_NonIpBoundary_t
{
    int32_t nonIpBoundaryEnabled;   /**< 0 = keep IP boundary; 1 = don`t keep ip boundary; default = 0; */
} SlNetSock_NonIpBoundary_t;

/*!
    \brief The SlNetSock_Winsize_t structure is used in #SLNETSOCK_OPSOCK_RCV_BUF socket level option
*/
typedef struct SlNetSock_Winsize_t
{
    uint32_t winSize;               /**< receive window size for tcp sockets   */
} SlNetSock_Winsize_t;

/*!
    \brief The SlNetSock_Nonblocking_t structure is used in #SLNETSOCK_OPSOCK_NON_BLOCKING socket level option
*/
typedef struct SlNetSock_Nonblocking_t
{
    uint32_t nonBlockingEnabled;    /**< 0 = disabled, 1 = enabled, default = 1*/
} SlNetSock_Nonblocking_t;

/*!
    \brief The SlNetSock_Broadcast_t structure is used in #SLNETSOCK_OPSOCK_BROADCAST socket level option
*/
typedef struct SlNetSock_Broadcast_t
{
    uint32_t broadcastEnabled;    /**< 0 = disabled, 1 = enabled, default = 0*/
} SlNetSock_Broadcast_t;

/*!
    \brief Secure socket attribute context
*/
typedef struct SlNetSock_SecAttribNode_t
{
    SlNetSockSecAttrib_e              attribName;    /**< Security attribute name          */
    uint8_t                          *attribBuff;    /**< Security attribute buffer        */
    uint16_t                          attribBuffLen; /**< Security attribute buffer length */
    struct SlNetSock_SecAttribNode_t *next;
} SlNetSock_SecAttribNode_t;

/*!
    \brief Secure socket attribute handler
*/
typedef SlNetSock_SecAttribNode_t * SlNetSockSecAttrib_t;

/*!
    \brief Secure ALPN structure
*/
typedef struct SlNetSock_SecureALPN_t
{
    uint32_t secureALPN;
} SlNetSock_SecureALPN_t;

/*!
    \brief Secure Mask structure
*/
typedef struct SlNetSock_SecureMask_t
{
    uint32_t secureMask;
} SlNetSock_SecureMask_t;

/*!
    \brief Secure Method structure
*/
typedef struct SlNetSock_SecureMethod_t
{
    uint8_t secureMethod;
} SlNetSock_SecureMethod_t;

/*!
    \brief The SlNetSock_IpMreq_t structure is used in #SLNETSOCK_OPIP_ADD_MEMBERSHIP and #SLNETSOCK_OPIP_DROP_MEMBERSHIP IP level option
*/
typedef struct SlNetSock_IpMreq_t
{
    SlNetSock_InAddr_t imr_multiaddr;     /**< The IPv4 multicast address to join  */
    uint32_t           imr_interface;     /**< The interface to use for this group */
} SlNetSock_IpMreq_t;

/*!
    \brief The SlNetSock_IpV6Mreq_t structure is used in #SLNETSOCK_OPIPV6_ADD_MEMBERSHIP and #SLNETSOCK_OPIPV6_DROP_MEMBERSHIP IP level option
*/
typedef struct SlNetSock_IpV6Mreq_t
{
    SlNetSock_In6Addr_t ipv6mr_multiaddr; /**< IPv6 multicast address of group                       */
    uint32_t            ipv6mr_interface; /**< should be 0 to choose the default multicast interface */
} SlNetSock_IpV6Mreq_t;

/*!
    \brief The SlNetSock_linger_t structure is used in #SLNETSOCK_OPSOCK_LINGER socket level option
*/
typedef struct SlNetSock_linger_t
{
    uint32_t l_onoff;                    /**< 0 = disabled; 1 = enabled; default = 0; */
    uint32_t l_linger;                   /**< linger time in seconds; default = 0;    */
} SlNetSock_linger_t;

/*!
    \brief      The @c SlNetSock_Timeval_t structure is used in
                #SLNETSOCK_OPSOCK_RCV_TIMEO socket level option

    \remarks    Note that @c SlNetSock_Timeval_t is intentionally defined
                to be equivalent to the POSIX-defined <tt>struct
                timeval</tt> data type.
*/
typedef struct timeval SlNetSock_Timeval_t;

/*!
    \brief The SlNetSocklen_t is used for declaring the socket length parameter
*/
typedef uint16_t SlNetSocklen_t;

/*!
    \brief IpV4 socket address
*/
typedef struct SlNetSock_Addr_t
{
    uint16_t sa_family;                  /**< Address family (e.g. AF_INET)          */
    uint8_t  sa_data[14];                /**< Protocol- specific address information */
} SlNetSock_Addr_t;

/*!
    \brief SlNetSock IPv6 address, Internet style
*/
typedef struct SlNetSock_AddrIn6_t
{
    uint16_t            sin6_family;     /**< SLNETSOCK_AF_INET6             */
    uint16_t            sin6_port;       /**< Transport layer port.          */
    uint32_t            sin6_flowinfo;   /**< IPv6 flow information.         */
    SlNetSock_In6Addr_t sin6_addr;       /**< IPv6 address.                  */
    uint32_t            sin6_scope_id;   /**< set of interfaces for a scope. */
} SlNetSock_AddrIn6_t;

/*!
    \brief SlNetSock IPv4 address, Internet style
*/
typedef struct SlNetSock_AddrIn_t
{
    uint16_t           sin_family;       /**< Internet Protocol (AF_INET). */
    uint16_t           sin_port;         /**< Address port (16 bits).      */
    SlNetSock_InAddr_t sin_addr;         /**< Internet address (32 bits).  */
    int8_t             sin_zero[8];      /**< Not used.                    */
} SlNetSock_AddrIn_t;

/* ss_family + pad must be large enough to hold max of
 * _SlNetSock_AddrIn6_t or _SlNetSock_AddrIn_t
 */
/*!
    \brief Generic socket address type to hold either IPv4 or IPv6 address
*/
typedef struct SlNetSock_SockAddrStorage_t
{
    uint16_t ss_family;
    uint8_t  pad[26];
} SlNetSock_SockAddrStorage_t;

/*!
    \brief The SlNetSock_SdSet_t structure holds the sd array for SlNetSock_select function
*/
typedef struct SlNetSock_SdSet_t         /**< The select socket array manager */
{
    uint32_t sdSetBitmap[(SLNETSOCK_MAX_CONCURRENT_SOCKETS + (uint8_t)31)/(uint8_t)32]; /* Bitmap of SOCKET Descriptors */
} SlNetSock_SdSet_t;


/*!
    \brief The SlNetSock_TransceiverRxOverHead_t structure holds the data for Rx transceiver mode using a raw socket when using SlNetSock_recv function
*/
typedef struct SlNetSock_TransceiverRxOverHead_t
{
    uint8_t  rate;                       /**< Received Rate                                  */
    uint8_t  channel;                    /**< The received channel                           */
    int8_t   rssi;                       /**< The computed RSSI value in db of current frame */
    uint8_t  padding;                    /**< pad to align to 32 bits                        */
    uint32_t timestamp;                  /**< Timestamp in microseconds                      */
} SlNetSock_TransceiverRxOverHead_t;


/*****************************************************************************/
/* Function prototypes                                                       */
/*****************************************************************************/

/*!

    \brief Initialize the SlNetSock module

    \param[in] flags            Reserved

    \return                     Zero on success, or negative error code on failure

    \par    Examples
    \snippet ti/net/test/snippets/slnetif.c SlNetSock_init snippet

*/
int32_t SlNetSock_init(int32_t flags);

/*!

    \brief Create an endpoint for communication

    SlNetSock_create() creates a new socket of a certain socket type,
    identified by an integer number, and allocates system resources to it.\n
    This function is called by the application layer to obtain a socket descriptor (handle).

    \param[in] domain           Specifies the protocol family of the created socket.
                                For example:
                                   - #SLNETSOCK_AF_INET for network protocol IPv4
                                   - #SLNETSOCK_AF_INET6 for network protocol IPv6
                                   - #SLNETSOCK_AF_RF for starting transceiver mode.
                                        Notes:
                                        - sending and receiving any packet overriding 802.11 header
                                        - for optimized power consumption the socket will be started in TX
                                            only mode until receive command is activated
    \param[in] type             Specifies the socket type, which determines the semantics of communication over
                                the socket. The socket types supported by the system are implementation-dependent.
                                Possible socket types include:
                                   - #SLNETSOCK_SOCK_STREAM (reliable stream-oriented service or Stream Sockets)
                                   - #SLNETSOCK_SOCK_DGRAM  (datagram service or Datagram Sockets)
                                   - #SLNETSOCK_SOCK_RAW    (raw protocols atop the network layer)
                                   - when used with AF_RF:
                                      - #SLNETSOCK_SOCK_RX_MTR
                                      - #SLNETSOCK_SOCK_MAC_WITH_CCA
                                      - #SLNETSOCK_SOCK_MAC_WITH_NO_CCA
                                      - #SLNETSOCK_SOCK_BRIDGE
                                      - #SLNETSOCK_SOCK_ROUTER
    \param[in] protocol         Specifies a particular transport to be used with the socket.\n
                                The most common are
                                    - #SLNETSOCK_PROTO_TCP
                                    - #SLNETSOCK_PROTO_UDP
                                    - #SLNETSOCK_PROTO_RAW
                                    - #SLNETSOCK_PROTO_SECURE
    \param[in] ifBitmap         Specifies the interface(s) which the socket will be created on
                                according to priority until one of them will return an answer.\n
                                Value 0 is used in order to choose automatic interfaces selection
                                according to the priority interface list.
                                Value can be a combination of interfaces by OR'ing multiple interfaces bit identifiers
                                (SLNETIFC_IDENT_ defined in slnetif.h)
                                Note: interface identifier bit must be configured prior to this socket creation
                                using SlNetIf_add().
    \param[in] flags            Specifies flags.
                                   - #SLNETSOCK_CREATE_IF_STATE_ENABLE     - Creation of the socket will be on enabled state
                                   - #SLNETSOCK_CREATE_IF_STATUS_CONNECTED - Creation of the socket will be on status connected
                                   - #SLNETSOCK_CREATE_ALLOW_PARTIAL_MATCH - Creation of the socket will be on the interface with
                                                                            the highest priority if the other flags will fail
                                The value 0 may be used in order to run the default flags:
                                   - #SLNETSOCK_CREATE_IF_STATE_ENABLE
                                   - #SLNETSOCK_CREATE_IF_STATUS_CONNECTED

    \return                     On success, socket descriptor (handle) that is used for consequent socket operations. \n
                                A successful return code should be a positive number\n
                                On error, a negative value will be returned specifying the error code.
                                   - #SLNETERR_BSD_EAFNOSUPPORT    - illegal domain parameter
                                   - #SLNETERR_BSD_EPROTOTYPE      - illegal type parameter
                                   - #SLNETERR_BSD_EACCES          - permission denied
                                   - #SLNETERR_BSD_ENSOCK          - exceeded maximal number of socket
                                   - #SLNETERR_BSD_ENOMEM          - memory allocation error
                                   - #SLNETERR_BSD_EINVAL          - error in socket configuration
                                   - #SLNETERR_BSD_EPROTONOSUPPORT - illegal protocol parameter
                                   - #SLNETERR_BSD_EOPNOTSUPP      - illegal combination of protocol and type parameters

    \slnetsock_init_precondition

    \remark     Not all platforms support all options.

    \remark     A @c protocol value of zero can be used to select the default protocol from the selected @c domain and @c type.

    \remark     SlNetSock_create() uses the highest priority interface from the ifBitmap, subject to the constraints specified
                in the flags parameter. An interface that does not satisfy the constraints is ignored, without regards to its
                priority level.

    \par    Examples
    \snippet ti/net/test/snippets/slnetif.c SlNetSock_create TCP IPv4 snippet
    \snippet ti/net/test/snippets/slnetif.c SlNetSock_create TCP IPv6 snippet
    \snippet ti/net/test/snippets/slnetif.c SlNetSock_create UDP IPv4 snippet

    \sa         SlNetSock_close()
*/
int16_t SlNetSock_create(int16_t domain, int16_t type, int16_t protocol, uint32_t ifBitmap, int16_t flags);


/*!
    \brief Gracefully close socket

    Release resources allocated to a socket.

    \param[in] sd               Socket descriptor (handle), received in SlNetSock_create()

    \return                     Zero on success, or negative error code on failure

    \slnetsock_init_precondition

    \remark     In the case of TCP, the connection is terminated.

    \par    Examples
    \snippet ti/net/test/snippets/slnetif.c SlNetSock_close snippet

    \sa                         SlNetSock_create()
*/
int32_t SlNetSock_close(int16_t sd);


/*!
    \brief Shutting down parts of a full-duplex connection

    Shuts down parts of a full-duplex connection according to how parameter.\n

    \param[in] sd               Socket descriptor (handle), received in SlNetSock_create
    \param[in] how              Specifies which part of a full-duplex connection to shutdown. \n
                                The options are
                                    - #SLNETSOCK_SHUT_RD   - further receptions will be disallowed
                                    - #SLNETSOCK_SHUT_WR   - further transmissions will be disallowed
                                    - #SLNETSOCK_SHUT_RDWR - further receptions and transmissions will be disallowed

    \return                     Zero on success, or negative error code on failure

    \slnetsock_init_precondition

    \sa                         SlNetSock_create()
    \sa                         SlNetSock_connect()
    \sa                         SlNetSock_accept()
*/
int32_t SlNetSock_shutdown(int16_t sd, int16_t how);


/*!
    \brief Accept a connection on a socket

    The SlNetSock_accept function is used with connection-based socket types (#SLNETSOCK_SOCK_STREAM).

    It extracts the first connection request on the queue of pending
    connections, creates a new connected socket, and returns a new file
    descriptor referring to that socket.

    The newly created socket is not in the listening state. The
    original socket sd is unaffected by this call.

    The argument sd is a socket that has been created with
    SlNetSock_create(), bound to a local address with
    SlNetSock_bind(), and is listening for connections after a
    SlNetSock_listen().

    The argument \c addr is a pointer to a sockaddr structure. This
    structure is filled in with the address of the peer socket, as
    known to the communications layer.

    The exact format of the address returned \c addr is determined by the socket's address family.

    \c addrlen is a value-result argument: it should initially contain
    the size of the structure pointed to by addr, on return it will
    contain the actual length (in bytes) of the address returned.

    \param[in]  sd              Socket descriptor (handle)
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
                                address format.\n -
                                socket address, the length
                                depends on the code format
    \param[out] addrlen         The addrlen argument is a value-result
                                argument: it should initially contain the
                                size of the structure pointed to by addr

    \return                     On success, a socket descriptor.\n
                                On a non-blocking accept a possible negative value is #SLNETERR_BSD_EAGAIN.\n
                                On failure, negative error code.\n
                                #SLNETERR_BSD_ENOMEM may be return in case there are no resources in the system

    \slnetsock_init_precondition

    \sa                         SlNetSock_create()
    \sa                         SlNetSock_bind()
    \sa                         SlNetSock_listen()
*/
int16_t SlNetSock_accept(int16_t sd, SlNetSock_Addr_t *addr, SlNetSocklen_t *addrlen);


/*!
    \brief Assign a name to a socket

    This SlNetSock_bind function gives the socket the local address
    addr.  addr is addrlen bytes long.

    Traditionally, this is called when a socket is created with
    socket, it exists in a name space (address family) but has no name
    assigned.

    It is necessary to assign a local address before a #SLNETSOCK_SOCK_STREAM
    socket may receive connections.

    \param[in] sd               Socket descriptor (handle)
    \param[in] addr             Specifies the destination
                                addrs\n sockaddr:\n - code for
                                the address format.\n - socket address,
                                the length depends on the code
                                format
    \param[in] addrlen          Contains the size of the structure pointed to by addr

    \return                     Zero on success, or negative error code on failure

    \slnetsock_init_precondition

    \sa                         SlNetSock_create()
    \sa                         SlNetSock_accept()
    \sa                         SlNetSock_listen()
*/
int32_t SlNetSock_bind(int16_t sd, const SlNetSock_Addr_t *addr, int16_t addrlen);


/*!
    \brief Listen for connections on a socket

    The willingness to accept incoming connections and a queue
    limit for incoming connections are specified with SlNetSock_listen(),
    and then the connections are accepted with SlNetSock_accept().

    \param[in] sd               Socket descriptor (handle)
    \param[in] backlog          Specifies the listen queue depth.

    \return                     Zero on success, or negative error code on failure

    \slnetsock_init_precondition

    \remark     The SlNetSock_listen() call applies only to sockets of
                type #SLNETSOCK_SOCK_STREAM.

    \remark     The \c backlog parameter defines the maximum length the queue of
                pending connections may grow to.

    \sa                         SlNetSock_create()
    \sa                         SlNetSock_accept()
    \sa                         SlNetSock_bind()
*/
int32_t SlNetSock_listen(int16_t sd, int16_t backlog);


/*!
    \brief Initiate a connection on a socket

    Function connects the socket referred to by the socket
    descriptor sd, to the address specified by \c addr.

    The format of the address in addr is determined by the address
    space of the socket.

    If it is of type #SLNETSOCK_SOCK_DGRAM, this call specifies the
    peer with which the socket is to be associated; this address is
    that to which datagrams are to be sent, and the only address from
    which datagrams are to be received.

    If the socket is of type #SLNETSOCK_SOCK_STREAM, this call
    attempts to make a connection to another socket.

    The other socket is specified by address, which is an address in
    the communications space of the socket.

    \param[in] sd               Socket descriptor (handle)
    \param[in] addr             Specifies the destination addr\n
                                sockaddr:\n - code for the
                                address format.\n -
                                socket address, the length
                                depends on the code format
    \param[in] addrlen          Contains the size of the structure pointed
                                to by addr

    \return                     On success, a socket descriptor (handle).\n
                                On failure, negative value.\n
                                On a non-blocking connect a possible negative value is #SLNETERR_BSD_EALREADY.
                                #SLNETERR_POOL_IS_EMPTY may be returned in case there are no resources in the system

    \slnetsock_init_precondition

    \sa                         SlNetSock_create()
*/
int32_t SlNetSock_connect(int16_t sd, const SlNetSock_Addr_t *addr, SlNetSocklen_t addrlen);

/*!
    \brief Return address info about the remote side of the connection

    Returns a struct SlNetSock_AddrIn_t
    filled with information about the peer device that is connected
    on the other side of the socket descriptor.

    \param[in]  sd              Socket descriptor (handle)
    \param[out] addr            returns the struct addr\n
                                SlNetSockAddrIn filled with information
                                about the peer device:\n - code for the
                                address format.\n -
                                socket address, the length
                                depends on the code format
    \param[out] addrlen         Contains the size of the structure pointed
                                to by addr

    \return                     Zero on success, or negative error code on failure

    \slnetsock_init_precondition

    \sa                         SlNetSock_accept()
    \sa                         SlNetSock_connect()
*/
int32_t SlNetSock_getPeerName(int16_t sd, SlNetSock_Addr_t *addr, SlNetSocklen_t *addrlen);


/*!
    \brief Get local address info by socket descriptor

    Returns the local address info of the socket descriptor.

    \param[in]  sd              Socket descriptor (handle)
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

    \return                     Zero on success, or negative on failure.

    \slnetsock_init_precondition

    \remark     If the provided buffer is too small the returned address
                will be truncated and \c addrlen will contain the
                actual size of the socket address.

    \sa         SlNetSock_create()
    \sa         SlNetSock_bind()
*/
int32_t SlNetSock_getSockName(int16_t sd, SlNetSock_Addr_t *addr, SlNetSocklen_t *addrlen);


/*!
    \brief Monitor socket activity

    SlNetSock_select() allow a program to monitor multiple file descriptors,
    waiting until one or more of the file descriptors become
    "ready" for some class of I/O operation.

    \param[in]     nsds        The highest-numbered file descriptor in any of the
                               three sets, plus 1.
    \param[in,out] readsds     Socket descriptors list for read monitoring and accept monitoring
    \param[in,out] writesds    Socket descriptors list for connect monitoring only, write monitoring is not supported
    \param[in,out] exceptsds   Socket descriptors list for exception monitoring, not supported.
    \param[in]     timeout     Is an upper bound on the amount of time elapsed
                               before SlNetSock_select() returns. Null or above 0xffff seconds means
                               infinity timeout. The minimum timeout is 10 milliseconds,
                               less than 10 milliseconds will be set automatically to 10 milliseconds.
                               Max microseconds supported is 0xfffc00.
                               In trigger mode the timeout fields must be set to zero.

    \return                    On success, SlNetSock_select() returns the number of
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
                               #SLNETERR_BSD_ENOMEM may be return in case there are no resources in the system

    \slnetsock_init_precondition

    \remark     If \c timeout is set to less than 10ms it will
                automatically set to 10ms to prevent overload of the
                system

    \sa         SlNetSock_create()
*/
int32_t SlNetSock_select(int16_t nsds, SlNetSock_SdSet_t *readsds, SlNetSock_SdSet_t *writesds, SlNetSock_SdSet_t *exceptsds, SlNetSock_Timeval_t *timeout);


/*!
    \brief SlNetSock_select's SlNetSock_SdSet_t SET function

    Sets current socket descriptor on SlNetSock_SdSet_t container
*/
int32_t SlNetSock_sdsSet(int16_t sd, SlNetSock_SdSet_t *sdset);


/*!
    \brief SlNetSock_select's SlNetSock_SdSet_t CLR function

    Clears current socket descriptor on SlNetSock_SdSet_t container
*/
int32_t SlNetSock_sdsClr(int16_t sd, SlNetSock_SdSet_t *sdset);


/*!
    \brief SlNetSock_select's SlNetSock_SdSet_t ZERO function

    Clears all socket descriptors from SlNetSock_SdSet_t
*/
int32_t SlNetSock_sdsClrAll(SlNetSock_SdSet_t *sdset);


/*!
    \brief SlNetSock_select's SlNetSock_SdSet_t ISSET function

    Checks if current socket descriptor is set (true/false)

    \return            Returns true if set, false if unset

*/
int32_t SlNetSock_sdsIsSet(int16_t sd, SlNetSock_SdSet_t *sdset);


/*!
    \brief Set socket options

    SlNetSock_setOpt() manipulates the options associated with a socket.

    Options may exist at multiple protocol levels; they are always
    present at the uppermost socket level.

    When manipulating socket options the level at which the option resides
    and the name of the option must be specified.  To manipulate options at
    the socket level, level is specified as #SLNETSOCK_LVL_SOCKET.  To manipulate
    options at any other level the protocol number of the appropriate protocol
    controlling the option is supplied.  For example, to indicate that an
    option is to be interpreted by the TCP protocol, level should be set to
    the protocol number of TCP.

    \c optval and \c optlen are used to access opt_values
    for SlNetSock_setOpt().  For SlNetSock_getOpt() they identify a
    buffer in which the value for the requested option(s) are to
    be returned.  For SlNetSock_getOpt(), \c optlen is a value-result
    parameter, initially containing the size of the buffer
    pointed to by option_value, and modified on return to
    indicate the actual size of the value returned.  If no option
    value is to be supplied or returned, \c optval may be \c NULL.

    \param[in] sd               Socket descriptor (handle)
    \param[in] level            Defines the protocol level for this option
                                - #SLNETSOCK_LVL_SOCKET - Socket level configurations (L4, transport layer)
                                - #SLNETSOCK_LVL_IP - IP level configurations (L3, network layer)
                                - #SLNETSOCK_LVL_PHY - Link level configurations (L2, link layer)
    \param[in] optname          Defines the option name to interrogate
                                - #SLNETSOCK_LVL_SOCKET
                                  - #SLNETSOCK_OPSOCK_RCV_BUF\n
                                                 Sets tcp max recv window size.\n
                                                 This options takes SlNetSock_Winsize_t struct as parameter
                                  - #SLNETSOCK_OPSOCK_RCV_TIMEO\n
                                                 Sets the timeout value that specifies the maximum amount of time an input function waits until it completes.\n
                                                 Default: No timeout\n
                                                 This options takes SlNetSock_Timeval_t struct as parameter
                                  - #SLNETSOCK_OPSOCK_KEEPALIVE\n
                                                 Enable or Disable periodic keep alive.
                                                 Keeps TCP connections active by enabling the periodic transmission of messages \n
                                                 Timeout is 5 minutes.\n
                                                 Default: Enabled \n
                                                 This options takes SlNetSock_Keepalive_t struct as parameter
                                  - #SLNETSOCK_OPSOCK_KEEPALIVE_TIME\n
                                                 Set keep alive timeout.
                                                 Value is in seconds \n
                                                 Default: 5 minutes \n
                                  - #SLNETSOCK_OPSOCK_LINGER\n
                                                 Socket lingers on close pending remaining send/receive packets\n
                                  - #SLNETSOCK_OPSOCK_NON_BLOCKING\n
                                                 Sets socket to non-blocking operation Impacts: connect, accept, send, sendto, recv and recvfrom. \n
                                                 Default: Blocking.
                                                 This options takes SlNetSock_Nonblocking_t struct as parameter
                                  - #SLNETSOCK_OPSOCK_NON_IP_BOUNDARY\n
                                                 Enable or Disable rx ip boundary.
                                                 In connectionless socket (udp/raw), unread data is dropped (when SlNetSock_recvFrom() len parameter < data size), Enable this option in order to read the left data on the next SlNetSock_recvFrom() iteration\n
                                                 Default: Disabled, IP boundary kept\n
                                                 This options takes SlNetSock_NonIpBoundary_t struct as parameter
                                - #SLNETSOCK_LVL_IP
                                  - #SLNETSOCK_OPIP_MULTICAST_TTL\n
                                                 Set the time-to-live value of outgoing multicast packets for this socket. \n
                                                 This options takes <b>uint8_t</b> as parameter
                                  - #SLNETSOCK_OPIP_ADD_MEMBERSHIP \n
                                                 UDP socket, Join a multicast group. \n
                                                 This options takes SlNetSock_IpMreq_t struct as parameter
                                  - #SLNETSOCK_OPIP_DROP_MEMBERSHIP \n
                                                 UDP socket, Leave a multicast group \n
                                                 This options takes SlNetSock_IpMreq_t struct as parameter
                                  - #SLNETSOCK_OPIP_HDRINCL \n
                                                 RAW socket only, the IPv4 layer generates an IP header when sending a packet unless \n
                                                 the IP_HDRINCL socket option is enabled on the socket.    \n
                                                 When it is enabled, the packet must contain an IP header. \n
                                                 Default: disabled, IPv4 header generated by Network Stack \n
                                                 This options takes <b>uint32_t</b> as parameter
                                  - #SLNETSOCK_OPIP_RAW_RX_NO_HEADER \n
                                                 Raw socket remove IP header from received data. \n
                                                 Default: data includes ip header \n
                                                 This options takes <b>uint32_t</b> as parameter
                                  - #SLNETSOCK_OPIP_RAW_IPV6_HDRINCL (inactive) \n
                                                 RAW socket only, the IPv6 layer generates an IP header when sending a packet unless \n
                                                 the IP_HDRINCL socket option is enabled on the socket. When it is enabled, the packet must contain an IP header \n
                                                 Default: disabled, IPv4 header generated by Network Stack \n
                                                 This options takes <b>uint32_t</b> as parameter
                                - #SLNETSOCK_LVL_PHY
                                  - #SLNETSOCK_OPPHY_CHANNEL \n
                                                 Sets channel in transceiver mode.
                                                 This options takes <b>uint32_t</b> as channel number parameter
                                  - #SLNETSOCK_OPPHY_RATE \n
                                                 RAW socket, set WLAN PHY transmit rate \n
                                                 The values are based on SlWlanRateIndex_e    \n
                                                 This options takes <b>uint32_t</b> as parameter
                                  - #SLNETSOCK_OPPHY_TX_POWER \n
                                                 RAW socket, set WLAN PHY TX power \n
                                                 Valid rage is 1-15 \n
                                                 This options takes <b>uint32_t</b> as parameter
                                  - #SLNETSOCK_OPPHY_NUM_FRAMES_TO_TX \n
                                                 RAW socket, set number of frames to transmit in transceiver mode.
                                                 Default: 1 packet
                                                 This options takes <b>uint32_t</b> as parameter
                                  - #SLNETSOCK_OPPHY_PREAMBLE \n
                                                 RAW socket, set WLAN PHY preamble for Long/Short\n
                                                 This options takes <b>uint32_t</b> as parameter
                                  - #SLNETSOCK_OPPHY_TX_INHIBIT_THRESHOLD \n
                                                 RAW socket, set WLAN Tx - Set CCA threshold. \n
                                                 The values are based on SlNetSockTxInhibitThreshold_e \n
                                                 This options takes <b>uint32_t</b> as parameter
                                  - #SLNETSOCK_OPPHY_TX_TIMEOUT \n
                                                 RAW socket, set WLAN Tx - changes the TX timeout (lifetime) of transceiver frames. \n
                                                 Value in Ms, maximum value is 10ms \n
                                                 This options takes <b>uint32_t</b> as parameter
                                  - #SLNETSOCK_OPPHY_ALLOW_ACKS  \n
                                                 RAW socket, set WLAN Tx - Enable or Disable sending ACKs in transceiver mode \n
                                                 0 = disabled / 1 = enabled \n
                                                 This options takes <b>uint32_t</b> as parameter
                                - #SLNETSOCK_PROTO_TCP
                                  - #SLNETSOCK_TCP_NODELAY \n
                                                 Disables TCP send delay/coalesce algorithm. \n
                                                 This option takes SLNetSock_NoDelay_t struct as a parameter.
                                  - #SLNETSOCK_TCP_MAXSEG \n
                                                 Set the maximum TCP segment size \n
                                                 This option takes SLNetSock_MaxSeg_t struct as a parameter.
                                  - #SLNETSOCK_TCP_NOPUSH \n
                                                 Do not send data just to finish a data block (attempt to coalesce). \n
                                                 This option takes SLNetSock_NoPush_t struct as a parameter
                                  - #SLNETSOCK_TCP_NOOPT \n
                                                 Do not use TCP options. \n
                                                 This option takes SLNetSock_NoOpt_t struct as a parameter.
                                  - #SLNETSOCK_TCP_SACKPERMITTED \n
                                                 Permit RFC-2018 Selective Acknowledgment(SACK) conformant connection
                                                 This option takes SLNetSock_SackPermitted_t struct as a parameter
                                  - #SLNETSOCK_TCP_MAXRTT \n
                                                 The maximum TCP Round Trip Time value allowed in the determination of the estimated TCP RTT. \n
                                                 This option takes SLNetSock_MaxRtt_t struct as a parameter

    \param[in] optval           Specifies a value for the option
    \param[in] optlen           Specifies the length of the
        option value

    \return                     Zero on success, or negative error code on failure

    \par Persistent
                All params are <b>Non- Persistent</b>

    \slnetsock_init_precondition

    \par    Examples

    - SLNETSOCK_OPSOCK_RCV_BUF:
    \code
           SlNetSock_Winsize_t size;
           size.winsize = 3000; // bytes
           SlNetSock_setOpt(SockID, SLNETSOCK_LVL_SOCKET, SLNETSOCK_OPSOCK_RCV_BUF, (uint8_t *)&size, sizeof(size));
    \endcode
    <br>

    - SLNETSOCK_OPSOCK_RCV_TIMEO:
    \code
        struct SlNetSock_Timeval_t timeVal;
        timeVal.tv_sec =  1; // Seconds
        timeVal.tv_usec = 0; // Microseconds. 10000 microseconds resolution
        SlNetSock_setOpt(SockID, SLNETSOCK_LVL_SOCKET, SLNETSOCK_OPSOCK_RCV_TIMEO, (uint8_t *)&timeVal, sizeof(timeVal)); // Enable receive timeout
    \endcode
    <br>

    - SLNETSOCK_OPSOCK_KEEPALIVE: //disable Keepalive
    \code
        SlNetSock_Keepalive_t enableOption;
        enableOption.keepaliveEnabled = 0;
        SlNetSock_setOpt(SockID, SLNETSOCK_LVL_SOCKET, SLNETSOCK_OPSOCK_KEEPALIVE, (uint8_t *)&enableOption, sizeof(enableOption));
    \endcode
    <br>

    - SLNETSOCK_OPSOCK_KEEPALIVE_TIME: //Set Keepalive timeout
    \code
        int16_t Status;
        uint32_t TimeOut = 120;
        SlNetSock_setOpt(Sd, SLNETSOCK_LVL_SOCKET, SLNETSOCK_OPSOCK_KEEPALIVE_TIME, (uint8_t *)&TimeOut, sizeof(TimeOut));
    \endcode
    <br>

    - SLNETSOCK_OPSOCK_NON_BLOCKING: //Enable or disable nonblocking mode
    \code
           SlNetSock_Nonblocking_t enableOption;
           enableOption.nonBlockingEnabled = 1;
           SlNetSock_setOpt(SockID, SLNETSOCK_LVL_SOCKET, SLNETSOCK_OPSOCK_NON_BLOCKING, (uint8_t *)&enableOption, sizeof(enableOption));
    \endcode
    <br>

    - SLNETSOCK_OPSOCK_NON_IP_BOUNDARY: //disable boundary
    \code
        SlNetSock_NonIpBoundary_t enableOption;
        enableOption.nonIpBoundaryEnabled = 1;
        SlNetSock_setOpt(SockID, SLNETSOCK_LVL_SOCKET, SLNETSOCK_OPSOCK_NON_IP_BOUNDARY, (uint8_t *)&enableOption, sizeof(enableOption));
    \endcode
    <br>

    - SLNETSOCK_OPSOCK_LINGER:
    \code
        SlNetSock_linger_t linger;
        linger.l_onoff = 1;
        linger.l_linger = 10;
        SlNetSock_setOpt(SockID, SLNETSOCK_LVL_SOCKET, SLNETSOCK_OPSOCK_LINGER, &linger, sizeof(linger));
    \endcode
    <br>

    - SLNETSOCK_OPIP_MULTICAST_TTL:
     \code
           uint8_t ttl = 20;
           SlNetSock_setOpt(SockID, SLNETSOCK_LVL_IP, SLNETSOCK_OPIP_MULTICAST_TTL, &ttl, sizeof(ttl));
     \endcode
     <br>

    - SLNETSOCK_OPIP_ADD_MEMBERSHIP:
     \code
           SlNetSock_IpMreq_t mreq;
           SlNetSock_setOpt(SockID, SLNETSOCK_LVL_IP, SLNETSOCK_OPIP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));
    \endcode
    <br>

    - SLNETSOCK_OPIP_DROP_MEMBERSHIP:
    \code
        SlNetSock_IpMreq_t mreq;
        SlNetSock_setOpt(SockID, SLNETSOCK_LVL_IP, SLNETSOCK_OPIP_DROP_MEMBERSHIP, &mreq, sizeof(mreq));
    \endcode
    <br>

    - SLNETSOCK_OPIP_RAW_RX_NO_HEADER:
    \code
        uint32_t header = 1;  // remove ip header
        SlNetSock_setOpt(SockID, SLNETSOCK_LVL_IP, SLNETSOCK_OPIP_RAW_RX_NO_HEADER, &header, sizeof(header));
    \endcode
    <br>

    - SLNETSOCK_OPIP_HDRINCL:
    \code
        uint32_t header = 1;
        SlNetSock_setOpt(SockID, SLNETSOCK_LVL_IP, SLNETSOCK_OPIP_HDRINCL, &header, sizeof(header));
    \endcode
    <br>

    - SLNETSOCK_OPIP_RAW_IPV6_HDRINCL:
    \code
        uint32_t header = 1;
        SlNetSock_setOpt(SockID, SLNETSOCK_LVL_IP, SLNETSOCK_OPIP_RAW_IPV6_HDRINCL, &header, sizeof(header));
    \endcode
    <br>

    - SLNETSOCK_OPPHY_CHANNEL:
    \code
        uint32_t newChannel = 6; // range is 1-13
        SlNetSock_setOpt(SockID, SLNETSOCK_LVL_SOCKET, SLNETSOCK_OPPHY_CHANNEL, &newChannel, sizeof(newChannel));
    \endcode
    <br>

    - SLNETSOCK_OPPHY_RATE:
    \code
        uint32_t rate = 6; // see wlan.h SlWlanRateIndex_e for values
        SlNetSock_setOpt(SockID, SLNETSOCK_LVL_PHY, SLNETSOCK_OPPHY_RATE, &rate, sizeof(rate));
    \endcode
    <br>

    - SLNETSOCK_OPPHY_TX_POWER:
    \code
        uint32_t txpower = 1; // valid range is 1-15
        SlNetSock_setOpt(SockID, SLNETSOCK_LVL_PHY, SLNETSOCK_OPPHY_TX_POWER, &txpower, sizeof(txpower));
    \endcode
    <br>

    - SLNETSOCK_OPPHY_NUM_FRAMES_TO_TX:
    \code
        uint32_t numframes = 1;
        SlNetSock_setOpt(SockID, SLNETSOCK_LVL_PHY, SLNETSOCK_OPPHY_NUM_FRAMES_TO_TX, &numframes, sizeof(numframes));
    \endcode
    <br>

    - SLNETSOCK_OPPHY_PREAMBLE:
    \code
        uint32_t preamble = 1;
        SlNetSock_setOpt(SockID, SLNETSOCK_LVL_PHY, SLNETSOCK_OPPHY_PREAMBLE, &preamble, sizeof(preamble));
    \endcode
    <br>

    - SLNETSOCK_OPPHY_TX_INHIBIT_THRESHOLD:
    \code
        uint32_t thrshld = SLNETSOCK_TX_INHIBIT_THRESHOLD_MED;
        SlNetSock_setOpt(SockID, SLNETSOCK_LVL_PHY, SLNETSOCK_OPPHY_TX_INHIBIT_THRESHOLD, &thrshld, sizeof(thrshld));
    \endcode
    <br>

    - SLNETSOCK_OPPHY_TX_TIMEOUT:
    \code
        uint32_t timeout = 50;
        SlNetSock_setOpt(SockID, SLNETSOCK_LVL_PHY, SLNETSOCK_OPPHY_TX_TIMEOUT, &timeout, sizeof(timeout));
    \endcode
    <br>

    - SLNETSOCK_OPPHY_ALLOW_ACKS:
    \code
        uint32_t acks = 1; // 0 = disabled / 1 = enabled
        SlNetSock_setOpt(SockID, SLNETSOCK_LVL_PHY, SLNETSOCK_OPPHY_ALLOW_ACKS, &acks, sizeof(acks));
    \endcode

    \sa     slNetSock_create()
    \sa     SlNetSock_getOpt()
*/
int32_t SlNetSock_setOpt(int16_t sd, int16_t level, int16_t optname, void *optval, SlNetSocklen_t optlen);


/*!
    \brief Get socket options

    The SlNetSock_getOpt function gets the options associated with a socket.
    Options may exist at multiple protocol levels; they are always
    present at the uppermost socket level.

    The parameters optval and optlen identify a
    buffer in which the value for the requested option(s) are to
    be returned.  \c optlen is a value-result
    parameter, initially containing the size of the buffer
    pointed to by option_value, and modified on return to
    indicate the actual size of the value returned.  If no option
    value is to be supplied or returned, \c optval may be \c NULL.


    \param[in]  sd              Socket descriptor (handle)
    \param[in]  level           Defines the protocol level for this option
    \param[in]  optname         defines the option name to interrogate
    \param[out] optval          Specifies a value for the option
    \param[out] optlen          Specifies the length of the
                                option value

    \return                     Zero on success, or negative error code on failure

    \slnetsock_init_precondition

    \sa     SlNetSock_create()
    \sa     SlNetSock_setOpt()
*/
int32_t SlNetSock_getOpt(int16_t sd, int16_t level, int16_t optname, void *optval, SlNetSocklen_t *optlen);


/*!
    \brief Read data from TCP socket

    The SlNetSock_recv function receives a message from a connection-mode socket

    \param[in]  sd              Socket descriptor (handle)
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
                                Using a non-blocking recv a possible negative value is #SLNETERR_BSD_EAGAIN.\n
                                #SLNETERR_BSD_ENOMEM may be return in case there are no resources in the system

    \slnetsock_init_precondition

    \par    Examples
    \snippet ti/net/test/snippets/slnetif.c SlNetSock_recv snippet

    \sa     SlNetSock_create()
    \sa     SlNetSock_recvFrom()
*/
int32_t SlNetSock_recv(int16_t sd, void *buf, uint32_t len, uint32_t flags);


/*!
    \brief Read data from socket

    SlNetSock_recvFrom function receives a message from a connection-mode or
    connectionless-mode socket

    \param[in]  sd              Socket descriptor (handle)
    \param[out] buf             Points to the buffer where the message should be stored.
    \param[in]  len             Specifies the length in bytes of the buffer pointed to by the buffer argument.
                                Range: 1-16000 bytes
    \param[in]  flags           Specifies the type of message
                                reception. On this version, this parameter is not
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
                                Using a non-blocking recv a possible negative value is #SLNETERR_BSD_EAGAIN.
                                #SLNETERR_RET_CODE_INVALID_INPUT will be returned if fromlen has incorrect length.\n
                                #SLNETERR_BSD_ENOMEM may be return in case there are no resources in the system

    \slnetsock_init_precondition

    \par        Example
    \snippet ti/net/test/snippets/slnetif.c SlNetSock_recvFrom snippet

    \sa     SlNetSock_create()
    \sa     SlNetSock_recv()
*/
int32_t SlNetSock_recvFrom(int16_t sd, void *buf, uint32_t len, uint32_t flags, SlNetSock_Addr_t *from, SlNetSocklen_t *fromlen);


/*!
    \brief Write data to TCP socket

    Transmits a message to another socket.
    Returns immediately after sending data to device.
    In case of a RAW socket (transceiver mode), extra 4 bytes should be reserved at the end of the
    frame data buffer for WLAN FCS

    \param[in] sd               Socket descriptor (handle)
    \param[in] buf              Points to a buffer containing
                                the message to be sent
    \param[in] len              Message size in bytes.
    \param[in] flags            Specifies the type of message
                                transmission. On this version, this parameter is not
                                supported for TCP.

    \return                     Return the number of bytes sent,
                                or a negative value if an error occurred.

    \slnetsock_init_precondition

    \par        Example
    \snippet ti/net/test/snippets/slnetif.c SlNetSock_send snippet

    \sa     SlNetSock_create()
    \sa     SlNetSock_sendTo()
*/
int32_t SlNetSock_send(int16_t sd, const void *buf, uint32_t len, uint32_t flags);


/*!
    \brief Write data to socket

    The SlNetSock_sendTo function is used to transmit a message on a connectionless socket
    (connection less socket #SLNETSOCK_SOCK_DGRAM, #SLNETSOCK_SOCK_RAW).

    Returns immediately after sending data to device.

    \param[in] sd               Socket descriptor (handle)
    \param[in] buf              Points to a buffer containing
                                the message to be sent
    \param[in] len              message size in bytes.
    \param[in] flags            Specifies the type of message
                                transmission. On this version, this parameter is not
                                supported
    \param[in] to               Pointer to an address structure
                                indicating the destination
                                address.\n sockaddr:\n - code
                                for the address format.\n - socket address,
                                the length depends on the code
                                format
    \param[in] tolen            Destination address structure size

    \return                     Return the number of bytes sent,
                                or a negative value if an error occurred.\n

    \slnetsock_init_precondition

    \par    Example
    \snippet ti/net/test/snippets/slnetif.c SlNetSock_sendTo snippet

    \sa     SlNetSock_create()
    \sa     SlNetSock_send()
*/
int32_t SlNetSock_sendTo(int16_t sd, const void *buf, uint32_t len, uint32_t flags, const SlNetSock_Addr_t *to, SlNetSocklen_t tolen);


/*!
    \brief Get interface ID from socket descriptor (sd)

    \param[in] sd         Specifies the socket descriptor which its
                          interface identifier needs to be retrieved.\n

    \return               The interface identifier value of the
                          interface on success, or negative error code
                          on failure. The values of the interface
                          identifier is defined with the prefix
                          @c SLNETIF_ID_ in slnetif.h.

    \slnetsock_init_precondition

    \par    Examples
    \snippet ti/net/test/snippets/slnetif.c SlNetSock_getIfID snippet

    \sa         SlNetSock_create()
    \sa         SlNetIf_add()
    \sa         SlNetIf_getIDByName()
*/
int32_t SlNetSock_getIfID(uint16_t sd);


/*!
    \brief Creates a security attributes object

    Create a security attribute, which is required in order to start a secure session.

    \remark     When the security attributes object is no longer needed, call
                SlNetSock_secAttribDelete() to destroy it.

    \remark     A single security object can be used to initiate several secure
                sessions (provided they all have the same security attributes).

    \slnetsock_init_precondition

    \sa         SlNetSock_startSec()
    \sa         SlNetSock_secAttribDelete()
*/
SlNetSockSecAttrib_t *SlNetSock_secAttribCreate(void);


/*!
    \brief Deletes a security attributes object

    \param[in] secAttrib        Secure attribute handle

    \return                     Zero on success, or negative error code
                                on failure

    \slnetsock_init_precondition

    \remark     \c secAttrib must be created using SlNetSock_secAttribCreate()

    \sa         SlNetSock_secAttribCreate()
    \sa         SlNetSock_secAttribSet()
    \sa         SlNetSock_startSec()
*/
int32_t SlNetSock_secAttribDelete(SlNetSockSecAttrib_t *secAttrib);


/*!
    \brief set a security attribute

    The SlNetSock_secAttribSet function is used to set a security
    attribute of a security attribute object.

    \param[in] secAttrib        Secure attribute handle
    \param[in] attribName       Define the actual attribute to set. Applicable values:
                                    - #SLNETSOCK_SEC_ATTRIB_PRIVATE_KEY \n
                                          Sets the private key corresponding to the local certificate \n
                                          This attribute takes the name of security object containing the private key and the name's length (including the NULL terminating character) as parameters \n
                                    - #SLNETSOCK_SEC_ATTRIB_LOCAL_CERT \n
                                          Sets the local certificate chain \n
                                          This attribute takes the name of the security object containing the certificate and the name's length (including the NULL terminating character) as parameters \n
                                          For certificate chains, each certificate in the chain can be added via a separate call to SlNetSock_secAttribSet, starting with the root certificate of the chain \n
                                    - #SLNETSOCK_SEC_ATTRIB_PEER_ROOT_CA \n
                                          Sets the root CA certificate \n
                                          This attribute takes the name of the security object containing the certificate and the name's length (including the NULL terminating character) as parameters \n
                                    - #SLNETSOCK_SEC_ATTRIB_DH_KEY \n
                                          Sets the DH Key \n
                                          This attribute takes the name of the security object containing the DH Key and the name's length (including the NULL terminating character) as parameters \n
                                    - #SLNETSOCK_SEC_ATTRIB_METHOD \n
                                          Sets the TLS protocol version \n
                                          This attribute takes a <b>SLNETSOCK_SEC_METHOD_*</b> option and <b>sizeof(uint8_t)</b> as parameters \n
                                    - #SLNETSOCK_SEC_ATTRIB_CIPHERS \n
                                          Sets the ciphersuites to be used for the connection \n
                                          This attribute takes a bit mask formed using <b>SLNETSOCK_SEC_CIPHER_*</b> options and <b>sizeof(uint32_t)</b> as parameters \n
                                    - #SLNETSOCK_SEC_ATTRIB_ALPN \n
                                          Sets the ALPN \n
                                          This attribute takes a bit mask formed using <b>SLNETSOCK_SEC_ALPN_*</b> options and <b>sizeof(uint32_t)</b> as parameters \n
                                    - #SLNETSOCK_SEC_ATTRIB_EXT_CLIENT_CHLNG_RESP \n
                                          Sets the EXT CLIENT CHLNG RESP \n
                                          Format TBD \n
                                    - #SLNETSOCK_SEC_ATTRIB_DOMAIN_NAME \n
                                          Sets the domain name for verification during connection \n
                                          This attribute takes a string with the domain name and the string's length (including the NULL-terminating character) as parameters \n
                                    - #SLNETSOCK_SEC_ATTRIB_DISABLE_CERT_STORE\n
                                          Sets whether to disable the certificate store \n
                                          This attribute takes <b>1</b> (disable) or <b>0</b> (enable) and <b>sizeof(uint32_t)</b> as parameters \n

    \param[in] val
    \param[in] len

    \return                     Zero on success, or negative error code
                                on failure

    \slnetsock_init_precondition

    \note   Once an attribute is set, it cannot be unset or set to something
            different. Doing so may result in undefined behavior.
            Instead, SlNetSock_secAttribDelete() should be called on the
            existing object, and a new security object should be created with
            the new attribute set.

    \note   The #SLNETSOCK_SEC_ATTRIB_DISABLE_CERT_STORE value
            is currently being evaluated, and may be removed in a
            future release.  It is currently only supported on CC3xxx
            devices.

    \par    Examples

    - SLNETSOCK_SEC_ATTRIB_PRIVATE_KEY:
    \code
           #define PRIVATE_KEY_FILE      "DummyKey"
           SlNetIf_loadSecObj(SLNETIF_SEC_OBJ_TYPE_RSA_PRIVATE_KEY, PRIVATE_KEY_FILE, strlen(PRIVATE_KEY_FILE), srvKeyPem, srvKeyPemLen, SLNETIF_ID_2);
           SlNetSock_secAttribSet(secAttrib, SLNETSOCK_SEC_ATTRIB_PRIVATE_KEY, PRIVATE_KEY_FILE, sizeof(PRIVATE_KEY_FILE));
    \endcode
    <br>

    - SLNETSOCK_SEC_ATTRIB_LOCAL_CERT:
    \code
           #define ROOT_CA_CERT_FILE     "DummyCA"
           #define TRUSTED_CERT_FILE     "DummyTrustedCert"

           // create a local certificate chain
           SlNetIf_loadSecObj(SLNETIF_SEC_OBJ_TYPE_CERTIFICATE, ROOT_CA_CERT_FILE, strlen(ROOT_CA_CERT_FILE), srvCAPem, srvCAPemLen, SLNETIF_ID_2);
           SlNetIf_loadSecObj(SLNETIF_SEC_OBJ_TYPE_CERTIFICATE, TRUSTED_CERT_FILE, strlen(TRUSTED_CERT_FILE), srvCertPem, srvCertPemLen, SLNETIF_ID_2);
           SlNetSock_secAttribSet(secAttrib, SLNETSOCK_SEC_ATTRIB_LOCAL_CERT, ROOT_CA_CERT_FILE, sizeof(ROOT_CA_CERT_FILE));
           SlNetSock_secAttribSet(secAttrib, SLNETSOCK_SEC_ATTRIB_LOCAL_CERT, TRUSTED_CERT_FILE, sizeof(TRUSTED_CERT_FILE));
    \endcode
    <br>

    - SLNETSOCK_SEC_ATTRIB_PEER_ROOT_CA:
    \code
           #define ROOT_CA_CERT_FILE     "DummyCA"
           SlNetIf_loadSecObj(SLNETIF_SEC_OBJ_TYPE_CERTIFICATE, ROOT_CA_CERT_FILE, strlen(ROOT_CA_CERT_FILE), srvCAPem, srvCAPemLen, SLNETIF_ID_2);
           SlNetSock_secAttribSet(secAttrib, SLNETSOCK_SEC_ATTRIB_PEER_ROOT_CA, ROOT_CA_CERT_FILE, sizeof(ROOT_CA_CERT_FILE));
    \endcode
    <br>

    - SLNETSOCK_SEC_ATTRIB_METHOD:
    \code
        uint8_t  SecurityMethod = SLNETSOCK_SEC_METHOD_SSLV3;
        SlNetSock_secAttribSet(secAttrib, SLNETSOCK_SEC_ATTRIB_METHOD, (void *)&(SecurityMethod), sizeof(SecurityMethod));
    \endcode
    <br>

    - SLNETSOCK_SEC_ATTRIB_CIPHERS:
    \code
        uint32_t SecurityCipher = SLNETSOCK_SEC_CIPHER_SSL_RSA_WITH_RC4_128_SHA | SLNETSOCK_SEC_CIPHER_TLS_RSA_WITH_AES_256_CBC_SHA;
        SlNetSock_secAttribSet(secAttrib, SLNETSOCK_SEC_ATTRIB_METHOD, (void *)&(SecurityCipher), sizeof(SecurityCipher));
    \endcode
    <br>

    - SLNETSOCK_SEC_ATTRIB_DOMAIN_NAME:
    \code
        char addr[] = "www.ti.com";
        SlNetSock_secAttribSet(secAttrib, SLNETSOCK_SEC_ATTRIB_DOMAIN_NAME, (void *)addr, strlen(addr) + 1);
    \endcode
    <br>


    \sa         SlNetSock_secAttribCreate()
*/
int32_t SlNetSock_secAttribSet(SlNetSockSecAttrib_t *secAttrib, SlNetSockSecAttrib_e attribName, void *val, uint16_t len);


/*!
    \brief Start a security session on an opened socket

    \param[in] sd               Socket descriptor (handle)
    \param[in] secAttrib        Secure attribute handle. This can be NULL only
                                if the SLNETSOCK_SEC_BIND_CONTEXT_ONLY flag is
                                not thrown.
    \param[in] flags            Specifies flags. \n
                                The available flags are:
                                    - #SLNETSOCK_SEC_START_SECURITY_SESSION_ONLY
                                    - #SLNETSOCK_SEC_BIND_CONTEXT_ONLY
                                    - #SLNETSOCK_SEC_IS_SERVER

    \return                     Zero on success, or negative error code
                                on failure

    \slnetsock_init_precondition

    \remark     If \c secAttrib is \c NULL, the session will be started with
                default security settings.

    \sa         SlNetSock_create()
    \sa         SlNetSock_secAttribCreate()
*/
int32_t SlNetSock_startSec(int16_t sd, SlNetSockSecAttrib_t *secAttrib, uint8_t flags);


/*!

 Close the Doxygen group.
 @}

*/


#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif /* __NET_SOCK_H__ */
