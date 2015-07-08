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
#include <errno.h>

#if CONFIG_NETWORK_IP_STACK_DEBUG_COAP_CONTEXT
#define DEBUG 1
#endif
#include "contiki/ip/uip-debug.h"

#ifndef COAP_CONTEXT_CONF_MAX_CONTEXTS
#define MAX_CONTEXTS 1
#else
#define MAX_CONTEXTS COAP_CONTEXT_CONF_MAX_CONTEXTS
#endif /* COAP_CONTEXT_CONF_MAX_CONTEXTS */

static coap_context_t coap_contexts[MAX_CONTEXTS];

#ifdef WITH_DTLS

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
    uip_len(coap_ctx->buf) = len;
    memmove(uip_appdata(coap_ctx->buf), data, len);
    coap_engine_receive(coap_ctx);
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
static int prepare_and_send_buf(coap_context_t *ctx, session_t *session,
				uint8_t *data, size_t len)
{
  struct net_buf *buf;
  int max_data_len;

  /* This net_buf gets sent to network, so it is not released
   * by this function unless there was an error and buf was
   * not actually sent.
   */
  buf = ip_buf_get_tx(ctx->net_ctx);
  if (!buf) {
    len = -ENOBUFS;
    goto out;
  }

  max_data_len = IP_BUF_MAX_DATA - UIP_IPUDPH_LEN;

  PRINTF("%s: reply to peer data %p len %d\n", __FUNCTION__, data, len);

  if (len > max_data_len) {
    PRINTF("%s: too much (%d bytes) data to send (max %d bytes)\n",
	  __FUNCTION__, len, max_data_len);
    ip_buf_unref(buf);
    len = -EINVAL;
    goto out;
  }

  /* Note that we have reversed the addresses here
   * because net_reply() will reverse them again.
   */
#ifdef CONFIG_NETWORKING_WITH_IPV6
  uip_ip6addr_copy(&NET_BUF_IP(buf)->destipaddr, (uip_ip6addr_t *)&ctx->my_addr.in6_addr);
  uip_ip6addr_copy(&NET_BUF_IP(buf)->srcipaddr,
		   (uip_ip6addr_t *)&session->addr.ipaddr);
#else
  uip_ip4addr_copy(&NET_BUF_IP(buf)->destipaddr,
		   (uip_ip4addr_t *)&ctx->my_addr.in_addr);
  uip_ip4addr_copy(&NET_BUF_IP(buf)->srcipaddr,
		   (uip_ip4addr_t *)&session->addr.ipaddr);
#endif
  NET_BUF_UDP(buf)->destport = uip_ntohs(ctx->my_port);
  NET_BUF_UDP(buf)->srcport = session->addr.port;

  uip_set_udp_conn(buf) = net_context_get_udp_connection(ctx->net_ctx);

  memcpy(net_buf_add(buf, len), data, len);
  ip_buf_appdatalen(buf) = len;
  ip_buf_appdata(buf) = buf->data + ip_buf_reserve(buf);

  if (net_reply(ctx->net_ctx, buf)) {
    ip_buf_unref(buf);
  }
out:
  return len;
}
/*---------------------------------------------------------------------------*/
static int
send_to_peer(struct dtls_context_t *ctx, session_t *session,
             uint8 *data, size_t len)
{
  int ret = 0;
  coap_context_t *coap_ctx;
  coap_ctx = dtls_get_app_data(ctx);

  PRINTF("%s(): ctx %p session %p data %p len %d\n", __FUNCTION__, ctx,
	 session, data, len);
  if(coap_ctx != COAP_CONTEXT_NONE && coap_ctx->is_used && session != NULL) {
    ret = prepare_and_send_buf(coap_ctx, session, data, len);
    if (ret < 0) {
      PRINTF("%s(): cannot send data, msg discarded\n", __FUNCTION__);
    }
  } else {
      PRINTF("%s(): msg discarded ctx %p is_used %d buf %p udp %p session %p\n",
	     __FUNCTION__, coap_ctx, coap_ctx ? coap_ctx->is_used : 0,
	     coap_ctx->buf, coap_ctx ? (coap_ctx->buf ?
				uip_udp_conn(coap_ctx->buf) : NULL) : NULL,
	     session);
      ret = -EINVAL;
  }

  return ret;
}
/*---------------------------------------------------------------------------*/
int coap_context_wait_data(coap_context_t *coap_ctx, int32_t ticks)
{
  struct net_buf *buf;

  buf = net_receive(coap_ctx->net_ctx, ticks);
  if (buf) {
    session_t session;
    int ret;

    uip_ipaddr_copy(&session.addr.ipaddr, &UIP_IP_BUF(buf)->srcipaddr);
    session.addr.port = UIP_UDP_BUF(buf)->srcport;
    session.size = sizeof(session.addr);
    session.ifindex = 1;

    PRINTF("coap-context: got dtls message from ");
    PRINT6ADDR(&session.addr.ipaddr);
    PRINTF(":%d %u bytes\n", uip_ntohs(session.addr.port), uip_appdatalen(buf));

    PRINTF("Received appdata %p appdatalen %d\n",
	   ip_buf_appdata(buf), ip_buf_appdatalen(buf));

    coap_ctx->buf = buf;

    ret = dtls_handle_message(coap_ctx->dtls_context, &session,
			      ip_buf_appdata(buf), ip_buf_appdatalen(buf));

    /* We always release the buffer here as this buffer is never sent
     * to network anyway.
     */
    if (coap_ctx->buf) {
      ip_buf_unref(coap_ctx->buf);
      coap_ctx->buf = NULL;
    }

    return ret;
  }

  return 0;
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
    if (coap_ctx && coap_ctx->buf) {
      ip_buf_unref(coap_ctx->buf);
      coap_ctx->buf = NULL;
    }
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
coap_context_new(uip_ipaddr_t *my_addr, uint16_t port)
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

  /* initialize context */
  ctx->dtls_context = dtls_new_context(ctx);
  if(ctx->dtls_context == NULL) {
    PRINTF("coap-context: failed to get DTLS context\n");
    uip_udp_remove(uip_udp_conn(ctx->buf));
    return NULL;
  }

  ctx->dtls_handler.write = send_to_peer;
  ctx->dtls_handler.read = get_from_peer;
  ctx->dtls_handler.event = event;

  dtls_set_handler(ctx->dtls_context, &ctx->dtls_handler);

#ifdef NETSTACK_CONF_WITH_IPV6
  memcpy(&ctx->my_addr.in6_addr, my_addr, sizeof(ctx->my_addr.in6_addr));
#else
  memcpy(&ctx->my_addr.in_addr, my_addr, sizeof(ctx->my_addr.in_addr));
#endif
  ctx->my_port = port;

  ctx->process = PROCESS_CURRENT();
  ctx->is_used = 1;
  PRINTF("Secure listening on port %u\n", port);

  return ctx;
}
/*---------------------------------------------------------------------------*/
void
coap_context_set_key_handlers(coap_context_t *ctx,
			      dtls_get_psk_info_t get_psk_info,
			      dtls_get_ecdsa_key_t get_ecdsa_key,
			      dtls_verify_ecdsa_key_t verify_ecdsa_key)
{
  if (!ctx) {
    return;
  }

#ifdef DTLS_PSK
  ctx->dtls_handler.get_psk_info = get_psk_info;
#endif /* DTLS_PSK */
#ifdef DTLS_ECC
  ctx->dtls_handler.get_ecdsa_key = get_ecdsa_key;
  ctx->dtls_handler.verify_ecdsa_key = verify_ecdsa_key;
#endif /* DTLS_ECC */
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
  if(coap_ctx->buf && uip_udp_conn(coap_ctx->buf) != NULL) {
    uip_udp_remove(uip_udp_conn(coap_ctx->buf));
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

#ifdef NETSTACK_CONF_WITH_IPV6
  memcpy(&coap_ctx->addr.in6_addr, addr, sizeof(coap_ctx->addr.in6_addr));
  coap_ctx->addr.family = AF_INET6;
#else
  memcpy(&coap_ctx->addr.in_addr, addr, sizeof(coap_ctx->addr.in_addr));
  coap_ctx->addr.family = AF_INET;
#endif
  coap_ctx->port = port;

  coap_ctx->net_ctx = net_context_get(IPPROTO_UDP,
				      (const struct net_addr *)&coap_ctx->addr,
				      coap_ctx->port,
				      (const struct net_addr *)&coap_ctx->my_addr,
				      coap_ctx->my_port);
  if (!coap_ctx->net_ctx) {
    PRINTF("%s: Cannot get network context\n", __FUNCTION__);
    return 0;
  }

  uip_ipaddr_copy(&session.addr.ipaddr, addr);
  session.addr.port = UIP_HTONS(port);
  session.size = sizeof(session.addr);
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

  uip_ipaddr_copy(&sn.addr.ipaddr, addr);
  sn.addr.port = port;
  sn.size = sizeof(sn.addr);
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

  process_start(&coap_context_process, NULL, NULL);
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(coap_context_process, ev, data, buf, user_data)
{
  PROCESS_BEGIN();

  PROCESS_PAUSE();

  while(1) {
    PROCESS_WAIT_EVENT();

    if(ev == tcpip_event) {
      /* The data reading in Zephyr is done in coap_context_wait_data() or
       * equiv. so this code block is no-op atm.
       */
    }
  } /* while (1) */

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/

#else /* WITH_DTLS */

/*---------------------------------------------------------------------------*/
coap_context_t *
coap_context_new(uip_ipaddr_t *my_addr, uint16_t port)
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

#ifdef NETSTACK_CONF_WITH_IPV6
  memcpy(&ctx->my_addr.in6_addr, my_addr, sizeof(ctx->my_addr.in6_addr));
#else
  memcpy(&ctx->my_addr.in_addr, my_addr, sizeof(ctx->my_addr.in_addr));
#endif
  ctx->my_port = port;

  ctx->is_used = 1;
  PRINTF("Listening on port %u\n", port);

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
  if(coap_ctx->buf && uip_udp_conn(coap_ctx->buf) != NULL) {
    uip_udp_remove(uip_udp_conn(coap_ctx->buf));
  }

  net_context_put(coap_ctx->net_ctx);

  coap_ctx->is_used = 0;
}
/*---------------------------------------------------------------------------*/
int coap_context_reply(coap_context_t *ctx, struct net_buf *buf)
{
  int max_data_len, ret;

  max_data_len = IP_BUF_MAX_DATA - UIP_IPUDPH_LEN;

  PRINTF("%s: reply to peer data %p len %d\n", __FUNCTION__,
	 ip_buf_appdata(buf), ip_buf_appdatalen(buf));

  if (ip_buf_appdatalen(buf) > max_data_len) {
    PRINTF("%s: too much (%d bytes) data to send (max %d bytes)\n",
	  __FUNCTION__, ip_buf_appdatalen(buf), max_data_len);
    ip_buf_unref(buf);
    ret = -EINVAL;
    goto out;
  }

  if (net_reply(ctx->net_ctx, buf)) {
    ip_buf_unref(buf);
  }
out:
  return ret;
}
/*---------------------------------------------------------------------------*/
int coap_context_wait_data(coap_context_t *coap_ctx, int32_t ticks)
{
  struct net_buf *buf;

  buf = net_receive(coap_ctx->net_ctx, ticks);
  if (buf) {
    PRINTF("coap-context: got message from ");
    PRINT6ADDR(&UIP_IP_BUF(buf)->srcipaddr);
    PRINTF(":%d %u bytes\n", uip_ntohs(UIP_UDP_BUF(buf)->srcport),
	   uip_appdatalen(buf));

    PRINTF("Received data appdata %p appdatalen %d\n",
	   ip_buf_appdata(buf), ip_buf_appdatalen(buf));

    coap_ctx->buf = buf;

    coap_engine_receive(coap_ctx);

    return 1;
  }

  return 0;
}
/*---------------------------------------------------------------------------*/
static int coap_context_send(coap_context_t *ctx, struct net_buf *buf)
{
  int max_data_len, ret;

  max_data_len = IP_BUF_MAX_DATA - UIP_IPUDPH_LEN;

  PRINTF("%s: send to peer data %p len %d\n", __FUNCTION__,
	 ip_buf_appdata(buf), ip_buf_appdatalen(buf));

  if (ip_buf_appdatalen(buf) > max_data_len) {
    PRINTF("%s: too much (%d bytes) data to send (max %d bytes)\n",
	   __FUNCTION__, ip_buf_appdatalen(buf), max_data_len);
    ip_buf_unref(buf);
    ret = -EINVAL;
    goto out;
  }

  ret = net_send(buf);
  if (ret < 0) {
    PRINT("%s: sending %d bytes failed\n", __FUNCTION__,
	  ip_buf_appdatalen(buf));
    ip_buf_unref(buf);
  }

out:
  return ret;
}
/*---------------------------------------------------------------------------*/
int
coap_context_send_message(coap_context_t *coap_ctx,
                          uip_ipaddr_t *addr, uint16_t port,
                          const uint8_t *data, uint16_t length)
{
  int ret;

  if(coap_ctx == NULL || coap_ctx->is_used == 0) {
    PRINTF("coap-context: can not send on non-used context\n");
    return 0;
  }

  ip_buf_appdatalen(coap_ctx->buf) = length;

  if (!uip_udp_conn(coap_ctx->buf)) {
    /* Normal send, not a reply */
    ret = coap_context_send(coap_ctx, coap_ctx->buf);

  } else {

    /* We need to set the uIP buffer lengths in net_buf properly as
     * otherwise the final buffer length in
     * net_init.c:udp_prepare_and_send() will be usually incorrect
     * (uip_len() will be length of the received packet). So by setting
     * uip_len() to 0 here, we let the sending in net_init.c to
     * set the send buffer length correctly.
     */
    uip_len(coap_ctx->buf) = 0;
    memcpy(ip_buf_appdata(coap_ctx->buf), data, length);

    ret = coap_context_reply(coap_ctx, coap_ctx->buf);
  }

  coap_ctx->buf = NULL;
  return ret;
}
/*---------------------------------------------------------------------------*/
int
coap_context_connect(coap_context_t *coap_ctx, uip_ipaddr_t *addr, uint16_t port)
{
  if(coap_ctx == NULL || coap_ctx->is_used == 0) {
    return 0;
  }

#ifdef NETSTACK_CONF_WITH_IPV6
  memcpy(&coap_ctx->addr.in6_addr, addr, sizeof(coap_ctx->addr.in6_addr));
  coap_ctx->addr.family = AF_INET6;
#else
  memcpy(&coap_ctx->addr.in_addr, addr, sizeof(coap_ctx->addr.in_addr));
  coap_ctx->addr.family = AF_INET;
#endif
  coap_ctx->port = port;

  coap_ctx->net_ctx = net_context_get(IPPROTO_UDP,
				      (const struct net_addr *)&coap_ctx->addr,
				      coap_ctx->port,
				      (struct net_addr *)&coap_ctx->my_addr,
				      coap_ctx->my_port);
  if (!coap_ctx->net_ctx) {
    PRINTF("%s: Cannot get network context\n", __FUNCTION__);
    return 0;
  }

  PRINTF("coap-context: normal connect to [");
  PRINT6ADDR(addr);
  PRINTF("]:%u\n", port);

  return 1;
}

#endif /* WITH_DTLS */

/*---------------------------------------------------------------------------*/
int coap_context_listen(coap_context_t *coap_ctx, uip_ipaddr_t *peer_addr,
			uint16_t peer_port)
{
  if(coap_ctx == NULL || coap_ctx->is_used == 0) {
    return 0;
  }

#ifdef NETSTACK_CONF_WITH_IPV6
  memcpy(&coap_ctx->addr.in6_addr, peer_addr, sizeof(coap_ctx->addr.in6_addr));
  coap_ctx->addr.family = AF_INET6;
#else
  memcpy(&coap_ctx->addr.in_addr, peer_addr, sizeof(coap_ctx->addr.in_addr));
  coap_ctx->addr.family = AF_INET;
#endif
  coap_ctx->port = peer_port;

  coap_ctx->net_ctx = net_context_get(IPPROTO_UDP,
				      (const struct net_addr *)&coap_ctx->addr,
				      coap_ctx->port,
				      (struct net_addr *)&coap_ctx->my_addr,
				      coap_ctx->my_port);
  if (!coap_ctx->net_ctx) {
    PRINTF("%s: Cannot get network context\n", __FUNCTION__);
    return 0;
  }

  return 1;
}
