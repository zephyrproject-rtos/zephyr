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

#include "er-coap.h"
#include "er-coap-engine.h"
#include <string.h>

#define DEBUG DEBUG_NONE
#include "net/ip/uip-debug.h"

#ifdef WITH_DTLS

#ifndef COAP_CONTEXT_CONF_MAX_CONTEXTS
#define MAX_CONTEXTS 1
#else
#define MAX_CONTEXTS COAP_CONTEXT_CONF_MAX_CONTEXTS
#endif /* COAP_CONTEXT_CONF_MAX_CONTEXTS */

static coap_context_t coap_contexts[MAX_CONTEXTS];

process_event_t coap_context_event;

#define STATUS_CONNECTING 1
#define STATUS_CONNECTED  2
#define STATUS_ALERT      4

PROCESS(coap_context_process, "CoAP Context Engine");

/*---------------------------------------------------------------------------*/
static int
get_from_peer(struct dtls_context_t *ctx, session_t *session,
              uint8 *data, size_t len)
{
  coap_context_t *coap_ctx;
  coap_ctx = dtls_get_app_data(ctx);

  if(coap_ctx != COAP_CONTEXT_NONE && coap_ctx->is_used) {
    uip_len = len;
    memmove(uip_appdata, data, len);
    coap_engine_receive(coap_ctx);
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
static int
send_to_peer(struct dtls_context_t *ctx, session_t *session,
             uint8 *data, size_t len)
{
  coap_context_t *coap_ctx;
  coap_ctx = dtls_get_app_data(ctx);

  if(coap_ctx != COAP_CONTEXT_NONE && coap_ctx->is_used &&
     coap_ctx->conn && session != NULL) {
    uip_udp_packet_sendto(coap_ctx->conn, data, len, &session->addr, session->port);
  }

  return len;
}
/*---------------------------------------------------------------------------*/
static int
event(struct dtls_context_t *ctx, session_t *session,
      dtls_alert_level_t level, unsigned short code)
{
  coap_context_t *coap_ctx;
  coap_ctx = dtls_get_app_data(ctx);

  if(coap_ctx == COAP_CONTEXT_NONE || coap_ctx->is_used == 0) {
    /* The context is no longer in use */
  } else if(code == DTLS_EVENT_CONNECTED) {
    coap_ctx->status = STATUS_CONNECTED;
    PRINTF("coap-context: DTLS CLIENT CONNECTED!\n");
    process_post(coap_ctx->process, coap_context_event, coap_ctx);
  } else if(code == DTLS_EVENT_CONNECT || code == DTLS_EVENT_RENEGOTIATE) {
    coap_ctx->status = STATUS_CONNECTING;
    PRINTF("coap-context: DTLS CLIENT NOT CONNECTED!\n");
    process_post(coap_ctx->process, coap_context_event, coap_ctx);
  } else if(level == DTLS_ALERT_LEVEL_FATAL && code < 256) {
    /* Fatal alert */
    coap_ctx->status = STATUS_ALERT;
    PRINTF("coap-context: DTLS CLIENT ALERT %u!\n", code);
    process_post(coap_ctx->process, coap_context_event, coap_ctx);
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
int
coap_context_is_secure(const coap_context_t *coap_ctx)
{
  /* Assume all contexts are secure except for the no context */
  if(coap_ctx != COAP_CONTEXT_NONE) {
    return coap_ctx->is_used;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
int
coap_context_is_connecting(const coap_context_t *coap_ctx)
{
  /* Assume all non secure contexts are connected by default */
  if(coap_ctx == COAP_CONTEXT_NONE || !coap_ctx->is_used) {
    return 0;
  }
  return coap_ctx->status == STATUS_CONNECTING;
}
/*---------------------------------------------------------------------------*/
int
coap_context_is_connected(const coap_context_t *coap_ctx)
{
  /* Assume all non secure contexts are connected by default */
  if(coap_ctx == COAP_CONTEXT_NONE || !coap_ctx->is_used) {
    return 1;
  }
  return coap_ctx->status == STATUS_CONNECTED;
}
/*---------------------------------------------------------------------------*/
int
coap_context_has_errors(const coap_context_t *coap_ctx)
{
  /* Assume all non secure contexts are connected by default */
  if(coap_ctx == COAP_CONTEXT_NONE || !coap_ctx->is_used) {
    return 0;
  }
  return coap_ctx->status == STATUS_ALERT;
}
/*---------------------------------------------------------------------------*/
coap_context_t *
coap_context_new(uint16_t port)
{
  coap_context_t *ctx = NULL;
  int i;
  for(i = 0; i < MAX_CONTEXTS; i++) {
    if(!coap_contexts[i].is_used) {
      ctx = &coap_contexts[i];
      break;
    }
  }

  if(ctx == NULL) {
    PRINTF("coap-context: no free contexts\n");
    return NULL;
  }

  memset(ctx, 0, sizeof(coap_context_t));

  /* new connection with remote host */
  PROCESS_CONTEXT_BEGIN(&coap_context_process);
  ctx->conn = udp_new(NULL, 0, ctx);
  PROCESS_CONTEXT_END(&coap_context_process);

  if(ctx->conn == NULL) {
    PRINTF("coap-context: failed to open new UDP conn\n");
    return NULL;
  }
  udp_bind(ctx->conn, port);

  /* initialize context */
  ctx->dtls_context = dtls_new_context(ctx);
  if(ctx->dtls_context == NULL) {
    PRINTF("coap-context: failed to get DTLS context\n");
    uip_udp_remove(ctx->conn);
    return NULL;
  }

  ctx->dtls_handler.write = send_to_peer;
  ctx->dtls_handler.read = get_from_peer;
  ctx->dtls_handler.event = event;
  dtls_set_handler(ctx->dtls_context, &ctx->dtls_handler);

  ctx->process = PROCESS_CURRENT();
  ctx->is_used = 1;
  PRINTF("Secure listening on port %u\n", uip_ntohs(port));

  return ctx;
}
/*---------------------------------------------------------------------------*/
void
coap_context_close(coap_context_t *coap_ctx)
{
  if(coap_ctx == NULL || coap_ctx->is_used == 0) {
    /* Not opened */
    return;
  }
  if(coap_ctx->dtls_context != NULL) {
    dtls_free_context(coap_ctx->dtls_context);
  }
  if(coap_ctx->conn != NULL) {
    uip_udp_remove(coap_ctx->conn);
  }
  coap_ctx->is_used = 0;
}
/*---------------------------------------------------------------------------*/
int
coap_context_connect(coap_context_t *coap_ctx, uip_ipaddr_t *addr, uint16_t port)
{
  session_t session;

  if(coap_ctx == NULL || coap_ctx->is_used == 0) {
    return 0;
  }

  uip_ipaddr_copy(&session.addr, addr);
  session.port = port;
  session.size = sizeof(session.addr) + sizeof(session.port);
  session.ifindex = 1;
  coap_ctx->status = STATUS_CONNECTING;
  PRINTF("coap-context: DTLS CONNECT TO [");
  PRINT6ADDR(addr);
  PRINTF("]:%u\n", uip_ntohs(port));
  if(dtls_connect(coap_ctx->dtls_context, &session) >= 0) {
    return 1;
  }

  /* Failed to initiate connection */
  coap_ctx->status = STATUS_ALERT;
  return 0;
}
/*---------------------------------------------------------------------------*/
int
coap_context_send_message(coap_context_t *coap_ctx,
                          uip_ipaddr_t *addr, uint16_t port,
                          const uint8_t *data, uint16_t length)
{
  int res;
  session_t sn;

  if(coap_ctx == NULL || coap_ctx->is_used == 0) {
    PRINTF("coap-context: can not send on non-used context\n");
    return 0;
  }

  uip_ipaddr_copy(&sn.addr, addr);
  sn.port = port;
  sn.size = sizeof(sn.addr) + sizeof(sn.port);
  sn.ifindex = 1;

  res = dtls_write(coap_ctx->dtls_context, &sn, (uint8 *)data, length);
  if(res < 0) {
    PRINTF("coap-context: Failed to send with dtls (%d)\n", res);
  } else if (res == 0) {
    PRINTF("coap-context: No data sent with DTLS but connection made.\n");
  }
  return res;
}
/*---------------------------------------------------------------------------*/
void
coap_context_init(void)
{
  dtls_init();

  coap_context_event = process_alloc_event();

  process_start(&coap_context_process, NULL);
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(coap_context_process, ev, data)
{
  coap_context_t *coap_ctx;
  session_t session;

  PROCESS_BEGIN();

  PROCESS_PAUSE();

  while(1) {
    PROCESS_WAIT_EVENT();

    if(ev == tcpip_event) {
      /* The CoAP context is stored as the appdata for the UDP connection */
      coap_ctx = data;

      if(coap_ctx != COAP_CONTEXT_NONE) {

        if(uip_newdata()) {
          uip_ipaddr_copy(&session.addr, &UIP_IP_BUF->srcipaddr);
          session.port = UIP_UDP_BUF->srcport;
          session.size = sizeof(session.addr) + sizeof(session.port);
          session.ifindex = 1;

          PRINTF("coap-context: got message from ");
          PRINT6ADDR(&session.addr);
          PRINTF(":%d %u bytes\n", uip_ntohs(session.port), uip_datalen());

          dtls_handle_message(coap_ctx->dtls_context, &session,
                              uip_appdata, uip_datalen());
        }

      } else if(uip_newdata()) {
        PRINTF("coap-context: got message from ");
        PRINT6ADDR(&UIP_IP_BUF->srcipaddr);
        PRINTF(" but no context found\n");

      } else {
        PRINTF("coap-context: no context for tcpip event\n");
      }
    }
  } /* while (1) */

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
#endif /* WITH_DTLS */
