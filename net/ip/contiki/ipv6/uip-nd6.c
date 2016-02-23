/*
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project.
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
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
/*
 * Copyright (c) 2006, Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *   may be used to endorse or promote products derived from this software
 *   without specific prior written permission.
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
 */

/**
 * \addtogroup uip6
 * @{
 */

/**
 * \file
 *    Neighbor discovery (RFC 4861)
 * \author Mathilde Durvy <mdurvy@cisco.com>
 * \author Julien Abeille <jabeille@cisco.com>
 */

#include <net/ip_buf.h>

#include <string.h>
#include "contiki/ipv6/uip-icmp6.h"
#include "contiki/ipv6/uip-nd6.h"
#include "contiki/ipv6/uip-ds6.h"
#include "contiki/ip/uip-nameserver.h"
#include "lib/random.h"

/*------------------------------------------------------------------*/
#ifdef CONFIG_NETWORK_IP_STACK_DEBUG_IPV6_ND
#define DEBUG 1
#endif
#include "contiki/ip/uip-debug.h"

#if UIP_LOGGING
#include <stdio.h>
void uip_log(char *msg);

#define UIP_LOG(m) uip_log(m)
#else
#define UIP_LOG(m)
#endif /* UIP_LOGGING == 1 */

/*------------------------------------------------------------------*/
/** @{ */
/** \name Pointers to the header structures.
 *  All pointers except UIP_IP_BUF depend on uip_ext_len, which at
 *  packet reception, is the total length of the extension headers.
 *  
 *  The pointer to ND6 options header also depends on nd6_opt_offset,
 *  which we set in each function.
 *
 *  Care should be taken when manipulating these buffers about the
 *  value of these length variables
 */

#define UIP_IP_BUF(buf)                ((struct uip_ip_hdr *)&uip_buf(buf)[UIP_LLH_LEN])  /**< Pointer to IP header */
#define UIP_ICMP_BUF(buf)            ((struct uip_icmp_hdr *)&uip_buf(buf)[uip_l2_l3_hdr_len(buf)])  /**< Pointer to ICMP header*/
/**@{  Pointers to messages just after icmp header */
#define UIP_ND6_RS_BUF(buf)            ((uip_nd6_rs *)&uip_buf(buf)[uip_l2_l3_icmp_hdr_len(buf)])
#define UIP_ND6_RA_BUF(buf)            ((uip_nd6_ra *)&uip_buf(buf)[uip_l2_l3_icmp_hdr_len(buf)])
#define UIP_ND6_NS_BUF(buf)            ((uip_nd6_ns *)&uip_buf(buf)[uip_l2_l3_icmp_hdr_len(buf)])
#define UIP_ND6_NA_BUF(buf)            ((uip_nd6_na *)&uip_buf(buf)[uip_l2_l3_icmp_hdr_len(buf)])
/** @} */
/** Pointer to ND option */
#define UIP_ND6_OPT_HDR_BUF(buf)  ((uip_nd6_opt_hdr *)&uip_buf(buf)[uip_l2_l3_icmp_hdr_len(buf) + uip_nd6_opt_offset(buf)])
#define UIP_ND6_OPT_PREFIX_BUF(buf) ((uip_nd6_opt_prefix_info *)&uip_buf(buf)[uip_l2_l3_icmp_hdr_len(buf) + uip_nd6_opt_offset(buf)])
#define UIP_ND6_OPT_MTU_BUF(buf) ((uip_nd6_opt_mtu *)&uip_buf(buf)[uip_l2_l3_icmp_hdr_len(buf) + uip_nd6_opt_offset(buf)])
#define UIP_ND6_OPT_RDNSS_BUF(buf) ((uip_nd6_opt_dns *)&uip_buf(buf)[uip_l2_l3_icmp_hdr_len(buf) + uip_nd6_opt_offset(buf)])
/** @} */

#if 0
/* The vars moved to net_buf */
static uint8_t nd6_opt_offset;                     /** Offset from the end of the icmpv6 header to the option in uip_buf*/
static uint8_t *nd6_opt_llao;   /**  Pointer to llao option in uip_buf */

#if !UIP_CONF_ROUTER            // TBD see if we move it to ra_input
static uip_nd6_opt_prefix_info *nd6_opt_prefix_info; /**  Pointer to prefix information option in uip_buf */
static uip_ipaddr_t ipaddr;
static uip_ds6_prefix_t *prefix; /**  Pointer to a prefix list entry */
#endif
static uip_ds6_nbr_t *nbr; /**  Pointer to a nbr cache entry*/
static uip_ds6_defrt_t *defrt; /**  Pointer to a router list entry */
static uip_ds6_addr_t *addr; /**  Pointer to an interface address */
#endif

/*------------------------------------------------------------------*/
/* create a llao */ 
static void
create_llao(uint8_t *llao, uint8_t type) {
  llao[UIP_ND6_OPT_TYPE_OFFSET] = type;
  llao[UIP_ND6_OPT_LEN_OFFSET] = UIP_ND6_OPT_LLAO_LEN >> 3;
  memcpy(&llao[UIP_ND6_OPT_DATA_OFFSET], &uip_lladdr, UIP_LLADDR_LEN);
  /* padding on some */
  memset(&llao[UIP_ND6_OPT_DATA_OFFSET + UIP_LLADDR_LEN], 0,
         UIP_ND6_OPT_LLAO_LEN - 2 - UIP_LLADDR_LEN);
}

/*------------------------------------------------------------------*/

#if UIP_ND6_SEND_NA
static void
ns_input(struct net_buf *buf)
{
  PRINTF("Received NS from ");
  PRINT6ADDR(&UIP_IP_BUF(buf)->srcipaddr);
  PRINTF(" to ");
  PRINT6ADDR(&UIP_IP_BUF(buf)->destipaddr);
  PRINTF(" with target address ");
  PRINT6ADDR((uip_ipaddr_t *) (&UIP_ND6_NS_BUF(buf)->tgtipaddr));
  PRINTF("\n");
  UIP_STAT(++uip_stat.nd6.recv);

#if UIP_CONF_IPV6_CHECKS
  if((UIP_IP_BUF(buf)->ttl != UIP_ND6_HOP_LIMIT) ||
     (uip_is_addr_mcast(&UIP_ND6_NS_BUF(buf)->tgtipaddr)) ||
     (UIP_ICMP_BUF(buf)->icode != 0)) {
    PRINTF("NS received is bad\n");
    goto discard;
  }
#endif /* UIP_CONF_IPV6_CHECKS */

  /* Options processing */
  uip_set_nd6_opt_llao(buf) = NULL;
  uip_nd6_opt_offset(buf) = UIP_ND6_NS_LEN;
  while(uip_l3_icmp_hdr_len(buf) + uip_nd6_opt_offset(buf) < uip_len(buf)) {
#if UIP_CONF_IPV6_CHECKS
    if(UIP_ND6_OPT_HDR_BUF(buf)->len == 0) {
      PRINTF("NS received is bad\n");
      goto discard;
    }
#endif /* UIP_CONF_IPV6_CHECKS */
    switch (UIP_ND6_OPT_HDR_BUF(buf)->type) {
    case UIP_ND6_OPT_SLLAO:
      uip_set_nd6_opt_llao(buf) = &uip_buf(buf)[uip_l2_l3_icmp_hdr_len(buf) + uip_nd6_opt_offset(buf)];
#if UIP_CONF_IPV6_CHECKS
      /* There must be NO option in a DAD NS */
      if(uip_is_addr_unspecified(&UIP_IP_BUF(buf)->srcipaddr)) {
        PRINTF("NS received is bad\n");
        goto discard;
      } else {
#endif /*UIP_CONF_IPV6_CHECKS */
        uip_set_nbr(buf) = uip_ds6_nbr_lookup(&UIP_IP_BUF(buf)->srcipaddr);
        if(uip_nbr(buf) == NULL) {
          uip_ds6_nbr_add(&UIP_IP_BUF(buf)->srcipaddr,
			  (uip_lladdr_t *)&uip_nd6_opt_llao(buf)[UIP_ND6_OPT_DATA_OFFSET],
			  0, NBR_STALE);
        } else {
          uip_lladdr_t *lladdr = (uip_lladdr_t *)uip_ds6_nbr_get_ll(uip_nbr(buf));
          if(memcmp(&uip_nd6_opt_llao(buf)[UIP_ND6_OPT_DATA_OFFSET],
		    lladdr, UIP_LLADDR_LEN) != 0) {
            memcpy(lladdr, &uip_nd6_opt_llao(buf)[UIP_ND6_OPT_DATA_OFFSET],
		   UIP_LLADDR_LEN);
            uip_nbr(buf)->state = NBR_STALE;
          } else {
            if(uip_nbr(buf)->state == NBR_INCOMPLETE) {
              uip_nbr(buf)->state = NBR_STALE;
            }
          }
        }
#if UIP_CONF_IPV6_CHECKS
      }
#endif /*UIP_CONF_IPV6_CHECKS */
      break;
    default:
      PRINTF("ND option (0x%x) not supported in NS\n",
	     UIP_ND6_OPT_HDR_BUF(buf)->type);
      break;
    }
    uip_nd6_opt_offset(buf) += (UIP_ND6_OPT_HDR_BUF(buf)->len << 3);
  }

  uip_set_addr(buf) = uip_ds6_addr_lookup(&UIP_ND6_NS_BUF(buf)->tgtipaddr);
  if(uip_addr(buf) != NULL) {
#if UIP_ND6_DEF_MAXDADNS > 0
    if(uip_is_addr_unspecified(&UIP_IP_BUF(buf)->srcipaddr)) {
      /* DAD CASE */
#if UIP_CONF_IPV6_CHECKS
      if(!uip_is_addr_solicited_node(&UIP_IP_BUF(buf)->destipaddr)) {
        PRINTF("NS received is bad\n");
        goto discard;
      }
#endif /* UIP_CONF_IPV6_CHECKS */
      if(uip_addr(buf)->state != ADDR_TENTATIVE) {
        uip_create_linklocal_allnodes_mcast(&UIP_IP_BUF(buf)->destipaddr);
        uip_ds6_select_src(&UIP_IP_BUF(buf)->srcipaddr, &UIP_IP_BUF(buf)->destipaddr);
        uip_flags(buf) = UIP_ND6_NA_FLAG_OVERRIDE;
        goto create_na;
      } else {
          /** \todo if I sent a NS before him, I win */
        uip_ds6_dad_failed(uip_addr(buf));
        goto discard;
      }
#else /* UIP_ND6_DEF_MAXDADNS > 0 */
    if(uip_is_addr_unspecified(&UIP_IP_BUF(buf)->srcipaddr)) {
      /* DAD CASE */
      goto discard;
#endif /* UIP_ND6_DEF_MAXDADNS > 0 */
    }
#if UIP_CONF_IPV6_CHECKS
    if(uip_ds6_is_my_addr(&UIP_IP_BUF(buf)->srcipaddr)) {
        /**
         * \NOTE do we do something here? we both are using the same address.
         * If we are doing dad, we could cancel it, though we should receive a
         * NA in response of DAD NS we sent, hence DAD will fail anyway. If we
         * were not doing DAD, it means there is a duplicate in the network!
         */
      PRINTF("NS received is bad\n");
      goto discard;
    }
#endif /*UIP_CONF_IPV6_CHECKS */

    /* Address resolution case */
    if(uip_is_addr_solicited_node(&UIP_IP_BUF(buf)->destipaddr)) {
      uip_ipaddr_copy(&UIP_IP_BUF(buf)->destipaddr, &UIP_IP_BUF(buf)->srcipaddr);
      uip_ipaddr_copy(&UIP_IP_BUF(buf)->srcipaddr, &UIP_ND6_NS_BUF(buf)->tgtipaddr);
      uip_flags(buf) = UIP_ND6_NA_FLAG_SOLICITED | UIP_ND6_NA_FLAG_OVERRIDE;
      goto create_na;
    }

    /* NUD CASE */
    if(uip_ds6_addr_lookup(&UIP_IP_BUF(buf)->destipaddr) == uip_addr(buf)) {
      uip_ipaddr_copy(&UIP_IP_BUF(buf)->destipaddr, &UIP_IP_BUF(buf)->srcipaddr);
      uip_ipaddr_copy(&UIP_IP_BUF(buf)->srcipaddr, &UIP_ND6_NS_BUF(buf)->tgtipaddr);
      uip_flags(buf) = UIP_ND6_NA_FLAG_SOLICITED | UIP_ND6_NA_FLAG_OVERRIDE;
      goto create_na;
    } else {
#if UIP_CONF_IPV6_CHECKS
      PRINTF("NS received is bad\n");
      goto discard;
#endif /* UIP_CONF_IPV6_CHECKS */
    }
  } else {
    goto discard;
  }


create_na:
    /* If the node is a router it should set R flag in NAs */
#if UIP_CONF_ROUTER
    uip_flags(buf) = uip_flags(buf) | UIP_ND6_NA_FLAG_ROUTER;
#endif
  uip_ext_len(buf) = 0;
  UIP_IP_BUF(buf)->vtc = 0x60;
  UIP_IP_BUF(buf)->tcflow = 0;
  UIP_IP_BUF(buf)->flow = 0;
  UIP_IP_BUF(buf)->len[0] = 0;       /* length will not be more than 255 */
  UIP_IP_BUF(buf)->len[1] = UIP_ICMPH_LEN + UIP_ND6_NA_LEN + UIP_ND6_OPT_LLAO_LEN;
  UIP_IP_BUF(buf)->proto = UIP_PROTO_ICMP6;
  UIP_IP_BUF(buf)->ttl = UIP_ND6_HOP_LIMIT;

  UIP_ICMP_BUF(buf)->type = ICMP6_NA;
  UIP_ICMP_BUF(buf)->icode = 0;

  UIP_ND6_NA_BUF(buf)->flagsreserved = uip_flags(buf);
  memcpy(&UIP_ND6_NA_BUF(buf)->tgtipaddr, &uip_addr(buf)->ipaddr, sizeof(uip_ipaddr_t));

  create_llao(&uip_buf(buf)[uip_l2_l3_icmp_hdr_len(buf) + UIP_ND6_NA_LEN],
              UIP_ND6_OPT_TLLAO);

  UIP_ICMP_BUF(buf)->icmpchksum = 0;
  UIP_ICMP_BUF(buf)->icmpchksum = ~uip_icmp6chksum(buf);

  uip_len(buf) = buf->len =
    UIP_IPH_LEN + UIP_ICMPH_LEN + UIP_ND6_NA_LEN + UIP_ND6_OPT_LLAO_LEN;

  UIP_STAT(++uip_stat.nd6.sent);
  PRINTF("Sending NA to ");
  PRINT6ADDR(&UIP_IP_BUF(buf)->destipaddr);
  PRINTF(" from ");
  PRINT6ADDR(&UIP_IP_BUF(buf)->srcipaddr);
  PRINTF(" with target address ");
  PRINT6ADDR(&UIP_ND6_NA_BUF(buf)->tgtipaddr);
  PRINTF("\n");
  return;

discard:
  uip_len(buf) = 0;
  uip_ext_len(buf) = 0;
  return;
}
#endif /* UIP_ND6_SEND_NA */



/*------------------------------------------------------------------*/
void
uip_nd6_ns_output(struct net_buf *buf, uip_ipaddr_t * src, uip_ipaddr_t * dest, uip_ipaddr_t * tgt)
{
  bool send_from_here = true;

  if (!buf) {
    buf = ip_buf_get_reserve_tx(UIP_IPICMPH_LEN);
    if (!buf) {
      PRINTF("%s(): Cannot send NS, no net buffers\n", __FUNCTION__);
      return;
    }
    send_from_here = true;
  }
  uip_ext_len(buf) = 0;
  UIP_IP_BUF(buf)->vtc = 0x60;
  UIP_IP_BUF(buf)->tcflow = 0;
  UIP_IP_BUF(buf)->flow = 0;
  UIP_IP_BUF(buf)->proto = UIP_PROTO_ICMP6;
  UIP_IP_BUF(buf)->ttl = UIP_ND6_HOP_LIMIT;

  if(dest == NULL) {
    uip_create_solicited_node(tgt, &UIP_IP_BUF(buf)->destipaddr);
  } else {
    uip_ipaddr_copy(&UIP_IP_BUF(buf)->destipaddr, dest);
  }
  UIP_ICMP_BUF(buf)->type = ICMP6_NS;
  UIP_ICMP_BUF(buf)->icode = 0;
  UIP_ND6_NS_BUF(buf)->reserved = 0;
  uip_ipaddr_copy((uip_ipaddr_t *) &UIP_ND6_NS_BUF(buf)->tgtipaddr, tgt);
  UIP_IP_BUF(buf)->len[0] = 0;       /* length will not be more than 255 */
  /*
   * check if we add a SLLAO option: for DAD, MUST NOT, for NUD, MAY
   * (here yes), for Address resolution , MUST 
   */
  if(!(uip_ds6_is_my_addr(tgt))) {
    if(src != NULL) {
      uip_ipaddr_copy(&UIP_IP_BUF(buf)->srcipaddr, src);
    } else {
      uip_ds6_select_src(&UIP_IP_BUF(buf)->srcipaddr, &UIP_IP_BUF(buf)->destipaddr);
    }
    if (uip_is_addr_unspecified(&UIP_IP_BUF(buf)->srcipaddr)) {
      PRINTF("Dropping NS due to no suitable source address\n");
      uip_len(buf) = 0;
      if (send_from_here) {
        ip_buf_unref(buf);
      }
      return;
    }
    UIP_IP_BUF(buf)->len[1] =
      UIP_ICMPH_LEN + UIP_ND6_NS_LEN + UIP_ND6_OPT_LLAO_LEN;

    create_llao(&uip_buf(buf)[uip_l2_l3_icmp_hdr_len(buf) + UIP_ND6_NS_LEN],
		UIP_ND6_OPT_SLLAO);

    uip_len(buf) =
      UIP_IPH_LEN + UIP_ICMPH_LEN + UIP_ND6_NS_LEN + UIP_ND6_OPT_LLAO_LEN;
    net_buf_add(buf, UIP_ND6_NS_LEN + UIP_ND6_OPT_LLAO_LEN);
  } else {
    uip_create_unspecified(&UIP_IP_BUF(buf)->srcipaddr);
    UIP_IP_BUF(buf)->len[1] = UIP_ICMPH_LEN + UIP_ND6_NS_LEN;
    uip_len(buf) = UIP_IPH_LEN + UIP_ICMPH_LEN + UIP_ND6_NS_LEN;
    net_buf_add(buf, UIP_ND6_NS_LEN);
  }

  UIP_ICMP_BUF(buf)->icmpchksum = 0;
  UIP_ICMP_BUF(buf)->icmpchksum = ~uip_icmp6chksum(buf);

  UIP_STAT(++uip_stat.nd6.sent);
  PRINTF("Sending NS to ");
  PRINT6ADDR(&UIP_IP_BUF(buf)->destipaddr);
  PRINTF(" from ");
  PRINT6ADDR(&UIP_IP_BUF(buf)->srcipaddr);
  PRINTF(" with target address ");
  PRINT6ADDR(tgt);
  PRINTF("\n");

  if (send_from_here) {
    if (tcpip_ipv6_output(buf) == 0) {
      ip_buf_unref(buf);
    }
  }
  return;
}
/*------------------------------------------------------------------*/
/**
 * Neighbor Advertisement Processing
 *
 * we might have to send a pkt that had been buffered while address
 * resolution was performed (if we support buffering, see UIP_CONF_QUEUE_PKT)
 *
 * As per RFC 4861, on link layer that have addresses, TLLAO options MUST be
 * included when responding to multicast solicitations, SHOULD be included in
 * response to unicast (here we assume it is for now)
 *
 * NA can be received after sending NS for DAD, Address resolution or NUD. Can
 * be unsolicited as well.
 * It can trigger update of the state of the neighbor in the neighbor cache,
 * router in the router list.
 * If the NS was for DAD, it means DAD failed
 *
 */
#if UIP_ND6_SEND_NA
static void
na_input(struct net_buf *buf)
{
  uint8_t is_llchange;
  uint8_t is_router;
  uint8_t is_solicited;
  uint8_t is_override;

  PRINTF("Received NA from ");
  PRINT6ADDR(&UIP_IP_BUF(buf)->srcipaddr);
  PRINTF(" to ");
  PRINT6ADDR(&UIP_IP_BUF(buf)->destipaddr);
  PRINTF(" with target address ");
  PRINT6ADDR((uip_ipaddr_t *) (&UIP_ND6_NA_BUF(buf)->tgtipaddr));
  PRINTF("\n");
  UIP_STAT(++uip_stat.nd6.recv);

  /* 
   * booleans. the three last one are not 0 or 1 but 0 or 0x80, 0x40, 0x20
   * but it works. Be careful though, do not use tests such as is_router == 1 
   */
  is_llchange = 0;
  is_router = ((UIP_ND6_NA_BUF(buf)->flagsreserved & UIP_ND6_NA_FLAG_ROUTER));
  is_solicited =
    ((UIP_ND6_NA_BUF(buf)->flagsreserved & UIP_ND6_NA_FLAG_SOLICITED));
  is_override =
    ((UIP_ND6_NA_BUF(buf)->flagsreserved & UIP_ND6_NA_FLAG_OVERRIDE));

#if UIP_CONF_IPV6_CHECKS
  if((UIP_IP_BUF(buf)->ttl != UIP_ND6_HOP_LIMIT) ||
     (UIP_ICMP_BUF(buf)->icode != 0) ||
     (uip_is_addr_mcast(&UIP_ND6_NA_BUF(buf)->tgtipaddr)) ||
     (is_solicited && uip_is_addr_mcast(&UIP_IP_BUF(buf)->destipaddr))) {
    PRINTF("NA received is bad\n");
    goto discard;
  }
#endif /*UIP_CONF_IPV6_CHECKS */

  /* Options processing: we handle TLLAO, and must ignore others */
  uip_nd6_opt_offset(buf) = UIP_ND6_NA_LEN;
  uip_set_nd6_opt_llao(buf) = NULL;
  while(uip_l3_icmp_hdr_len(buf) + uip_nd6_opt_offset(buf) < uip_len(buf)) {
#if UIP_CONF_IPV6_CHECKS
    if(UIP_ND6_OPT_HDR_BUF(buf)->len == 0) {
      PRINTF("NA received is bad\n");
      goto discard;
    }
#endif /*UIP_CONF_IPV6_CHECKS */
    switch (UIP_ND6_OPT_HDR_BUF(buf)->type) {
    case UIP_ND6_OPT_TLLAO:
      uip_set_nd6_opt_llao(buf) = (uint8_t *)UIP_ND6_OPT_HDR_BUF(buf);
      break;
    default:
      PRINTF("ND option (0x%x) not supported in NA\n",
	     UIP_ND6_OPT_HDR_BUF(buf)->type);
      break;
    }
    uip_nd6_opt_offset(buf) += (UIP_ND6_OPT_HDR_BUF(buf)->len << 3);
  }
  uip_set_addr(buf) = uip_ds6_addr_lookup(&UIP_ND6_NA_BUF(buf)->tgtipaddr);
  /* Message processing, including TLLAO if any */
  if(uip_addr(buf) != NULL) {
#if UIP_ND6_DEF_MAXDADNS > 0
    if(uip_addr(buf)->state == ADDR_TENTATIVE) {
      uip_ds6_dad_failed(uip_addr(buf));
    }
#endif /*UIP_ND6_DEF_MAXDADNS > 0 */
    PRINTF("NA received is bad\n");
    goto discard;
  } else {
    uip_lladdr_t *lladdr;
    uip_set_nbr(buf) = uip_ds6_nbr_lookup(&UIP_ND6_NA_BUF(buf)->tgtipaddr);
    lladdr = (uip_lladdr_t *)uip_ds6_nbr_get_ll(uip_nbr(buf));
    if(uip_nbr(buf) == NULL) {
      goto discard;
    }
    if(uip_nd6_opt_llao(buf) != 0) {
      is_llchange =
        memcmp(&uip_nd6_opt_llao(buf)[UIP_ND6_OPT_DATA_OFFSET], (void *)lladdr,
               UIP_LLADDR_LEN);
    }
    if(uip_nbr(buf)->state == NBR_INCOMPLETE) {
      if(uip_nd6_opt_llao(buf) == NULL) {
        goto discard;
      }
      memcpy(lladdr, &uip_nd6_opt_llao(buf)[UIP_ND6_OPT_DATA_OFFSET],
	     UIP_LLADDR_LEN);
      if(is_solicited) {
        uip_nbr(buf)->state = NBR_REACHABLE;
        uip_nbr(buf)->nscount = 0;

        /* reachable time is stored in ms */
        stimer_set(&(uip_nbr(buf)->reachable), uip_ds6_if.reachable_time / 1000);

      } else {
        uip_nbr(buf)->state = NBR_STALE;
      }
      uip_nbr(buf)->isrouter = is_router;
    } else {
      if(!is_override && is_llchange) {
        if(uip_nbr(buf)->state == NBR_REACHABLE) {
          uip_nbr(buf)->state = NBR_STALE;
        }
        goto discard;
      } else {
        if(is_override || (!is_override && uip_nd6_opt_llao(buf) != 0 && !is_llchange)
           || uip_nd6_opt_llao(buf) == 0) {
          if(uip_nd6_opt_llao(buf) != 0) {
            memcpy(lladdr, &uip_nd6_opt_llao(buf)[UIP_ND6_OPT_DATA_OFFSET],
		   UIP_LLADDR_LEN);
          }
          if(is_solicited) {
            uip_nbr(buf)->state = NBR_REACHABLE;
            /* reachable time is stored in ms */
            stimer_set(&(uip_nbr(buf)->reachable), uip_ds6_if.reachable_time / 1000);
          } else {
            if(uip_nd6_opt_llao(buf) != 0 && is_llchange) {
              uip_nbr(buf)->state = NBR_STALE;
            }
          }
        }
      }
      if(uip_nbr(buf)->isrouter && !is_router) {
        uip_set_defrt(buf) = uip_ds6_defrt_lookup(&UIP_IP_BUF(buf)->srcipaddr);
        if(uip_defrt(buf) != NULL) {
          uip_ds6_defrt_rm(uip_defrt(buf));
        }
      }
      uip_nbr(buf)->isrouter = is_router;
    }
  }
#if UIP_CONF_IPV6_QUEUE_PKT
  /* The nbr is now reachable, check if we had buffered a pkt for it */
  /*if(nbr->queue_buf_len != 0) {
    uip_len = nbr->queue_buf_len;
    memcpy(UIP_IP_BUF, nbr->queue_buf, uip_len);
    nbr->queue_buf_len = 0;
    return;
    }*/
  if(uip_packetqueue_buflen(&uip_nbr(buf)->packethandle) != 0) {
    uip_len(buf) = buf->len = uip_packetqueue_buflen(&uip_nbr(buf)->packethandle);
    memcpy(UIP_IP_BUF(buf), uip_packetqueue_buf(&uip_nbr(buf)->packethandle), uip_len(buf));
    uip_packetqueue_free(&uip_nbr(buf)->packethandle);
    return;
  }
  
#endif /*UIP_CONF_IPV6_QUEUE_PKT */

discard:
  uip_len(buf) = 0;
  uip_ext_len(buf) = 0;
  return;
}
#endif /* UIP_ND6_SEND_NA */


#if UIP_CONF_ROUTER
#if UIP_ND6_SEND_RA
/*---------------------------------------------------------------------------*/
static void
rs_input(void)
{

  PRINTF("Received RS from ");
  PRINT6ADDR(&UIP_IP_BUF(buf)->srcipaddr);
  PRINTF(" to ");
  PRINT6ADDR(&UIP_IP_BUF(buf)->destipaddr);
  PRINTF("\n");
  UIP_STAT(++uip_stat.nd6.recv);


#if UIP_CONF_IPV6_CHECKS
  /*
   * Check hop limit / icmp code 
   * target address must not be multicast
   * if the NA is solicited, dest must not be multicast
   */
  if((UIP_IP_BUF(buf)->ttl != UIP_ND6_HOP_LIMIT) || (UIP_ICMP_BUF->icode != 0)) {
    PRINTF("RS received is bad\n");
    goto discard;
  }
#endif /*UIP_CONF_IPV6_CHECKS */

  /* Only valid option is Source Link-Layer Address option any thing
     else is discarded */
  uip_nd6_opt_offset(buf) = UIP_ND6_RS_LEN;
  uip_set_nd6_opt_llao = NULL;

  while(uip_l3_icmp_hdr_len + uip_nd6_opt_offset(buf) < uip_len) {
#if UIP_CONF_IPV6_CHECKS
    if(UIP_ND6_OPT_HDR_BUF->len == 0) {
      PRINTF("RS received is bad\n");
      goto discard;
    }
#endif /*UIP_CONF_IPV6_CHECKS */
    switch (UIP_ND6_OPT_HDR_BUF->type) {
    case UIP_ND6_OPT_SLLAO:
      uip_set_nd6_opt_llao = (uint8_t *)UIP_ND6_OPT_HDR_BUF;
      break;
    default:
      PRINTF("ND option (0x%x) not supported in RS\n",
	     UIP_ND6_OPT_HDR_BUF(buf)->type);
      break;
    }
    uip_nd6_opt_offset(buf) += (UIP_ND6_OPT_HDR_BUF->len << 3);
  }
  /* Options processing: only SLLAO */
  if(nd6_opt_llao != NULL) {
#if UIP_CONF_IPV6_CHECKS
    if(uip_is_addr_unspecified(&UIP_IP_BUF(buf)->srcipaddr)) {
      PRINTF("RS received is bad\n");
      goto discard;
    } else {
#endif /*UIP_CONF_IPV6_CHECKS */
      if((uip_set_nbr(buf) = uip_ds6_nbr_lookup(&UIP_IP_BUF(buf)->srcipaddr)) == NULL) {
        /* we need to add the neighbor */
        uip_ds6_nbr_add(&UIP_IP_BUF(buf)->srcipaddr,
                        (uip_lladdr_t *)&nd6_opt_llao[UIP_ND6_OPT_DATA_OFFSET], 0, NBR_STALE);
      } else {
        /* If LL address changed, set neighbor state to stale */
        if(memcmp(&nd6_opt_llao[UIP_ND6_OPT_DATA_OFFSET],
            uip_ds6_nbr_get_ll(uip_nbr(buf)), UIP_LLADDR_LEN) != 0) {
          uip_ds6_nbr_t nbr_data = *(uip_ds6_nbr_t *)uip_nbr(buf);
          uip_ds6_nbr_rm(uip_nbr(buf));
          uip_set_nbr(buf) = uip_ds6_nbr_add(&UIP_IP_BUF(buf)->srcipaddr,
                                (uip_lladdr_t *)&nd6_opt_llao[UIP_ND6_OPT_DATA_OFFSET], 0, NBR_STALE);
          uip_nbr(buf)->reachable = nbr_data.reachable;
          uip_nbr(buf)->sendns = nbr_data.sendns;
          uip_nbr(buf)->nscount = nbr_data.nscount;
        }
        uip_nbr(buf)->isrouter = 0;
      }
#if UIP_CONF_IPV6_CHECKS
    }
#endif /*UIP_CONF_IPV6_CHECKS */
  }

  /* Schedule a sollicited RA */
  uip_ds6_send_ra_sollicited();

discard:
  uip_len = 0;
  uip_ext_len = 0;
  return;
}

/*---------------------------------------------------------------------------*/
void
uip_nd6_ra_output(uip_ipaddr_t * dest)
{
  bool send_from_here = true;

  if (!buf) {
    buf = ip_buf_get_reserve_tx(UIP_IPICMPH_LEN);
    if (!buf) {
      PRINTF("%s(): Cannot send RA, no net buffers\n", __FUNCTION__);
      return;
    }

    send_from_here = true;
  }

  UIP_IP_BUF(buf)->vtc = 0x60;
  UIP_IP_BUF(buf)->tcflow = 0;
  UIP_IP_BUF(buf)->flow = 0;
  UIP_IP_BUF(buf)->proto = UIP_PROTO_ICMP6;
  UIP_IP_BUF(buf)->ttl = UIP_ND6_HOP_LIMIT;

  if(dest == NULL) {
    uip_create_linklocal_allnodes_mcast(&UIP_IP_BUF(buf)->destipaddr);
  } else {
    /* For sollicited RA */
    uip_ipaddr_copy(&UIP_IP_BUF(buf)->destipaddr, dest);
  }
  uip_ds6_select_src(&UIP_IP_BUF(buf)->srcipaddr, &UIP_IP_BUF(buf)->destipaddr);

  UIP_ICMP_BUF->type = ICMP6_RA;
  UIP_ICMP_BUF->icode = 0;

  UIP_ND6_RA_BUF->cur_ttl = uip_ds6_if.cur_hop_limit;

  UIP_ND6_RA_BUF->flags_reserved =
    (UIP_ND6_M_FLAG << 7) | (UIP_ND6_O_FLAG << 6);

  UIP_ND6_RA_BUF->router_lifetime = uip_htons(UIP_ND6_ROUTER_LIFETIME);
  //UIP_ND6_RA_BUF->reachable_time = uip_htonl(uip_ds6_if.reachable_time);
  //UIP_ND6_RA_BUF->retrans_timer = uip_htonl(uip_ds6_if.retrans_timer);
  UIP_ND6_RA_BUF->reachable_time = 0;
  UIP_ND6_RA_BUF->retrans_timer = 0;

  uip_len = UIP_IPH_LEN + UIP_ICMPH_LEN + UIP_ND6_RA_LEN;
  uip_nd6_opt_offset(buf) = UIP_ND6_RA_LEN;
  net_buf_add(buf, UIP_ND6_RA_LEN);


#if !UIP_CONF_ROUTER
  /* Prefix list */
  for(uip_prefix(buf) = uip_ds6_prefix_list;
      uip_prefix(buf) < uip_ds6_prefix_list + UIP_DS6_PREFIX_NB; uip_prefix(buf)++) {
    if((uip_prefix(buf)->isused) && (uip_prefix(buf)->advertise)) {
      UIP_ND6_OPT_PREFIX_BUF(buf)->type = UIP_ND6_OPT_PREFIX_INFO;
      UIP_ND6_OPT_PREFIX_BUF(buf)->len = UIP_ND6_OPT_PREFIX_INFO_LEN / 8;
      UIP_ND6_OPT_PREFIX_BUF(buf)->preflen = uip_prefix(buf)->length;
      UIP_ND6_OPT_PREFIX_BUF(buf)->flagsreserved1 = uip_prefix(buf)->l_a_reserved;
      UIP_ND6_OPT_PREFIX_BUF(buf)->validlt = uip_htonl(uip_prefix(buf)->vlifetime);
      UIP_ND6_OPT_PREFIX_BUF(buf)->preferredlt = uip_htonl(uip_prefix(buf)->plifetime);
      UIP_ND6_OPT_PREFIX_BUF(buf)->reserved2 = 0;
      uip_ipaddr_copy(&(UIP_ND6_OPT_PREFIX_BUF(buf)->prefix), &(uip_prefix(buf)->ipaddr));
      uip_nd6_opt_offset(buf) += UIP_ND6_OPT_PREFIX_INFO_LEN;
      uip_len(buf) += UIP_ND6_OPT_PREFIX_INFO_LEN;
      net_buf_add(buf, UIP_ND6_OPT_PREFIX_INFO_LEN);
    }
  }
#endif /* !UIP_CONF_ROUTER */

  /* Source link-layer option */
  create_llao((uint8_t *)UIP_ND6_OPT_HDR_BUF(buf), UIP_ND6_OPT_SLLAO);

  uip_len(buf) += UIP_ND6_OPT_LLAO_LEN;
  uip_nd6_opt_offset(buf) += UIP_ND6_OPT_LLAO_LEN;
  net_buf_add(buf, UIP_ND6_OPT_LLAO_LEN);

  /* MTU */
  UIP_ND6_OPT_MTU_BUF(buf)->type = UIP_ND6_OPT_MTU;
  UIP_ND6_OPT_MTU_BUF(buf)->len = UIP_ND6_OPT_MTU_LEN >> 3;
  UIP_ND6_OPT_MTU_BUF(buf)->reserved = 0;
  //UIP_ND6_OPT_MTU_BUF(buf)->mtu = uip_htonl(uip_ds6_if.link_mtu);
  UIP_ND6_OPT_MTU_BUF(buf)->mtu = uip_htonl(1500);

  uip_len(buf) += UIP_ND6_OPT_MTU_LEN;
  uip_nd6_opt_offset(buf) += UIP_ND6_OPT_MTU_LEN;
  net_buf_add(buf, UIP_ND6_OPT_MTU_LEN);

#if UIP_ND6_RA_RDNSS
  if(uip_nameserver_count() > 0) {
    uint8_t i = 0;
    uip_ipaddr_t *ip = &UIP_ND6_OPT_RDNSS_BUF(buf)->ip;
    uip_ipaddr_t *dns = NULL;
    UIP_ND6_OPT_RDNSS_BUF(buf)->type = UIP_ND6_OPT_RDNSS;
    UIP_ND6_OPT_RDNSS_BUF(buf)->reserved = 0;
    UIP_ND6_OPT_RDNSS_BUF(buf)->lifetime = uip_nameserver_next_expiration();
    if(UIP_ND6_OPT_RDNSS_BUF(buf)->lifetime != UIP_NAMESERVER_INFINITE_LIFETIME) {
      UIP_ND6_OPT_RDNSS_BUF(buf)->lifetime -= clock_seconds();
    }
    while((dns = uip_nameserver_get(i)) != NULL) {
      uip_ipaddr_copy(ip++, dns);
      i++;
    }
    UIP_ND6_OPT_RDNSS_BUF(buf)->len = UIP_ND6_OPT_RDNSS_LEN + (i << 1);
    PRINTF("%d nameservers reported\n", i);
    uip_len(buf) += UIP_ND6_OPT_RDNSS_BUF(buf)->len << 3;
    uip_nd6_opt_offset(buf) += UIP_ND6_OPT_RDNSS_BUF(buf)->len << 3;
    net_buf_add(buf, UIP_ND6_OPT_RDNSS_BUF(buf)->len << 3);
  }
#endif /* UIP_ND6_RA_RDNSS */

  UIP_IP_BUF(buf)->len[0] = ((uip_len(buf) - UIP_IPH_LEN) >> 8);
  UIP_IP_BUF(buf)->len[1] = ((uip_len(buf) - UIP_IPH_LEN) & 0xff);

  /*ICMP checksum */
  UIP_ICMP_BUF->icmpchksum = 0;
  UIP_ICMP_BUF->icmpchksum = ~uip_icmp6chksum(buf);

  UIP_STAT(++uip_stat.nd6.sent);
  PRINTF("Sending RA to ");
  PRINT6ADDR(&UIP_IP_BUF(buf)->destipaddr);
  PRINTF(" from ");
  PRINT6ADDR(&UIP_IP_BUF(buf)->srcipaddr);
  PRINTF("\n");

  if (send_from_here) {
    if (tcpip_ipv6_output(buf) == 0) {
      ip_buf_unref(buf);
    }
  }
  return;
}
#endif /* UIP_ND6_SEND_RA */
#endif /* UIP_CONF_ROUTER */

#if !UIP_CONF_ROUTER
/*---------------------------------------------------------------------------*/
void
uip_nd6_rs_output(struct net_buf *buf)
{
  bool send_from_here = false;

  if (!buf) {
    buf = ip_buf_get_reserve_tx(UIP_IPICMPH_LEN);
    if (!buf) {
      PRINTF("%s(): Cannot send RS, no net buffers\n", __FUNCTION__);
      return;
    }
    send_from_here = true;
  }
  UIP_IP_BUF(buf)->vtc = 0x60;
  UIP_IP_BUF(buf)->tcflow = 0;
  UIP_IP_BUF(buf)->flow = 0;
  UIP_IP_BUF(buf)->proto = UIP_PROTO_ICMP6;
  UIP_IP_BUF(buf)->ttl = UIP_ND6_HOP_LIMIT;
  uip_create_linklocal_allrouters_mcast(&UIP_IP_BUF(buf)->destipaddr);
  uip_ds6_select_src(&UIP_IP_BUF(buf)->srcipaddr, &UIP_IP_BUF(buf)->destipaddr);
  UIP_ICMP_BUF(buf)->type = ICMP6_RS;
  UIP_ICMP_BUF(buf)->icode = 0;
  UIP_IP_BUF(buf)->len[0] = 0;       /* length will not be more than 255 */

  if(uip_is_addr_unspecified(&UIP_IP_BUF(buf)->srcipaddr)) {
    UIP_IP_BUF(buf)->len[1] = UIP_ICMPH_LEN + UIP_ND6_RS_LEN;
    uip_len(buf) = uip_l3_icmp_hdr_len(buf) + UIP_ND6_RS_LEN;
    memset(&uip_buf(buf)[uip_l2_l3_icmp_hdr_len(buf)], UIP_ND6_RS_LEN, 0);
    net_buf_add(buf, UIP_ND6_RS_LEN);
  } else {
    uip_len(buf) = uip_l3_icmp_hdr_len(buf) + UIP_ND6_RS_LEN + UIP_ND6_OPT_LLAO_LEN;
    UIP_IP_BUF(buf)->len[1] =
      UIP_ICMPH_LEN + UIP_ND6_RS_LEN + UIP_ND6_OPT_LLAO_LEN;
    net_buf_add(buf, UIP_ND6_RS_LEN + UIP_ND6_OPT_LLAO_LEN);

    create_llao(&uip_buf(buf)[uip_l2_l3_icmp_hdr_len(buf) + UIP_ND6_RS_LEN],
		UIP_ND6_OPT_SLLAO);
  }

  UIP_ICMP_BUF(buf)->icmpchksum = 0;
  UIP_ICMP_BUF(buf)->icmpchksum = ~uip_icmp6chksum(buf);

  UIP_STAT(++uip_stat.nd6.sent);
  PRINTF("Sending RS to ");
  PRINT6ADDR(&UIP_IP_BUF(buf)->destipaddr);
  PRINTF(" from ");
  PRINT6ADDR(&UIP_IP_BUF(buf)->srcipaddr);
  PRINTF("\n");

  if (send_from_here) {
    if (tcpip_ipv6_output(buf) == 0) {
      ip_buf_unref(buf);
    }
  }
  return;
}
/*---------------------------------------------------------------------------*/
/*
 * Process a Router Advertisement
 *
 * - Possible actions when receiving a RA: add router to router list,
 *   recalculate reachable time, update link hop limit, update retrans timer.
 * - If MTU option: update MTU.
 * - If SLLAO option: update entry in neighbor cache
 * - If prefix option: start autoconf, add prefix to prefix list
 */
void
ra_input(struct net_buf *buf)
{
  PRINTF("Received RA from ");
  PRINT6ADDR(&UIP_IP_BUF(buf)->srcipaddr);
  PRINTF(" to ");
  PRINT6ADDR(&UIP_IP_BUF(buf)->destipaddr);
  PRINTF("\n");
  UIP_STAT(++uip_stat.nd6.recv);

#if UIP_CONF_IPV6_CHECKS
  if((UIP_IP_BUF(buf)->ttl != UIP_ND6_HOP_LIMIT) ||
     (!uip_is_addr_link_local(&UIP_IP_BUF(buf)->srcipaddr)) ||
     (UIP_ICMP_BUF(buf)->icode != 0)) {
    PRINTF("RA received is bad\n");
    goto discard;
  }
#endif /*UIP_CONF_IPV6_CHECKS */

  if(UIP_ND6_RA_BUF(buf)->cur_ttl != 0) {
    uip_ds6_if.cur_hop_limit = UIP_ND6_RA_BUF(buf)->cur_ttl;
    PRINTF("uip_ds6_if.cur_hop_limit %u\n", uip_ds6_if.cur_hop_limit);
  }

  if(UIP_ND6_RA_BUF(buf)->reachable_time != 0) {
    if(uip_ds6_if.base_reachable_time !=
       uip_ntohl(UIP_ND6_RA_BUF(buf)->reachable_time)) {
      uip_ds6_if.base_reachable_time = uip_ntohl(UIP_ND6_RA_BUF(buf)->reachable_time);
      uip_ds6_if.reachable_time = uip_ds6_compute_reachable_time();
    }
  }
  if(UIP_ND6_RA_BUF(buf)->retrans_timer != 0) {
    uip_ds6_if.retrans_timer = uip_ntohl(UIP_ND6_RA_BUF(buf)->retrans_timer);
  }

  /* Options processing */
  uip_nd6_opt_offset(buf) = UIP_ND6_RA_LEN;
  while(uip_l3_icmp_hdr_len(buf) + uip_nd6_opt_offset(buf) < uip_len(buf)) {
    if(UIP_ND6_OPT_HDR_BUF(buf)->len == 0) {
      PRINTF("RA received is bad\n");
      goto discard;
    }
    switch (UIP_ND6_OPT_HDR_BUF(buf)->type) {
    case UIP_ND6_OPT_SLLAO:
      PRINTF("Processing SLLAO option in RA\n");
      uip_set_nd6_opt_llao(buf) = (uint8_t *) UIP_ND6_OPT_HDR_BUF(buf);
      uip_set_nbr(buf) = uip_ds6_nbr_lookup(&UIP_IP_BUF(buf)->srcipaddr);
      if(uip_nbr(buf) == NULL) {
        uip_set_nbr(buf) = uip_ds6_nbr_add(&UIP_IP_BUF(buf)->srcipaddr,
                              (uip_lladdr_t *)&uip_nd6_opt_llao(buf)[UIP_ND6_OPT_DATA_OFFSET],
			      1, NBR_STALE);
      } else {
        uip_lladdr_t *lladdr = (uip_lladdr_t *)uip_ds6_nbr_get_ll(uip_nbr(buf));
        if(uip_nbr(buf)->state == NBR_INCOMPLETE) {
          uip_nbr(buf)->state = NBR_STALE;
        }
        if(memcmp(&uip_nd6_opt_llao(buf)[UIP_ND6_OPT_DATA_OFFSET],
		  lladdr, UIP_LLADDR_LEN) != 0) {
          memcpy(lladdr, &uip_nd6_opt_llao(buf)[UIP_ND6_OPT_DATA_OFFSET],
		 UIP_LLADDR_LEN);
          uip_nbr(buf)->state = NBR_STALE;
        }
        uip_nbr(buf)->isrouter = 1;
      }
      break;
    case UIP_ND6_OPT_MTU:
      PRINTF("Processing MTU option in RA\n");
      uip_ds6_if.link_mtu =
        uip_ntohl(((uip_nd6_opt_mtu *) UIP_ND6_OPT_HDR_BUF(buf))->mtu);
      break;
    case UIP_ND6_OPT_PREFIX_INFO:
      PRINTF("Processing PREFIX option in RA\n");
      uip_set_nd6_opt_prefix_info(buf) = (uip_nd6_opt_prefix_info *) UIP_ND6_OPT_HDR_BUF(buf);
      if((uip_ntohl(uip_nd6_opt_prefix_info(buf)->validlt) >=
          uip_ntohl(uip_nd6_opt_prefix_info(buf)->preferredlt))
         && (!uip_is_addr_link_local(&uip_nd6_opt_prefix_info(buf)->prefix))) {
        /* on-link flag related processing */
        if(uip_nd6_opt_prefix_info(buf)->flagsreserved1 & UIP_ND6_RA_FLAG_ONLINK) {
          uip_set_prefix(buf) =
            uip_ds6_prefix_lookup(&uip_nd6_opt_prefix_info(buf)->prefix,
                                  uip_nd6_opt_prefix_info(buf)->preflen);
          if(uip_prefix(buf) == NULL) {
            if(uip_nd6_opt_prefix_info(buf)->validlt != 0) {
              if(uip_nd6_opt_prefix_info(buf)->validlt != UIP_ND6_INFINITE_LIFETIME) {
                uip_set_prefix(buf) = uip_ds6_prefix_add(&uip_nd6_opt_prefix_info(buf)->prefix,
                                            uip_nd6_opt_prefix_info(buf)->preflen,
                                            uip_ntohl(uip_nd6_opt_prefix_info(buf)->
                                                  validlt));
              } else {
                uip_set_prefix(buf) = uip_ds6_prefix_add(&uip_nd6_opt_prefix_info(buf)->prefix,
                                            uip_nd6_opt_prefix_info(buf)->preflen, 0);
              }
            }
          } else {
            switch (uip_nd6_opt_prefix_info(buf)->validlt) {
            case 0:
              uip_ds6_prefix_rm(uip_prefix(buf));
              break;
            case UIP_ND6_INFINITE_LIFETIME:
              uip_prefix(buf)->isinfinite = 1;
              break;
            default:
              PRINTF("Updating timer of prefix ");
              PRINT6ADDR(&uip_prefix(buf)->ipaddr);
              PRINTF(" new value %lu\n", uip_ntohl(uip_nd6_opt_prefix_info(buf)->validlt));
              stimer_set(&uip_prefix(buf)->vlifetime,
                         uip_ntohl(uip_nd6_opt_prefix_info(buf)->validlt));
              uip_prefix(buf)->isinfinite = 0;
              break;
            }
          }
        }
        /* End of on-link flag related processing */
        /* autonomous flag related processing */
        if((uip_nd6_opt_prefix_info(buf)->flagsreserved1 & UIP_ND6_RA_FLAG_AUTONOMOUS)
           && (uip_nd6_opt_prefix_info(buf)->validlt != 0)
           && (uip_nd6_opt_prefix_info(buf)->preflen == UIP_DEFAULT_PREFIX_LEN)) {
	  
          uip_ipaddr_copy(&uip_nd6_ipaddr(buf), &uip_nd6_opt_prefix_info(buf)->prefix);
          uip_ds6_set_addr_iid(&uip_nd6_ipaddr(buf), &uip_lladdr);
          uip_set_addr(buf) = uip_ds6_addr_lookup(&uip_nd6_ipaddr(buf));
          if((uip_addr(buf) != NULL) && (uip_addr(buf)->type == ADDR_AUTOCONF)) {
            if(uip_nd6_opt_prefix_info(buf)->validlt != UIP_ND6_INFINITE_LIFETIME) {
              /* The processing below is defined in RFC4862 section 5.5.3 e */
              if((uip_ntohl(uip_nd6_opt_prefix_info(buf)->validlt) > 2 * 60 * 60) ||
                 (uip_ntohl(uip_nd6_opt_prefix_info(buf)->validlt) >
                  stimer_remaining(&uip_addr(buf)->vlifetime))) {
                PRINTF("Updating timer of address ");
                PRINT6ADDR(&uip_addr(buf)->ipaddr);
                PRINTF(" new value %lu\n",
                       uip_ntohl(uip_nd6_opt_prefix_info(buf)->validlt));
                stimer_set(&uip_addr(buf)->vlifetime,
                           uip_ntohl(uip_nd6_opt_prefix_info(buf)->validlt));
              } else {
                stimer_set(&uip_addr(buf)->vlifetime, 2 * 60 * 60);
                PRINTF("Updating timer of address ");
                PRINT6ADDR(&uip_addr(buf)->ipaddr);
                PRINTF(" new value %lu\n", (unsigned long)(2 * 60 * 60));
              }
              uip_addr(buf)->isinfinite = 0;
            } else {
              uip_addr(buf)->isinfinite = 1;
            }
          } else {
            if(uip_ntohl(uip_nd6_opt_prefix_info(buf)->validlt) ==
               UIP_ND6_INFINITE_LIFETIME) {
              uip_ds6_addr_add(&uip_nd6_ipaddr(buf), 0, ADDR_AUTOCONF);
            } else {
              uip_ds6_addr_add(&uip_nd6_ipaddr(buf), uip_ntohl(uip_nd6_opt_prefix_info(buf)->validlt),
                               ADDR_AUTOCONF);
            }
          }
        }
        /* End of autonomous flag related processing */
      }
      break;
#if UIP_ND6_RA_RDNSS
    case UIP_ND6_OPT_RDNSS:
      if(UIP_ND6_RA_BUF(buf)->flags_reserved & (UIP_ND6_O_FLAG << 6)) {
        PRINTF("Processing RDNSS option\n");
        uint8_t naddr = (UIP_ND6_OPT_RDNSS_BUF->len - 1) / 2;
        uip_ipaddr_t *ip = (uip_ipaddr_t *)(&UIP_ND6_OPT_RDNSS_BUF->ip);
        PRINTF("got %d nameservers\n", naddr);
        while(naddr-- > 0) {
          PRINTF(" nameserver: ");
          PRINT6ADDR(ip);
          PRINTF(" lifetime: %lx\n", uip_ntohl(UIP_ND6_OPT_RDNSS_BUF->lifetime));
          uip_nameserver_update(ip, uip_ntohl(UIP_ND6_OPT_RDNSS_BUF->lifetime));
          ip++;
        }
      }
      break;
#endif /* UIP_ND6_RA_RDNSS */
    default:
      PRINTF("ND option (0x%x) not supported in RA\n",
	     UIP_ND6_OPT_HDR_BUF(buf)->type);
      break;
    }
    uip_nd6_opt_offset(buf) += (UIP_ND6_OPT_HDR_BUF(buf)->len << 3);
  }

  uip_set_defrt(buf) = uip_ds6_defrt_lookup(&UIP_IP_BUF(buf)->srcipaddr);
  if(UIP_ND6_RA_BUF(buf)->router_lifetime != 0) {
    if(uip_nbr(buf) != NULL) {
      uip_nbr(buf)->isrouter = 1;
    }
    if(uip_defrt(buf) == NULL) {
      uip_ds6_defrt_add(&UIP_IP_BUF(buf)->srcipaddr,
                        (unsigned
                         long)(uip_ntohs(UIP_ND6_RA_BUF(buf)->router_lifetime)));
    } else {
      stimer_set(&(uip_defrt(buf)->lifetime),
                 (unsigned long)(uip_ntohs(UIP_ND6_RA_BUF(buf)->router_lifetime)));
    }
  } else {
    if(uip_defrt(buf) != NULL) {
      uip_ds6_defrt_rm(uip_defrt(buf));
    }
  }

#if UIP_CONF_IPV6_QUEUE_PKT
  /* If the nbr just became reachable (e.g. it was in NBR_INCOMPLETE state
   * and we got a SLLAO), check if we had buffered a pkt for it */
  /*  if((nbr != NULL) && (nbr->queue_buf_len != 0)) {
    uip_len = nbr->queue_buf_len;
    memcpy(UIP_IP_BUF, nbr->queue_buf, uip_len);
    nbr->queue_buf_len = 0;
    return;
    }*/
  if(uip_nbr(buf) != NULL && uip_packetqueue_buflen(&uip_nbr(buf)->packethandle) != 0) {
    uip_len(buf) = buf->len = uip_packetqueue_buflen(&uip_nbr(buf)->packethandle);
    memcpy(UIP_IP_BUF(buf), uip_packetqueue_buf(&uip_nbr(buf)->packethandle), uip_len(buf));
    uip_packetqueue_free(&uip_nbr(buf)->packethandle);
    return;
  }

#endif /*UIP_CONF_IPV6_QUEUE_PKT */

discard:
  uip_len(buf) = 0;
  uip_ext_len(buf) = 0;
  return;
}
#endif /* !UIP_CONF_ROUTER */
/*------------------------------------------------------------------*/
/* ICMPv6 input handlers */
#if UIP_ND6_SEND_NA
UIP_ICMP6_HANDLER(ns_input_handler, ICMP6_NS, UIP_ICMP6_HANDLER_CODE_ANY,
                  ns_input);
UIP_ICMP6_HANDLER(na_input_handler, ICMP6_NA, UIP_ICMP6_HANDLER_CODE_ANY,
                  na_input);
#endif

#if UIP_CONF_ROUTER && UIP_ND6_SEND_RA
UIP_ICMP6_HANDLER(rs_input_handler, ICMP6_RS, UIP_ICMP6_HANDLER_CODE_ANY,
                  rs_input);
#endif

#if !UIP_CONF_ROUTER
UIP_ICMP6_HANDLER(ra_input_handler, ICMP6_RA, UIP_ICMP6_HANDLER_CODE_ANY,
                  ra_input);
#endif
/*---------------------------------------------------------------------------*/
void
uip_nd6_init()
{

#if UIP_ND6_SEND_NA
  /* Only handle NSs if we are prepared to send out NAs */
  uip_icmp6_register_input_handler(&ns_input_handler);

  /*
   * Only handle NAs if we are prepared to send out NAs.
   * This is perhaps logically incorrect, but this condition was present in
   * uip_process and we keep it until proven wrong
   */
  uip_icmp6_register_input_handler(&na_input_handler);
#endif


#if UIP_CONF_ROUTER && UIP_ND6_SEND_RA
  /* Only accept RS if we are a router and happy to send out RAs */
  uip_icmp6_register_input_handler(&rs_input_handler);
#endif

#if !UIP_CONF_ROUTER
  /* Only process RAs if we are not a router */
  uip_icmp6_register_input_handler(&ra_input_handler);
#endif
}
/*---------------------------------------------------------------------------*/
 /** @} */
