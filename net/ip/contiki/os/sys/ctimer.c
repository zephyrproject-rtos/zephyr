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
 * \file
 *         Callback timer implementation
 * \author
 *         Adam Dunkels <adam@sics.se>
 */

/**
 * \addtogroup ctimer
 * @{
 */

#include <net/buf.h>

#include "sys/ctimer.h"
#include "contiki.h"
#include "lib/list.h"

LIST(ctimer_list);

static char initialized;

#define DEBUG DEBUG_NONE
#include "contiki/ip/uip-debug.h"

#if UIP_LOGGING
#include <stdio.h>
void uip_log(char *msg);
#define UIP_LOG(m) uip_log(m)
#else
#define UIP_LOG(m)
#endif

/*---------------------------------------------------------------------------*/
PROCESS(ctimer_process, "Ctimer process");
PROCESS_THREAD(ctimer_process, ev, data, buf, user_data)
{
  struct ctimer *c;
  PROCESS_BEGIN();

  for(c = list_head(ctimer_list); c != NULL; c = c->next) {
    etimer_set(&c->etimer, c->etimer.timer.interval, &ctimer_process);
  }
  initialized = 1;

  while(1) {
    PROCESS_YIELD_UNTIL(ev == PROCESS_EVENT_TIMER);
    for(c = list_head(ctimer_list); c != NULL; c = c->next) {
      if(&c->etimer == data) {
	list_remove(ctimer_list, c);
	PROCESS_CONTEXT_BEGIN(c->p);
	if(c->f != NULL) {
          c->f(c->buf, c->ptr);
	}
	PROCESS_CONTEXT_END(c->p);
	break;
      }
    }
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
void
ctimer_init(void)
{
  initialized = 0;
  list_init(ctimer_list);
  process_start(&ctimer_process, NULL, NULL);
}
/*---------------------------------------------------------------------------*/
void
ctimer_set(struct net_buf *buf, struct ctimer *c, clock_time_t t,
	   void (*f)(struct net_buf *, void *), void *ptr)
{
  c->p = &ctimer_process;
  c->f = f;
  c->ptr = ptr;
  c->buf = buf;
  PRINTF("ctimer_set %p t %u process %p buf %p etimer %p\n",
	 c, (unsigned)t, c->p, buf, &c->etimer);
  if(initialized) {
    PROCESS_CONTEXT_BEGIN(&ctimer_process);
    etimer_set(&c->etimer, t, &ctimer_process);
    PROCESS_CONTEXT_END(&ctimer_process);
  } else {
    c->etimer.timer.interval = t;
  }

  list_add(ctimer_list, c);
}
/*---------------------------------------------------------------------------*/
void
ctimer_reset(struct ctimer *c)
{
  if(initialized) {
    PROCESS_CONTEXT_BEGIN(&ctimer_process);
    etimer_reset(&c->etimer);
    PROCESS_CONTEXT_END(&ctimer_process);
  }

  list_add(ctimer_list, c);
}
/*---------------------------------------------------------------------------*/
void
ctimer_restart(struct ctimer *c)
{
  if(initialized) {
    PROCESS_CONTEXT_BEGIN(&ctimer_process);
    etimer_restart(&c->etimer);
    PROCESS_CONTEXT_END(&ctimer_process);
  }

  list_add(ctimer_list, c);
}
/*---------------------------------------------------------------------------*/
void
ctimer_stop(struct ctimer *c)
{
  if(initialized) {
    etimer_stop(&c->etimer);
  } else {
    c->etimer.next = NULL;
    //c->etimer.p = PROCESS_NONE;
  }
  list_remove(ctimer_list, c);
}
/*---------------------------------------------------------------------------*/
int
ctimer_expired(struct ctimer *c)
{
  struct ctimer *t;
  if(initialized) {
    return etimer_expired(&c->etimer);
  }
  for(t = list_head(ctimer_list); t != NULL; t = t->next) {
    if(t == c) {
      return 0;
    }
  }
  return 1;
}
/*---------------------------------------------------------------------------*/
/** @} */
