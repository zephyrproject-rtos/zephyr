/*
 * Copyright (c) 2009, Swedish Institute of Computer Science.
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
 * \file
 *         Management of extension headers for ContikiRPL.
 *
 * \author Vincent Brillault <vincent.brillault@imag.fr>,
 *         Joakim Eriksson <joakime@sics.se>,
 *         Niclas Finne <nfi@sics.se>,
 *         Nicolas Tsiftes <nvt@sics.se>.
 */

/**
 * \addtogroup uip6
 * @{
 */

#include "net/ip/uip.h"
#include "net/ip/tcpip.h"
#include "net/ipv6/uip-ds6.h"
#include "net/rpl/rpl-private.h"
#include "net/packetbuf.h"

#define DEBUG DEBUG_NONE
#include "net/ip/uip-debug.h"

#include <limits.h>
#include <string.h>

/*---------------------------------------------------------------------------*/
#define UIP_IP_BUF                ((struct uip_ip_hdr *)&uip_buf[UIP_LLH_LEN])
#define UIP_EXT_BUF               ((struct uip_ext_hdr *)&uip_buf[uip_l2_l3_hdr_len])
#define UIP_HBHO_BUF              ((struct uip_hbho_hdr *)&uip_buf[uip_l2_l3_hdr_len])
#define UIP_HBHO_NEXT_BUF         ((struct uip_ext_hdr *)&uip_buf[uip_l2_l3_hdr_len + RPL_HOP_BY_HOP_LEN])
#define UIP_EXT_HDR_OPT_BUF       ((struct uip_ext_hdr_opt *)&uip_buf[uip_l2_l3_hdr_len + uip_ext_opt_offset])
#define UIP_EXT_HDR_OPT_PADN_BUF  ((struct uip_ext_hdr_opt_padn *)&uip_buf[uip_l2_l3_hdr_len + uip_ext_opt_offset])
#define UIP_EXT_HDR_OPT_RPL_BUF   ((struct uip_ext_hdr_opt_rpl *)&uip_buf[uip_l2_l3_hdr_len + uip_ext_opt_offset])
/*---------------------------------------------------------------------------*/
int
rpl_verify_header(int uip_ext_opt_offset)
{
  rpl_instance_t *instance;
  int down;
  uint16_t sender_rank;
  uint8_t sender_closer;
  uip_ds6_route_t *route;

  if(UIP_HBHO_BUF->len != RPL_HOP_BY_HOP_LEN - 8) {
    PRINTF("RPL: Hop-by-hop extension header has wrong size\n");
    return 1;
  }

  if(UIP_EXT_HDR_OPT_RPL_BUF->opt_type != UIP_EXT_HDR_OPT_RPL) {
    PRINTF("RPL: Non RPL Hop-by-hop option\n");
    return 1;
  }

  if(UIP_EXT_HDR_OPT_RPL_BUF->opt_len != RPL_HDR_OPT_LEN) {
    PRINTF("RPL: Bad header option! (wrong length)\n");
    return 1;
  }

  instance = rpl_get_instance(UIP_EXT_HDR_OPT_RPL_BUF->instance);
  if(instance == NULL) {
    PRINTF("RPL: Unknown instance: %u\n",
           UIP_EXT_HDR_OPT_RPL_BUF->instance);
    return 1;
  }

  if(UIP_EXT_HDR_OPT_RPL_BUF->flags & RPL_HDR_OPT_FWD_ERR) {
    PRINTF("RPL: Forward error!\n");
    /* We should try to repair it by removing the neighbor that caused
       the packet to be forwareded in the first place. We drop any
       routes that go through the neighbor that sent the packet to
       us. */
    route = uip_ds6_route_lookup(&UIP_IP_BUF->destipaddr);
    if(route != NULL) {
      uip_ds6_route_rm(route);
    }
    RPL_STAT(rpl_stats.forward_errors++);
    /* Trigger DAO retransmission */
    rpl_reset_dio_timer(instance);
    /* drop the packet as it is not routable */
    return 1;
  }

  if(!instance->current_dag->joined) {
    PRINTF("RPL: No DAG in the instance\n");
    return 1;
  }

  down = 0;
  if(UIP_EXT_HDR_OPT_RPL_BUF->flags & RPL_HDR_OPT_DOWN) {
    down = 1;
  }

  sender_rank = UIP_HTONS(UIP_EXT_HDR_OPT_RPL_BUF->senderrank);
  sender_closer = sender_rank < instance->current_dag->rank;

  PRINTF("RPL: Packet going %s, sender closer %d (%d < %d)\n", down == 1 ? "down" : "up",
	 sender_closer,
	 sender_rank,
	 instance->current_dag->rank
	 );

  if((down && !sender_closer) || (!down && sender_closer)) {
    PRINTF("RPL: Loop detected - senderrank: %d my-rank: %d sender_closer: %d\n",
	   sender_rank, instance->current_dag->rank,
	   sender_closer);
    if(UIP_EXT_HDR_OPT_RPL_BUF->flags & RPL_HDR_OPT_RANK_ERR) {
      RPL_STAT(rpl_stats.loop_errors++);
      PRINTF("RPL: Rank error signalled in RPL option!\n");
      /* Packet must be dropped and dio trickle timer reset, see RFC6550 - 11.2.2.2 */
      rpl_reset_dio_timer(instance);
      return 1;
    }
    PRINTF("RPL: Single error tolerated\n");
    RPL_STAT(rpl_stats.loop_warnings++);
    UIP_EXT_HDR_OPT_RPL_BUF->flags |= RPL_HDR_OPT_RANK_ERR;
    return 0;
  }

  PRINTF("RPL: Rank OK\n");

  return 0;
}
/*---------------------------------------------------------------------------*/
static void
set_rpl_opt(unsigned uip_ext_opt_offset)
{
  uint8_t temp_len;

  memmove(UIP_HBHO_NEXT_BUF, UIP_EXT_BUF, uip_len - UIP_IPH_LEN);
  memset(UIP_HBHO_BUF, 0, RPL_HOP_BY_HOP_LEN);
  UIP_HBHO_BUF->next = UIP_IP_BUF->proto;
  UIP_IP_BUF->proto = UIP_PROTO_HBHO;
  UIP_HBHO_BUF->len = RPL_HOP_BY_HOP_LEN - 8;
  UIP_EXT_HDR_OPT_RPL_BUF->opt_type = UIP_EXT_HDR_OPT_RPL;
  UIP_EXT_HDR_OPT_RPL_BUF->opt_len = RPL_HDR_OPT_LEN;
  UIP_EXT_HDR_OPT_RPL_BUF->flags = 0;
  UIP_EXT_HDR_OPT_RPL_BUF->instance = 0;
  UIP_EXT_HDR_OPT_RPL_BUF->senderrank = 0;
  uip_len += RPL_HOP_BY_HOP_LEN;
  temp_len = UIP_IP_BUF->len[1];
  UIP_IP_BUF->len[1] += UIP_HBHO_BUF->len + 8;
  if(UIP_IP_BUF->len[1] < temp_len) {
    UIP_IP_BUF->len[0]++;
  }
}
/*---------------------------------------------------------------------------*/
int
rpl_update_header_empty(void)
{
  rpl_instance_t *instance;
  int uip_ext_opt_offset;
  int last_uip_ext_len;
  rpl_parent_t *parent;

  last_uip_ext_len = uip_ext_len;
  uip_ext_len = 0;
  uip_ext_opt_offset = 2;

  PRINTF("RPL: Verifying the presence of the RPL header option\n");

  switch(UIP_IP_BUF->proto) {
  case UIP_PROTO_HBHO:
    if(UIP_HBHO_BUF->len != RPL_HOP_BY_HOP_LEN - 8) {
      PRINTF("RPL: Hop-by-hop extension header has wrong size\n");
      uip_ext_len = last_uip_ext_len;
      return 0;
    }
    if(UIP_EXT_HDR_OPT_RPL_BUF->opt_type != UIP_EXT_HDR_OPT_RPL) {
      PRINTF("RPL: Non RPL Hop-by-hop option support not implemented\n");
      uip_ext_len = last_uip_ext_len;
      return 0;
    }
    if(UIP_EXT_HDR_OPT_RPL_BUF->opt_len != RPL_HDR_OPT_LEN) {
      PRINTF("RPL: RPL Hop-by-hop option has wrong length\n");
      uip_ext_len = last_uip_ext_len;
      return 0;
    }
    instance = rpl_get_instance(UIP_EXT_HDR_OPT_RPL_BUF->instance);
    if(instance == NULL || !instance->used || !instance->current_dag->joined) {
      PRINTF("RPL: Unable to add hop-by-hop extension header: incorrect instance\n");
      return 0;
    }
    break;
  default:
#if RPL_INSERT_HBH_OPTION
    PRINTF("RPL: No hop-by-hop option found, creating it\n");
    if(uip_len + RPL_HOP_BY_HOP_LEN > UIP_BUFSIZE) {
      PRINTF("RPL: Packet too long: impossible to add hop-by-hop option\n");
      uip_ext_len = last_uip_ext_len;
      return 0;
    }
    set_rpl_opt(uip_ext_opt_offset);
    uip_ext_len = last_uip_ext_len + RPL_HOP_BY_HOP_LEN;
#endif
    return 0;
  }

  switch(UIP_EXT_HDR_OPT_BUF->type) {
  case UIP_EXT_HDR_OPT_RPL:
    PRINTF("RPL: Updating RPL option\n");
    UIP_EXT_HDR_OPT_RPL_BUF->senderrank = UIP_HTONS(instance->current_dag->rank);

    /* Check the direction of the down flag, as per Section 11.2.2.3,
       which states that if a packet is going down it should in
       general not go back up again. If this happens, a
       RPL_HDR_OPT_FWD_ERR should be flagged. */
    if((UIP_EXT_HDR_OPT_RPL_BUF->flags & RPL_HDR_OPT_DOWN)) {
      if(uip_ds6_route_lookup(&UIP_IP_BUF->destipaddr) == NULL) {
        UIP_EXT_HDR_OPT_RPL_BUF->flags |= RPL_HDR_OPT_FWD_ERR;
        PRINTF("RPL forwarding error\n");
        /* We should send back the packet to the originating parent,
           but it is not feasible yet, so we send a No-Path DAO instead */
        PRINTF("RPL generate No-Path DAO\n");
        parent = rpl_get_parent((uip_lladdr_t *)packetbuf_addr(PACKETBUF_ADDR_SENDER));
        if(parent != NULL) {
          dao_output_target(parent, &UIP_IP_BUF->destipaddr, RPL_ZERO_LIFETIME);
        }
        /* Drop packet */
        return 1;
      }
    } else {
      /* Set the down extension flag correctly as described in Section
         11.2 of RFC6550. If the packet progresses along a DAO route,
         the down flag should be set. */
      if(uip_ds6_route_lookup(&UIP_IP_BUF->destipaddr) == NULL) {
        /* No route was found, so this packet will go towards the RPL
           root. If so, we should not set the down flag. */
        UIP_EXT_HDR_OPT_RPL_BUF->flags &= ~RPL_HDR_OPT_DOWN;
        PRINTF("RPL option going up\n");
      } else {
        /* A DAO route was found so we set the down flag. */
        UIP_EXT_HDR_OPT_RPL_BUF->flags |= RPL_HDR_OPT_DOWN;
        PRINTF("RPL option going down\n");
      }
    }

    uip_ext_len = last_uip_ext_len;
    return 0;
  default:
    PRINTF("RPL: Multi Hop-by-hop options not implemented\n");
    uip_ext_len = last_uip_ext_len;
    return 0;
  }
}
/*---------------------------------------------------------------------------*/
int
rpl_update_header_final(uip_ipaddr_t *addr)
{
  rpl_parent_t *parent;
  int uip_ext_opt_offset;
  int last_uip_ext_len;

  last_uip_ext_len = uip_ext_len;
  uip_ext_len = 0;
  uip_ext_opt_offset = 2;

  if(UIP_IP_BUF->proto == UIP_PROTO_HBHO) {
    if(UIP_HBHO_BUF->len != RPL_HOP_BY_HOP_LEN - 8) {
      PRINTF("RPL: Non RPL Hop-by-hop options support not implemented\n");
      uip_ext_len = last_uip_ext_len;
      return 0;
    }

    if(UIP_EXT_HDR_OPT_BUF->type == UIP_EXT_HDR_OPT_RPL) {
      if(UIP_EXT_HDR_OPT_RPL_BUF->senderrank == 0) {
        PRINTF("RPL: Updating RPL option\n");
        if(default_instance == NULL || !default_instance->used || !default_instance->current_dag->joined) {
          PRINTF("RPL: Unable to add hop-by-hop extension header: incorrect default instance\n");
          return 1;
        }
        parent = rpl_find_parent(default_instance->current_dag, addr);
        if(parent == NULL || parent != parent->dag->preferred_parent) {
          UIP_EXT_HDR_OPT_RPL_BUF->flags = RPL_HDR_OPT_DOWN;
        }
        UIP_EXT_HDR_OPT_RPL_BUF->instance = default_instance->instance_id;
        UIP_EXT_HDR_OPT_RPL_BUF->senderrank = UIP_HTONS(default_instance->current_dag->rank);
      }
    }
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
void
rpl_remove_header(void)
{
  uint8_t temp_len;

  uip_ext_len = 0;

  PRINTF("RPL: Verifying the presence of the RPL header option\n");
  switch(UIP_IP_BUF->proto){
  case UIP_PROTO_HBHO:
    PRINTF("RPL: Removing the RPL header option\n");
    UIP_IP_BUF->proto = UIP_HBHO_BUF->next;
    temp_len = UIP_IP_BUF->len[1];
    uip_len -= UIP_HBHO_BUF->len + 8;
    UIP_IP_BUF->len[1] -= UIP_HBHO_BUF->len + 8;
    if(UIP_IP_BUF->len[1] > temp_len) {
      UIP_IP_BUF->len[0]--;
    }
    memmove(UIP_EXT_BUF, UIP_HBHO_NEXT_BUF, uip_len - UIP_IPH_LEN);
    break;
  default:
    PRINTF("RPL: No hop-by-hop Option found\n");
  }
}
/*---------------------------------------------------------------------------*/
uint8_t
rpl_invert_header(void)
{
  uint8_t uip_ext_opt_offset;
  uint8_t last_uip_ext_len;

  last_uip_ext_len = uip_ext_len;
  uip_ext_len = 0;
  uip_ext_opt_offset = 2;

  PRINTF("RPL: Verifying the presence of the RPL header option\n");
  switch(UIP_IP_BUF->proto) {
  case UIP_PROTO_HBHO:
    break;
  default:
    PRINTF("RPL: No hop-by-hop Option found\n");
    uip_ext_len = last_uip_ext_len;
    return 0;
  }

  switch (UIP_EXT_HDR_OPT_BUF->type) {
  case UIP_EXT_HDR_OPT_RPL:
    PRINTF("RPL: Updating RPL option (switching direction)\n");
    UIP_EXT_HDR_OPT_RPL_BUF->flags &= RPL_HDR_OPT_DOWN;
    UIP_EXT_HDR_OPT_RPL_BUF->flags ^= RPL_HDR_OPT_DOWN;
    UIP_EXT_HDR_OPT_RPL_BUF->senderrank = UIP_HTONS(rpl_get_instance(UIP_EXT_HDR_OPT_RPL_BUF->instance)->current_dag->rank);
    uip_ext_len = last_uip_ext_len;
    return RPL_HOP_BY_HOP_LEN;
  default:
    PRINTF("RPL: Multi Hop-by-hop options not implemented\n");
    uip_ext_len = last_uip_ext_len;
    return 0;
  }
}
/*---------------------------------------------------------------------------*/
void
rpl_insert_header(void)
{
#if RPL_INSERT_HBH_OPTION
  if(default_instance != NULL && !uip_is_addr_mcast(&UIP_IP_BUF->destipaddr)) {
    rpl_update_header_empty();
  }
#endif
}
/*---------------------------------------------------------------------------*/

/** @}*/
