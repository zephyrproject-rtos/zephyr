/** @file
 @brief Network buffers

 Network data is passed between application and IP stack via
 a net_buf struct.
 */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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
static int num_free_rx_bufs = NET_BUF_RX_SIZE;
static int num_free_tx_bufs = NET_BUF_TX_SIZE;
static int num_free_mbufs = NET_NUM_MAC_BUFS;

static inline void dec_free_rx_bufs(struct net_buf *buf)
{
	if (!buf) {
		return;
	}

	num_free_rx_bufs--;
	if (num_free_rx_bufs < 0) {
		NET_DBG("*** ERROR *** Invalid RX buffer count.\n");
		num_free_rx_bufs = 0;
	}
}

static inline void inc_free_rx_bufs(struct net_buf *buf)
{
	if (!buf) {
		return;
	}

	num_free_rx_bufs++;
}

static inline void dec_free_tx_bufs(struct net_buf *buf)
{
	if (!buf) {
		return;
	}

	num_free_tx_bufs--;
	if (num_free_tx_bufs < 0) {
		NET_DBG("*** ERROR *** Invalid TX buffer count.\n");
		num_free_tx_bufs = 0;
	}
}

static inline void inc_free_tx_bufs(struct net_buf *buf)
{
	if (!buf) {
		return;
	}

	num_free_tx_bufs++;
}

static inline int get_frees(enum net_buf_type type)
{
	switch (type) {
	case NET_BUF_RX:
		return num_free_rx_bufs;
	case NET_BUF_TX:
		return num_free_tx_bufs;
	}

	return 0xffffffff;
}

static inline void dec_free_mbufs(struct net_mbuf *buf)
{
	if (!buf) {
		return;
	}

	num_free_mbufs--;
	if (num_free_mbufs < 0) {
		NET_DBG("*** ERROR *** Invalid L2 buffer count.\n");
		num_free_mbufs = 0;
	}
}

static inline void inc_free_mbufs(struct net_mbuf *buf)
{
	if (!buf) {
		return;
	}

	num_free_mbufs++;
}

static inline int get_free_mbufs(void)
{
	return num_free_mbufs;
}
#else
#define dec_free_rx_bufs(...)
#define inc_free_rx_bufs(...)
#define dec_free_tx_bufs(...)
#define inc_free_tx_bufs(...)
#define dec_free_mbufs(...)
#define inc_free_mbufs(...)
#define get_free_mbufs(...)
#endif

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
		dec_free_rx_bufs(buf);
		break;
	case NET_BUF_TX:
		buf = nano_fifo_get(&free_tx_bufs);
		dec_free_tx_bufs(buf);
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
	NET_DBG("%s [%d] buf %p reserve %u inuse %d (%s():%d)\n",
		type2str(type), get_frees(type),
		buf, reserve_head, buf->in_use, caller, line);
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
	NET_DBG("%s [%d] buf %p inuse %d (%s():%d)\n", type2str(buf->type),
		get_frees(buf->type) + 1, buf, buf->in_use, caller, line);
#else
	NET_DBG("%s buf %p inuse %d\n", type2str(buf->type), buf, buf->in_use);
#endif

	buf->in_use = false;

	switch (buf->type) {
	case NET_BUF_RX:
		nano_fifo_put(&free_rx_bufs, buf);
		inc_free_rx_bufs(buf);
		break;
	case NET_BUF_TX:
		nano_fifo_put(&free_tx_bufs, buf);
		inc_free_tx_bufs(buf);
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

	dec_free_mbufs(buf);

	NET_BUF_CHECK_IF_IN_USE(buf);

#ifdef DEBUG_NET_BUFS
	NET_DBG("[%d] buf %p reserve %u inuse %d (%s():%d)\n",
		get_free_mbufs(), buf, reserve_head, buf->in_use,
		caller, line);
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
	NET_DBG("[%d] buf %p inuse %d (%s():%d)\n",
		get_free_mbufs() + 1, buf, buf->in_use, caller, line);
#else
	NET_DBG("buf %p inuse %d\n", buf, buf->in_use);
#endif

	buf->in_use = false;
	inc_free_mbufs(buf);

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
