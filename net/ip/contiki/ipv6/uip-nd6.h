/*
 * Copyright (c) 2006, Swedish Institute of Computer Science.
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
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */

/**
 * \addtogroup uip6
 * @{
 */

/**
 * \file
 *    Header file for IPv6 Neighbor discovery (RFC 4861)
 * \author Julien Abeille <jabeille@cisco.com>
 * \author Mathilde Durvy <mdurvy@cisco.com>
 */

#ifndef UIP_ND6_H_
#define UIP_ND6_H_

#include "net/ip/uip.h"
#include "sys/stimer.h"
/**
 *  \name General
 * @{
 */
/** \brief HOP LIMIT to be used when sending ND messages (255) */
#define UIP_ND6_HOP_LIMIT               255
/** \brief INFINITE lifetime */
#define UIP_ND6_INFINITE_LIFETIME       0xFFFFFFFF
/** @} */

/** \name RFC 4861 Host constant */
/** @{ */
#define UIP_ND6_MAX_RTR_SOLICITATION_DELAY 1
#define UIP_ND6_RTR_SOLICITATION_INTERVAL  4
#define UIP_ND6_MAX_RTR_SOLICITATIONS	   3
/** @} */

/** \name RFC 4861 Router constants */
/** @{ */
#ifndef UIP_CONF_ND6_SEND_RA
#define UIP_ND6_SEND_RA                     1   /* enable/disable RA sending */
#else
#define UIP_ND6_SEND_RA UIP_CONF_ND6_SEND_RA
#endif
#ifndef UIP_CONF_ND6_SEND_NA
#define UIP_ND6_SEND_NA                     1   /* enable/disable NA sending */
#else
#define UIP_ND6_SEND_NA UIP_CONF_ND6_SEND_NA
#endif
#ifndef UIP_CONF_ND6_MAX_RA_INTERVAL
#define UIP_ND6_MAX_RA_INTERVAL             600
#else
#define UIP_ND6_MAX_RA_INTERVAL             UIP_CONF_ND6_MAX_RA_INTERVAL
#endif
#ifndef UIP_CONF_ND6_MIN_RA_INTERVAL
#define UIP_ND6_MIN_RA_INTERVAL             (UIP_ND6_MAX_RA_INTERVAL / 3)
#else
#define UIP_ND6_MIN_RA_INTERVAL             UIP_CONF_ND6_MIN_RA_INTERVAL
#endif
#define UIP_ND6_M_FLAG                      0
#define UIP_ND6_O_FLAG                      (UIP_ND6_RA_RDNSS || UIP_ND6_RA_DNSSL)
#define UIP_ND6_ROUTER_LIFETIME             3 * UIP_ND6_MAX_RA_INTERVAL

#define UIP_ND6_MAX_INITIAL_RA_INTERVAL     16  /*seconds*/
#define UIP_ND6_MAX_INITIAL_RAS             3   /*transmissions*/
#ifndef UIP_CONF_ND6_MIN_DELAY_BETWEEN_RAS
#define UIP_ND6_MIN_DELAY_BETWEEN_RAS       3   /*seconds*/
#else
#define UIP_ND6_MIN_DELAY_BETWEEN_RAS       UIP_CONF_ND6_MIN_DELAY_BETWEEN_RAS
#endif
//#define UIP_ND6_MAX_RA_DELAY_TIME           0.5 /*seconds*/
#define UIP_ND6_MAX_RA_DELAY_TIME_MS        500 /*milli seconds*/
/** @} */

#ifndef UIP_CONF_ND6_DEF_MAXDADNS
/** \brief Do not try DAD when using EUI-64 as allowed by draft-ietf-6lowpan-nd-15 section 8.2 */
#if UIP_CONF_LL_802154
#define UIP_ND6_DEF_MAXDADNS 0
#else /* UIP_CONF_LL_802154 */
#define UIP_ND6_DEF_MAXDADNS UIP_ND6_SEND_NA
#endif /* UIP_CONF_LL_802154 */
#else /* UIP_CONF_ND6_DEF_MAXDADNS */
#define UIP_ND6_DEF_MAXDADNS UIP_CONF_ND6_DEF_MAXDADNS
#endif /* UIP_CONF_ND6_DEF_MAXDADNS */

/** \name RFC 4861 Node constant */
#define UIP_ND6_MAX_MULTICAST_SOLICIT  3

#ifdef UIP_CONF_ND6_MAX_UNICAST_SOLICIT
#define UIP_ND6_MAX_UNICAST_SOLICIT    UIP_CONF_ND6_MAX_UNICAST_SOLICIT
#else /* UIP_CONF_ND6_MAX_UNICAST_SOLICIT */
#define UIP_ND6_MAX_UNICAST_SOLICIT    3
#endif /* UIP_CONF_ND6_MAX_UNICAST_SOLICIT */

#ifdef UIP_CONF_ND6_REACHABLE_TIME
#define UIP_ND6_REACHABLE_TIME         UIP_CONF_ND6_REACHABLE_TIME
#else
#define UIP_ND6_REACHABLE_TIME         30000
#endif

#ifdef UIP_CONF_ND6_RETRANS_TIMER
#define UIP_ND6_RETRANS_TIMER	       UIP_CONF_ND6_RETRANS_TIMER
#else
#define UIP_ND6_RETRANS_TIMER	       1000
#endif

#define UIP_ND6_DELAY_FIRST_PROBE_TIME 5
#define UIP_ND6_MIN_RANDOM_FACTOR(x)   (x / 2)
#define UIP_ND6_MAX_RANDOM_FACTOR(x)   ((x) + (x) / 2)
/** @} */


/** \name RFC 6106 RA DNS Options Constants  */
/** @{ */
#ifndef UIP_CONF_ND6_RA_RDNSS
#define UIP_ND6_RA_RDNSS                0
#else
#define UIP_ND6_RA_RDNSS                UIP_CONF_ND6_RA_RDNSS
#endif

#ifndef UIP_CONF_ND6_RA_DNSSL
#define UIP_ND6_RA_DNSSL                0
#else
#error Not implemented
#define UIP_ND6_RA_DNSSL                UIP_CONF_ND6_RA_DNSSL
#endif
/** @} */


/** \name ND6 option types */
/** @{ */
#define UIP_ND6_OPT_SLLAO               1
#define UIP_ND6_OPT_TLLAO               2
#define UIP_ND6_OPT_PREFIX_INFO         3
#define UIP_ND6_OPT_REDIRECTED_HDR      4
#define UIP_ND6_OPT_MTU                 5
#define UIP_ND6_OPT_RDNSS               25
#define UIP_ND6_OPT_DNSSL               31
/** @} */

/** \name ND6 option types */
/** @{ */
#define UIP_ND6_OPT_TYPE_OFFSET         0
#define UIP_ND6_OPT_LEN_OFFSET          1
#define UIP_ND6_OPT_DATA_OFFSET         2

/** \name ND6 message length (excluding options) */
/** @{ */
#define UIP_ND6_NA_LEN                  20
#define UIP_ND6_NS_LEN                  20
#define UIP_ND6_RA_LEN                  12
#define UIP_ND6_RS_LEN                  4
/** @} */


/** \name ND6 option length in bytes */
/** @{ */
#define UIP_ND6_OPT_HDR_LEN            2
#define UIP_ND6_OPT_PREFIX_INFO_LEN    32
#define UIP_ND6_OPT_MTU_LEN            8
#define UIP_ND6_OPT_RDNSS_LEN          1
#define UIP_ND6_OPT_DNSSL_LEN          1


/* Length of TLLAO and SLLAO options, it is L2 dependant */
#if UIP_CONF_LL_802154
/* If the interface is 802.15.4. For now we use only long addresses */
#define UIP_ND6_OPT_SHORT_LLAO_LEN     8
#define UIP_ND6_OPT_LONG_LLAO_LEN      16
/** \brief length of a ND6 LLAO option for 802.15.4 */
#define UIP_ND6_OPT_LLAO_LEN UIP_ND6_OPT_LONG_LLAO_LEN
#else /*UIP_CONF_LL_802154*/
#if UIP_CONF_LL_80211
/* If the interface is 802.11 */
/** \brief length of a ND6 LLAO option for 802.11 */
#define UIP_ND6_OPT_LLAO_LEN           8
#else /*UIP_CONF_LL_80211*/
/** \brief length of a ND6 LLAO option for default L2 type (e.g. Ethernet) */
#define UIP_ND6_OPT_LLAO_LEN           8
#endif /*UIP_CONF_LL_80211*/
#endif /*UIP_CONF_LL_802154*/
/** @} */


/** \name Neighbor Advertisement flags masks */
/** @{ */
#define UIP_ND6_NA_FLAG_ROUTER          0x80
#define UIP_ND6_NA_FLAG_SOLICITED       0x40
#define UIP_ND6_NA_FLAG_OVERRIDE        0x20
#define UIP_ND6_RA_FLAG_ONLINK          0x80
#define UIP_ND6_RA_FLAG_AUTONOMOUS      0x40
/** @} */

/**
 * \name ND message structures
 * @{
 */

/**
 * \brief A neighbor solicitation constant part
 *
 * Possible option is: SLLAO
 */
typedef struct uip_nd6_ns {
  uint32_t reserved;
  uip_ipaddr_t tgtipaddr;
} uip_nd6_ns;

/**
 * \brief A neighbor advertisement constant part.
 *
 * Possible option is: TLLAO
 */
typedef struct uip_nd6_na {
  uint8_t flagsreserved;
  uint8_t reserved[3];
  uip_ipaddr_t tgtipaddr;
} uip_nd6_na;

/**
 * \brief A router solicitation  constant part
 *
 * Possible option is: SLLAO
 */
typedef struct uip_nd6_rs {
  uint32_t reserved;
} uip_nd6_rs;

/**
 * \brief A router advertisement constant part
 *
 * Possible options are: SLLAO, MTU, Prefix Information
 */
typedef struct uip_nd6_ra {
  uint8_t cur_ttl;
  uint8_t flags_reserved;
  uint16_t router_lifetime;
  uint32_t reachable_time;
  uint32_t retrans_timer;
} uip_nd6_ra;

/**
 * \brief A redirect message constant part
 *
 * Possible options are: TLLAO, redirected header
 */
typedef struct uip_nd6_redirect {
  uint32_t reserved;
  uip_ipaddr_t tgtipaddress;
  uip_ipaddr_t destipaddress;
} uip_nd6_redirect;
/** @} */

/**
 * \name ND Option structures
 * @{
 */

/** \brief ND option header */
typedef struct uip_nd6_opt_hdr {
  uint8_t type;
  uint8_t len;
} uip_nd6_opt_hdr;

/** \brief ND option prefix information */
typedef struct uip_nd6_opt_prefix_info {
  uint8_t type;
  uint8_t len;
  uint8_t preflen;
  uint8_t flagsreserved1;
  uint32_t validlt;
  uint32_t preferredlt;
  uint32_t reserved2;
  uip_ipaddr_t prefix;
} uip_nd6_opt_prefix_info ;

/** \brief ND option MTU */
typedef struct uip_nd6_opt_mtu {
  uint8_t type;
  uint8_t len;
  uint16_t reserved;
  uint32_t mtu;
} uip_nd6_opt_mtu;

/** \brief ND option RDNSS */
typedef struct uip_nd6_opt_dns {
  uint8_t type;
  uint8_t len;
  uint16_t reserved;
  uint32_t lifetime;
  uip_ipaddr_t ip;
} uip_nd6_opt_dns;

/** \struct Redirected header option */
typedef struct uip_nd6_opt_redirected_hdr {
  uint8_t type;
  uint8_t len;
  uint8_t reserved[6];
} uip_nd6_opt_redirected_hdr;
/** @} */

/**
 * \name ND Messages Processing and Generation
 * @{
 */
 /**
 * \brief Process a neighbor solicitation
 *
 * The NS can be received in 3 cases (procedures):
 * - sender is performing DAD (ip src = unspecified, no SLLAO option)
 * - sender is performing NUD (ip dst = unicast)
 * - sender is performing address resolution (ip dest = solicited node mcast
 * address)
 *
 * We do:
 * - if the tgt belongs to me, reply, otherwise ignore
 * - if i was performing DAD for the same address, two cases:
 * -- I already sent a NS, hence I win
 * -- I did not send a NS yet, hence I lose
 *
 * If we need to send a NA in response (i.e. the NS was done for NUD, or
 * address resolution, or DAD and there is a conflict), we do it in this
 * function: set src, dst, tgt address in the three cases, then for all cases
 * set the rest, including  SLLAO
 *
 */
void
uip_nd6_ns_input(void);

/**
 * \brief Send a neighbor solicitation, send a Neighbor Advertisement
 * \param src pointer to the src of the NS if known
 * \param dest pointer to ip address to send the NS, for DAD or ADDR Resol,
 * MUST be NULL, for NUD, must be correct unicast dest
 * \param tgt  pointer to ip address to fill the target address field, must
 * not be NULL
 *
 * - RFC 4861, 7.2.2 :
 *   "If the source address of the packet prompting the solicitation is the
 *   same as one of the addresses assigned to the outgoing interface, that
 *   address SHOULD be placed in the IP Source Address of the outgoing
 *   solicitation.  Otherwise, any one of the addresses assigned to the
 *   interface should be used."
 *   This is why we have a src ip address as argument. If NULL, we will do
 *   src address selection, otherwise we use the argument.
 *
 * - we check if it is a NS for Address resolution  or NUD, if yes we include
 *   a SLLAO option, otherwise no.
 */
void
uip_nd6_ns_output(uip_ipaddr_t *src, uip_ipaddr_t *dest, uip_ipaddr_t *tgt);

#if UIP_CONF_ROUTER
#if UIP_ND6_SEND_RA
/**
 * \brief send a Router Advertisement
 *
 * Only for router, for periodic as well as sollicited RA
 */
void uip_nd6_ra_output(uip_ipaddr_t *dest);
#endif /* UIP_ND6_SEND_RA */
#endif /*UIP_CONF_ROUTER*/

/**
 * \brief Send a Router Solicitation
 *
 * src is chosen through the uip_netif_select_src function. If src is
 * unspecified  (i.e. we do not have a preferred address yet), then we do not
 * put a SLLAO option (MUST NOT in RFC 4861). Otherwise we do.
 *
 * RS message format,
 * possible option is SLLAO, MUST NOT be included if source = unspecified
 * SHOULD be included otherwise
 */
void uip_nd6_rs_output(void);

/**
 * \brief Initialise the uIP ND core
 */
void uip_nd6_init(void);
/** @} */


void
uip_appserver_addr_get(uip_ipaddr_t *ipaddr);
/*--------------------------------------*/
/******* ANNEX - message formats ********/
/*--------------------------------------*/

/*
 * RS format. possible option is SLLAO
 *    0                   1                   2                   3
 *    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *    |     Type      |     Code      |          Checksum             |
 *    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *    |                            Reserved                           |
 *    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *    |   Options ...
 *    +-+-+-+-+-+-+-+-+-+-+-+-
 *
 *
 * RA format. possible options: prefix information, MTU, SLLAO
 *    0                   1                   2                   3
 *    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *    |     Type      |     Code      |          Checksum             |
 *    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *    | Cur Hop Limit |M|O|  Reserved |       Router Lifetime         |
 *    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *    |                         Reachable Time                        |
 *    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *    |                          Retrans Timer                        |
 *    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *    |   Options ...
 *    +-+-+-+-+-+-+-+-+-+-+-+-
 *
 *
 * NS format: options should be SLLAO
 *    0                   1                   2                   3
 *    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *    |     Type      |     Code      |          Checksum             |
 *    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *    |                           Reserved                            |
 *    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *    |                                                               |
 *    +                                                               +
 *    |                                                               |
 *    +                       Target Address                          +
 *    |                                                               |
 *    +                                                               +
 *    |                                                               |
 *    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *    |   Options ...
 *    +-+-+-+-+-+-+-+-+-+-+-+-
 *
 *
 * NA message format. possible options is TLLAO
 *
 *    0                   1                   2                   3
 *    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *    |     Type      |     Code      |          Checksum             |
 *    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *    |R|S|O|                     Reserved                            |
 *    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *    |                                                               |
 *    +                                                               +
 *    |                                                               |
 *    +                       Target Address                          +
 *    |                                                               |
 *    +                                                               +
 *    |                                                               |
 *    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *    |   Options ...
 *    +-+-+-+-+-+-+-+-+-+-+-+-
 *
 *
 * Redirect message format. Possible options are TLLAO and Redirected header
 *
 *    0                   1                   2                   3
 *    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *    |     Type      |     Code      |          Checksum             |
 *    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *    |                           Reserved                            |
 *    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *    |                                                               |
 *    +                                                               +
 *    |                                                               |
 *    +                       Target Address                          +
 *    |                                                               |
 *    +                                                               +
 *    |                                                               |
 *    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *    |                                                               |
 *    +                                                               +
 *    |                                                               |
 *    +                     Destination Address                       +
 *    |                                                               |
 *    +                                                               +
 *    |                                                               |
 *    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *    |   Options ...
 *    +-+-+-+-+-+-+-+-+-+-+-+-
 *
 *
 * SLLAO/TLLAO option:
 *    0                   1                   2                   3
 *    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *    |     Type      |    Length     |    Link-Layer Address ...
 *    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 *
 * Prefix information option
 *    0                   1                   2                   3
 *    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *    |     Type      |    Length     | Prefix Length |L|A| Reserved1 |
 *    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *    |                         Valid Lifetime                        |
 *    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *    |                       Preferred Lifetime                      |
 *    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *    |                           Reserved2                           |
 *    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *    |                                                               |
 *    +                                                               +
 *    |                                                               |
 *    +                            Prefix                             +
 *    |                                                               |
 *    +                                                               +
 *    |                                                               |
 *    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 *
 * MTU option
 *    0                   1                   2                   3
 *    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *    |     Type      |    Length     |           Reserved            |
 *    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *    |                              MTU                              |
 *    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 *
 * Redirected header option
 *
 *    0                   1                   2                   3
 *    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *    |     Type      |    Length     |            Reserved           |
 *    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *    |                           Reserved                            |
 *    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *    |                                                               |
 *    ~                       IP header + data                        ~
 *    |                                                               |
 *    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 */
#endif /* UIP_ND6_H_ */

/** @} */
