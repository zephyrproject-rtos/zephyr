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
 */

/**
 * \file
 *         RPL timer management.
 *
 * \author Joakim Eriksson <joakime@sics.se>, Nicolas Tsiftes <nvt@sics.se>
 */

/**
 * \addtogroup uip6
 * @{
 */

#include "contiki-conf.h"
#include "contiki/rpl/rpl-private.h"
#include "contiki/ipv6/multicast/uip-mcast6.h"
#include "lib/random.h"
#include "sys/ctimer.h"

#ifdef CONFIG_NETWORK_IP_STACK_DEBUG_RPL_TIMERS
#define DEBUG 1
#endif
#include "contiki/ip/uip-debug.h"

/*---------------------------------------------------------------------------*/
static struct ctimer periodic_timer;

static void handle_periodic_timer(struct net_buf *mbuf, void *ptr);
static void new_dio_interval(rpl_instance_t *instance);
static void handle_dio_timer(struct net_buf *mbuf, void *ptr);

static uint16_t next_dis;

/* dio_send_ok is true if the node is ready to send DIOs */
static uint8_t dio_send_ok;

/*---------------------------------------------------------------------------*/
static void
handle_periodic_timer(struct net_buf *not_used, void *ptr)
{
  rpl_purge_routes();
  rpl_recalculate_ranks();

  /* handle DIS */
#if RPL_DIS_SEND
  next_dis++;
  if(rpl_get_any_dag() == NULL && next_dis >= RPL_DIS_INTERVAL) {
    next_dis = 0;
    dis_output(NULL, NULL);
  }
#endif
  ctimer_reset(&periodic_timer);
}
/*---------------------------------------------------------------------------*/
static void
new_dio_interval(rpl_instance_t *instance)
{
  uint32_t time;
  clock_time_t ticks;

  /* TODO: too small timer intervals for many cases */
  time = 1UL << instance->dio_intcurrent;

  /* Convert from milliseconds to CLOCK_TICKS. */
  ticks = (time * CLOCK_SECOND) / 1000;
  instance->dio_next_delay = ticks;

  /* random number between I/2 and I */
  ticks = ticks / 2 + (ticks / 2 * (uint32_t)random_rand()) / RANDOM_RAND_MAX;

  /*
   * The intervals must be equally long among the nodes for Trickle to
   * operate efficiently. Therefore we need to calculate the delay between
   * the randomized time and the start time of the next interval.
   */
  instance->dio_next_delay -= ticks;
  instance->dio_send = 1;

#if RPL_CONF_STATS
  /* keep some stats */
  instance->dio_totint++;
  instance->dio_totrecv += instance->dio_counter;
  ANNOTATE("#A rank=%u.%u(%u),stats=%d %d %d %d,color=%s\n",
	   DAG_RANK(instance->current_dag->rank, instance),
           (10 * (instance->current_dag->rank % instance->min_hoprankinc)) / instance->min_hoprankinc,
           instance->current_dag->version,
           instance->dio_totint, instance->dio_totsend,
           instance->dio_totrecv,instance->dio_intcurrent,
	   instance->current_dag->rank == ROOT_RANK(instance) ? "BLUE" : "ORANGE");
#endif /* RPL_CONF_STATS */

  /* reset the redundancy counter */
  instance->dio_counter = 0;

  /* schedule the timer */
  PRINTF("RPL: Scheduling DIO timer %lu ticks in future (Interval)\n", ticks);
  ctimer_set(NULL, &instance->dio_timer, ticks, &handle_dio_timer, instance);
}
/*---------------------------------------------------------------------------*/
static void
handle_dio_timer(struct net_buf *not_used, void *ptr)
{
  rpl_instance_t *instance;

  instance = (rpl_instance_t *)ptr;

  PRINTF("RPL: DIO Timer triggered\n");
  if(!dio_send_ok) {
    if(uip_ds6_get_link_local(ADDR_PREFERRED) != NULL) {
      dio_send_ok = 1;
    } else {
      PRINTF("RPL: Postponing DIO transmission since link local address is not ok\n");
      ctimer_set(NULL, &instance->dio_timer, CLOCK_SECOND, &handle_dio_timer,
		 instance);
      return;
    }
  }

  if(instance->dio_send) {
    /* send DIO if counter is less than desired redundancy */
    if(instance->dio_redundancy != 0 && instance->dio_counter < instance->dio_redundancy) {
#if RPL_CONF_STATS
      instance->dio_totsend++;
#endif /* RPL_CONF_STATS */
      dio_output(instance, NULL);
    } else {
      PRINTF("RPL: Supressing DIO transmission (%d >= %d)\n",
             instance->dio_counter, instance->dio_redundancy);
    }
    instance->dio_send = 0;
    PRINTF("RPL: Scheduling DIO timer %lu ticks in future (sent)\n",
           instance->dio_next_delay);
    ctimer_set(NULL, &instance->dio_timer, instance->dio_next_delay,
	       handle_dio_timer, instance);
  } else {
    /* check if we need to double interval */
    if(instance->dio_intcurrent < instance->dio_intmin + instance->dio_intdoubl) {
      instance->dio_intcurrent++;
      PRINTF("RPL: DIO Timer interval doubled %d\n", instance->dio_intcurrent);
    }
    new_dio_interval(instance);
  }

#if DEBUG
  rpl_print_neighbor_list();
#endif
}
/*---------------------------------------------------------------------------*/
void
rpl_reset_periodic_timer()
{
  next_dis = RPL_DIS_INTERVAL / 2 +
    ((uint32_t)RPL_DIS_INTERVAL * (uint32_t)random_rand()) / RANDOM_RAND_MAX -
    RPL_DIS_START_DELAY;
  ctimer_set(NULL, &periodic_timer, CLOCK_SECOND, handle_periodic_timer, NULL);
}
/*---------------------------------------------------------------------------*/
/* Resets the DIO timer in the instance to its minimal interval. */
void
rpl_reset_dio_timer(rpl_instance_t *instance)
{
#if !RPL_LEAF_ONLY
  /* Do not reset if we are already on the minimum interval,
     unless forced to do so. */
  if(instance->dio_intcurrent > instance->dio_intmin) {
    instance->dio_counter = 0;
    instance->dio_intcurrent = instance->dio_intmin;
    new_dio_interval(instance);
  }
#if RPL_CONF_STATS
  rpl_stats.resets++;
#endif /* RPL_CONF_STATS */
#endif /* RPL_LEAF_ONLY */
}
/*---------------------------------------------------------------------------*/
static void handle_dao_timer(struct net_buf *mbuf, void *ptr);
static void
set_dao_lifetime_timer(rpl_instance_t *instance)
{
  if(rpl_get_mode() == RPL_MODE_FEATHER) {
    return;
  }

  /* Set up another DAO within half the expiration time, if such a
     time has been configured */
  if(instance->lifetime_unit != 0xffff && instance->default_lifetime != 0xff) {
    clock_time_t expiration_time;
    expiration_time = (clock_time_t)instance->default_lifetime *
      (clock_time_t)instance->lifetime_unit *
      CLOCK_SECOND / 2;
    PRINTF("RPL: Scheduling DAO lifetime timer %u ticks in the future\n",
           (unsigned)expiration_time);
    ctimer_set(NULL, &instance->dao_lifetime_timer, expiration_time,
               handle_dao_timer, instance);
  }
}
/*---------------------------------------------------------------------------*/
static void
handle_dao_timer(struct net_buf *not_used, void *ptr)
{
  rpl_instance_t *instance;
#if RPL_CONF_MULTICAST
  uip_mcast6_route_t *mcast_route;
  uint8_t i;
#endif

  instance = (rpl_instance_t *)ptr;

  if(!dio_send_ok && uip_ds6_get_link_local(ADDR_PREFERRED) == NULL) {
    PRINTF("RPL: Postpone DAO transmission\n");
    ctimer_set(NULL, &instance->dao_timer, CLOCK_SECOND, handle_dao_timer, instance);
    return;
  }

  /* Send the DAO to the DAO parent set -- the preferred parent in our case. */
  if(instance->current_dag->preferred_parent != NULL) {
    PRINTF("RPL: handle_dao_timer - sending DAO\n");
    /* Set the route lifetime to the default value. */
    dao_output(instance->current_dag->preferred_parent,
	       instance->default_lifetime);

#if RPL_CONF_MULTICAST
    /* Send DAOs for multicast prefixes only if the instance is in MOP 3 */
    if(instance->mop == RPL_MOP_STORING_MULTICAST) {
      /* Send a DAO for own multicast addresses */
      for(i = 0; i < UIP_DS6_MADDR_NB; i++) {
        if(uip_ds6_if.maddr_list[i].isused
            && uip_is_addr_mcast_global(&uip_ds6_if.maddr_list[i].ipaddr)) {
          dao_output_target(instance->current_dag->preferred_parent,
			    &uip_ds6_if.maddr_list[i].ipaddr,
			    RPL_MCAST_LIFETIME);
        }
      }

      /* Iterate over multicast routes and send DAOs */
      mcast_route = uip_mcast6_route_list_head();
      while(mcast_route != NULL) {
        /* Don't send if it's also our own address, done that already */
        if(uip_ds6_maddr_lookup(&mcast_route->group) == NULL) {
          dao_output_target(instance->current_dag->preferred_parent,
			    &mcast_route->group, RPL_MCAST_LIFETIME);
        }
        mcast_route = list_item_next(mcast_route);
      }
    }
#endif
  } else {
    PRINTF("RPL: No suitable DAO parent\n");
  }

  ctimer_stop(&instance->dao_timer);

  if(etimer_expired(&instance->dao_lifetime_timer.etimer)) {
    set_dao_lifetime_timer(instance);
  }
}
/*---------------------------------------------------------------------------*/
static void
schedule_dao(rpl_instance_t *instance, clock_time_t latency)
{
  clock_time_t expiration_time;

  if(rpl_get_mode() == RPL_MODE_FEATHER) {
    return;
  }

  expiration_time = etimer_expiration_time(&instance->dao_timer.etimer);

  if(!etimer_expired(&instance->dao_timer.etimer)) {
    PRINTF("RPL: DAO timer already scheduled\n");
  } else {
    if(latency != 0) {
      expiration_time = latency / 2 +
        (random_rand() % (latency));
    } else {
      expiration_time = 0;
    }
    PRINTF("RPL: Scheduling DAO timer %u ticks in the future\n",
           (unsigned)expiration_time);
    ctimer_set(NULL, &instance->dao_timer, expiration_time,
               handle_dao_timer, instance);

    set_dao_lifetime_timer(instance);
  }
}
/*---------------------------------------------------------------------------*/
void
rpl_schedule_dao(rpl_instance_t *instance)
{
  schedule_dao(instance, RPL_DAO_LATENCY);
}
/*---------------------------------------------------------------------------*/
void
rpl_schedule_dao_immediately(rpl_instance_t *instance)
{
  schedule_dao(instance, 0);
}
/*---------------------------------------------------------------------------*/
void
rpl_cancel_dao(rpl_instance_t *instance)
{
  ctimer_stop(&instance->dao_timer);
  ctimer_stop(&instance->dao_lifetime_timer);
}
/*---------------------------------------------------------------------------*/
#if RPL_WITH_PROBING
static rpl_parent_t *
get_probing_target(rpl_dag_t *dag)
{
  /* Returns the next probing target. The current implementation probes the current
   * preferred parent if we have not updated its link for RPL_PROBING_EXPIRATION_TIME.
   * Otherwise, it picks at random between:
   * (1) selecting the best parent not updated for RPL_PROBING_EXPIRATION_TIME
   * (2) selecting the least recently updated parent
   */

  rpl_parent_t *p;
  rpl_parent_t *probing_target = NULL;
  rpl_rank_t probing_target_rank = INFINITE_RANK;
  /* min_last_tx is the clock time RPL_PROBING_EXPIRATION_TIME in the past */
  clock_time_t min_last_tx = clock_time();
  min_last_tx = min_last_tx > 2 * RPL_PROBING_EXPIRATION_TIME
      ? min_last_tx - RPL_PROBING_EXPIRATION_TIME : 1;

  if(dag == NULL ||
      dag->instance == NULL ||
      dag->preferred_parent == NULL) {
    return NULL;
  }

  /* Our preferred parent needs probing */
  if(dag->preferred_parent->last_tx_time < min_last_tx) {
    probing_target = dag->preferred_parent;
  }

  /* With 50% probability: probe best parent not updated for RPL_PROBING_EXPIRATION_TIME */
  if(probing_target == NULL && (random_rand() % 2) == 0) {
    p = nbr_table_head(rpl_parents);
    while(p != NULL) {
      if(p->dag == dag && p->last_tx_time < min_last_tx) {
        /* p is in our dag and needs probing */
        rpl_rank_t p_rank = dag->instance->of->calculate_rank(p, 0);
        if(probing_target == NULL
            || p_rank < probing_target_rank) {
          probing_target = p;
          probing_target_rank = p_rank;
        }
      }
      p = nbr_table_next(rpl_parents, p);
    }
  }

  /* The default probing target is the least recently updated parent */
  if(probing_target == NULL) {
    p = nbr_table_head(rpl_parents);
    while(p != NULL) {
      if(p->dag == dag) {
        if(probing_target == NULL
            || p->last_tx_time < probing_target->last_tx_time) {
          probing_target = p;
        }
      }
      p = nbr_table_next(rpl_parents, p);
    }
  }

  return probing_target;
}
/*---------------------------------------------------------------------------*/
static void
handle_probing_timer(struct net_buf *not_used, void *ptr)
{
  rpl_instance_t *instance = (rpl_instance_t *)ptr;
  rpl_parent_t *probing_target = RPL_PROBING_SELECT_FUNC(instance->current_dag);

  /* Perform probing */
  if(probing_target != NULL && rpl_get_parent_ipaddr(probing_target) != NULL) {
    PRINTF("RPL: probing %3u\n",
        nbr_table_get_lladdr(rpl_parents, probing_target)->u8[7]);
    /* Send probe, e.g. unicast DIO or DIS */
    RPL_PROBING_SEND_FUNC(instance, rpl_get_parent_ipaddr(probing_target));
  }

  /* Schedule next probing */
  rpl_schedule_probing(instance);

#if DEBUG
  rpl_print_neighbor_list();
#endif
}
/*---------------------------------------------------------------------------*/
void
rpl_schedule_probing(rpl_instance_t *instance)
{
  ctimer_set(NULL, &instance->probing_timer, RPL_PROBING_DELAY_FUNC(),
                  handle_probing_timer, instance);
}
#endif /* RPL_WITH_PROBING */
/** @}*/
