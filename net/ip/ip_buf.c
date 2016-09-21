/** @file
 @brief Network buffers for IP stack

 IP data is passed between application and IP stack via
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
#include <net/buf.h>
#include <net/ip_buf.h>
#include <net/net_ip.h>

#include "ip/uip.h"

#if !defined(CONFIG_NETWORK_IP_STACK_DEBUG_NET_BUF)
#undef NET_DBG
#define NET_DBG(...)
#endif

extern struct net_tuple *net_context_get_tuple(struct net_context *context);
extern void *net_context_get_internal_connection(struct net_context *context);

/* Available (free) buffers queue */
#ifndef IP_BUF_RX_SIZE
#if CONFIG_IP_BUF_RX_SIZE > 0
#define IP_BUF_RX_SIZE		CONFIG_IP_BUF_RX_SIZE
#else
#define IP_BUF_RX_SIZE		1
#endif
#endif

#ifndef IP_BUF_TX_SIZE
#if CONFIG_IP_BUF_TX_SIZE > 0
#define IP_BUF_TX_SIZE		CONFIG_IP_BUF_TX_SIZE
#else
#define IP_BUF_TX_SIZE		1
#endif
#endif

#ifdef DEBUG_IP_BUFS
static int num_free_rx_bufs = IP_BUF_RX_SIZE;
static int num_free_tx_bufs = IP_BUF_TX_SIZE;

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

	if (num_free_rx_bufs > IP_BUF_RX_SIZE) {
		num_free_rx_bufs = IP_BUF_RX_SIZE;
	} else {
		num_free_rx_bufs++;
	}
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

	if (num_free_tx_bufs > IP_BUF_TX_SIZE) {
		num_free_tx_bufs = IP_BUF_TX_SIZE;
	} else {
		num_free_tx_bufs++;
	}
}

static inline int get_frees(enum ip_buf_type type)
{
	switch (type) {
	case IP_BUF_RX:
		return num_free_rx_bufs;
	case IP_BUF_TX:
		return num_free_tx_bufs;
	}

	return 0xffffffff;
}

#define inc_free_rx_bufs_func inc_free_rx_bufs
#define inc_free_tx_bufs_func inc_free_tx_bufs

#else
#define dec_free_rx_bufs(...)
#define inc_free_rx_bufs(...)
#define dec_free_tx_bufs(...)
#define inc_free_tx_bufs(...)
#define inc_free_rx_bufs_func(...)
#define inc_free_tx_bufs_func(...)
#endif

static struct nano_fifo free_rx_bufs;
static struct nano_fifo free_tx_bufs;

static inline void free_rx_bufs_func(struct net_buf *buf)
{
	inc_free_rx_bufs_func(buf);

	nano_fifo_put(buf->free, buf);
}

static inline void free_tx_bufs_func(struct net_buf *buf)
{
	inc_free_tx_bufs_func(buf);

	nano_fifo_put(buf->free, buf);
}

static NET_BUF_POOL(rx_buffers, IP_BUF_RX_SIZE, IP_BUF_MAX_DATA, \
		    &free_rx_bufs, free_rx_bufs_func,		 \
		    sizeof(struct ip_buf));
static NET_BUF_POOL(tx_buffers, IP_BUF_TX_SIZE, IP_BUF_MAX_DATA, \
		    &free_tx_bufs, free_tx_bufs_func, \
		    sizeof(struct ip_buf));

static inline const char *type2str(enum ip_buf_type type)
{
	switch (type) {
	case IP_BUF_RX:
		return "RX";
	case IP_BUF_TX:
		return "TX";
	}

	return NULL;
}

#ifdef DEBUG_IP_BUFS
static struct net_buf *ip_buf_get_reserve_debug(enum ip_buf_type type,
						uint16_t reserve_head,
						const char *caller,
						int line)
#else
static struct net_buf *ip_buf_get_reserve(enum ip_buf_type type,
					  uint16_t reserve_head)
#endif
{
	struct net_buf *buf = NULL;

	/* Note that we do not reserve any space in front of the
	 * buffer so buf->data points to first byte of the IP header.
	 * This is done like this so that IP stack works the same
	 * way as BT and 802.15.4 stacks.
	 *
	 * The reserve_head variable in the function will tell
	 * the size of the IP + other headers if there are any.
	 * That variable is only used to calculate the pointer
	 * where the application data starts.
	 */
	switch (type) {
	case IP_BUF_RX:
		buf = net_buf_get(&free_rx_bufs, 0);
		dec_free_rx_bufs(buf);
		break;
	case IP_BUF_TX:
		buf = net_buf_get(&free_tx_bufs, 0);
		dec_free_tx_bufs(buf);
		break;
	}

	if (!buf) {
#ifdef DEBUG_IP_BUFS
		NET_ERR("Failed to get free %s buffer (%s():%d)\n",
			type2str(type), caller, line);
#else
		NET_ERR("Failed to get free %s buffer\n", type2str(type));
#endif
		return NULL;
	}

	ip_buf_type(buf) = type;
	ip_buf_appdata(buf) = buf->data + reserve_head;
	ip_buf_appdatalen(buf) = 0;
	ip_buf_reserve(buf) = reserve_head;
	net_buf_add(buf, reserve_head);

	NET_BUF_CHECK_IF_NOT_IN_USE(buf);

#ifdef DEBUG_IP_BUFS
	NET_DBG("%s [%d] buf %p reserve %u ref %d (%s():%d)\n",
		type2str(type), get_frees(type),
		buf, reserve_head, buf->ref, caller, line);
#else
	NET_DBG("%s buf %p reserve %u ref %d\n", type2str(type), buf,
		reserve_head, buf->ref);
#endif
	return buf;
}

#ifdef DEBUG_IP_BUFS
struct net_buf *ip_buf_get_reserve_rx_debug(uint16_t reserve_head, const char *caller, int line)
#else
struct net_buf *ip_buf_get_reserve_rx(uint16_t reserve_head)
#endif
{
#ifdef DEBUG_IP_BUFS
	return ip_buf_get_reserve_debug(IP_BUF_RX, reserve_head,
					caller, line);
#else
	return ip_buf_get_reserve(IP_BUF_RX, reserve_head);
#endif
}

#ifdef DEBUG_IP_BUFS
struct net_buf *ip_buf_get_reserve_tx_debug(uint16_t reserve_head, const char *caller, int line)
#else
struct net_buf *ip_buf_get_reserve_tx(uint16_t reserve_head)
#endif
{
#ifdef DEBUG_IP_BUFS
	return ip_buf_get_reserve_debug(IP_BUF_TX, reserve_head,
					caller, line);
#else
	return ip_buf_get_reserve(IP_BUF_TX, reserve_head);
#endif
}

#ifdef DEBUG_IP_BUFS
static struct net_buf *ip_buf_get_debug(enum ip_buf_type type,
					struct net_context *context,
					const char *caller, int line)
#else
static struct net_buf *ip_buf_get(enum ip_buf_type type,
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
		reserve = UIP_IPUDPH_LEN + UIP_LLH_LEN;
		break;
	case IPPROTO_TCP:
		reserve = UIP_IPTCPH_LEN + UIP_LLH_LEN;
		break;
	case IPPROTO_ICMPV6:
		reserve = UIP_IPICMPH_LEN + UIP_LLH_LEN;
		break;
	}

#ifdef DEBUG_IP_BUFS
	buf = ip_buf_get_reserve_debug(type, reserve, caller, line);
#else
	buf = ip_buf_get_reserve(type, reserve);
#endif
	if (!buf) {
		return buf;
	}

	ip_buf_context(buf) = context;
	uip_set_conn(buf) = net_context_get_internal_connection(context);

	return buf;
}

#ifdef DEBUG_IP_BUFS
struct net_buf *ip_buf_get_rx_debug(struct net_context *context,
				     const char *caller, int line)
#else
struct net_buf *ip_buf_get_rx(struct net_context *context)
#endif
{
#ifdef DEBUG_IP_BUFS
	return ip_buf_get_debug(IP_BUF_RX, context, caller, line);
#else
	return ip_buf_get(IP_BUF_RX, context);
#endif
}

#ifdef DEBUG_IP_BUFS
struct net_buf *ip_buf_get_tx_debug(struct net_context *context,
				     const char *caller, int line)
#else
struct net_buf *ip_buf_get_tx(struct net_context *context)
#endif
{
#ifdef DEBUG_IP_BUFS
	return ip_buf_get_debug(IP_BUF_TX, context, caller, line);
#else
	return ip_buf_get(IP_BUF_TX, context);
#endif
}

#ifdef DEBUG_IP_BUFS
void ip_buf_unref_debug(struct net_buf *buf, const char *caller, int line)
#else
void ip_buf_unref(struct net_buf *buf)
#endif
{
	if (!buf) {
#ifdef DEBUG_IP_BUFS
		NET_DBG("*** ERROR *** buf %p (%s():%d)\n", buf, caller, line);
#else
		NET_DBG("*** ERROR *** buf %p\n", buf);
#endif
		return;
	}

	if (!buf->ref) {
#ifdef DEBUG_IP_BUFS
		NET_DBG("*** ERROR *** buf %p is freed already (%s():%d)\n",
			buf, caller, line);
#else
		NET_DBG("*** ERROR *** buf %p is freed already\n", buf);
#endif
		return;
	}

#ifdef DEBUG_IP_BUFS
	NET_DBG("%s [%d] buf %p ref %d (%s():%d)\n",
		type2str(ip_buf_type(buf)), get_frees(ip_buf_type(buf)),
		buf, buf->ref - 1, caller, line);
#else
	NET_DBG("%s buf %p ref %d\n",
		type2str(ip_buf_type(buf)), buf, buf->ref - 1);
#endif

	net_buf_unref(buf);
}

#ifdef DEBUG_IP_BUFS
struct net_buf *ip_buf_ref_debug(struct net_buf *buf, const char *caller, int line)
#else
struct net_buf *ip_buf_ref(struct net_buf *buf)
#endif
{
	if (!buf) {
#ifdef DEBUG_IP_BUFS
		NET_DBG("*** ERROR *** buf %p (%s():%d)\n", buf, caller, line);
#else
		NET_DBG("*** ERROR *** buf %p\n", buf);
#endif
		return NULL;
	}

#ifdef DEBUG_IP_BUFS
	NET_DBG("%s [%d] buf %p ref %d (%s():%d)\n",
		type2str(ip_buf_type(buf)), get_frees(ip_buf_type(buf)),
		buf, buf->ref + 1, caller, line);
#else
	NET_DBG("%s buf %p ref %d\n",
		type2str(ip_buf_type(buf)), buf, buf->ref + 1);
#endif

	return net_buf_ref(buf);
}

void ip_buf_init(void)
{
	NET_DBG("Allocating %d RX and %d TX buffers for IP stack\n",
		IP_BUF_RX_SIZE, IP_BUF_TX_SIZE);

	net_buf_pool_init(rx_buffers);
	net_buf_pool_init(tx_buffers);
}
