/*
 * Copyright (c) 2012, Thingsquare, http://www.thingsquare.com/.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef CONTIKI_DEFAULT_CONF_H
#define CONTIKI_DEFAULT_CONF_H

/*---------------------------------------------------------------------------*/
/* Netstack configuration
 *
 * The netstack configuration is typically overridden by the platform
 * configuration, as defined in contiki-conf.h
 */

/* NETSTACK_CONF_RADIO specifies the radio driver. The radio driver
   typically depends on the radio used on the target hardware. */
#ifndef NETSTACK_CONF_RADIO
#define NETSTACK_CONF_RADIO nullradio_driver
/* #define NETSTACK_CONF_RADIO cc2420_driver */
#endif /* NETSTACK_CONF_RADIO */

/* NETSTACK_CONF_FRAMER specifies the over-the-air frame format used
   by Contiki radio packets. For IEEE 802.15.4 radios, use the
   framer_802154 driver. */
#ifndef NETSTACK_CONF_FRAMER
#define NETSTACK_CONF_FRAMER framer_nullmac
/* #define NETSTACK_CONF_FRAMER framer_802154 */
#endif /* NETSTACK_CONF_FRAMER */

/* NETSTACK_CONF_RDC specifies the Radio Duty Cycling (RDC) layer. The
   nullrdc_driver never turns the radio off and is compatible with all
   radios, but consumes a lot of power. The contikimac_driver is
   highly power-efficent and allows sleepy routers, but is not
   compatible with all radios. */
#ifndef NETSTACK_CONF_RDC
#define NETSTACK_CONF_RDC   nullrdc_driver
/* #define NETSTACK_CONF_RDC   contikimac_driver */
#endif /* NETSTACK_CONF_RDC */

/* NETSTACK_CONF_MAC specifies the Medium Access Control (MAC)
   layer. The nullmac_driver does not provide any MAC
   functionality. The csma_driver is the default CSMA MAC layer, but
   is not compatible with all radios. */
#ifndef NETSTACK_CONF_MAC
#define NETSTACK_CONF_MAC   nullmac_driver
/* #define NETSTACK_CONF_MAC   csma_driver */
#endif /* NETSTACK_CONF_MAC */

/* NETSTACK_CONF_LLSEC specifies the link layer security driver. */
#ifndef NETSTACK_CONF_LLSEC
#define NETSTACK_CONF_LLSEC nullsec_driver
#endif /* NETSTACK_CONF_LLSEC */

/* To avoid unnecessary complexity, we assume the common case of
   a constant LoWPAN-wide IEEE 802.15.4 security level, which
   can be specified by defining LLSEC802154_CONF_SECURITY_LEVEL. */
#ifndef LLSEC802154_CONF_SECURITY_LEVEL
#define LLSEC802154_CONF_SECURITY_LEVEL 0
#endif /* LLSEC802154_CONF_SECURITY_LEVEL */

/* NETSTACK_CONF_NETWORK specifies the network layer and can be either
   sicslowpan_driver, for IPv6 networking, or rime_driver, for the
   custom Rime network stack. */
#ifndef NETSTACK_CONF_NETWORK
#define NETSTACK_CONF_NETWORK rime_driver
/* #define NETSTACK_CONF_NETWORK sicslowpan_driver */
#endif /* NETSTACK_CONF_NETWORK */

/* NETSTACK_CONF_RDC_CHANNEL_CHECK_RATE specifies the channel check
   rate of the RDC layer. This defines how often the RDC will wake up
   and check for radio channel activity. A higher check rate results
   in higher communication performance at the cost of a higher power
   consumption. */
#ifndef NETSTACK_CONF_RDC_CHANNEL_CHECK_RATE
#define NETSTACK_CONF_RDC_CHANNEL_CHECK_RATE 8
#endif /* NETSTACK_CONF_RDC_CHANNEL_CHECK_RATE */

/*---------------------------------------------------------------------------*/
/* Packet buffer size options.
 *
 * The packet buffer size options can be tweaked on a per-project
 * basis to reduce memory consumption.
 */

/* QUEUEBUF_CONF_NUM specifies the number of queue buffers. Queue
   buffers are used throughout the Contiki netstack but the
   configuration option can be tweaked to save memory. Performance can
   suffer with a too low number of queue buffers though. */
#ifndef QUEUEBUF_CONF_NUM
#define QUEUEBUF_CONF_NUM 8
#endif /* QUEUEBUF_CONF_NUM */
/*---------------------------------------------------------------------------*/
/* uIPv6 configuration options.
 *
 * Many of the uIPv6 configuration options can be overriden by a
 * project-specific configuration to save memory.
 */

/* NETSTACK_CONF_WITH_IPV6 specifies whether or not IPv6 should be used. If IPv6
   is not used, IPv4 is used instead. */
#ifndef NETSTACK_CONF_WITH_IPV6
#define NETSTACK_CONF_WITH_IPV6 0
#endif /* NETSTACK_CONF_WITH_IPV6 */

/* UIP_CONF_BUFFER_SIZE specifies how much memory should be reserved
   for the uIP packet buffer. This sets an upper bound on the largest
   IP packet that can be received by the system. */
#ifndef UIP_CONF_BUFFER_SIZE
#define UIP_CONF_BUFFER_SIZE 128
#endif /* UIP_CONF_BUFFER_SIZE */

/* UIP_CONF_ROUTER specifies if the IPv6 node should be a router or
   not. By default, all Contiki nodes are routers. */
#ifndef UIP_CONF_ROUTER
#define UIP_CONF_ROUTER 1
#endif /* UIP_CONF_ROUTER */

/* UIP_CONF_IPV6_RPL specifies if RPL is to be used for IPv6
   routing. */
#ifndef UIP_CONF_IPV6_RPL
#define UIP_CONF_IPV6_RPL 1
#endif /* UIP_CONF_IPV6_RPL */

/* UIP_CONF_MAX_ROUTES specifies the maximum number of routes that each
   node will be able to handle. */
#ifndef UIP_CONF_MAX_ROUTES
#define UIP_CONF_MAX_ROUTES 20
#endif /* UIP_CONF_MAX_ROUTES */

/* UIP_CONF_UDP specifies if UDP support should be included or
   not. Disabling UDP saves memory but breaks a lot of stuff. */
#ifndef UIP_CONF_UDP
#define UIP_CONF_UDP 1
#endif /* UIP_CONF_UDP */

/* UIP_CONF_MAX_CONNECTIONS specifies the maximum number of
   simultaneous TCP connections. */
#ifndef UIP_CONF_MAX_CONNECTIONS
#define UIP_CONF_MAX_CONNECTIONS 8
#endif /* UIP_CONF_MAX_CONNECTIONS */

/* UIP_CONF_TCP specifies if TCP support should be included or
   not. Disabling TCP saves memory. */
#ifndef UIP_CONF_TCP
#define UIP_CONF_TCP 1
#endif /* UIP_CONF_TCP */

/* UIP_CONF_MAX_CONNECTIONS specifies the maximum number of
   simultaneous TCP connections. */
#ifndef UIP_CONF_MAX_CONNECTIONS
#define UIP_CONF_MAX_CONNECTIONS 8
#endif /* UIP_CONF_MAX_CONNECTIONS */


/* UIP_CONF_TCP_SPLIT enables a performance optimization hack, where
   each maximum-sized TCP segment is split into two, to avoid the
   performance degradation that is caused by delayed ACKs. */
#ifndef UIP_CONF_TCP_SPLIT
#define UIP_CONF_TCP_SPLIT 0
#endif /* UIP_CONF_TCP_SPLIT */

/* NBR_TABLE_CONF_MAX_NEIGHBORS specifies the maximum number of neighbors
   that each node will be able to handle. */
#ifndef NBR_TABLE_CONF_MAX_NEIGHBORS
#define NBR_TABLE_CONF_MAX_NEIGHBORS 8
#endif /* NBR_TABLE_CONF_MAX_NEIGHBORS */

/* UIP_CONF_ND6_SEND_NA enables standard IPv6 Neighbor Discovery Protocol.
   This is unneeded when RPL is used. Disable to save ROM and a little RAM. */
#ifndef UIP_CONF_ND6_SEND_NA
#define UIP_CONF_ND6_SEND_NA 1
#endif /* UIP_CONF_ND6_SEND_NA */

/*---------------------------------------------------------------------------*/
/* 6lowpan configuration options.
 *
 * These options change the behavior of the 6lowpan header compression
 * code (sicslowpan). They typically depend on the type of radio used
 * on the target platform, and are therefore platform-specific.
 */

/* SICSLOWPAN_CONF_MAX_MAC_TRANSMISSIONS specifies how many times the
   MAC layer should resend packets if no link-layer ACK was
   received. This only makes sense with the csma_driver
   NETSTACK_CONF_MAC. */
#ifndef SICSLOWPAN_CONF_MAX_MAC_TRANSMISSIONS
#define SICSLOWPAN_CONF_MAX_MAC_TRANSMISSIONS 4
#endif /* SICSLOWPAN_CONF_MAX_MAC_TRANSMISSIONS */

/* SICSLOWPAN_CONF_FRAG specifies if 6lowpan fragmentation should be
   used or not. Fragmentation is on by default. */
#ifndef SICSLOWPAN_CONF_FRAG
#define SICSLOWPAN_CONF_FRAG 1
#endif /* SICSLOWPAN_CONF_FRAG */

/* SICSLOWPAN_CONF_MAC_MAX_PAYLOAD is the maximum available size for
   frame headers, link layer security-related overhead,  as well as
   6LoWPAN payload. By default, SICSLOWPAN_CONF_MAC_MAX_PAYLOAD is
   127 bytes (MTU of 802.15.4) - 2 bytes (Footer of 802.15.4). */
#ifndef SICSLOWPAN_CONF_MAC_MAX_PAYLOAD
#define SICSLOWPAN_CONF_MAC_MAX_PAYLOAD (127 - 2)
#endif /* SICSLOWPAN_CONF_MAC_MAX_PAYLOAD */

/* SICSLOWPAN_CONF_COMPRESSION_THRESHOLD sets a lower threshold for
   when packets should not be compressed. This is used by ContikiMAC,
   which requires packets to be larger than a given minimum size. */
#ifndef SICSLOWPAN_CONF_COMPRESSION_THRESHOLD
#define SICSLOWPAN_CONF_COMPRESSION_THRESHOLD 0
/* #define SICSLOWPAN_CONF_COMPRESSION_THRESHOLD 63 */
#endif /* SICSLOWPAN_CONF_COMPRESSION_THRESHOLD */

/* SICSLOWPAN_CONF_COMPRESSION specifies what 6lowpan compression
   mechanism to be used. 6lowpan hc06 is the default in Contiki. */
#ifndef SICSLOWPAN_CONF_COMPRESSION
#define SICSLOWPAN_CONF_COMPRESSION SICSLOWPAN_COMPRESSION_HC06
#endif /* SICSLOWPAN_CONF_COMPRESSION */

/*---------------------------------------------------------------------------*/
/* ContikiMAC configuration options.
 *
 * These are typically configured on a per-platform basis.
 */

/* CONTIKIMAC_CONF_WITH_PHASE_OPTIMIZATION specifies if ContikiMAC
   should optimize for the phase of neighbors. The phase optimization
   may reduce power consumption but is not compatible with all timer
   settings and is therefore off by default. */
#ifndef CONTIKIMAC_CONF_WITH_PHASE_OPTIMIZATION
#define CONTIKIMAC_CONF_WITH_PHASE_OPTIMIZATION 0
#endif /* CONTIKIMAC_CONF_WITH_PHASE_OPTIMIZATION */


#endif /* CONTIKI_DEFAULT_CONF_H */
