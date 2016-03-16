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
 */

/**
 * \file
 *         A Carrier Sense Multiple Access (CSMA) MAC layer
 * \author
 *         Adam Dunkels <adam@sics.se>
 */

#include <net/l2_buf.h>

#include "contiki/mac/csma.h"
#include "contiki/packetbuf.h"
#include "contiki/queuebuf.h"

#include "sys/ctimer.h"
#include "sys/clock.h"

#include "lib/random.h"

#include "contiki/netstack.h"

#include "lib/list.h"
#include "lib/memb.h"

#include <string.h>

#include <stdio.h>

#if UIP_LOGGING
#include <stdio.h>
void uip_log(char *msg);
#define UIP_LOG(m) uip_log(m)
#else
#define UIP_LOG(m)
#endif

#ifdef CONFIG_NETWORK_IP_STACK_DEBUG_15_4_MAC
#define DEBUG 1
#endif
#include "contiki/ip/uip-debug.h"

#if UIP_LOGGING
#include <stdio.h>
void uip_log(char *msg);
#define UIP_LOG(m) uip_log(m)
#else
#define UIP_LOG(m)
#endif

#ifndef CSMA_MAX_BACKOFF_EXPONENT
#ifdef CSMA_CONF_MAX_BACKOFF_EXPONENT
#define CSMA_MAX_BACKOFF_EXPONENT CSMA_CONF_MAX_BACKOFF_EXPONENT
#else
#define CSMA_MAX_BACKOFF_EXPONENT 3
#endif /* CSMA_CONF_MAX_BACKOFF_EXPONENT */
#endif /* CSMA_MAX_BACKOFF_EXPONENT */

#ifndef CSMA_MAX_MAC_TRANSMISSIONS
#ifdef CSMA_CONF_MAX_MAC_TRANSMISSIONS
#define CSMA_MAX_MAC_TRANSMISSIONS CSMA_CONF_MAX_MAC_TRANSMISSIONS
#else
#define CSMA_MAX_MAC_TRANSMISSIONS 3
#endif /* CSMA_CONF_MAX_MAC_TRANSMISSIONS */
#endif /* CSMA_MAX_MAC_TRANSMISSIONS */

#if CSMA_MAX_MAC_TRANSMISSIONS < 1
#error CSMA_CONF_MAX_MAC_TRANSMISSIONS must be at least 1.
#error Change CSMA_CONF_MAX_MAC_TRANSMISSIONS in contiki-conf.h or in your Makefile.
#endif /* CSMA_CONF_MAX_MAC_TRANSMISSIONS < 1 */

/* Packet metadata */
struct qbuf_metadata {
  mac_callback_t sent;
  void *cptr;
  uint8_t max_transmissions;
};

/* Every neighbor has its own packet queue */
struct neighbor_queue {
  struct neighbor_queue *next;
  linkaddr_t addr;
  struct ctimer transmit_timer;
  uint8_t transmissions;
  uint8_t collisions, deferrals;
  LIST_STRUCT(queued_packet_list);
};

/* The maximum number of co-existing neighbor queues */
#ifdef CSMA_CONF_MAX_NEIGHBOR_QUEUES
#define CSMA_MAX_NEIGHBOR_QUEUES CSMA_CONF_MAX_NEIGHBOR_QUEUES
#else
#define CSMA_MAX_NEIGHBOR_QUEUES 2
#endif /* CSMA_CONF_MAX_NEIGHBOR_QUEUES */

/* The maximum number of pending packet per neighbor */
#ifdef CSMA_CONF_MAX_PACKET_PER_NEIGHBOR
#define CSMA_MAX_PACKET_PER_NEIGHBOR CSMA_CONF_MAX_PACKET_PER_NEIGHBOR
#else
#define CSMA_MAX_PACKET_PER_NEIGHBOR MAX_QUEUED_PACKETS
#endif /* CSMA_CONF_MAX_PACKET_PER_NEIGHBOR */

#define MAX_QUEUED_PACKETS QUEUEBUF_NUM
MEMB(neighbor_memb, struct neighbor_queue, CSMA_MAX_NEIGHBOR_QUEUES);
MEMB(packet_memb, struct rdc_buf_list, MAX_QUEUED_PACKETS);
MEMB(metadata_memb, struct qbuf_metadata, MAX_QUEUED_PACKETS);

static void packet_sent(struct net_buf *buf, void *ptr, int status, int num_transmissions);
static void transmit_packet_list(struct net_buf *buf, void *ptr);

/*---------------------------------------------------------------------------*/
static struct neighbor_queue *
neighbor_queue_from_addr(struct net_buf *buf, const linkaddr_t *addr)
{
  struct neighbor_queue *n = list_head(uip_neighbor_list(buf));
  while(n != NULL) {
    if(linkaddr_cmp(&n->addr, addr)) {
      return n;
    }
    n = list_item_next(n);
  }
  return NULL;
}
/*---------------------------------------------------------------------------*/
static clock_time_t
default_timebase(void)
{
  clock_time_t time;
  /* The retransmission time must be proportional to the channel
     check interval of the underlying radio duty cycling layer. */
  time = NETSTACK_RDC.channel_check_interval();

  /* If the radio duty cycle has no channel check interval (i.e., it
     does not turn the radio off), we make the retransmission time
     proportional to the configured MAC channel check rate. */
  if(time == 0) {
    time = CLOCK_SECOND / NETSTACK_RDC_CHANNEL_CHECK_RATE;
  }
  return time;
}
/*---------------------------------------------------------------------------*/
static void
free_packet(struct net_buf *buf, struct neighbor_queue *n, struct rdc_buf_list *p)
{
  if(p != NULL) {
    /* Remove packet from list and deallocate */
    list_remove(n->queued_packet_list, p);

    queuebuf_free(p->buf);
    memb_free(&metadata_memb, p->ptr);
    memb_free(&packet_memb, p);
    PRINTF("csma: free_queued_packet, queue length %d, free packets %d\n",
           list_length(n->queued_packet_list), memb_numfree(&packet_memb));
    if(list_head(n->queued_packet_list) != NULL) {
      /* There is a next packet. We reset current tx information */
      n->transmissions = 0;
      n->collisions = 0;
      n->deferrals = 0;
      transmit_packet_list(buf, n);
    } else {
      /* This was the last packet in the queue, we free the neighbor */
      list_remove(uip_neighbor_list(buf), n);
      memb_free(&neighbor_memb, n);
    }
  }
}
/*---------------------------------------------------------------------------*/
static void
transmit_packet_list(struct net_buf *buf, void *ptr)
{
  struct neighbor_queue *n = ptr;
  if(n) {
    struct rdc_buf_list *q = list_head(n->queued_packet_list);
    if(q != NULL) {
      PRINTF("csma: preparing number %d %p, queue len %d\n", n->transmissions, q,
          list_length(n->queued_packet_list));
      /* Send packets in the neighbor's list */
      NETSTACK_RDC.send_list(buf, packet_sent, n, q);
    }
  }
}
/*---------------------------------------------------------------------------*/
static void
packet_sent(struct net_buf *buf, void *ptr, int status, int num_transmissions)
{
  struct neighbor_queue *n;
  struct rdc_buf_list *q;
  struct qbuf_metadata *metadata;
  clock_time_t time = 0;
  mac_callback_t sent;
  void *cptr;
  int num_tx;
  int backoff_exponent;
  int backoff_transmissions;

  n = ptr;
  if(n == NULL) {
    return;
  }
  switch(status) {
  case MAC_TX_OK:
  case MAC_TX_NOACK:
    n->transmissions += num_transmissions;
    break;
  case MAC_TX_COLLISION:
    n->collisions += num_transmissions;
    n->transmissions += num_transmissions;
    break;
  case MAC_TX_DEFERRED:
    n->deferrals += num_transmissions;
    break;
  }

  /* Find out what packet this callback refers to */
  for(q = list_head(n->queued_packet_list);
      q != NULL; q = list_item_next(q)) {
    if(queuebuf_attr(q->buf, PACKETBUF_ATTR_MAC_SEQNO) ==
       packetbuf_attr(buf, PACKETBUF_ATTR_MAC_SEQNO)) {
      break;
    }
  }

  if(q != NULL) {
    metadata = (struct qbuf_metadata *)q->ptr;

    if(metadata != NULL) {
      sent = metadata->sent;
      cptr = metadata->cptr;
      num_tx = n->transmissions;
      if(status == MAC_TX_COLLISION ||
         status == MAC_TX_NOACK) {

        /* If the transmission was not performed because of a
           collision or noack, we must retransmit the packet. */

        switch(status) {
        case MAC_TX_COLLISION:
          PRINTF("csma: rexmit collision %d transmission %d\n",
		 n->collisions, n->transmissions);
          break;
        case MAC_TX_NOACK:
          PRINTF("csma: rexmit noack %d\n", n->transmissions);
          break;
        default:
          PRINTF("csma: rexmit err %d, %d\n", status, n->transmissions);
        }

        /* The retransmission time must be proportional to the channel
           check interval of the underlying radio duty cycling layer. */
        time = default_timebase();

        /* The retransmission time uses a truncated exponential backoff
         * so that the interval between the transmissions increase with
         * each retransmit. */
        backoff_exponent = num_tx;

        /* Truncate the exponent if needed. */
        if(backoff_exponent > CSMA_MAX_BACKOFF_EXPONENT) {
          backoff_exponent = CSMA_MAX_BACKOFF_EXPONENT;
        }

        /* Proceed to exponentiation. */
        backoff_transmissions = 1 << backoff_exponent;

        /* Pick a time for next transmission, within the interval:
         * [time, time + 2^backoff_exponent * time[ */
        time = time + (random_rand() % (backoff_transmissions * time));

        if(n->transmissions < metadata->max_transmissions) {
          PRINTF("csma: retransmitting with time %lu %p\n", time, q);
          ctimer_set(buf, &n->transmit_timer, time,
                     transmit_packet_list, n);
          /* This is needed to correctly attribute energy that we spent
             transmitting this packet. */
          queuebuf_update_attr_from_packetbuf(buf, q->buf);
        } else {
          PRINTF("csma: drop with status %d after %d transmissions, %d collisions\n",
                 status, n->transmissions, n->collisions);
          free_packet(buf, n, q);
          mac_call_sent_callback(buf, sent, cptr, status, num_tx);
        }
      } else {
        if(status == MAC_TX_OK) {
          PRINTF("csma: rexmit ok %d\n", n->transmissions);
        } else {
          PRINTF("csma: rexmit failed %d: %d\n", n->transmissions, status);
        }
        free_packet(buf, n, q);
        mac_call_sent_callback(buf, sent, cptr, status, num_tx);
      }
    } else {
      PRINTF("csma: no metadata\n");
    }
  } else {
    PRINTF("csma: seqno %d not found\n", packetbuf_attr(buf, PACKETBUF_ATTR_MAC_SEQNO));
  }
}
/*---------------------------------------------------------------------------*/
static uint8_t
send_packet(struct net_buf *buf, mac_callback_t sent, bool last_fragment, void *ptr)
{
  struct rdc_buf_list *q;
  struct neighbor_queue *n;
  static uint8_t initialized = 0;
  static uint16_t seqno;
  const linkaddr_t *addr = packetbuf_addr(buf, PACKETBUF_ADDR_RECEIVER);

  if (!buf) {
    UIP_LOG("csma: send_packet(): net_buf is NULL, cannot send packet");
    return 0;
  }

  if(!initialized) {
    initialized = 1;
    /* Initialize the sequence number to a random value as per 802.15.4. */
    seqno = random_rand();
  }

  if(seqno == 0) {
    /* PACKETBUF_ATTR_MAC_SEQNO cannot be zero, due to a pecuilarity
       in framer-802154.c. */
    seqno++;
  }
  packetbuf_set_attr(buf, PACKETBUF_ATTR_MAC_SEQNO, seqno++);

  /* Look for the neighbor entry */
  n = neighbor_queue_from_addr(buf, addr);
  if(n == NULL) {
    /* Allocate a new neighbor entry */
    n = memb_alloc(&neighbor_memb);
    if(n != NULL) {
      /* Init neighbor entry */
      linkaddr_copy(&n->addr, addr);
      n->transmissions = 0;
      n->collisions = 0;
      n->deferrals = 0;
      /* Init packet list for this neighbor */
      LIST_STRUCT_INIT(n, queued_packet_list);
      /* Add neighbor to the list */
      list_add(uip_neighbor_list(buf), n);
    }
  }

  if(n != NULL) {
    /* Add packet to the neighbor's queue */
    if(list_length(n->queued_packet_list) < CSMA_MAX_PACKET_PER_NEIGHBOR) {
      q = memb_alloc(&packet_memb);
      if(q != NULL) {
        q->ptr = memb_alloc(&metadata_memb);
        if(q->ptr != NULL) {
          q->buf = queuebuf_new_from_packetbuf(buf);
          if(q->buf != NULL) {
            struct qbuf_metadata *metadata = (struct qbuf_metadata *)q->ptr;
            /* Neighbor and packet successfully allocated */
            if(packetbuf_attr(buf, PACKETBUF_ATTR_MAX_MAC_TRANSMISSIONS) == 0) {
              /* Use default configuration for max transmissions */
              metadata->max_transmissions = CSMA_MAX_MAC_TRANSMISSIONS;
            } else {
              metadata->max_transmissions =
                packetbuf_attr(buf, PACKETBUF_ATTR_MAX_MAC_TRANSMISSIONS);
            }
            metadata->sent = sent;
            metadata->cptr = ptr;

            if(packetbuf_attr(buf, PACKETBUF_ATTR_PACKET_TYPE) ==
               PACKETBUF_ATTR_PACKET_TYPE_ACK) {
              list_push(n->queued_packet_list, q);
            } else {
              list_add(n->queued_packet_list, q);
            }

            PRINTF("csma: send_packet, queue length %d, free packets %d\n",
                   list_length(n->queued_packet_list), memb_numfree(&packet_memb));
            /* if received packet is last fragment/only one packet start sending
             * packets in list, do not start any timer.*/
            if (last_fragment) {
               transmit_packet_list(buf, n);
            }
            return 1;
          }
          memb_free(&metadata_memb, q->ptr);
          PRINTF("csma: could not allocate queuebuf, dropping packet\n");
        }
        memb_free(&packet_memb, q);
        PRINTF("csma: could not allocate queuebuf, dropping packet\n");
      }
      /* The packet allocation failed. Remove and free neighbor entry if empty. */
      if(list_length(n->queued_packet_list) == 0) {
        list_remove(uip_neighbor_list(buf), n);
        memb_free(&neighbor_memb, n);
      }
    } else {
      PRINTF("csma: Neighbor queue full\n");
    }
    PRINTF("csma: could not allocate packet, dropping packet\n");
  } else {
    PRINTF("csma: could not allocate neighbor, dropping packet\n");
  }
  mac_call_sent_callback(buf, sent, ptr, MAC_TX_ERR, 1);
  return 0;
}
/*---------------------------------------------------------------------------*/
static uint8_t
input_packet(struct net_buf *buf)
{
  return NETSTACK_LLSEC.input(buf);
}
/*---------------------------------------------------------------------------*/
static int
on(void)
{
  return NETSTACK_RDC.on();
}
/*---------------------------------------------------------------------------*/
static int
off(int keep_radio_on)
{
  return NETSTACK_RDC.off(keep_radio_on);
}
/*---------------------------------------------------------------------------*/
static unsigned short
channel_check_interval(void)
{
  if(NETSTACK_RDC.channel_check_interval) {
    return NETSTACK_RDC.channel_check_interval();
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
static void
init(void)
{
  queuebuf_init();

  memb_init(&packet_memb);
  memb_init(&metadata_memb);
  memb_init(&neighbor_memb);
}
/*---------------------------------------------------------------------------*/
const struct mac_driver csma_driver = {
  "CSMA",
  init,
  send_packet,
  input_packet,
  on,
  off,
  channel_check_interval,
};
/*---------------------------------------------------------------------------*/
