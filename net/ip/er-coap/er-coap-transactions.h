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

#ifndef COAP_TRANSACTIONS_H_
#define COAP_TRANSACTIONS_H_

#include "er-coap.h"

/*
 * Modulo mask (thus +1) for a random number to get the tick number for the random
 * retransmission time between COAP_RESPONSE_TIMEOUT and COAP_RESPONSE_TIMEOUT*COAP_RESPONSE_RANDOM_FACTOR.
 */
#define COAP_RESPONSE_TIMEOUT_TICKS         (CLOCK_SECOND * COAP_RESPONSE_TIMEOUT)
//#define COAP_RESPONSE_TIMEOUT_BACKOFF_MASK  (long)((CLOCK_SECOND * COAP_RESPONSE_TIMEOUT * ((float)COAP_RESPONSE_RANDOM_FACTOR - 1.0)) + 0.5) + 1
/* Convert to use integer arithmetics */
#define COAP_RESPONSE_TIMEOUT_BACKOFF_MASK				\
	(long)((CLOCK_SECOND * COAP_RESPONSE_TIMEOUT * \
		(COAP_RESPONSE_RANDOM_FACTOR_INT - 10)) + 5) / 10 + 1

/* container for transactions with message buffer and retransmission info */
typedef struct coap_transaction {
  struct coap_transaction *next;        /* for LIST */

  uint16_t mid;
  struct etimer retrans_timer;
  uint8_t retrans_counter;

  uip_ipaddr_t addr;
  uint16_t port;
  coap_context_t *coap_ctx;

  restful_response_handler callback;
  void *callback_data;

  uint16_t packet_len;
  uint8_t packet[COAP_MAX_PACKET_SIZE + 1];     /* +1 for the terminating '\0' which will not be sent
                                                 * Use snprintf(buf, len+1, "", ...) to completely fill payload */
} coap_transaction_t;

void coap_register_as_transaction_handler();

coap_transaction_t *coap_new_transaction(uint16_t mid, coap_context_t *coap_ctx,
                                         uip_ipaddr_t *addr, uint16_t port);
void coap_send_transaction(coap_transaction_t *t);
void coap_clear_transaction(coap_transaction_t *t);
coap_transaction_t *coap_get_transaction_by_mid(uint16_t mid);

void coap_check_transactions();

#endif /* COAP_TRANSACTIONS_H_ */
