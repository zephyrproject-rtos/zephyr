/*
 * Copyright (c) 2010, Swedish Institute of Computer Science.
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
 * \file
 *   Private declarations for ContikiRPL.
 * \author
 *   Joakim Eriksson <joakime@sics.se>, Nicolas Tsiftes <nvt@sics.se>
 */

#ifndef RPL_PRIVATE_H
#define RPL_PRIVATE_H

#include "contiki/rpl/rpl.h"

#include "lib/list.h"
#include "contiki/ip/uip.h"
#include "sys/clock.h"
#include "sys/ctimer.h"
#include "contiki/ipv6/uip-ds6.h"
#include "contiki/ipv6/multicast/uip-mcast6.h"

/*---------------------------------------------------------------------------*/
/** \brief Is IPv6 address addr the link-local, all-RPL-nodes
    multicast address? */
#define uip_is_addr_linklocal_rplnodes_mcast(addr)	    \
  ((addr)->u8[0] == 0xff) &&				    \
  ((addr)->u8[1] == 0x02) &&				    \
  ((addr)->u16[1] == 0) &&				    \
  ((addr)->u16[2] == 0) &&				    \
  ((addr)->u16[3] == 0) &&				    \
  ((addr)->u16[4] == 0) &&				    \
  ((addr)->u16[5] == 0) &&				    \
  ((addr)->u16[6] == 0) &&				    \
  ((addr)->u8[14] == 0) &&				    \
  ((addr)->u8[15] == 0x1a))

/** \brief Set IP address addr to the link-local, all-rpl-nodes
    multicast address. */
#define uip_create_linklocal_rplnodes_mcast(addr)	\
  uip_ip6addr((addr), 0xff02, 0, 0, 0, 0, 0, 0, 0x001a)
/*---------------------------------------------------------------------------*/
/* RPL message types */
#define RPL_CODE_DIS                   0x00   /* DAG Information Solicitation */
#define RPL_CODE_DIO                   0x01   /* DAG Information Option */
#define RPL_CODE_DAO                   0x02   /* Destination Advertisement Option */
#define RPL_CODE_DAO_ACK               0x03   /* DAO acknowledgment */
#define RPL_CODE_SEC_DIS               0x80   /* Secure DIS */
#define RPL_CODE_SEC_DIO               0x81   /* Secure DIO */
#define RPL_CODE_SEC_DAO               0x82   /* Secure DAO */
#define RPL_CODE_SEC_DAO_ACK           0x83   /* Secure DAO ACK */

/* RPL control message options. */
#define RPL_OPTION_PAD1                  0
#define RPL_OPTION_PADN                  1
#define RPL_OPTION_DAG_METRIC_CONTAINER  2
#define RPL_OPTION_ROUTE_INFO            3
#define RPL_OPTION_DAG_CONF              4
#define RPL_OPTION_TARGET                5
#define RPL_OPTION_TRANSIT               6
#define RPL_OPTION_SOLICITED_INFO        7
#define RPL_OPTION_PREFIX_INFO           8
#define RPL_OPTION_TARGET_DESC           9

#define RPL_DAO_K_FLAG                   0x80 /* DAO ACK requested */
#define RPL_DAO_D_FLAG                   0x40 /* DODAG ID present */
/*---------------------------------------------------------------------------*/
/* RPL IPv6 extension header option. */
#define RPL_HDR_OPT_LEN			4
#define RPL_HOP_BY_HOP_LEN		(RPL_HDR_OPT_LEN + 2 + 2)
#define RPL_HDR_OPT_DOWN		0x80
#define RPL_HDR_OPT_DOWN_SHIFT  	7
#define RPL_HDR_OPT_RANK_ERR		0x40
#define RPL_HDR_OPT_RANK_ERR_SHIFT   	6
#define RPL_HDR_OPT_FWD_ERR		0x20
#define RPL_HDR_OPT_FWD_ERR_SHIFT   	5
/*---------------------------------------------------------------------------*/
/* Default values for RPL constants and variables. */

/* The default value for the DAO timer. */
#ifdef RPL_CONF_DAO_LATENCY
#define RPL_DAO_LATENCY                 RPL_CONF_DAO_LATENCY
#else /* RPL_CONF_DAO_LATENCY */
#define RPL_DAO_LATENCY                 (CLOCK_SECOND * 4)
#endif /* RPL_DAO_LATENCY */

/* Special value indicating immediate removal. */
#define RPL_ZERO_LIFETIME               0

#define RPL_LIFETIME(instance, lifetime) \
          ((unsigned long)(instance)->lifetime_unit * (lifetime))

#ifndef RPL_CONF_MIN_HOPRANKINC
#define RPL_MIN_HOPRANKINC          256
#else
#define RPL_MIN_HOPRANKINC          RPL_CONF_MIN_HOPRANKINC
#endif
#define RPL_MAX_RANKINC             (7 * RPL_MIN_HOPRANKINC)

#define DAG_RANK(fixpt_rank, instance) \
  ((fixpt_rank) / (instance)->min_hoprankinc)

/* Rank of a virtual root node that coordinates DAG root nodes. */
#define BASE_RANK                       0

/* Rank of a root node. */
#define ROOT_RANK(instance)             (instance)->min_hoprankinc

#define INFINITE_RANK                   0xffff


/* Expire DAOs from neighbors that do not respond in this time. (seconds) */
#define DAO_EXPIRATION_TIMEOUT          60
/*---------------------------------------------------------------------------*/
#define RPL_INSTANCE_LOCAL_FLAG         0x80
#define RPL_INSTANCE_D_FLAG             0x40

/* Values that tell where a route came from. */
#define RPL_ROUTE_FROM_INTERNAL         0
#define RPL_ROUTE_FROM_UNICAST_DAO      1
#define RPL_ROUTE_FROM_MULTICAST_DAO    2
#define RPL_ROUTE_FROM_DIO              3

/* DAG Mode of Operation */
#define RPL_MOP_NO_DOWNWARD_ROUTES      0
#define RPL_MOP_NON_STORING             1
#define RPL_MOP_STORING_NO_MULTICAST    2
#define RPL_MOP_STORING_MULTICAST       3

#ifdef  RPL_CONF_MOP
#define RPL_MOP_DEFAULT                 RPL_CONF_MOP
#else /* RPL_CONF_MOP */
#if RPL_CONF_MULTICAST
#define RPL_MOP_DEFAULT                 RPL_MOP_STORING_MULTICAST
#else
#define RPL_MOP_DEFAULT                 RPL_MOP_STORING_NO_MULTICAST
#endif /* UIP_IPV6_MULTICAST_RPL */
#endif /* RPL_CONF_MOP */

/* Emit a pre-processor error if the user configured multicast with bad MOP */
#if RPL_CONF_MULTICAST && (RPL_MOP_DEFAULT != RPL_MOP_STORING_MULTICAST)
#error "RPL Multicast requires RPL_MOP_DEFAULT==3. Check contiki-conf.h"
#endif

/* Multicast Route Lifetime as a multiple of the lifetime unit */
#ifdef RPL_CONF_MCAST_LIFETIME
#define RPL_MCAST_LIFETIME RPL_CONF_MCAST_LIFETIME
#else
#define RPL_MCAST_LIFETIME 3
#endif

/*
 * The ETX in the metric container is expressed as a fixed-point value 
 * whose integer part can be obtained by dividing the value by 
 * RPL_DAG_MC_ETX_DIVISOR.
 */
#define RPL_DAG_MC_ETX_DIVISOR		256

/* DIS related */
#define RPL_DIS_SEND                    1
#ifdef  RPL_DIS_INTERVAL_CONF
#define RPL_DIS_INTERVAL                RPL_DIS_INTERVAL_CONF
#else
#define RPL_DIS_INTERVAL                60
#endif
#define RPL_DIS_START_DELAY             5
/*---------------------------------------------------------------------------*/
/* Lollipop counters */

#define RPL_LOLLIPOP_MAX_VALUE           255
#define RPL_LOLLIPOP_CIRCULAR_REGION     127
#define RPL_LOLLIPOP_SEQUENCE_WINDOWS    16
#define RPL_LOLLIPOP_INIT                (RPL_LOLLIPOP_MAX_VALUE - RPL_LOLLIPOP_SEQUENCE_WINDOWS + 1)
#define RPL_LOLLIPOP_INCREMENT(counter)                                 \
  do {                                                                  \
    if((counter) > RPL_LOLLIPOP_CIRCULAR_REGION) {                      \
      (counter) = ((counter) + 1) & RPL_LOLLIPOP_MAX_VALUE;             \
    } else {                                                            \
      (counter) = ((counter) + 1) & RPL_LOLLIPOP_CIRCULAR_REGION;       \
    }                                                                   \
  } while(0)

#define RPL_LOLLIPOP_IS_INIT(counter)		\
  ((counter) > RPL_LOLLIPOP_CIRCULAR_REGION)
/*---------------------------------------------------------------------------*/
/* Logical representation of a DAG Information Object (DIO.) */
struct rpl_dio {
  uip_ipaddr_t dag_id;
  rpl_ocp_t ocp;
  rpl_rank_t rank;
  uint8_t grounded;
  uint8_t mop;
  uint8_t preference;
  uint8_t version;
  uint8_t instance_id;
  uint8_t dtsn;
  uint8_t dag_intdoubl;
  uint8_t dag_intmin;
  uint8_t dag_redund;
  uint8_t default_lifetime;
  uint16_t lifetime_unit;
  rpl_rank_t dag_max_rankinc;
  rpl_rank_t dag_min_hoprankinc;
  rpl_prefix_t destination_prefix;
  rpl_prefix_t prefix_info;
  struct rpl_metric_container mc;
};
typedef struct rpl_dio rpl_dio_t;

/* Callback for evaluating if this is a network to join or not */
typedef int (*join_callback_t) (rpl_dio_t* dio);
void rpl_set_join_callback(join_callback_t callback);

#if RPL_CONF_STATS
/* Statistics for fault management. */
struct rpl_stats {
  uint16_t mem_overflows;
  uint16_t local_repairs;
  uint16_t global_repairs;
  uint16_t malformed_msgs;
  uint16_t resets;
  uint16_t parent_switch;
  uint16_t forward_errors;
  uint16_t loop_errors;
  uint16_t loop_warnings;
  uint16_t root_repairs;
};
typedef struct rpl_stats rpl_stats_t;

extern rpl_stats_t rpl_stats;
#endif
/*---------------------------------------------------------------------------*/
/* RPL macros. */

#if RPL_CONF_STATS
#define RPL_STAT(code)	(code) 
#else
#define RPL_STAT(code)
#endif /* RPL_CONF_STATS */
/*---------------------------------------------------------------------------*/
/* Instances */
extern rpl_instance_t instance_table[];
extern rpl_instance_t *default_instance;

/* ICMPv6 functions for RPL. */
void dis_output(struct net_buf *buf, uip_ipaddr_t *addr);
void dio_output(rpl_instance_t *, uip_ipaddr_t *uc_addr);
void dao_output(rpl_parent_t *, uint8_t lifetime);
void dao_output_target(rpl_parent_t *, uip_ipaddr_t *, uint8_t lifetime);
void dao_ack_output(struct net_buf *buf, rpl_instance_t *, uip_ipaddr_t *, uint8_t);
void rpl_icmp6_register_handlers(void);

/* RPL logic functions. */
void rpl_join_dag(uip_ipaddr_t *from, rpl_dio_t *dio);
void rpl_join_instance(uip_ipaddr_t *from, rpl_dio_t *dio);
void rpl_local_repair(rpl_instance_t *instance);
void rpl_process_dio(uip_ipaddr_t *, rpl_dio_t *);
int rpl_process_parent_event(rpl_instance_t *, rpl_parent_t *);

/* DAG object management. */
rpl_dag_t *rpl_alloc_dag(uint8_t, uip_ipaddr_t *);
rpl_instance_t *rpl_alloc_instance(uint8_t);
void rpl_free_dag(rpl_dag_t *);
void rpl_free_instance(rpl_instance_t *);

/* DAG parent management function. */
rpl_parent_t *rpl_add_parent(rpl_dag_t *, rpl_dio_t *dio, uip_ipaddr_t *);
rpl_parent_t *rpl_find_parent(rpl_dag_t *, uip_ipaddr_t *);
rpl_parent_t *rpl_find_parent_any_dag(rpl_instance_t *instance, uip_ipaddr_t *addr);
void rpl_nullify_parent(rpl_parent_t *);
void rpl_remove_parent(rpl_parent_t *);
void rpl_move_parent(rpl_dag_t *dag_src, rpl_dag_t *dag_dst, rpl_parent_t *parent);
rpl_parent_t *rpl_select_parent(rpl_dag_t *dag);
rpl_dag_t *rpl_select_dag(rpl_instance_t *instance,rpl_parent_t *parent);
void rpl_recalculate_ranks(void);

/* RPL routing table functions. */
void rpl_remove_routes(rpl_dag_t *dag);
void rpl_remove_routes_by_nexthop(uip_ipaddr_t *nexthop, rpl_dag_t *dag);
uip_ds6_route_t *rpl_add_route(rpl_dag_t *dag, uip_ipaddr_t *prefix,
                               int prefix_len, uip_ipaddr_t *next_hop);
void rpl_purge_routes(void);

/* Lock a parent in the neighbor cache. */
void rpl_lock_parent(rpl_parent_t *p);

/* Objective function. */
rpl_of_t *rpl_find_of(rpl_ocp_t);

/* Timer functions. */
void rpl_schedule_dao(rpl_instance_t *);
void rpl_schedule_dao_immediately(rpl_instance_t *);
void rpl_cancel_dao(rpl_instance_t *instance);
void rpl_schedule_probing(rpl_instance_t *instance);

void rpl_reset_dio_timer(rpl_instance_t *);
void rpl_reset_periodic_timer(void);

/* Route poisoning. */
void rpl_poison_routes(rpl_dag_t *, rpl_parent_t *);

rpl_instance_t *rpl_get_default_instance(void);

#endif /* RPL_PRIVATE_H */
