/** @file
 @brief Network buffers

 Network data is passed between application and IP stack via
 a net_buf struct.
 */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Intel Corporation nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <nanokernel.h>
#include <toolchain.h>
#include <string.h>
#include <stdint.h>

#include <net/net_core.h>
#include <net/net_buf.h>
#include <net/net_ip.h>

#include "ip/uip.h"

extern struct net_tuple *net_context_get_tuple(struct net_context *context);

/* Available (free) buffers queue */
#ifndef NET_BUF_RX_SIZE
#if CONFIG_NET_BUF_RX_SIZE > 0
#define NET_BUF_RX_SIZE		CONFIG_NET_BUF_RX_SIZE
#else
#define NET_BUF_RX_SIZE		1
#endif
#endif

#ifndef NET_BUF_TX_SIZE
#if CONFIG_NET_BUF_TX_SIZE > 0
#define NET_BUF_TX_SIZE		CONFIG_NET_BUF_TX_SIZE
#else
#define NET_BUF_TX_SIZE		1
#endif
#endif

static struct net_buf		rx_buffers[NET_BUF_RX_SIZE];
static struct net_buf		tx_buffers[NET_BUF_TX_SIZE];
static struct nano_fifo		free_rx_bufs;
static struct nano_fifo		free_tx_bufs;

/* Available (free) MAC buffers queue */
#ifndef NET_NUM_MAC_BUFS
/* Default value is 13 (receiving side) which means that max. UDP data
 * (1232 bytes) can be received in one go. In sending side we need 1
 * mbuf + some extras.
 */
#define NET_NUM_MAC_BUFS		16
#endif
static struct net_mbuf		mac_buffers[NET_NUM_MAC_BUFS];
static struct nano_fifo		free_mbufs;

static inline const char *type2str(enum net_buf_type type)
{
	switch (type) {
	case NET_BUF_RX:
		return "RX";
	case NET_BUF_TX:
		return "TX";
	}

	return NULL;
}

#ifdef DEBUG_NET_BUFS
static struct net_buf *net_buf_get_reserve_debug(enum net_buf_type type,
						 uint16_t reserve_head,
						 const char *caller,
						 int line)
#else
static struct net_buf *net_buf_get_reserve(enum net_buf_type type,
					   uint16_t reserve_head)
#endif
{
	struct net_buf *buf = NULL;

	switch (type) {
	case NET_BUF_RX:
		buf = nano_fifo_get(&free_rx_bufs);
		break;
	case NET_BUF_TX:
		buf = nano_fifo_get(&free_tx_bufs);
		break;
	}

	if (!buf) {
#ifdef DEBUG_NET_BUFS
		NET_ERR("Failed to get free %s buffer (%s():%d)\n",
			type2str(type), caller, line);
#else
		NET_ERR("Failed to get free %s buffer\n", type2str(type));
#endif
		return NULL;
	}

	buf->data = buf->buf + reserve_head;
	buf->len = 0;
	buf->type = type;

	NET_BUF_CHECK_IF_IN_USE(buf);

#ifdef DEBUG_NET_BUFS
	NET_DBG("%s buf %p reserve %u inuse %d (%s():%d)\n",
		type2str(type),	buf, reserve_head, buf->in_use,
		caller, line);
#else
	NET_DBG("%s buf %p reserve %u inuse %d\n", type2str(type), buf,
		reserve_head, buf->in_use);
#endif
	buf->in_use = true;

	return buf;
}

#ifdef DEBUG_NET_BUFS
struct net_buf *net_buf_get_reserve_rx_debug(uint16_t reserve_head, const char *caller, int line)
#else
struct net_buf *net_buf_get_reserve_rx(uint16_t reserve_head)
#endif
{
#ifdef DEBUG_NET_BUFS
	return net_buf_get_reserve_debug(NET_BUF_RX, reserve_head,
					 caller, line);
#else
	return net_buf_get_reserve(NET_BUF_RX, reserve_head);
#endif
}

#ifdef DEBUG_NET_BUFS
struct net_buf *net_buf_get_reserve_tx_debug(uint16_t reserve_head, const char *caller, int line)
#else
struct net_buf *net_buf_get_reserve_tx(uint16_t reserve_head)
#endif
{
#ifdef DEBUG_NET_BUFS
	return net_buf_get_reserve_debug(NET_BUF_TX, reserve_head,
					 caller, line);
#else
	return net_buf_get_reserve(NET_BUF_TX, reserve_head);
#endif
}

#ifdef DEBUG_NET_BUFS
static struct net_buf *net_buf_get_debug(enum net_buf_type type,
					 struct net_context *context,
					 const char *caller, int line)
#else
static struct net_buf *net_buf_get(enum net_buf_type type,
				   struct net_context *context)
#endif
{
	struct net_buf *buf;
	struct net_tuple *tuple;
	uint16_t reserve = 0;

	tuple = net_context_get_tuple(context);
	if (!tuple) {
		return NULL;
	}

	switch (tuple->ip_proto) {
	case IPPROTO_UDP:
		reserve = UIP_IPUDPH_LEN;
		break;
	case IPPROTO_TCP:
		reserve = UIP_IPTCPH_LEN;
		break;
	case IPPROTO_ICMPV6:
		reserve = UIP_IPICMPH_LEN;
		break;
	}

#ifdef DEBUG_NET_BUFS
	buf = net_buf_get_reserve_debug(type, reserve, caller, line);
#else
	buf = net_buf_get_reserve(type, reserve);
#endif
	if (!buf) {
		return buf;
	}

	buf->context = context;

	return buf;
}

#ifdef DEBUG_NET_BUFS
struct net_buf *net_buf_get_rx_debug(struct net_context *context,
				     const char *caller, int line)
#else
struct net_buf *net_buf_get_rx(struct net_context *context)
#endif
{
#ifdef DEBUG_NET_BUFS
	return net_buf_get_debug(NET_BUF_RX, context, caller, line);
#else
	return net_buf_get(NET_BUF_RX, context);
#endif
}

#ifdef DEBUG_NET_BUFS
struct net_buf *net_buf_get_tx_debug(struct net_context *context,
				     const char *caller, int line)
#else
struct net_buf *net_buf_get_tx(struct net_context *context)
#endif
{
#ifdef DEBUG_NET_BUFS
	return net_buf_get_debug(NET_BUF_TX, context, caller, line);
#else
	return net_buf_get(NET_BUF_TX, context);
#endif
}

#ifdef DEBUG_NET_BUFS
void net_buf_put_debug(struct net_buf *buf, const char *caller, int line)
#else
void net_buf_put(struct net_buf *buf)
#endif
{
	if (!buf) {
#ifdef DEBUG_NET_BUFS
		NET_DBG("*** ERROR *** buf %p (%s():%d)\n", buf, caller, line);
#else
		NET_DBG("*** ERROR *** buf %p\n", buf);
#endif
		return;
	}

	NET_BUF_CHECK_IF_NOT_IN_USE(buf);

#ifdef DEBUG_NET_BUFS
	NET_DBG("%s buf %p inuse %d (%s():%d)\n", type2str(buf->type),
		buf, buf->in_use, caller, line);
#else
	NET_DBG("%s buf %p inuse %d\n", type2str(buf->type), buf, buf->in_use);
#endif

	buf->in_use = false;

	switch (buf->type) {
	case NET_BUF_RX:
		nano_fifo_put(&free_rx_bufs, buf);
		break;
	case NET_BUF_TX:
		nano_fifo_put(&free_tx_bufs, buf);
		break;
	}
}

uint8_t *net_buf_add(struct net_buf *buf, uint16_t len)
{
	uint8_t *tail = buf->data + buf->len;
	NET_BUF_CHECK_IF_NOT_IN_USE(buf);
	buf->len += len;
	return tail;
}

uint8_t *net_buf_push(struct net_buf *buf, uint16_t len)
{
	NET_BUF_CHECK_IF_NOT_IN_USE(buf);
	buf->data -= len;
	buf->len += len;
	return buf->data;
}

uint8_t *net_buf_pull(struct net_buf *buf, uint16_t len)
{
	NET_BUF_CHECK_IF_NOT_IN_USE(buf);
	buf->len -= len;
	return buf->data += len;
}

#ifdef DEBUG_NET_BUFS
struct net_mbuf *net_mbuf_get_reserve_debug(uint16_t reserve_head, const char *caller, int line)
#else
struct net_mbuf *net_mbuf_get_reserve(uint16_t reserve_head)
#endif
{
	struct net_mbuf *buf;

	buf = nano_fifo_get(&free_mbufs);
	if (!buf) {
#ifdef DEBUG_NET_BUFS
		NET_ERR("Failed to get free mac buffer (%s():%d)\n", caller, line);
#else
		NET_ERR("Failed to get free mac buffer\n");
#endif
		return NULL;
	}

	NET_BUF_CHECK_IF_IN_USE(buf);

#ifdef DEBUG_NET_BUFS
	NET_DBG("buf %p reserve %u inuse %d (%s():%d)\n", buf, reserve_head,
		buf->in_use, caller, line);
#else
	NET_DBG("buf %p reserve %u inuse %d\n", buf, reserve_head, buf->in_use);
#endif
	buf->in_use = true;

	return buf;
}

#ifdef DEBUG_NET_BUFS
void net_mbuf_put_debug(struct net_mbuf *buf, const char *caller, int line)
#else
void net_mbuf_put(struct net_mbuf *buf)
#endif
{
	if (!buf) {
#ifdef DEBUG_NET_BUFS
		NET_DBG("*** ERROR *** buf %p (%s():%d)\n", buf, caller, line);
#else
		NET_DBG("*** ERROR *** buf %p\n", buf);
#endif
		return;
	}

	NET_BUF_CHECK_IF_NOT_IN_USE(buf);

#ifdef DEBUG_NET_BUFS
	NET_DBG("buf %p inuse %d (%s():%d)\n", buf, buf->in_use, caller, line);
#else
	NET_DBG("buf %p inuse %d\n", buf, buf->in_use);
#endif

	buf->in_use = false;

	nano_fifo_put(&free_mbufs, buf);
}

static void net_mbuf_init(void)
{
	nano_fifo_init(&free_mbufs);

	for (int i = 0; i < NET_NUM_MAC_BUFS; i++) {
		nano_fifo_put(&free_mbufs, &mac_buffers[i]);
	}
}

void net_buf_init(void)
{
	int i;

	NET_DBG("Allocating %d RX and %d TX buffers\n",
		NET_BUF_RX_SIZE, NET_BUF_TX_SIZE);

	nano_fifo_init(&free_rx_bufs);
	nano_fifo_init(&free_tx_bufs);

	for (i = 0; i < NET_BUF_RX_SIZE; i++) {
		nano_fifo_put(&free_rx_bufs, &rx_buffers[i]);
	}

	for (i = 0; i < NET_BUF_TX_SIZE; i++) {
		nano_fifo_put(&free_tx_bufs, &tx_buffers[i]);
	}

	net_mbuf_init();
}
