/*
 * Copyright (c) 2015, SICS, Swedish ICT AB.
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
 */

/**
 * \file
 *         CoAP context handling
 * \author
 *         Niclas Finne <nfi@sics.se>
 *         Joakim Eriksson <joakime@sics.se>
 */

#ifndef COAP_CONTEXT_H_
#define COAP_CONTEXT_H_

#include <net/ip_buf.h>
#include <net/net_socket.h>

#include "contiki-conf.h"
#include "sys/process.h"
#include "contiki/ip/uip.h"

#if ER_COAP_WITH_DTLS
#define WITH_DTLS 1
#else
#undef WITH_DTLS
#endif

#ifdef WITH_DTLS
#include "dtls.h"
#endif /* WITH_DTLS */

typedef struct coap_context {
  struct net_buf *buf;
  struct net_context *net_ctx;
  struct net_addr my_addr;
  uint16_t my_port;
  //struct uip_udp_conn *conn; This is found in net_buf
  struct net_addr addr;
  uint16_t port;
#ifdef WITH_DTLS
  struct dtls_context_t *dtls_context;
  dtls_handler_t dtls_handler;
  struct process *process;
#endif /* WITH_DTLS */
  uint8_t status;
  uint8_t is_used;
} coap_context_t;

#define COAP_CONTEXT_NONE  NULL

int coap_context_listen(coap_context_t *coap_ctx, uip_ipaddr_t *peer_addr,
			uint16_t peer_port);
coap_context_t *coap_context_new(uip_ipaddr_t *my_addr, uint16_t port);
void coap_context_close(coap_context_t *coap_ctx);
int coap_context_send_message(coap_context_t *coap_ctx,
    uip_ipaddr_t *addr, uint16_t port,
    const uint8_t *data, uint16_t length);
int coap_context_wait_data(coap_context_t *coap_ctx, int32_t ticks);
int coap_context_connect(coap_context_t *coap_ctx, uip_ipaddr_t *addr,
			 uint16_t port);

#if WITH_DTLS

extern process_event_t coap_context_event;

int coap_context_is_secure(const coap_context_t *coap_ctx);
int coap_context_is_connected(const coap_context_t *coap_ctx);
int coap_context_is_connecting(const coap_context_t *coap_ctx);
int coap_context_has_errors(const coap_context_t *coap_ctx);
int coap_context_connect(coap_context_t *coap_ctx, uip_ipaddr_t *addr, uint16_t port);

typedef int (*dtls_get_psk_info_t)(struct dtls_context_t *ctx,
				   const session_t *session,
				   dtls_credentials_type_t type,
				   const unsigned char *id, size_t id_len,
				  unsigned char *result, size_t result_length);
typedef int (*dtls_get_ecdsa_key_t)(struct dtls_context_t *ctx,
				    const session_t *session,
				    const dtls_ecdsa_key_t **result);
typedef int (*dtls_verify_ecdsa_key_t)(struct dtls_context_t *ctx,
				       const session_t *session,
				       const unsigned char *other_pub_x,
				       const unsigned char *other_pub_y,
				       size_t key_size);
void coap_context_set_key_handlers(coap_context_t *ctx,
				   dtls_get_psk_info_t get_psk_info,
				   dtls_get_ecdsa_key_t get_ecdsa_key,
				   dtls_verify_ecdsa_key_t verify_ecdsa_key);

void coap_context_init(void);

#define COAP_CONTEXT_CONNECT(coap_ctx, server_addr, server_port)        \
  do {                                                                  \
    if(coap_context_connect(coap_ctx, server_addr, server_port)) {      \
      PROCESS_WAIT_EVENT_UNTIL(!coap_context_is_connecting(coap_ctx);   \
    }                                                                   \
  } while(0)

#else /* WITH_DTLS */

#define coap_context_event 0

static inline int
coap_context_is_secure(const coap_context_t *coap_ctx)
{
  return 0;
}
static inline int
coap_context_is_connected(const coap_context_t *coap_ctx)
{
  return 1;
}
static inline int
coap_context_is_connecting(const coap_context_t *coap_ctx)
{
  return 0;
}
static inline int
coap_context_has_errors(const coap_context_t *coap_ctx)
{
  return 0;
}
static inline void
coap_context_init(void)
{
}
int coap_context_reply(coap_context_t *ctx, struct net_buf *buf);

#define COAP_CONTEXT_CONNECT(coap_ctx, server_addr, server_port) \
  do { } while(0)

#define coap_context_set_key_handlers(...)

#endif /* WITH_DTLS */

#define COAP_CONTEXT_BLOCKING_REQUEST(coap_ctx, server_addr, server_port, request, chunk_handler) \
  {                                                                     \
    static struct request_state_t request_state;                        \
    PT_SPAWN(process_pt, &request_state.pt,                             \
             coap_blocking_request(&request_state, ev, coap_ctx,        \
                                   server_addr, server_port,            \
                                   request, chunk_handler)              \
             );                                                         \
  }

#endif /* COAP_CONTEXT_H_ */
