/** @file
 @brief Network buffers for IP stack

 Network data is passed between components using nbuf.
 */

/*
 * Copyright (c) 2016 Intel Corporation
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

#if defined(CONFIG_NETWORK_IP_STACK_DEBUG_NET_BUF)
#define SYS_LOG_DOMAIN "net/nbuf"
#define SYS_LOG_LEVEL SYS_LOG_LEVEL_DEBUG
#endif

#include <nanokernel.h>
#include <toolchain.h>
#include <string.h>
#include <stdint.h>

#include <net/net_ip.h>
#include <net/buf.h>
#include <net/nbuf.h>

#include <net/net_core.h>

#if !defined(NET_DEBUG_NBUFS)
#undef NET_DBG
#define NET_DBG(...)
#endif

#include "net_private.h"

/* Available (free) buffers queue */
#if !defined(NBUF_RX_COUNT)
#if CONFIG_NET_NBUF_RX_COUNT > 0
#define NBUF_RX_COUNT		CONFIG_NET_NBUF_RX_COUNT
#else
#define NBUF_RX_COUNT		1
#endif
#endif

#if !defined(NBUF_TX_COUNT)
#if CONFIG_NET_NBUF_TX_COUNT > 0
#define NBUF_TX_COUNT		CONFIG_NET_NBUF_TX_COUNT
#else
#define NBUF_TX_COUNT		1
#endif
#endif

#if !defined(NBUF_DATA_COUNT)
#if CONFIG_NET_NBUF_DATA_COUNT > 0
#define NBUF_DATA_COUNT CONFIG_NET_NBUF_DATA_COUNT
#else
#define NBUF_DATA_COUNT 13
#endif
#endif

#if !defined(NBUF_DATA_LEN)
#if CONFIG_NET_NBUF_DATA_SIZE > 0
#define NBUF_DATA_LEN CONFIG_NET_NBUF_DATA_SIZE
#else
#define NBUF_DATA_LEN 128
#endif
#endif

#if defined(CONFIG_NET_TCP)
#define APP_PROTO_LEN NET_TCPH_LEN
#else
#if defined(CONFIG_NET_UDP)
#define APP_PROTO_LEN NET_UDPH_LEN
#else
#define APP_PROTO_LEN 0
#endif /* UDP */
#endif /* TCP */

#if defined(CONFIG_NET_IPV6)
#define IP_PROTO_LEN NET_IPV6H_LEN
#else
#if defined(CONFIG_NET_IPV4)
#define IP_PROTO_LEN NET_IPV4H_LEN
#else
#error "Either IPv6 or IPv4 needs to be selected."
#endif /* IPv4 */
#endif /* IPv6 */

#define EXTRA_PROTO_LEN NET_ICMPH_LEN

/* Make sure that IP + TCP/UDP header fit into one
 * fragment. This makes possible to cast a protocol header
 * struct into memory area.
 */
#if NBUF_DATA_LEN < (IP_PROTO_LEN + APP_PROTO_LEN)
#if defined(STRING2)
#undef STRING2
#endif
#if defined(STRING)
#undef STRING
#endif
#define STRING2(x) #x
#define STRING(x) STRING2(x)
#pragma message "Data len " STRING(NBUF_DATA_LEN)
#pragma message "Minimum len " STRING(IP_PROTO_LEN + APP_PROTO_LEN)
#error "Too small net_buf fragment size"
#endif

#if defined(NET_DEBUG_NBUFS)
static int num_free_rx_bufs = NBUF_RX_COUNT;
static int num_free_tx_bufs = NBUF_TX_COUNT;
static int num_free_data_bufs = NBUF_DATA_COUNT;

static inline void dec_free_rx_bufs(struct net_buf *buf)
{
	if (!buf) {
		return;
	}

	num_free_rx_bufs--;
	if (num_free_rx_bufs < 0) {
		NET_DBG("*** ERROR *** Invalid RX buffer count.");
		num_free_rx_bufs = 0;
	}
}

static inline void inc_free_rx_bufs(struct net_buf *buf)
{
	if (!buf) {
		return;
	}

	if (num_free_rx_bufs > NBUF_RX_COUNT) {
		num_free_rx_bufs = NBUF_RX_COUNT;
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
		NET_DBG("*** ERROR *** Invalid TX buffer count.");
		num_free_tx_bufs = 0;
	}
}

static inline void inc_free_tx_bufs(struct net_buf *buf)
{
	if (!buf) {
		return;
	}

	if (num_free_tx_bufs > NBUF_TX_COUNT) {
		num_free_tx_bufs = NBUF_TX_COUNT;
	} else {
		num_free_tx_bufs++;
	}
}

static inline void dec_free_data_bufs(struct net_buf *buf)
{
	if (!buf) {
		return;
	}

	num_free_data_bufs--;
	if (num_free_data_bufs < 0) {
		NET_DBG("*** ERROR *** Invalid data buffer count.");
		num_free_data_bufs = 0;
	}
}

static inline void inc_free_data_bufs(struct net_buf *buf)
{
	if (!buf) {
		return;
	}

	if (num_free_data_bufs > NBUF_DATA_COUNT) {
		num_free_data_bufs = NBUF_DATA_COUNT;
	} else {
		num_free_data_bufs++;
	}
}

static inline int get_frees(enum net_nbuf_type type)
{
	switch (type) {
	case NET_NBUF_RX:
		return num_free_rx_bufs;
	case NET_NBUF_TX:
		return num_free_tx_bufs;
	case NET_NBUF_DATA:
		return num_free_data_bufs;
	}

	return 0xffffffff;
}

#define inc_free_rx_bufs_func inc_free_rx_bufs
#define inc_free_tx_bufs_func inc_free_tx_bufs
#define inc_free_data_bufs_func inc_free_data_bufs

#else /* NET_DEBUG_NBUFS */
#define dec_free_rx_bufs(...)
#define inc_free_rx_bufs(...)
#define dec_free_tx_bufs(...)
#define inc_free_tx_bufs(...)
#define dec_free_data_bufs(...)
#define inc_free_data_bufs(...)
#define inc_free_rx_bufs_func(...)
#define inc_free_tx_bufs_func(...)
#define inc_free_data_bufs_func(...)
#endif /* NET_DEBUG_NBUFS */

static struct nano_fifo free_rx_bufs;
static struct nano_fifo free_tx_bufs;
static struct nano_fifo free_data_bufs;

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

static inline void free_data_bufs_func(struct net_buf *buf)
{
	inc_free_data_bufs_func(buf);

	nano_fifo_put(buf->free, buf);
}

/* The RX and TX pools do not store any data. Only bearer / protocol
 * related data is stored here.
 */
static NET_BUF_POOL(rx_buffers, NBUF_RX_COUNT, 0,	\
		    &free_rx_bufs, free_rx_bufs_func,	\
		    sizeof(struct net_nbuf));
static NET_BUF_POOL(tx_buffers, NBUF_TX_COUNT, 0,	\
		    &free_tx_bufs, free_tx_bufs_func,	\
		    sizeof(struct net_nbuf));

/* The data fragment pool is for storing network data.
 * This pool does not need any user data because the rx/tx pool already
 * contains all the protocol/bearer specific information.
 */
static NET_BUF_POOL(data_buffers, NBUF_DATA_COUNT,	\
		    NBUF_DATA_LEN, &free_data_bufs,	\
		    free_data_bufs_func, 0);

static inline const char *type2str(enum net_nbuf_type type)
{
	switch (type) {
	case NET_NBUF_RX:
		return "RX";
	case NET_NBUF_TX:
		return "TX";
	case NET_NBUF_DATA:
		return "DATA";
	}

	return NULL;
}

#ifdef NET_DEBUG_NBUFS
static struct net_buf *net_nbuf_get_reserve_debug(enum net_nbuf_type type,
						  uint16_t reserve_head,
						  const char *caller,
						  int line)
#else
static struct net_buf *net_nbuf_get_reserve(enum net_nbuf_type type,
					    uint16_t reserve_head)
#endif
{
	struct net_buf *buf = NULL;

	/*
	 * The reserve_head variable in the function will tell
	 * the size of the link layer headers if there are any.
	 */
	switch (type) {
	case NET_NBUF_RX:
		buf = net_buf_get(&free_rx_bufs, 0);
		if (!buf) {
			return NULL;
		}

		NET_ASSERT(buf->ref);

		dec_free_rx_bufs(buf);
		net_nbuf_type(buf) = type;
		break;
	case NET_NBUF_TX:
		buf = net_buf_get(&free_tx_bufs, 0);
		if (!buf) {
			return NULL;
		}

		NET_ASSERT(buf->ref);

		dec_free_tx_bufs(buf);
		net_nbuf_type(buf) = type;
		break;
	case NET_NBUF_DATA:
		buf = net_buf_get(&free_data_bufs, 0);
		if (!buf) {
			return NULL;
		}

		NET_ASSERT(buf->ref);

		/* The buf->data will point to the start of the L3
		 * header (like IPv4 or IPv6 packet header) after the
		 * add() and pull().
		 */
		net_buf_add(buf, reserve_head);
		net_buf_pull(buf, reserve_head);

		dec_free_data_bufs(buf);
		break;
	default:
		NET_ERR("Invalid type %d for net_buf", type);
		return NULL;
	}

	if (!buf) {
#if defined(CONFIG_NETWORK_IP_STACK_DEBUG_NET_BUF)
#define PRINT_CYCLE (30 * sys_clock_ticks_per_sec)
		static uint32_t next_print;
		uint32_t curr = sys_tick_get_32();

		if (!next_print || (next_print < curr &&
				    (!((curr - next_print) > PRINT_CYCLE)))) {
			uint32_t new_print;

#ifdef NET_DEBUG_NBUFS
			NET_ERR("Failed to get free %s buffer (%s():%d)",
				type2str(type), caller, line);
#else
			NET_ERR("Failed to get free %s buffer",
				type2str(type));
#endif /* NET_DEBUG_NBUFS */

			new_print = curr + PRINT_CYCLE;
			if (new_print > curr) {
				next_print = new_print;
			} else {
				/* Overflow */
				next_print = PRINT_CYCLE -
					(0xffffffff - curr);
			}
		}
#endif
		return NULL;
	}

	NET_BUF_CHECK_IF_NOT_IN_USE(buf, buf->ref + 1);

#ifdef NET_DEBUG_NBUFS
	NET_DBG("%s [%d] buf %p reserve %u ref %d (%s():%d)",
		type2str(type), get_frees(type),
		buf, reserve_head, buf->ref, caller, line);
#else
	NET_DBG("%s buf %p reserve %u ref %d",
		type2str(type), buf, reserve_head, buf->ref);
#endif
	return buf;
}

#ifdef NET_DEBUG_NBUFS
struct net_buf *net_nbuf_get_reserve_rx_debug(uint16_t reserve_head,
					      const char *caller, int line)
#else
struct net_buf *net_nbuf_get_reserve_rx(uint16_t reserve_head)
#endif
{
#ifdef NET_DEBUG_NBUFS
	return net_nbuf_get_reserve_debug(NET_NBUF_RX, reserve_head,
				      caller, line);
#else
	return net_nbuf_get_reserve(NET_NBUF_RX, reserve_head);
#endif
}

#ifdef NET_DEBUG_NBUFS
struct net_buf *net_nbuf_get_reserve_tx_debug(uint16_t reserve_head,
					      const char *caller, int line)
#else
struct net_buf *net_nbuf_get_reserve_tx(uint16_t reserve_head)
#endif
{
#ifdef NET_DEBUG_NBUFS
	return net_nbuf_get_reserve_debug(NET_NBUF_TX, reserve_head,
				      caller, line);
#else
	return net_nbuf_get_reserve(NET_NBUF_TX, reserve_head);
#endif
}

#ifdef NET_DEBUG_NBUFS
struct net_buf *net_nbuf_get_reserve_data_debug(uint16_t reserve_head,
						const char *caller, int line)
#else
struct net_buf *net_nbuf_get_reserve_data(uint16_t reserve_head)
#endif
{
#ifdef NET_DEBUG_NBUFS
	return net_nbuf_get_reserve_debug(NET_NBUF_DATA, reserve_head,
				      caller, line);
#else
	return net_nbuf_get_reserve(NET_NBUF_DATA, reserve_head);
#endif
}

#ifdef NET_DEBUG_NBUFS
static struct net_buf *net_nbuf_get_debug(enum net_nbuf_type type,
					  struct net_context *context,
					  const char *caller, int line)
#else
static struct net_buf *net_nbuf_get(enum net_nbuf_type type,
				    struct net_context *context)
#endif
{
	struct net_buf *buf;
	int16_t reserve = 0;

	if (type == NET_NBUF_DATA) {
		reserve = NBUF_DATA_LEN -
			net_if_get_mtu(net_context_get_iface(context));
		if (reserve < 0) {
			NET_ERR("MTU %d bigger than fragment size %d",
				net_if_get_mtu(net_context_get_iface(context)),
				NBUF_DATA_LEN);
			return NULL;
		}
	}

#ifdef NET_DEBUG_NBUFS
	buf = net_nbuf_get_reserve_debug(type, (uint16_t)reserve, caller, line);
#else
	buf = net_nbuf_get_reserve(type, (uint16_t)reserve);
#endif
	if (!buf) {
		return buf;
	}

	if (type != NET_NBUF_DATA) {
		net_nbuf_context(buf) = context;
		net_nbuf_ll_reserve(buf) = (uint16_t)reserve;
	}

	return buf;
}

#ifdef NET_DEBUG_NBUFS
struct net_buf *net_nbuf_get_rx_debug(struct net_context *context,
				      const char *caller, int line)
#else
struct net_buf *net_nbuf_get_rx(struct net_context *context)
#endif
{
#ifdef NET_DEBUG_NBUFS
	return net_nbuf_get_debug(NET_NBUF_RX, context, caller, line);
#else
	return net_nbuf_get(NET_NBUF_RX, context);
#endif
}

#ifdef NET_DEBUG_NBUFS
struct net_buf *net_nbuf_get_tx_debug(struct net_context *context,
				      const char *caller, int line)
#else
struct net_buf *net_nbuf_get_tx(struct net_context *context)
#endif
{
#ifdef NET_DEBUG_NBUFS
	return net_nbuf_get_debug(NET_NBUF_TX, context, caller, line);
#else
	return net_nbuf_get(NET_NBUF_TX, context);
#endif
}

#ifdef NET_DEBUG_NBUFS
struct net_buf *net_nbuf_get_data_debug(struct net_context *context,
					const char *caller, int line)
#else
struct net_buf *net_nbuf_get_data(struct net_context *context)
#endif
{
#ifdef NET_DEBUG_NBUFS
	return net_nbuf_get_debug(NET_NBUF_DATA, context, caller, line);
#else
	return net_nbuf_get(NET_NBUF_DATA, context);
#endif
}

#ifdef NET_DEBUG_NBUFS
void net_nbuf_unref_debug(struct net_buf *buf, const char *caller, int line)
#else
void net_nbuf_unref(struct net_buf *buf)
#endif
{
	struct net_buf *frag;

	if (!buf) {
#ifdef NET_DEBUG_NBUFS
		NET_DBG("*** ERROR *** buf %p (%s():%d)", buf, caller, line);
#else
		NET_DBG("*** ERROR *** buf %p", buf);
#endif
		return;
	}

	if (!buf->ref) {
#ifdef NET_DEBUG_NBUFS
		NET_DBG("*** ERROR *** buf %p is freed already (%s():%d)",
			buf, caller, line);
#else
		NET_DBG("*** ERROR *** buf %p is freed already", buf);
#endif
		return;
	}

	if (buf->user_data_size) {
#ifdef NET_DEBUG_NBUFS
		NET_DBG("%s [%d] buf %p ref %d frags %p (%s():%d)",
			type2str(net_nbuf_type(buf)),
			get_frees(net_nbuf_type(buf)),
			buf, buf->ref - 1, buf->frags, caller, line);
#else
		NET_DBG("%s buf %p ref %d frags %p",
			type2str(net_nbuf_type(buf)), buf, buf->ref - 1,
			buf->frags);
#endif
	} else {
#ifdef NET_DEBUG_NBUFS
		NET_DBG("%s [%d] buf %p ref %d frags %p (%s():%d)",
			type2str(NET_NBUF_DATA),
			get_frees(NET_NBUF_DATA),
			buf, buf->ref - 1, buf->frags, caller, line);
#else
		NET_DBG("%s buf %p ref %d frags %p",
			type2str(NET_NBUF_DATA), buf, buf->ref - 1,
			buf->frags);
#endif
	}

	/* Remove the fragment list elements first, otherwise we
	 * have a memory leak. But only if we are to be remove the
	 * buffer.
	 */
	frag = buf->frags;
	while (!(buf->ref - 1) && frag) {
		struct net_buf *next = frag->frags;

		net_buf_frag_del(buf, frag);

#ifdef NET_DEBUG_NBUFS
		NET_DBG("%s [%d] buf %p ref %d frags %p (%s():%d)",
			type2str(NET_NBUF_DATA),
			get_frees(NET_NBUF_DATA),
			frag, frag->ref - 1, frag->frags, caller, line);
#else
		NET_DBG("%s buf %p ref %d frags %p",
			type2str(NET_NBUF_DATA), frag, frag->ref - 1,
			frag->frags);
#endif
		net_buf_unref(frag);

		frag = next;
	}

	net_buf_unref(buf);
}

#ifdef NET_DEBUG_NBUFS
struct net_buf *net_nbuf_ref_debug(struct net_buf *buf, const char *caller,
				   int line)
#else
struct net_buf *net_nbuf_ref(struct net_buf *buf)
#endif
{
	if (!buf) {
#ifdef NET_DEBUG_NBUFS
		NET_DBG("*** ERROR *** buf %p (%s():%d)", buf, caller, line);
#else
		NET_DBG("*** ERROR *** buf %p", buf);
#endif
		return NULL;
	}

	if (buf->user_data_size) {
#ifdef NET_DEBUG_NBUFS
		NET_DBG("%s [%d] buf %p ref %d (%s():%d)",
			type2str(net_nbuf_type(buf)),
			get_frees(net_nbuf_type(buf)),
			buf, buf->ref + 1, caller, line);
#else
		NET_DBG("%s buf %p ref %d",
			type2str(net_nbuf_type(buf)), buf, buf->ref + 1);
#endif
	} else {
#ifdef NET_DEBUG_NBUFS
		NET_DBG("%s buf %p ref %d (%s():%d)",
			type2str(NET_NBUF_DATA),
			buf, buf->ref + 1, caller, line);
#else
		NET_DBG("%s buf %p ref %d",
			type2str(NET_NBUF_DATA), buf, buf->ref + 1);
#endif
	}

	return net_buf_ref(buf);
}

void net_nbuf_init(void)
{
	NET_DBG("Allocating %d RX (%d bytes), %d TX (%d bytes) "
		"and %d data (%d bytes) buffers",
		NBUF_RX_COUNT, sizeof(rx_buffers),
		NBUF_TX_COUNT, sizeof(tx_buffers),
		NBUF_DATA_COUNT, sizeof(data_buffers));

	net_buf_pool_init(rx_buffers);
	net_buf_pool_init(tx_buffers);
	net_buf_pool_init(data_buffers);
}
