/*
 * Copyright (c) 2004, Swedish Institute of Computer Science.
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
 * Author: Adam Dunkels <adam@sics.se>
 *
 */

/**
 * \addtogroup etimer
 * @{
 */

/**
 * \file
 * Event timer library implementation.
 * \author
 * Adam Dunkels <adam@sics.se>
 */

#include "contiki-conf.h"

#define DEBUG DEBUG_NONE
#include "contiki/ip/uip-debug.h"

#include "sys/etimer.h"
#include "sys/process.h"

extern void net_timer_check(void);

static struct etimer *timerlist;
static clock_time_t next_expiration;

PROCESS(etimer_process, "Event timer");
/*---------------------------------------------------------------------------*/
static void
update_time(void)
{
  struct etimer *t;
  clock_time_t remaining;

  if (timerlist == NULL) {
    next_expiration = 0;
  } else {
    clock_time_t shortest = 0;
    for(t = timerlist; t != NULL; t = t->next) {
      remaining = timer_remaining(&t->timer);
      PRINTF("%s():%d etimer %p left %d shortest %d triggered %d\n",
	     __FUNCTION__, __LINE__,
	     t, remaining, shortest, etimer_is_triggered(t));
      if((shortest > remaining || shortest == 0) && remaining != 0) {
        shortest = remaining;
      }
    }
    next_expiration = shortest;
    PRINTF("%s():%d next expiration %d\n", __FUNCTION__, __LINE__,
	   next_expiration);
  }

  net_timer_check();
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(etimer_process, ev, data, buf, user_data)
{
  struct etimer *t;

  PROCESS_BEGIN();

  while(1) {
    PROCESS_YIELD();

    PRINTF("%s():%d timerlist %p\n", __FUNCTION__, __LINE__, timerlist);
    for(t = timerlist; t != NULL; t = t->next) {
      PRINTF("%s():%d timer %p remaining %d triggered %d\n",
	     __FUNCTION__, __LINE__,
	     t, timer_remaining(&t->timer), etimer_is_triggered(t));
      if(etimer_expired(t) && !etimer_is_triggered(t)) {
        PRINTF("%s():%d timer %p expired, process %p\n",
	       __FUNCTION__, __LINE__, t, t->p);

	if (t->p == NULL) {
          PRINTF("calling tcpip_process\n");
          process_post_synch(&tcpip_process, PROCESS_EVENT_TIMER, t, NULL);
	} else {
          process_post_synch(t->p, PROCESS_EVENT_TIMER, t, NULL);
	}
      }
    }
    update_time();

  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
clock_time_t
etimer_request_poll(void)
{
  process_post_synch(&etimer_process, PROCESS_EVENT_POLL,
		     NULL, NULL);
  return next_expiration;
}
/*---------------------------------------------------------------------------*/
static void
add_timer(struct etimer *timer)
{
  struct etimer *t;

  for(t = timerlist; t != NULL; t = t->next) {
    if(t == timer) {
      /* Timer already on list, bail out. */
	update_time();
	return;
    }
  }

  /* Timer not on list. */
  timer->next = timerlist;
  timerlist = timer;

  update_time();
}
/*---------------------------------------------------------------------------*/
void
etimer_set(struct etimer *et, clock_time_t interval, struct process *p)
{
  et->p = p;
  timer_set(&et->timer, interval);
  add_timer(et);
}
/*---------------------------------------------------------------------------*/
void
etimer_reset(struct etimer *et)
{
  timer_reset(&et->timer);
  add_timer(et);
}
/*---------------------------------------------------------------------------*/
void
etimer_restart(struct etimer *et)
{
  timer_restart(&et->timer);
  add_timer(et);
}
/*---------------------------------------------------------------------------*/
#if 1
void
etimer_adjust(struct etimer *et, int timediff)
{
  et->timer.start += timediff;
  update_time();
}
#endif
/*---------------------------------------------------------------------------*/
int
etimer_expired(struct etimer *et)
{
  return timer_expired(&et->timer);
}
/*---------------------------------------------------------------------------*/
clock_time_t
etimer_expiration_time(struct etimer *et)
{
  return et->timer.start + et->timer.interval;
}
/*---------------------------------------------------------------------------*/
clock_time_t
etimer_start_time(struct etimer *et)
{
  return et->timer.start;
}
/*---------------------------------------------------------------------------*/
int
etimer_pending(void)
{
  return timerlist != NULL;
}
/*---------------------------------------------------------------------------*/
clock_time_t
etimer_next_expiration_time(void)
{
  return etimer_pending() ? next_expiration : 0;
}
/*---------------------------------------------------------------------------*/
void
etimer_stop(struct etimer *et)
{
  struct etimer *t;

  timer_stop(&et->timer);

  /* First check if et is the first event timer on the list. */
  if(et == timerlist) {
    timerlist = timerlist->next;
    update_time();
  } else {
    /* Else walk through the list and try to find the item before the
       et timer. */
    for(t = timerlist; t != NULL && t->next != et; t = t->next);

    if(t != NULL) {
      /* We've found the item before the event timer that we are about
        to remove. We point the items next pointer to the event after
        the removed item. */
      t->next = et->next;

      update_time();
    }
  }

  /* Remove the next pointer from the item to be removed. */
  et->next = NULL;

  PRINTF("%s():%d timer %p removed\n", __FUNCTION__, __LINE__);
}
/*---------------------------------------------------------------------------*/
bool etimer_is_triggered(struct etimer *t)
{
  return timer_is_triggered(&t->timer);
}
void etimer_set_triggered(struct etimer *t)
{
  timer_set_triggered(&t->timer);
}
/** @} */
