/*
 * Copyright (c) 2001-2003, Adam Dunkels.
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
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * This file is part of the uIP TCP/IP stack.
 *
 *
 */

#ifndef UIPADDR_H_
#define UIPADDR_H_

/**
 * Representation of an IP address.
 *
 */
typedef union uip_ip4addr_t {
  uint8_t  u8[4];			/* Initializer, must come first. */
  uint16_t u16[2];
} uip_ip4addr_t;

typedef union uip_ip6addr_t {
  uint8_t  u8[16];			/* Initializer, must come first. */
  uint16_t u16[8];
} PACK_ALIAS_STRUCT uip_ip6addr_t;

#if NETSTACK_CONF_WITH_IPV6
typedef uip_ip6addr_t uip_ipaddr_t;
#else /* NETSTACK_CONF_WITH_IPV6 */
typedef uip_ip4addr_t uip_ipaddr_t;
#endif /* NETSTACK_CONF_WITH_IPV6 */

/*---------------------------------------------------------------------------*/

/** \brief 16 bit 802.15.4 address */
typedef struct uip_802154_shortaddr {
  uint8_t addr[2];
} uip_802154_shortaddr;
/** \brief 64 bit 802.15.4 address */
typedef struct uip_802154_longaddr {
  uint8_t addr[8];
} uip_802154_longaddr;

/** \brief 802.11 address */
typedef struct uip_80211_addr {
  uint8_t addr[6];
} uip_80211_addr;

/** \brief 802.3 address */
typedef struct uip_eth_addr {
  uint8_t addr[6];
} uip_eth_addr;


#if UIP_CONF_LL_802154
/** \brief 802.15.4 address */
typedef uip_802154_longaddr uip_lladdr_t;
#define UIP_802154_SHORTADDR_LEN 2
#define UIP_802154_LONGADDR_LEN  8
#define UIP_LLADDR_LEN UIP_802154_LONGADDR_LEN
#else /*UIP_CONF_LL_802154*/
#if UIP_CONF_LL_80211
/** \brief 802.11 address */
typedef uip_80211_addr uip_lladdr_t;
#define UIP_LLADDR_LEN 6
#else /*UIP_CONF_LL_80211*/
/** \brief Ethernet address */
typedef uip_eth_addr uip_lladdr_t;
#define UIP_LLADDR_LEN 6
#endif /*UIP_CONF_LL_80211*/
#endif /*UIP_CONF_LL_802154*/

#endif /* UIPADDR_H_ */
