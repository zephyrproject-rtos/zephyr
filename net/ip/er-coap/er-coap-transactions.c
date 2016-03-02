/*
 * Copyright (c) 2013, Institute for Pervasive Computing, ETH Zurich
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
 *      CoAP module for reliable transport
 * \author
 *      Matthias Kovatsch <kovatsch@inf.ethz.ch>
 */

#include "contiki.h"
#include "contiki-net.h"
#include "er-coap-transactions.h"
#include "er-coap-observe.h"

#if CONFIG_NETWORK_IP_STACK_DEBUG_COAP_TRANSACTION
#define DEBUG 1
#endif
#include "contiki/ip/uip-debug.h"

/*---------------------------------------------------------------------------*/
MEMB(transactions_memb, coap_transaction_t, COAP_MAX_OPEN_TRANSACTIONS);
LIST(transactions_list);

static struct process *transaction_handler_process = NULL;

/*---------------------------------------------------------------------------*/
/*- Internal API ------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
void
coap_register_as_transaction_handler()
{
  transaction_handler_process = PROCESS_CURRENT();
}
coap_transaction_t *
coap_new_transaction(uint16_t mid, coap_context_t *coap_ctx,
                     uip_ipaddr_t *addr, uint16_t port)
{
  coap_transaction_t *t = memb_alloc(&transactions_memb);

  if(t) {
    t->mid = mid;
    t->retrans_counter = 0;

    /* save client address */
    uip_ipaddr_copy(&t->addr, addr);
    t->port = port;
    t->coap_ctx = coap_ctx;

    list_add(transactions_list, t); /* list itself makes sure same element is not added twice */
  }

  return t;
}
/*---------------------------------------------------------------------------*/
void
coap_send_transaction(coap_transaction_t *t)
{
  PRINTF("Sending transaction %u appdata %p len %d (%d)\n",
	 t->mid,
	 ip_buf_appdata(t->coap_ctx->buf),
	 ip_buf_appdatalen(t->coap_ctx->buf),
	 t->packet_len);

  if (!t->coap_ctx || !t->coap_ctx->buf) {
    PRINTF("***ERROR*** ctx %p buf %p\n", t->coap_ctx, t->coap_ctx->buf);
    coap_clear_transaction(t);
    return;
  }

  NET_COAP_STAT(sent++);

  coap_send_message(t->coap_ctx, &t->addr, t->port,
		    t->packet, t->packet_len);

  if(COAP_TYPE_CON ==
     ((COAP_HEADER_TYPE_MASK & t->packet[0]) >> COAP_HEADER_TYPE_POSITION)) {
    if(t->retrans_counter < COAP_MAX_RETRANSMIT) {
      /* not timed out yet */
      PRINTF("Keeping transaction %u\n", t->mid);

      if(t->retrans_counter == 0) {
        t->retrans_timer.timer.interval =
          COAP_RESPONSE_TIMEOUT_TICKS + (random_rand()
                                         %
                                         (clock_time_t)
                                         COAP_RESPONSE_TIMEOUT_BACKOFF_MASK);
        PRINTF("Initial interval %d\n",
               t->retrans_timer.timer.interval / CLOCK_SECOND);
      } else {
        t->retrans_timer.timer.interval <<= 1;  /* double */
        PRINTF("Doubled (%u) interval %d\n", t->retrans_counter,
               t->retrans_timer.timer.interval / CLOCK_SECOND);
      }

      PROCESS_CONTEXT_BEGIN(transaction_handler_process);
      etimer_restart(&t->retrans_timer);        /* interval updated above */
      PROCESS_CONTEXT_END(transaction_handler_process);

      t = NULL;
    } else {
      /* timed out */
      PRINTF("Timeout\n");
      restful_response_handler callback = t->callback;
      void *callback_data = t->callback_data;

      /* handle observers */
      coap_remove_observer_by_client(&t->addr, t->port);

      coap_clear_transaction(t);

      if(callback) {
        callback(callback_data, NULL);
      }
    }
  } else {
    coap_clear_transaction(t);
  }
}
/*---------------------------------------------------------------------------*/
void
coap_clear_transaction(coap_transaction_t *t)
{
  if(t) {
    PRINTF("Freeing transaction %u: %p\n", t->mid, t);

    etimer_stop(&t->retrans_timer);
    list_remove(transactions_list, t);
    memb_free(&transactions_memb, t);
  }
}
coap_transaction_t *
coap_get_transaction_by_mid(uint16_t mid)
{
  coap_transaction_t *t = NULL;

  for(t = (coap_transaction_t *)list_head(transactions_list); t; t = t->next) {
    if(t->mid == mid) {
      PRINTF("Found transaction for MID %u: %p\n", t->mid, t);
      return t;
    }
  }
  return NULL;
}
/*---------------------------------------------------------------------------*/
static inline struct net_buf *get_retransmit_buf(coap_transaction_t *t)
{
  coap_context_t *coap_ctx = t->coap_ctx;

  if (coap_ctx->buf) {
    return coap_ctx->buf;
  }

  coap_ctx->buf = ip_buf_get_tx(coap_ctx->net_ctx);
  if (!coap_ctx->buf) {
    return NULL;
  }

  /* We set the major buf params correctly. The application data pointer
   * should point to start of the coap packet data.
   * The tail of the packet points now to byte after coap packet.
   */
  ip_buf_appdata(coap_ctx->buf) = net_buf_add(coap_ctx->buf, t->packet_len);
  ip_buf_appdatalen(coap_ctx->buf) = t->packet_len;

  /* The total length of the packet is the coap packet + all the UDP/IP
   * headers.
   */
  uip_len(coap_ctx->buf) = ip_buf_len(coap_ctx->buf);

  return coap_ctx->buf;
}

void
coap_check_transactions()
{
  coap_transaction_t *t = NULL;

  for(t = (coap_transaction_t *)list_head(transactions_list); t; t = t->next) {
    if(etimer_expired(&t->retrans_timer)) {
      ++(t->retrans_counter);
      PRINTF("Retransmitting %u (%u)\n", t->mid, t->retrans_counter);
      if (get_retransmit_buf(t)) {
        coap_send_transaction(t);
	NET_COAP_STAT(re_sent++);
      }
    }
  }
}
/*---------------------------------------------------------------------------*/
