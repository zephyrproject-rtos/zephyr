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
 *      CoAP module for observing resources (draft-ietf-core-observe-11).
 * \author
 *      Matthias Kovatsch <kovatsch@inf.ethz.ch>
 */

#ifndef COAP_OBSERVE_H_
#define COAP_OBSERVE_H_

#include "er-coap.h"
#include "er-coap-transactions.h"
#include <sys/stimer.h>

#define COAP_OBSERVER_URL_LEN 20

typedef struct coap_observable {
  uint32_t observe_clock;
  struct stimer orphan_timer;
  list_t observers;
  coap_packet_t notification;
  uint8_t buffer[COAP_MAX_PACKET_SIZE + 1];
} coap_observable_t;

typedef struct coap_observer {
  struct coap_observer *next;   /* for LIST */

  char url[COAP_OBSERVER_URL_LEN];
  uip_ipaddr_t addr;
  uint16_t port;
  coap_context_t *coap_ctx;
  uint8_t token_len;
  uint8_t token[COAP_TOKEN_LEN];
  uint16_t last_mid;

  int32_t obs_counter;

  struct etimer retrans_timer;
  uint8_t retrans_counter;
} coap_observer_t;

list_t coap_get_observers(void);

/* coap_observer_t *coap_add_observer(coap_context_t *coap_ctx, */
/*                                    uip_ipaddr_t *addr, uint16_t port, */
/*                                    const uint8_t *token, size_t token_len, */
/*                                    const char *url); */

void coap_remove_observer(coap_observer_t *o);
int coap_remove_observer_by_client(uip_ipaddr_t *addr, uint16_t port);
int coap_remove_observer_by_token(uip_ipaddr_t *addr, uint16_t port,
                                  uint8_t *token, size_t token_len);
int coap_remove_observer_by_uri(const uip_ipaddr_t *addr, uint16_t port,
                                const char *uri);
int coap_remove_observer_by_mid(coap_context_t *coap_ctx,
                                uip_ipaddr_t *addr, uint16_t port,
                                uint16_t mid);

void coap_notify_observers(resource_t *resource);
void coap_notify_observers_sub(resource_t *resource, char *subpath);

void coap_observe_handler(resource_t *resource, void *request,
                          void *response);

#endif /* COAP_OBSERVE_H_ */
