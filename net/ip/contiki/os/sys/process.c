/*
 * Copyright (c) 2005, Swedish Institute of Computer Science
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
 * \addtogroup process
 * @{
 */

/**
 * \file
 *         Implementation of the Contiki process kernel.
 * \author
 *         Adam Dunkels <adam@sics.se>
 *
 */

#include <stdio.h>

#include <net/buf.h>
#include <net/net_core.h>

#include "sys/process.h"
#include "sys/arg.h"

/*
 * Pointer to the currently running process structure.
 */
struct process *process_list = NULL;
struct process *process_current = NULL;
 
static process_event_t lastevent;

/*
 * Structure used for keeping the queue of active events.
 */
struct event_data {
  process_event_t ev;
  process_data_t data;
  struct process *p;
};

static process_num_events_t nevents, fevent;
static struct event_data events[PROCESS_CONF_NUMEVENTS];

#if PROCESS_CONF_STATS
process_num_events_t process_maxevents;
#endif

static volatile unsigned char poll_requested;

#define PROCESS_STATE_NONE        0
#define PROCESS_STATE_RUNNING     1
#define PROCESS_STATE_CALLED      2

static void call_process(struct process *p, process_event_t ev, process_data_t data, struct net_buf *buf);

#define DEBUG 0
#include "contiki/ip/uip-debug.h"

#if UIP_LOGGING
#include <stdio.h>
void uip_log(char *msg);
#define UIP_LOG(m) uip_log(m)
#else
#define UIP_LOG(m)
#endif

/*---------------------------------------------------------------------------*/
process_event_t
process_alloc_event(void)
{
  return lastevent++;
}
/*---------------------------------------------------------------------------*/
void
process_start(struct process *p, process_data_t data, void *user_data)
{
  struct process *q;

  /* First make sure that we don't try to start a process that is
     already running. */
  for(q = process_list; q != p && q != NULL; q = q->next);

  /* If we found the process on the process list, we bail out. */
  if(q == p) {
    return;
  }
  /* Put on the procs list.*/
  p->next = process_list;
  process_list = p;
  p->state = PROCESS_STATE_RUNNING;
  p->user_data = user_data;
  PT_INIT(&p->pt);

  PRINTF("process: starting '%s'\n", PROCESS_NAME_STRING(p));

  /* Post a synchronous initialization event to the process. */
  process_post_synch(p, PROCESS_EVENT_INIT, data, NULL);
}
/*---------------------------------------------------------------------------*/
static void
exit_process(struct process *p, struct process *fromprocess, struct net_buf *buf)
{
  register struct process *q;
  struct process *old_current = process_current;

  PRINTF("process: exit_process '%s'\n", PROCESS_NAME_STRING(p));

  /* Make sure the process is in the process list before we try to
     exit it. */
  for(q = process_list; q != p && q != NULL; q = q->next);
  if(q == NULL) {
    return;
  }

  if(process_is_running(p)) {
    /* Process was running */
    p->state = PROCESS_STATE_NONE;

    /*
     * Post a synchronous event to all processes to inform them that
     * this process is about to exit. This will allow services to
     * deallocate state associated with this process.
     */
    for(q = process_list; q != NULL; q = q->next) {
      if(p != q) {
        call_process(q, PROCESS_EVENT_EXITED, (process_data_t)p, NULL);
      }
    }

    if(p->thread != NULL && p != fromprocess) {
      /* Post the exit event to the process that is about to exit. */
      process_current = p;
      p->thread(&p->pt, PROCESS_EVENT_EXIT, NULL, buf, p->user_data);
    }
  }

  if(p == process_list) {
    process_list = process_list->next;
  } else {
    for(q = process_list; q != NULL; q = q->next) {
      if(q->next == p) {
	q->next = p->next;
	break;
      }
    }
  }

  process_current = old_current;
}
/*---------------------------------------------------------------------------*/
static void
call_process(struct process *p, process_event_t ev, process_data_t data,
		struct net_buf *buf)
{
  int ret;

#if DEBUG
  if(p->state == PROCESS_STATE_CALLED) {
    printf("process: process '%s' called again with event %d buf %p\n",
	   PROCESS_NAME_STRING(p), ev, buf);
  }
#endif /* DEBUG */
  
  if((p->state & PROCESS_STATE_RUNNING) &&
     p->thread != NULL) {
    PRINTF("process: calling process '%s' with event %d buf %p\n", PROCESS_NAME_STRING(p), ev, buf);
    process_current = p;
    p->state = PROCESS_STATE_CALLED;
    ret = p->thread(&p->pt, ev, data, buf, p->user_data);
    if(ret == PT_EXITED ||
       ret == PT_ENDED ||
       ev == PROCESS_EVENT_EXIT) {
	    exit_process(p, p, buf);
    } else {
      p->state = PROCESS_STATE_RUNNING;
    }
  }
}
/*---------------------------------------------------------------------------*/
void
process_exit(struct process *p)
{
	exit_process(p, PROCESS_CURRENT(), NULL);
}
/*---------------------------------------------------------------------------*/
void
process_init(void)
{
  lastevent = PROCESS_EVENT_MAX;

  nevents = fevent = 0;
#if PROCESS_CONF_STATS
  process_maxevents = 0;
#endif /* PROCESS_CONF_STATS */

  process_current = process_list = NULL;
}
/*---------------------------------------------------------------------------*/
/*
 * Call each process' poll handler.
 */
/*---------------------------------------------------------------------------*/
static void
do_poll(struct net_buf *buf)
{
  struct process *p;

  poll_requested = 0;
  /* Call the processes that needs to be polled. */
  for(p = process_list; p != NULL; p = p->next) {
    if(p->needspoll) {
      p->state = PROCESS_STATE_RUNNING;
      p->needspoll = 0;
      call_process(p, PROCESS_EVENT_POLL, NULL, buf);
    }
  }
}
/*---------------------------------------------------------------------------*/
/*
 * Process the next event in the event queue and deliver it to
 * listening processes.
 */
/*---------------------------------------------------------------------------*/
static void
do_event(struct net_buf *buf)
{
  static process_event_t ev;
  static process_data_t data;
  static struct process *receiver;
  static struct process *p;
  
  /*
   * If there are any events in the queue, take the first one and walk
   * through the list of processes to see if the event should be
   * delivered to any of them. If so, we call the event handler
   * function for the process. We only process one event at a time and
   * call the poll handlers inbetween.
   */

  if(nevents > 0) {
    
    /* There are events that we should deliver. */
    ev = events[fevent].ev;
    
    data = events[fevent].data;
    receiver = events[fevent].p;

    /* Since we have seen the new event, we move pointer upwards
       and decrease the number of events. */
    fevent = (fevent + 1) % PROCESS_CONF_NUMEVENTS;
    --nevents;

    /* If this is a broadcast event, we deliver it to all events, in
       order of their priority. */
    if(receiver == PROCESS_BROADCAST) {
      for(p = process_list; p != NULL; p = p->next) {

	/* If we have been requested to poll a process, we do this in
	   between processing the broadcast event. */
	if(poll_requested) {
	  do_poll(buf);
	}
	call_process(p, ev, data, buf);
      }
    } else {
      /* This is not a broadcast event, so we deliver it to the
	 specified process. */
      /* If the event was an INIT event, we should also update the
	 state of the process. */
      if(ev == PROCESS_EVENT_INIT) {
	receiver->state = PROCESS_STATE_RUNNING;
      }

      /* Make sure that the process actually is running. */
      call_process(receiver, ev, data, buf);
    }
  }
}
/*---------------------------------------------------------------------------*/
int
process_run(struct net_buf *buf)
{
  /* Process poll events. */
  if(poll_requested) {
    do_poll(buf);
  }

  /* Process one event from the queue */
  do_event(buf);

  return nevents + poll_requested;
}
/*---------------------------------------------------------------------------*/
int
process_nevents(void)
{
  return nevents + poll_requested;
}
/*---------------------------------------------------------------------------*/
int
process_post(struct process *p, process_event_t ev, process_data_t data)
{
  static process_num_events_t snum;

  if(PROCESS_CURRENT() == NULL) {
    PRINTF("process_post: NULL process posts event %d to process '%s', nevents %d\n",
	   ev,PROCESS_NAME_STRING(p), nevents);
  } else {
    PRINTF("process_post: Process '%s' posts event %d to process '%s', nevents %d\n",
	   PROCESS_NAME_STRING(PROCESS_CURRENT()), ev,
	   p == PROCESS_BROADCAST? "<broadcast>": PROCESS_NAME_STRING(p), nevents);
  }
  
  if(nevents == PROCESS_CONF_NUMEVENTS) {
#if DEBUG
    if(p == PROCESS_BROADCAST) {
      printf("soft panic: event queue is full when broadcast event %d was posted from %s\n", ev, PROCESS_NAME_STRING(process_current));
    } else {
      printf("soft panic: event queue is full when event %d was posted to %s from %s\n", ev, PROCESS_NAME_STRING(p), PROCESS_NAME_STRING(process_current));
    }
#endif /* DEBUG */
    return PROCESS_ERR_FULL;
  }
  
  snum = (process_num_events_t)(fevent + nevents) % PROCESS_CONF_NUMEVENTS;
  events[snum].ev = ev;
  events[snum].data = data;
  events[snum].p = p;
  ++nevents;

#if PROCESS_CONF_STATS
  if(nevents > process_maxevents) {
    process_maxevents = nevents;
  }
#endif /* PROCESS_CONF_STATS */
  
  return PROCESS_ERR_OK;
}
/*---------------------------------------------------------------------------*/
void
process_post_synch(struct process *p, process_event_t ev, process_data_t data,
		   struct net_buf *buf)
{
  struct process *caller = process_current;

  call_process(p, ev, data, buf);
  process_current = caller;
}
/*---------------------------------------------------------------------------*/
void
process_poll(struct process *p)
{
  if(p != NULL) {
    if(p->state == PROCESS_STATE_RUNNING ||
       p->state == PROCESS_STATE_CALLED) {
      p->needspoll = 1;
      poll_requested = 1;
    }
  }
}
/*---------------------------------------------------------------------------*/
int
process_is_running(struct process *p)
{
  return p->state != PROCESS_STATE_NONE;
}
/*---------------------------------------------------------------------------*/
/** @} */
