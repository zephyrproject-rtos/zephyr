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
 *         ContikiRPL, an implementation of RPL: IPv6 Routing Protocol
 *         for Low-Power and Lossy Networks (IETF RFC 6550)
 *
 * \author Joakim Eriksson <joakime@sics.se>, Nicolas Tsiftes <nvt@sics.se>
 */

/**
 * \addtogroup uip6
 * @{
 */

#include "contiki/ip/uip.h"
#include "contiki/ip/tcpip.h"
#include "contiki/ipv6/uip-ds6.h"
#include "contiki/ipv6/uip-icmp6.h"
#include "contiki/rpl/rpl-private.h"
#include "contiki/ipv6/multicast/uip-mcast6.h"

#ifdef CONFIG_NETWORK_IP_STACK_DEBUG_RPL
#define DEBUG 1
#endif
#include "contiki/ip/uip-debug.h"

#include <limits.h>
#include <string.h>

#if RPL_CONF_STATS
rpl_stats_t rpl_stats;
#endif

static enum rpl_mode mode = RPL_MODE_MESH;
/*---------------------------------------------------------------------------*/
enum rpl_mode
rpl_get_mode(void)
{
  return mode;
}
/*---------------------------------------------------------------------------*/
enum rpl_mode
rpl_set_mode(enum rpl_mode m)
{
  enum rpl_mode oldmode = mode;

  /* We need to do different things depending on what mode we are
     switching to. */
  if(m == RPL_MODE_MESH) {

    /* If we switch to mesh mode, we should send out a DAO message to
       inform our parent that we now are reachable. Before we do this,
       we must set the mode variable, since DAOs will not be sent if
       we are in feather mode. */
    PRINTF("RPL: switching to mesh mode\n");
    mode = m;

    if(default_instance != NULL) {
      rpl_schedule_dao_immediately(default_instance);
    }
  } else if(m == RPL_MODE_FEATHER) {

    PRINTF("RPL: switching to feather mode\n");
    mode = m;
    if(default_instance != NULL) {
      rpl_cancel_dao(default_instance);
    }

  } else {
    mode = m;
  }

  return oldmode;
}
/*---------------------------------------------------------------------------*/
void
rpl_purge_routes(void)
{
  uip_ds6_route_t *r;
  uip_ipaddr_t prefix;
  rpl_dag_t *dag;
#if RPL_CONF_MULTICAST
  uip_mcast6_route_t *mcast_route;
#endif

  /* First pass, decrement lifetime */
  r = uip_ds6_route_head();

  while(r != NULL) {
    if(r->state.lifetime >= 1) {
      /*
       * If a route is at lifetime == 1, set it to 0, scheduling it for
       * immediate removal below. This achieves the same as the original code,
       * which would delete lifetime <= 1
       */
      r->state.lifetime--;
    }
    r = uip_ds6_route_next(r);
  }

  /* Second pass, remove dead routes */
  r = uip_ds6_route_head();

  while(r != NULL) {
    if(r->state.lifetime < 1) {
      /* Routes with lifetime == 1 have only just been decremented from 2 to 1,
       * thus we want to keep them. Hence < and not <= */
      uip_ipaddr_copy(&prefix, &r->ipaddr);
      uip_ds6_route_rm(r);
      r = uip_ds6_route_head();
      PRINTF("No more routes to ");
      PRINT6ADDR(&prefix);
      dag = default_instance->current_dag;
      /* Propagate this information with a No-Path DAO to preferred parent if we are not a RPL Root */
      if(dag->rank != ROOT_RANK(default_instance)) {
        PRINTF(" -> generate No-Path DAO\n");
        dao_output_target(dag->preferred_parent, &prefix, RPL_ZERO_LIFETIME);
        /* Don't schedule more than 1 No-Path DAO, let next iteration handle that */
        return;
      }
      PRINTF("\n");
    } else {
      r = uip_ds6_route_next(r);
    }
  }

#if RPL_CONF_MULTICAST
  mcast_route = uip_mcast6_route_list_head();

  while(mcast_route != NULL) {
    if(mcast_route->lifetime <= 1) {
      uip_mcast6_route_rm(mcast_route);
      mcast_route = uip_mcast6_route_list_head();
    } else {
      mcast_route->lifetime--;
      mcast_route = list_item_next(mcast_route);
    }
  }
#endif
}
/*---------------------------------------------------------------------------*/
void
rpl_remove_routes(rpl_dag_t *dag)
{
  uip_ds6_route_t *r;
#if RPL_CONF_MULTICAST
  uip_mcast6_route_t *mcast_route;
#endif

  r = uip_ds6_route_head();

  while(r != NULL) {
    if(r->state.dag == dag) {
      uip_ds6_route_rm(r);
      r = uip_ds6_route_head();
    } else {
      r = uip_ds6_route_next(r);
    }
  }

#if RPL_CONF_MULTICAST
  mcast_route = uip_mcast6_route_list_head();

  while(mcast_route != NULL) {
    if(mcast_route->dag == dag) {
      uip_mcast6_route_rm(mcast_route);
      mcast_route = uip_mcast6_route_list_head();
    } else {
      mcast_route = list_item_next(mcast_route);
    }
  }
#endif
}
/*---------------------------------------------------------------------------*/
void
rpl_remove_routes_by_nexthop(uip_ipaddr_t *nexthop, rpl_dag_t *dag)
{
  uip_ds6_route_t *r;

  r = uip_ds6_route_head();

  while(r != NULL) {
    if(uip_ipaddr_cmp(uip_ds6_route_nexthop(r), nexthop) &&
       r->state.dag == dag) {
      uip_ds6_route_rm(r);
      r = uip_ds6_route_head();
    } else {
      r = uip_ds6_route_next(r);
    }
  }
  ANNOTATE("#L %u 0\n", nexthop->u8[sizeof(uip_ipaddr_t) - 1]);
}
/*---------------------------------------------------------------------------*/
uip_ds6_route_t *
rpl_add_route(rpl_dag_t *dag, uip_ipaddr_t *prefix, int prefix_len,
              uip_ipaddr_t *next_hop)
{
  uip_ds6_route_t *rep;

  if((rep = uip_ds6_route_add(prefix, prefix_len, next_hop)) == NULL) {
    PRINTF("RPL: No space for more route entries\n");
    return NULL;
  }

  rep->state.dag = dag;
  rep->state.lifetime = RPL_LIFETIME(dag->instance, dag->instance->default_lifetime);
  rep->state.learned_from = RPL_ROUTE_FROM_INTERNAL;

  PRINTF("RPL: Added a route to ");
  PRINT6ADDR(prefix);
  PRINTF("/%d via ", prefix_len);
  PRINT6ADDR(next_hop);
  PRINTF("\n");

  return rep;
}
/*---------------------------------------------------------------------------*/
void
rpl_link_neighbor_callback(const linkaddr_t *addr, int status, int numtx)
{
  uip_ipaddr_t ipaddr;
  rpl_parent_t *parent;
  rpl_instance_t *instance;
  rpl_instance_t *end;

  uip_ip6addr(&ipaddr, 0xfe80, 0, 0, 0, 0, 0, 0, 0);
  uip_ds6_set_addr_iid(&ipaddr, (uip_lladdr_t *)addr);

  for(instance = &instance_table[0], end = instance + RPL_MAX_INSTANCES; instance < end; ++instance) {
    if(instance->used == 1 ) {
      parent = rpl_find_parent_any_dag(instance, &ipaddr);
      if(parent != NULL) {
        /* Trigger DAG rank recalculation. */
        PRINTF("RPL: rpl_link_neighbor_callback triggering update\n");
        parent->flags |= RPL_PARENT_FLAG_UPDATED;
        if(instance->of->neighbor_link_callback != NULL) {
          instance->of->neighbor_link_callback(parent, status, numtx);
          parent->last_tx_time = clock_time();
        }
      }
    }
  }
}
/*---------------------------------------------------------------------------*/
void
rpl_ipv6_neighbor_callback(uip_ds6_nbr_t *nbr)
{
  rpl_parent_t *p;
  rpl_instance_t *instance;
  rpl_instance_t *end;

  PRINTF("RPL: Removing neighbor ");
  PRINT6ADDR(&nbr->ipaddr);
  PRINTF("\n");
  for(instance = &instance_table[0], end = instance + RPL_MAX_INSTANCES; instance < end; ++instance) {
    if(instance->used == 1 ) {
      p = rpl_find_parent_any_dag(instance, &nbr->ipaddr);
      if(p != NULL) {
        p->rank = INFINITE_RANK;
        /* Trigger DAG rank recalculation. */
        PRINTF("RPL: rpl_ipv6_neighbor_callback infinite rank\n");
        p->flags |= RPL_PARENT_FLAG_UPDATED;
      }
    }
  }
}
/*---------------------------------------------------------------------------*/
void
rpl_init(void)
{
  uip_ipaddr_t rplmaddr;
  PRINTF("RPL started\n");
  default_instance = NULL;

  rpl_dag_init();
  rpl_reset_periodic_timer();
  rpl_icmp6_register_handlers();

  /* add rpl multicast address */
  uip_create_linklocal_rplnodes_mcast(&rplmaddr);
  uip_ds6_maddr_add(&rplmaddr);

#if RPL_CONF_STATS
  memset(&rpl_stats, 0, sizeof(rpl_stats));
#endif

  RPL_OF.reset(NULL);
}
/*---------------------------------------------------------------------------*/

/** @}*/
