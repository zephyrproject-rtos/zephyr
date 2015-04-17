/*
 * Copyright (c) 2011, Loughborough University - Computer Science
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
 */

/**
 * \addtogroup uip6
 * @{
 */
/**
 * \defgroup uip6-multicast IPv6 Multicast Forwarding
 *
 *   We currently support 2 engines:
 *   - 'Stateless Multicast RPL Forwarding' (SMRF)
 *     RPL does group management as per the RPL docs, SMRF handles datagram
 *     forwarding
 *   - 'Multicast Forwarding with Trickle' according to the algorithm described
 *     in the internet draft:
 *     http://tools.ietf.org/html/draft-ietf-roll-trickle-mcast
 *
 * @{
 */

/**
 * \file
 *   This header file contains configuration directives for uIPv6
 *   multicast support.
 *
 * \author
 *   George Oikonomou - <oikonomou@users.sourceforge.net>
 */

#ifndef UIP_MCAST6_H_
#define UIP_MCAST6_H_

#include "contiki-conf.h"
#include "net/ipv6/multicast/uip-mcast6-engines.h"
#include "net/ipv6/multicast/uip-mcast6-route.h"
#include "net/ipv6/multicast/smrf.h"
#include "net/ipv6/multicast/roll-tm.h"

#include <string.h>
/*---------------------------------------------------------------------------*/
/* Constants */
/*---------------------------------------------------------------------------*/
#define UIP_MCAST6_DROP       0
#define UIP_MCAST6_ACCEPT     1
/*---------------------------------------------------------------------------*/
/* Multicast Address Scopes (RFC 4291 & draft-ietf-6man-multicast-scopes) */
#define UIP_MCAST6_SCOPE_INTERFACE        0x01
#define UIP_MCAST6_SCOPE_LINK_LOCAL       0x02
#define UIP_MCAST6_SCOPE_REALM_LOCAL      0x03 /* draft-ietf-6man-multicast-scopes */
#define UIP_MCAST6_SCOPE_ADMIN_LOCAL      0x04
#define UIP_MCAST6_SCOPE_SITE_LOCAL       0x05
#define UIP_MCAST6_SCOPE_ORG_LOCAL        0x08
#define UIP_MCAST6_SCOPE_GLOBAL           0x0E
/*---------------------------------------------------------------------------*/
/* Choose an engine or turn off based on user configuration */
/*---------------------------------------------------------------------------*/
#ifdef UIP_MCAST6_CONF_ENGINE
#define UIP_MCAST6_ENGINE UIP_MCAST6_CONF_ENGINE
#else
#define UIP_MCAST6_ENGINE UIP_MCAST6_ENGINE_NONE
#endif
/*---------------------------------------------------------------------------*/
/*
 * Multicast API. Similar to NETSTACK, each engine must define a driver and
 * populate the fields with suitable function pointers
 */
/**
 * \brief The data structure used to represent a multicast engine
 */
struct uip_mcast6_driver {
  /** The driver's name */
  char *name;

  /** Initialize the multicast engine */
  void (* init)(void);

  /**
   * \brief Process an outgoing datagram with a multicast IPv6 destination
   *        address
   *
   *        This may be needed if the multicast engine needs to, for example,
   *        add IPv6 extension headers to the datagram, cache it, decide it
   *        needs dropped etc.
   *
   *        It is sometimes desirable to let the engine handle datagram
   *        dispatch instead of letting the networking core do it. If the
   *        engine decides to send the datagram itself, it must afterwards
   *        set uip_len = 0 to prevent the networking core from sending too
   */
  void (* out)(void);

  /**
   * \brief Process an incoming multicast datagram and determine whether it
   *        should be delivered up the stack or not.
   *
   * \return 0: Drop, 1: Deliver
   *
   *
   *        When a datagram with a multicast destination address is received,
   *        the forwarding logic in core is bypassed. Instead, we let the
   *        multicast engine handle forwarding internally if and as necessary.
   *        This function is where forwarding logic must be hooked in.
   *
   *        Once the engine is done with forwarding, it must signal via the
   *        return value whether the datagram needs delivered up the network
   *        stack.
   */
  uint8_t (* in)(void);
};
/*---------------------------------------------------------------------------*/
/**
 * \brief Get a multicast address' scope.
 * a is of type uip_ip6addr_t*
 */
#define uip_mcast6_get_address_scope(a) ((a)->u8[1] & 0x0F)
/*---------------------------------------------------------------------------*/
/* Configure multicast and core/net to play nicely with the selected engine */
#if UIP_MCAST6_ENGINE

/* Enable Multicast hooks in the uip6 core */
#define UIP_CONF_IPV6_MULTICAST 1

#if UIP_MCAST6_ENGINE == UIP_MCAST6_ENGINE_ROLL_TM
#define RPL_CONF_MULTICAST     0        /* Not used by trickle */
#define UIP_CONF_IPV6_ROLL_TM  1        /* ROLL Trickle ICMP type support */

#define UIP_MCAST6             roll_tm_driver
#elif UIP_MCAST6_ENGINE == UIP_MCAST6_ENGINE_SMRF
#define RPL_CONF_MULTICAST     1

#define UIP_MCAST6             smrf_driver
#else
#error "Multicast Enabled with an Unknown Engine."
#error "Check the value of UIP_MCAST6_CONF_ENGINE in conf files."
#endif
#endif /* UIP_MCAST6_ENGINE */

extern const struct uip_mcast6_driver UIP_MCAST6;
/*---------------------------------------------------------------------------*/
/* Configuration Checks */
/*---------------------------------------------------------------------------*/
#if RPL_CONF_MULTICAST && (!UIP_CONF_IPV6_RPL)
#error "The selected Multicast mode requires UIP_CONF_IPV6_RPL != 0"
#error "Check the value of UIP_CONF_IPV6_RPL in conf files."
#endif
/*---------------------------------------------------------------------------*/
#endif /* UIP_MCAST6_H_ */
/*---------------------------------------------------------------------------*/
/** @} */
/** @} */
