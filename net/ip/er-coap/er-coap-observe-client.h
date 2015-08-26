/*
 * Copyright (c) 2014, Daniele Alessandrelli.
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

/*
 * \file
 *        Extension to Erbium for enabling CoAP observe clients
 * \author
 *        Daniele Alessandrelli <daniele.alessandrelli@gmail.com>
 */

#ifndef COAP_OBSERVING_CLIENT_H_
#define COAP_OBSERVING_CLIENT_H_

#include "er-coap.h"
#include "er-coap-transactions.h"

#ifndef COAP_OBSERVE_CLIENT
#define COAP_OBSERVE_CLIENT 0
#endif

#ifdef COAP_CONF_MAX_OBSERVEES
#define COAP_MAX_OBSERVEES COAP_CONF_MAX_OBSERVEES
#else
#define COAP_MAX_OBSERVEES      4
#endif /* COAP_CONF_MAX_OBSERVEES */

#if COAP_MAX_OPEN_TRANSACTIONS < COAP_MAX_OBSERVEES
#warning "COAP_MAX_OPEN_TRANSACTIONS smaller than COAP_MAX_OBSERVEES: " \
  "this may be a problem"
#endif

#define IS_RESPONSE_CODE_2_XX(message) (64 < message->code \
                                        && message->code < 128)

/*----------------------------------------------------------------------------*/
typedef enum {
  OBSERVE_OK,
  NOTIFICATION_OK,
  OBSERVE_NOT_SUPPORTED,
  ERROR_RESPONSE_CODE,
  NO_REPLY_FROM_SERVER,
} coap_notification_flag_t;

/*----------------------------------------------------------------------------*/
typedef struct coap_observee_s coap_observee_t;

typedef void (*notification_callback_t)(coap_observee_t *subject,
                                        void *notification,
                                        coap_notification_flag_t);

struct coap_observee_s {
  coap_observee_t *next;        /* for LIST */
  coap_context_t *coap_ctx;
  uip_ipaddr_t addr;
  uint16_t port;
  const char *url;
  uint8_t token_len;
  uint8_t token[COAP_TOKEN_LEN];
  void *data;                   /* generic pointer for storing user data */
  notification_callback_t notification_callback;
  uint32_t last_observe;
};

/*----------------------------------------------------------------------------*/
coap_observee_t *coap_obs_add_observee(coap_context_t *coap_ctx,
                                       uip_ipaddr_t *addr, uint16_t port,
                                       const uint8_t *token, size_t token_len,
                                       const char *url,
                                       notification_callback_t
                                       notification_callback, void *data);

void coap_obs_remove_observee(coap_observee_t *o);

coap_observee_t *coap_obs_get_observee_by_token(const uint8_t *token,
                                                size_t token_len);

int coap_obs_remove_observee_by_token(coap_context_t *coap_ctx,
                                      uip_ipaddr_t *addr, uint16_t port,
                                      uint8_t *token, size_t token_len);

int coap_obs_remove_observee_by_url(coap_context_t *coap_ctx,
                                    uip_ipaddr_t *addr, uint16_t port,
                                    const char *url);

void coap_handle_notification(coap_context_t *coap_ctx,
                              uip_ipaddr_t *addr, uint16_t port,
                              coap_packet_t *notification);

coap_observee_t *coap_obs_request_registration(coap_context_t *coap_ctx,
                                               uip_ipaddr_t *addr,
                                               uint16_t port, char *uri,
                                               notification_callback_t
                                               notification_callback,
                                               void *data);
/* TODO: this function may be moved to er-coap.c */
uint8_t coap_generate_token(uint8_t **token_ptr);

#endif /* COAP_OBSERVING_CLIENT_H_ */
