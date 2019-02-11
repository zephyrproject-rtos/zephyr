/** @file
 @brief Network packet buffers for IP stack

 Network data is passed between components using net_pkt.
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(net_pkt, CONFIG_NET_PKT_LOG_LEVEL);

/* This enables allocation debugging but does not print so much output
 * as that can slow things down a lot.
 */
#undef NET_LOG_LEVEL
#if defined(CONFIG_NET_DEBUG_NET_PKT_ALLOC)
#define NET_LOG_LEVEL 5
#else
#define NET_LOG_LEVEL CONFIG_NET_PKT_LOG_LEVEL
#endif

#include <kernel.h>
#include <toolchain.h>
#include <string.h>
#include <zephyr/types.h>
#include <sys/types.h>

#include <misc/util.h>

#include <net/net_core.h>
#include <net/net_ip.h>
#include <net/buf.h>
#include <net/net_pkt.h>
#include <net/ethernet.h>
#include <net/udp.h>

#include "net_private.h"
#include "tcp_internal.h"

/* Find max header size of IP protocol (IPv4 or IPv6) */
#if defined(CONFIG_NET_IPV6) || defined(CONFIG_NET_RAW_MODE) || \
	defined(CONFIG_NET_SOCKETS_PACKET)
#define MAX_IP_PROTO_LEN NET_IPV6H_LEN
#else
#if defined(CONFIG_NET_IPV4)
#define MAX_IP_PROTO_LEN NET_IPV4H_LEN
#else
#if defined(CONFIG_NET_SOCKETS_CAN)
/* TODO: Use CAN MTU here instead of hard coded value. There was
 * weird circular dependency issue so this needs more TLC.
 */
#define MAX_IP_PROTO_LEN 8
#else
#error "Either IPv6 or IPv4 needs to be selected."
#endif /* SOCKETS_CAN */
#endif /* IPv4 */
#endif /* IPv6 */

/* Find max header size of "next" protocol (TCP, UDP or ICMP) */
#if defined(CONFIG_NET_TCP)
#define MAX_NEXT_PROTO_LEN NET_TCPH_LEN
#else
#if defined(CONFIG_NET_UDP)
#define MAX_NEXT_PROTO_LEN NET_UDPH_LEN
#else
#if defined(CONFIG_NET_SOCKETS_CAN)
#define MAX_NEXT_PROTO_LEN 0
#else
/* If no TCP and no UDP, apparently we still want pings to work. */
#define MAX_NEXT_PROTO_LEN NET_ICMPH_LEN
#endif /* SOCKETS_CAN */
#endif /* UDP */
#endif /* TCP */

/* Make sure that IP + TCP/UDP/ICMP headers fit into one fragment. This
 * makes possible to cast a fragment pointer to protocol header struct.
 */
#if CONFIG_NET_BUF_DATA_SIZE < (MAX_IP_PROTO_LEN + MAX_NEXT_PROTO_LEN)
#if defined(STRING2)
#undef STRING2
#endif
#if defined(STRING)
#undef STRING
#endif
#define STRING2(x) #x
#define STRING(x) STRING2(x)
#pragma message "Data len " STRING(CONFIG_NET_BUF_DATA_SIZE)
#pragma message "Minimum len " STRING(MAX_IP_PROTO_LEN + MAX_NEXT_PROTO_LEN)
#error "Too small net_buf fragment size"
#endif

#if CONFIG_NET_PKT_RX_COUNT <= 0
#error "Minimum value for CONFIG_NET_PKT_RX_COUNT is 1"
#endif

#if CONFIG_NET_PKT_TX_COUNT <= 0
#error "Minimum value for CONFIG_NET_PKT_TX_COUNT is 1"
#endif

#if CONFIG_NET_BUF_RX_COUNT <= 0
#error "Minimum value for CONFIG_NET_BUF_RX_COUNT is 1"
#endif

#if CONFIG_NET_BUF_TX_COUNT <= 0
#error "Minimum value for CONFIG_NET_BUF_TX_COUNT is 1"
#endif

K_MEM_SLAB_DEFINE(rx_pkts, sizeof(struct net_pkt), CONFIG_NET_PKT_RX_COUNT, 4);
K_MEM_SLAB_DEFINE(tx_pkts, sizeof(struct net_pkt), CONFIG_NET_PKT_TX_COUNT, 4);

#if defined(CONFIG_NET_BUF_FIXED_DATA_SIZE)

NET_BUF_POOL_FIXED_DEFINE(rx_bufs, CONFIG_NET_BUF_RX_COUNT,
			  CONFIG_NET_BUF_DATA_SIZE, NULL);
NET_BUF_POOL_FIXED_DEFINE(tx_bufs, CONFIG_NET_BUF_TX_COUNT,
			  CONFIG_NET_BUF_DATA_SIZE, NULL);

#else /* !CONFIG_NET_BUF_FIXED_DATA_SIZE */

NET_BUF_POOL_VAR_DEFINE(rx_bufs, CONFIG_NET_BUF_RX_COUNT,
			CONFIG_NET_BUF_DATA_POOL_SIZE, NULL);
NET_BUF_POOL_VAR_DEFINE(tx_bufs, CONFIG_NET_BUF_TX_COUNT,
			CONFIG_NET_BUF_DATA_POOL_SIZE, NULL);

#endif /* CONFIG_NET_BUF_FIXED_DATA_SIZE */

/* Allocation tracking is only available if separately enabled */
#if defined(CONFIG_NET_DEBUG_NET_PKT_ALLOC)
struct net_pkt_alloc {
	union {
		struct net_pkt *pkt;
		struct net_buf *buf;
		void *alloc_data;
	};
	const char *func_alloc;
	const char *func_free;
	u16_t line_alloc;
	u16_t line_free;
	u8_t in_use;
	bool is_pkt;
};

#define MAX_NET_PKT_ALLOCS (CONFIG_NET_PKT_RX_COUNT + \
			    CONFIG_NET_PKT_TX_COUNT + \
			    CONFIG_NET_BUF_RX_COUNT + \
			    CONFIG_NET_BUF_TX_COUNT + \
			    CONFIG_NET_DEBUG_NET_PKT_EXTERNALS)

static struct net_pkt_alloc net_pkt_allocs[MAX_NET_PKT_ALLOCS];

static void net_pkt_alloc_add(void *alloc_data, bool is_pkt,
			      const char *func, int line)
{
	int i;

	for (i = 0; i < MAX_NET_PKT_ALLOCS; i++) {
		if (net_pkt_allocs[i].in_use) {
			continue;
		}

		net_pkt_allocs[i].in_use = true;
		net_pkt_allocs[i].is_pkt = is_pkt;
		net_pkt_allocs[i].alloc_data = alloc_data;
		net_pkt_allocs[i].func_alloc = func;
		net_pkt_allocs[i].line_alloc = line;

		return;
	}
}

static void net_pkt_alloc_del(void *alloc_data, const char *func, int line)
{
	int i;

	for (i = 0; i < MAX_NET_PKT_ALLOCS; i++) {
		if (net_pkt_allocs[i].in_use &&
		    net_pkt_allocs[i].alloc_data == alloc_data) {
			net_pkt_allocs[i].func_free = func;
			net_pkt_allocs[i].line_free = line;
			net_pkt_allocs[i].in_use = false;

			return;
		}
	}
}

static bool net_pkt_alloc_find(void *alloc_data,
			       const char **func_free,
			       int *line_free)
{
	int i;

	for (i = 0; i < MAX_NET_PKT_ALLOCS; i++) {
		if (!net_pkt_allocs[i].in_use &&
		    net_pkt_allocs[i].alloc_data == alloc_data) {
			*func_free = net_pkt_allocs[i].func_free;
			*line_free = net_pkt_allocs[i].line_free;

			return true;
		}
	}

	return false;
}

void net_pkt_allocs_foreach(net_pkt_allocs_cb_t cb, void *user_data)
{
	int i;

	for (i = 0; i < MAX_NET_PKT_ALLOCS; i++) {
		if (net_pkt_allocs[i].in_use) {
			cb(net_pkt_allocs[i].is_pkt ?
			   net_pkt_allocs[i].pkt : NULL,
			   net_pkt_allocs[i].is_pkt ?
			   NULL : net_pkt_allocs[i].buf,
			   net_pkt_allocs[i].func_alloc,
			   net_pkt_allocs[i].line_alloc,
			   net_pkt_allocs[i].func_free,
			   net_pkt_allocs[i].line_free,
			   net_pkt_allocs[i].in_use,
			   user_data);
		}
	}

	for (i = 0; i < MAX_NET_PKT_ALLOCS; i++) {
		if (!net_pkt_allocs[i].in_use) {
			cb(net_pkt_allocs[i].is_pkt ?
			   net_pkt_allocs[i].pkt : NULL,
			   net_pkt_allocs[i].is_pkt ?
			   NULL : net_pkt_allocs[i].buf,
			   net_pkt_allocs[i].func_alloc,
			   net_pkt_allocs[i].line_alloc,
			   net_pkt_allocs[i].func_free,
			   net_pkt_allocs[i].line_free,
			   net_pkt_allocs[i].in_use,
			   user_data);
		}
	}
}
#else
#define net_pkt_alloc_add(alloc_data, is_pkt, func, line)
#define net_pkt_alloc_del(alloc_data, func, line)
#define net_pkt_alloc_find(alloc_data, func_free, line_free) false
#endif /* CONFIG_NET_DEBUG_NET_PKT_ALLOC */

#if defined(CONFIG_NET_DEBUG_NET_PKT_ALLOC) || \
	CONFIG_NET_PKT_LOG_LEVEL >= LOG_LEVEL_DBG

#define NET_FRAG_CHECK_IF_NOT_IN_USE(frag, ref)				\
	do {								\
		if (!(ref)) {                                           \
			NET_ERR("**ERROR** frag %p not in use (%s:%s():%d)", \
				frag, __FILE__, __func__, __LINE__);     \
		}                                                       \
	} while (0)

const char *net_pkt_slab2str(struct k_mem_slab *slab)
{
	if (slab == &rx_pkts) {
		return "RX";
	} else if (slab == &tx_pkts) {
		return "TX";
	}

	return "EXT";
}

const char *net_pkt_pool2str(struct net_buf_pool *pool)
{
	if (pool == &rx_bufs) {
		return "RDATA";
	} else if (pool == &tx_bufs) {
		return "TDATA";
	}

	return "EDATA";
}
#endif

#if defined(CONFIG_NET_DEBUG_NET_PKT_ALLOC) || \
	CONFIG_NET_PKT_LOG_LEVEL >= LOG_LEVEL_DBG
static inline s16_t get_frees(struct net_buf_pool *pool)
{
#if defined(CONFIG_NET_BUF_POOL_USAGE)
	return pool->avail_count;
#else
	return 0;
#endif
}
#endif

#if CONFIG_NET_PKT_LOG_LEVEL >= LOG_LEVEL_DBG
static inline const char *get_name(struct net_buf_pool *pool)
{
#if defined(CONFIG_NET_BUF_POOL_USAGE)
	return pool->name;
#else
	return "?";
#endif
}

static inline s16_t get_size(struct net_buf_pool *pool)
{
#if defined(CONFIG_NET_BUF_POOL_USAGE)
	return pool->pool_size;
#else
	return 0;
#endif
}

static inline const char *slab2str(struct k_mem_slab *slab)
{
	return net_pkt_slab2str(slab);
}

static inline const char *pool2str(struct net_buf_pool *pool)
{
	return net_pkt_pool2str(pool);
}

void net_pkt_print_frags(struct net_pkt *pkt)
{
	struct net_buf *frag;
	size_t total = 0;
	int count = 0, frag_size = 0;

	if (!pkt) {
		NET_INFO("pkt %p", pkt);
		return;
	}

	NET_INFO("pkt %p frags %p", pkt, pkt->frags);

	NET_ASSERT(pkt->frags);

	frag = pkt->frags;
	while (frag) {
		total += frag->len;

		frag_size = frag->size;

		NET_INFO("[%d] frag %p len %d size %d pool %p",
			 count, frag, frag->len, frag_size,
			 net_buf_pool_get(frag->pool_id));

		count++;

		frag = frag->frags;
	}

	NET_INFO("Total data size %zu, occupied %d bytes, utilization %zu%%",
		 total, count * frag_size,
		 count ? (total * 100) / (count * frag_size) : 0);
}
#endif /* CONFIG_NET_PKT_LOG_LEVEL >= LOG_LEVEL_DBG */

#if NET_LOG_LEVEL >= LOG_LEVEL_DBG

struct net_pkt *net_pkt_get_reserve_debug(struct k_mem_slab *slab,
					  s32_t timeout,
					  const char *caller,
					  int line)
#else /* #if NET_LOG_LEVEL >= LOG_LEVEL_DBG */
struct net_pkt *net_pkt_get_reserve(struct k_mem_slab *slab,
				    s32_t timeout)
#endif /* #if NET_LOG_LEVEL >= LOG_LEVEL_DBG */
{
	struct net_pkt *pkt;
	int ret;

	if (k_is_in_isr()) {
		ret = k_mem_slab_alloc(slab, (void **)&pkt, K_NO_WAIT);
	} else {
		ret = k_mem_slab_alloc(slab, (void **)&pkt, timeout);
	}

	if (ret) {
		return NULL;
	}

	(void)memset(pkt, 0, sizeof(struct net_pkt));

	pkt->atomic_ref = ATOMIC_INIT(1);
	pkt->slab = slab;

	if (IS_ENABLED(CONFIG_NET_IPV6)) {
		net_pkt_set_ipv6_next_hdr(pkt, 255);
	}

	net_pkt_set_priority(pkt, CONFIG_NET_TX_DEFAULT_PRIORITY);
	net_pkt_set_vlan_tag(pkt, NET_VLAN_TAG_UNSPEC);

	net_pkt_alloc_add(pkt, true, caller, line);

#if CONFIG_NET_PKT_LOG_LEVEL >= LOG_LEVEL_DBG
	NET_DBG("%s [%u] pkt %p ref %d (%s():%d)",
		slab2str(slab), k_mem_slab_num_free_get(slab),
		pkt, atomic_get(&pkt->atomic_ref), caller, line);
#endif
	return pkt;
}

#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
struct net_buf *net_pkt_get_reserve_data_debug(struct net_buf_pool *pool,
					       s32_t timeout,
					       const char *caller,
					       int line)
#else /* NET_LOG_LEVEL >= LOG_LEVEL_DBG */
struct net_buf *net_pkt_get_reserve_data(struct net_buf_pool *pool,
					 s32_t timeout)
#endif /* NET_LOG_LEVEL >= LOG_LEVEL_DBG */
{
	struct net_buf *frag;

	/*
	 * The reserve_head variable in the function will tell
	 * the size of the link layer headers if there are any.
	 */

	if (k_is_in_isr()) {
		frag = net_buf_alloc(pool, K_NO_WAIT);
	} else {
		frag = net_buf_alloc(pool, timeout);
	}

	if (!frag) {
		return NULL;
	}

#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
	NET_FRAG_CHECK_IF_NOT_IN_USE(frag, frag->ref + 1);
#endif

	net_pkt_alloc_add(frag, false, caller, line);

#if CONFIG_NET_PKT_LOG_LEVEL >= LOG_LEVEL_DBG
	NET_DBG("%s (%s) [%d] frag %p ref %d (%s():%d)",
		pool2str(pool), get_name(pool), get_frees(pool),
		frag, frag->ref, caller, line);
#endif

	return frag;
}

/* Get a fragment, try to figure out the pool from where to get
 * the data.
 */
#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
struct net_buf *net_pkt_get_frag_debug(struct net_pkt *pkt,
				       s32_t timeout,
				       const char *caller, int line)
#else
struct net_buf *net_pkt_get_frag(struct net_pkt *pkt,
				 s32_t timeout)
#endif
{
#if defined(CONFIG_NET_CONTEXT_NET_PKT_POOL)
	struct net_context *context;

	context = net_pkt_context(pkt);
	if (context && context->data_pool) {
#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
		return net_pkt_get_reserve_data_debug(context->data_pool(),
						      timeout, caller, line);
#else
		return net_pkt_get_reserve_data(context->data_pool(), timeout);
#endif /* NET_LOG_LEVEL >= LOG_LEVEL_DBG */
	}
#endif /* CONFIG_NET_CONTEXT_NET_PKT_POOL */

	if (pkt->slab == &rx_pkts) {
#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
		return net_pkt_get_reserve_rx_data_debug(timeout,
							 caller, line);
#else
		return net_pkt_get_reserve_rx_data(timeout);
#endif
	}

#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
	return net_pkt_get_reserve_tx_data_debug(timeout, caller, line);
#else
	return net_pkt_get_reserve_tx_data(timeout);
#endif
}

#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
struct net_pkt *net_pkt_get_reserve_rx_debug(s32_t timeout,
					     const char *caller, int line)
{
	return net_pkt_get_reserve_debug(&rx_pkts, timeout, caller, line);
}

struct net_pkt *net_pkt_get_reserve_tx_debug(s32_t timeout,
					     const char *caller, int line)
{
	return net_pkt_get_reserve_debug(&tx_pkts, timeout, caller, line);
}

struct net_buf *net_pkt_get_reserve_rx_data_debug(s32_t timeout,
						  const char *caller, int line)
{
	return net_pkt_get_reserve_data_debug(&rx_bufs, timeout, caller, line);
}

struct net_buf *net_pkt_get_reserve_tx_data_debug(s32_t timeout,
						  const char *caller, int line)
{
	return net_pkt_get_reserve_data_debug(&tx_bufs, timeout, caller, line);
}

#else /* NET_LOG_LEVEL >= LOG_LEVEL_DBG */

struct net_pkt *net_pkt_get_reserve_rx(s32_t timeout)
{
	return net_pkt_get_reserve(&rx_pkts, timeout);
}

struct net_pkt *net_pkt_get_reserve_tx(s32_t timeout)
{
	return net_pkt_get_reserve(&tx_pkts, timeout);
}

struct net_buf *net_pkt_get_reserve_rx_data(s32_t timeout)
{
	return net_pkt_get_reserve_data(&rx_bufs, timeout);
}

struct net_buf *net_pkt_get_reserve_tx_data(s32_t timeout)
{
	return net_pkt_get_reserve_data(&tx_bufs, timeout);
}

#endif /* NET_LOG_LEVEL >= LOG_LEVEL_DBG */


#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
static struct net_pkt *net_pkt_get_debug(struct k_mem_slab *slab,
					 struct net_context *context,
					 s32_t timeout,
					 const char *caller, int line)
#else
static struct net_pkt *net_pkt_get(struct k_mem_slab *slab,
				   struct net_context *context,
				   s32_t timeout)
#endif /* NET_LOG_LEVEL >= LOG_LEVEL_DBG */
{
	struct in6_addr *addr6 = NULL;
	struct net_if *iface;
	struct net_pkt *pkt;
	sa_family_t family;

	if (!context) {
		return NULL;
	}

	iface = net_context_get_iface(context);
	if (!iface) {
		NET_ERR("Context has no interface");
		return NULL;
	}

	if (net_context_get_family(context) == AF_INET6) {
		addr6 = &((struct sockaddr_in6 *) &context->remote)->sin6_addr;
	}

#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
	pkt = net_pkt_get_reserve_debug(slab, timeout, caller, line);
#else
	pkt = net_pkt_get_reserve(slab, timeout);
#endif
	if (!pkt) {
		return NULL;
	}

	net_pkt_set_context(pkt, context);
	net_pkt_set_iface(pkt, iface);
	family = net_context_get_family(context);
	net_pkt_set_family(pkt, family);

#if defined(CONFIG_NET_CONTEXT_PRIORITY) && (NET_TC_COUNT > 1)
	{
		u8_t prio;

		if (net_context_get_option(context, NET_OPT_PRIORITY, &prio,
					   NULL) == 0) {
			net_pkt_set_priority(pkt, prio);
		}
	}
#endif /* CONFIG_NET_CONTEXT_PRIORITY */

	if (slab != &rx_pkts) {
		u16_t iface_len, data_len;
		enum net_ip_protocol proto;

		iface_len = data_len = net_if_get_mtu(iface);

		if (IS_ENABLED(CONFIG_NET_IPV6) && family == AF_INET6) {
			data_len = MAX(iface_len, NET_IPV6_MTU);
			data_len -= NET_IPV6H_LEN;
		}

		if (IS_ENABLED(CONFIG_NET_IPV4) && family == AF_INET) {
			data_len = MAX(iface_len, NET_IPV4_MTU);
			data_len -= NET_IPV4H_LEN;
		}

		proto = net_context_get_ip_proto(context);

		if (IS_ENABLED(CONFIG_NET_TCP) && proto == IPPROTO_TCP) {
			data_len -= NET_TCPH_LEN;
			data_len -= NET_TCP_MAX_OPT_SIZE;
		}

		if (IS_ENABLED(CONFIG_NET_UDP) && proto == IPPROTO_UDP) {
			data_len -= NET_UDPH_LEN;
		}

		if (proto == IPPROTO_ICMP || proto == IPPROTO_ICMPV6) {
			data_len -= NET_ICMPH_LEN;
		}

		pkt->data_len = data_len;
	}

	return pkt;
}

#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
static struct net_buf *_pkt_get_data_debug(struct net_buf_pool *pool,
					   struct net_context *context,
					   s32_t timeout,
					   const char *caller, int line)
#else
static struct net_buf *_pkt_get_data(struct net_buf_pool *pool,
				     struct net_context *context,
				     s32_t timeout)
#endif /* NET_LOG_LEVEL >= LOG_LEVEL_DBG */
{
	struct in6_addr *addr6 = NULL;
	struct net_if *iface;
	struct net_buf *frag;

	if (!context) {
		return NULL;
	}

	iface = net_context_get_iface(context);
	if (!iface) {
		NET_ERR("Context has no interface");
		return NULL;
	}

	if (net_context_get_family(context) == AF_INET6) {
		addr6 = &((struct sockaddr_in6 *) &context->remote)->sin6_addr;
	}

#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
	frag = net_pkt_get_reserve_data_debug(pool, timeout, caller, line);
#else
	frag = net_pkt_get_reserve_data(pool, timeout);
#endif
	return frag;
}


#if defined(CONFIG_NET_CONTEXT_NET_PKT_POOL)
static inline struct k_mem_slab *get_tx_slab(struct net_context *context)
{
	if (context->tx_slab) {
		return context->tx_slab();
	}

	return NULL;
}

static inline struct net_buf_pool *get_data_pool(struct net_context *context)
{
	if (context->data_pool) {
		return context->data_pool();
	}

	return NULL;
}
#else
#define get_tx_slab(...) NULL
#define get_data_pool(...) NULL
#endif /* CONFIG_NET_CONTEXT_NET_PKT_POOL */

#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
struct net_pkt *net_pkt_get_rx_debug(struct net_context *context,
				     s32_t timeout,
				     const char *caller, int line)
{
	return net_pkt_get_debug(&rx_pkts, context, timeout, caller, line);
}

struct net_pkt *net_pkt_get_tx_debug(struct net_context *context,
				     s32_t timeout,
				     const char *caller, int line)
{
	struct k_mem_slab *slab = get_tx_slab(context);

	return net_pkt_get_debug(slab ? slab : &tx_pkts, context,
				 timeout, caller, line);
}

struct net_buf *net_pkt_get_data_debug(struct net_context *context,
				       s32_t timeout,
				       const char *caller, int line)
{
	struct net_buf_pool *pool = get_data_pool(context);

	return _pkt_get_data_debug(pool ? pool : &tx_bufs, context,
				   timeout, caller, line);
}

#else /* NET_LOG_LEVEL >= LOG_LEVEL_DBG */

struct net_pkt *net_pkt_get_rx(struct net_context *context, s32_t timeout)
{
	NET_ASSERT_INFO(context, "RX context not set");

	return net_pkt_get(&rx_pkts, context, timeout);
}

struct net_pkt *net_pkt_get_tx(struct net_context *context, s32_t timeout)
{
	struct k_mem_slab *slab;

	NET_ASSERT_INFO(context, "TX context not set");

	slab = get_tx_slab(context);

	return net_pkt_get(slab ? slab : &tx_pkts, context, timeout);
}

struct net_buf *net_pkt_get_data(struct net_context *context, s32_t timeout)
{
	struct net_buf_pool *pool;

	NET_ASSERT_INFO(context, "Data context not set");

	pool = get_data_pool(context);

	/* The context is not known in RX path so we can only have TX
	 * data here.
	 */
	return _pkt_get_data(pool ? pool : &tx_bufs, context, timeout);
}

#endif /* NET_LOG_LEVEL >= LOG_LEVEL_DBG */


#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
void net_pkt_unref_debug(struct net_pkt *pkt, const char *caller, int line)
{
	struct net_buf *frag;

#else
void net_pkt_unref(struct net_pkt *pkt)
{
#endif /* NET_LOG_LEVEL >= LOG_LEVEL_DBG */
	atomic_val_t ref;

	if (!pkt) {
#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
		NET_ERR("*** ERROR *** pkt %p (%s():%d)", pkt, caller, line);
#endif
		return;
	}

	do {
		ref = atomic_get(&pkt->atomic_ref);
		if (!ref) {
#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
			const char *func_freed;
			int line_freed;

			if (net_pkt_alloc_find(pkt, &func_freed, &line_freed)) {
				NET_ERR("*** ERROR *** pkt %p is freed already "
					"by %s():%d (%s():%d)",
					pkt, func_freed, line_freed, caller,
					line);
			} else {
				NET_ERR("*** ERROR *** pkt %p is freed already "
					"(%s():%d)", pkt, caller, line);
			}
#endif
			return;
		}
	} while (!atomic_cas(&pkt->atomic_ref, ref, ref - 1));

#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
#if CONFIG_NET_PKT_LOG_LEVEL >= LOG_LEVEL_DBG
	NET_DBG("%s [%d] pkt %p ref %d frags %p (%s():%d)",
		slab2str(pkt->slab), k_mem_slab_num_free_get(pkt->slab),
		pkt, ref - 1, pkt->frags, caller, line);
#endif
	if (ref > 1) {
		goto done;
	}

	frag = pkt->frags;
	while (frag) {
#if CONFIG_NET_PKT_LOG_LEVEL >= LOG_LEVEL_DBG
		NET_DBG("%s (%s) [%d] frag %p ref %d frags %p (%s():%d)",
			pool2str(net_buf_pool_get(frag->pool_id)),
			get_name(net_buf_pool_get(frag->pool_id)),
			get_frees(net_buf_pool_get(frag->pool_id)), frag,
			frag->ref - 1, frag->frags, caller, line);
#endif

		if (!frag->ref) {
			const char *func_freed;
			int line_freed;

			if (net_pkt_alloc_find(frag,
					       &func_freed, &line_freed)) {
				NET_ERR("*** ERROR *** frag %p is freed "
					"already by %s():%d (%s():%d)",
					frag, func_freed, line_freed,
					caller, line);
			} else {
				NET_ERR("*** ERROR *** frag %p is freed "
					"already (%s():%d)",
					frag, caller, line);
			}
		}

		net_pkt_alloc_del(frag, caller, line);

		frag = frag->frags;
	}

	net_pkt_alloc_del(pkt, caller, line);
done:
#endif /* NET_LOG_LEVEL >= LOG_LEVEL_DBG */

	if (ref > 1) {
		return;
	}

	if (pkt->frags) {
		net_pkt_frag_unref(pkt->frags);
	}

	if (IS_ENABLED(CONFIG_NET_DEBUG_NET_PKT_NON_FRAGILE_ACCESS)) {
		pkt->buffer = NULL;
		net_pkt_cursor_init(pkt);
	}

	k_mem_slab_free(pkt->slab, (void **)&pkt);
}

#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
struct net_pkt *net_pkt_ref_debug(struct net_pkt *pkt, const char *caller,
				  int line)
#else
struct net_pkt *net_pkt_ref(struct net_pkt *pkt)
#endif /* NET_LOG_LEVEL >= LOG_LEVEL_DBG */
{
	atomic_val_t ref;

	do {
		ref = pkt ? atomic_get(&pkt->atomic_ref) : 0;
		if (!ref) {
#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
			NET_ERR("*** ERROR *** pkt %p (%s():%d)",
				pkt, caller, line);
#endif
			return NULL;
		}
	} while (!atomic_cas(&pkt->atomic_ref, ref, ref + 1));

#if CONFIG_NET_PKT_LOG_LEVEL >= LOG_LEVEL_DBG
	NET_DBG("%s [%d] pkt %p ref %d (%s():%d)",
		slab2str(pkt->slab), k_mem_slab_num_free_get(pkt->slab),
		pkt, ref + 1, caller, line);
#endif


	return pkt;
}

#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
struct net_buf *net_pkt_frag_ref_debug(struct net_buf *frag,
				       const char *caller, int line)
#else
struct net_buf *net_pkt_frag_ref(struct net_buf *frag)
#endif /* NET_LOG_LEVEL >= LOG_LEVEL_DBG */
{
	if (!frag) {
#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
		NET_ERR("*** ERROR *** frag %p (%s():%d)", frag, caller, line);
#endif
		return NULL;
	}

#if CONFIG_NET_PKT_LOG_LEVEL >= LOG_LEVEL_DBG
	NET_DBG("%s (%s) [%d] frag %p ref %d (%s():%d)",
		pool2str(net_buf_pool_get(frag->pool_id)),
		get_name(net_buf_pool_get(frag->pool_id)),
		get_frees(net_buf_pool_get(frag->pool_id)),
		frag, frag->ref + 1, caller, line);
#endif

	return net_buf_ref(frag);
}


#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
void net_pkt_frag_unref_debug(struct net_buf *frag,
			      const char *caller, int line)
#else
void net_pkt_frag_unref(struct net_buf *frag)
#endif /* NET_LOG_LEVEL >= LOG_LEVEL_DBG */
{
	if (!frag) {
#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
		NET_ERR("*** ERROR *** frag %p (%s():%d)", frag, caller, line);
#endif
		return;
	}

#if CONFIG_NET_PKT_LOG_LEVEL >= LOG_LEVEL_DBG
	NET_DBG("%s (%s) [%d] frag %p ref %d (%s():%d)",
		pool2str(net_buf_pool_get(frag->pool_id)),
		get_name(net_buf_pool_get(frag->pool_id)),
		get_frees(net_buf_pool_get(frag->pool_id)),
		frag, frag->ref - 1, caller, line);
#endif

	if (frag->ref == 1) {
		net_pkt_alloc_del(frag, caller, line);
	}

	net_buf_unref(frag);
}

#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
struct net_buf *net_pkt_frag_del_debug(struct net_pkt *pkt,
				       struct net_buf *parent,
				       struct net_buf *frag,
				       const char *caller, int line)
#else
struct net_buf *net_pkt_frag_del(struct net_pkt *pkt,
				 struct net_buf *parent,
				 struct net_buf *frag)
#endif
{
#if CONFIG_NET_PKT_LOG_LEVEL >= LOG_LEVEL_DBG
	NET_DBG("pkt %p parent %p frag %p ref %u (%s:%d)",
		pkt, parent, frag, frag->ref, caller, line);
#endif

	if (pkt->frags == frag && !parent) {
		struct net_buf *tmp;

		if (frag->ref == 1) {
			net_pkt_alloc_del(frag, caller, line);
		}

		tmp = net_buf_frag_del(NULL, frag);
		pkt->frags = tmp;

		return tmp;
	}

	if (frag->ref == 1) {
		net_pkt_alloc_del(frag, caller, line);
	}

	return net_buf_frag_del(parent, frag);
}

#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
void net_pkt_frag_add_debug(struct net_pkt *pkt, struct net_buf *frag,
			    const char *caller, int line)
#else
void net_pkt_frag_add(struct net_pkt *pkt, struct net_buf *frag)
#endif
{
#if CONFIG_NET_PKT_LOG_LEVEL >= LOG_LEVEL_DBG
	NET_DBG("pkt %p frag %p (%s:%d)", pkt, frag, caller, line);
#endif

	/* We do not use net_buf_frag_add() as this one will refcount
	 * the frag once more if !pkt->frags
	 */
	if (!pkt->frags) {
		pkt->frags = frag;
		return;
	}

	net_buf_frag_insert(net_buf_frag_last(pkt->frags), frag);
}

#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
void net_pkt_frag_insert_debug(struct net_pkt *pkt, struct net_buf *frag,
			       const char *caller, int line)
#else
void net_pkt_frag_insert(struct net_pkt *pkt, struct net_buf *frag)
#endif
{
#if CONFIG_NET_PKT_LOG_LEVEL >= LOG_LEVEL_DBG
	NET_DBG("pkt %p frag %p (%s:%d)", pkt, frag, caller, line);
#endif

	net_buf_frag_last(frag)->frags = pkt->frags;
	pkt->frags = frag;
}

int net_frag_linear_copy(struct net_buf *dst, struct net_buf *src,
			 u16_t offset, u16_t len)
{
	u16_t to_copy;
	u16_t copied;

	if (dst->size < len) {
		return -ENOMEM;
	}

	/* find the right fragment to start copying from */
	while (src && offset >= src->len) {
		offset -= src->len;
		src = src->frags;
	}

	/* traverse the fragment chain until len bytes are copied */
	copied = 0U;
	while (src && len > 0) {
		to_copy = MIN(len, src->len - offset);
		memcpy(dst->data + copied, src->data + offset, to_copy);

		copied += to_copy;
		/* to_copy is always <= len */
		len -= to_copy;
		src = src->frags;
		/* after the first iteration, this value will be 0 */
		offset = 0U;
	}

	if (len > 0) {
		return -ENOMEM;
	}

	dst->len = copied;

	return 0;
}

bool net_pkt_compact(struct net_pkt *pkt)
{
	struct net_buf *frag, *prev;

	NET_DBG("Compacting data in pkt %p", pkt);

	frag = pkt->frags;
	prev = NULL;

	while (frag) {
		if (frag->frags) {
			/* Copy amount of data from next fragment to this
			 * fragment.
			 */
			size_t copy_len;

			copy_len = frag->frags->len;
			if (copy_len > net_buf_tailroom(frag)) {
				copy_len = net_buf_tailroom(frag);
			}

			memcpy(net_buf_tail(frag), frag->frags->data, copy_len);
			net_buf_add(frag, copy_len);

			memmove(frag->frags->data,
				frag->frags->data + copy_len,
				frag->frags->len - copy_len);

			frag->frags->len -= copy_len;

			/* Is there any more space in this fragment */
			if (net_buf_tailroom(frag)) {
				/* There is. This also means that the next
				 * fragment is empty as otherwise we could
				 * not have copied all data. Remove next
				 * fragment as there is no data in it any more.
				 */
				net_pkt_frag_del(pkt, frag, frag->frags);

				/* Then check next fragment */
				continue;
			}
		} else {
			if (!frag->len) {
				/* Remove the last fragment because there is no
				 * data in it.
				 */
				net_pkt_frag_del(pkt, prev, frag);

				break;
			}
		}

		prev = frag;
		frag = frag->frags;
	}

	return true;
}

static inline struct net_buf *net_pkt_append_allocator(s32_t timeout,
						       void *user_data)
{
	return net_pkt_get_frag((struct net_pkt *)user_data, timeout);
}

u16_t net_pkt_append(struct net_pkt *pkt, u16_t len, const u8_t *data,
		    s32_t timeout)
{
	struct net_buf *frag;
	struct net_context *ctx = NULL;
	u16_t max_len, appended;

	if (!pkt || !data || !len) {
		return 0;
	}

	if (!pkt->frags) {
		frag = net_pkt_get_frag(pkt, timeout);
		if (!frag) {
			return 0;
		}

		net_pkt_frag_add(pkt, frag);
	}

	if (pkt->slab != &rx_pkts) {
		ctx = net_pkt_context(pkt);
	}

	if (ctx) {
		/* Make sure we don't send more data in one packet than
		 * protocol or MTU allows when there is a context for the
		 * packet.
		 */
		max_len = pkt->data_len;

#if defined(CONFIG_NET_TCP)
		if (ctx->tcp && (ctx->tcp->send_mss < max_len)) {
			max_len = ctx->tcp->send_mss;
		}
#endif

		if (len > max_len) {
			len = max_len;
		}
	}

	appended = net_buf_append_bytes(net_buf_frag_last(pkt->frags),
					len, data, timeout,
					net_pkt_append_allocator, pkt);

	if (ctx) {
		pkt->data_len -= appended;
	}

	return appended;
}

/* Helper routine to retrieve single byte from fragment and move
 * offset. If required byte is last byte in fragment then return
 * next fragment and set offset = 0.
 */
static inline struct net_buf *net_frag_read_byte(struct net_buf *frag,
						 u16_t offset,
						 u16_t *pos,
						 u8_t *data)
{
	if (data) {
		*data = frag->data[offset];
	}

	*pos = offset + 1;

	if (*pos >= frag->len) {
		*pos = 0U;

		return frag->frags;
	}

	return frag;
}

/* Helper function to adjust offset in net_frag_read() call
 * if given offset is more than current fragment length.
 */
static inline struct net_buf *adjust_offset(struct net_buf *frag,
					    u16_t offset, u16_t *pos)
{
	if (!frag) {
		return NULL;
	}

	while (frag) {
		if (offset < frag->len) {
			*pos = offset;

			return frag;
		}

		offset -= frag->len;
		frag = frag->frags;
	}

	return NULL;
}

struct net_buf *net_frag_read(struct net_buf *frag, u16_t offset,
			      u16_t *pos, u16_t len, u8_t *data)
{
	u16_t copy = 0U;

	frag = adjust_offset(frag, offset, pos);
	if (!frag) {
		goto error;
	}

	while (len-- > 0 && frag) {
		if (data) {
			frag = net_frag_read_byte(frag, *pos,
						  pos, data + copy++);
		} else {
			frag = net_frag_read_byte(frag, *pos, pos, NULL);
		}

		/* Error: Still remaining length to be read, but no data. */
		if (!frag && len) {
			NET_ERR("Not enough data to read");
			goto error;
		}
	}

	return frag;

error:
	*pos = 0xffff;

	return NULL;
}

struct net_buf *net_frag_read_be16(struct net_buf *frag, u16_t offset,
				   u16_t *pos, u16_t *value)
{
	struct net_buf *ret_frag;
	u8_t v16[2];

	ret_frag = net_frag_read(frag, offset, pos, sizeof(u16_t), v16);

	*value = v16[0] << 8 | v16[1];

	return ret_frag;
}

struct net_buf *net_frag_read_be32(struct net_buf *frag, u16_t offset,
				   u16_t *pos, u32_t *value)
{
	struct net_buf *ret_frag;
	u8_t v32[4];

	ret_frag = net_frag_read(frag, offset, pos, sizeof(u32_t), v32);

	*value = v32[0] << 24 | v32[1] << 16 | v32[2] << 8 | v32[3];

	return ret_frag;
}

static inline struct net_buf *check_and_create_data(struct net_pkt *pkt,
						    struct net_buf *data,
						    s32_t timeout)
{
	struct net_buf *frag;

	if (data) {
		return data;
	}

	frag = net_pkt_get_frag(pkt, timeout);
	if (!frag) {
		return NULL;
	}

	net_pkt_frag_add(pkt, frag);

	return frag;
}

static inline struct net_buf *adjust_write_offset(struct net_pkt *pkt,
						  struct net_buf *frag,
						  u16_t offset,
						  u16_t *pos,
						  s32_t timeout)
{
	u16_t tailroom;

	do {
		frag = check_and_create_data(pkt, frag, timeout);
		if (!frag) {
			return NULL;
		}

		/* Offset is less than current fragment length, so new data
		 *  will start from this "offset".
		 */
		if (offset < frag->len) {
			*pos = offset;

			return frag;
		}

		/* Offset is equal to fragment length. If some tailroom exists,
		 * offset start from same fragment otherwise offset starts from
		 * beginning of next fragment.
		 */
		if (offset == frag->len) {
			if (net_buf_tailroom(frag)) {
				*pos = offset;

				return frag;
			}

			*pos = 0U;

			return check_and_create_data(pkt, frag->frags,
						     timeout);
		}

		/* If the offset is more than current fragment length, remove
		 * current fragment length and verify with tailroom in the
		 * current fragment. From here on create empty (space/fragments)
		 * to reach proper offset.
		 */
		if (offset > frag->len) {

			offset -= frag->len;
			tailroom = net_buf_tailroom(frag);

			if (offset < tailroom) {
				/* Create empty space */
				net_buf_add(frag, offset);

				*pos = frag->len;

				return frag;
			}

			if (offset == tailroom) {
				/* Create empty space */
				net_buf_add(frag, tailroom);

				*pos = 0U;

				return check_and_create_data(pkt,
							     frag->frags,
							     timeout);
			}

			if (offset > tailroom) {
				/* Creating empty space */
				net_buf_add(frag, tailroom);
				offset -= tailroom;

				frag = check_and_create_data(pkt,
							     frag->frags,
							     timeout);
			}
		}

	} while (1);

	return NULL;
}

struct net_buf *net_pkt_write(struct net_pkt *pkt, struct net_buf *frag,
			      u16_t offset, u16_t *pos,
			      u16_t len, u8_t *data,
			      s32_t timeout)
{
	if (!pkt) {
		NET_ERR("Invalid packet");
		goto error;
	}

	frag = adjust_write_offset(pkt, frag, offset, &offset, timeout);
	if (!frag) {
		NET_DBG("Failed to adjust offset (%u)", offset);
		goto error;
	}

	do {
		u16_t space = frag->size - net_buf_headroom(frag) - offset;
		u16_t count = MIN(len, space);
		int size_to_add;

		memcpy(frag->data + offset, data, count);

		/* If we are overwriting on already available space then need
		 * not to update the length, otherwise increase it.
		 */
		size_to_add = offset + count - frag->len;
		if (size_to_add > 0) {
			net_buf_add(frag, size_to_add);
		}

		len -= count;
		if (len == 0) {
			*pos = offset + count;

			return frag;
		}

		data += count;
		offset = 0U;
		frag = frag->frags;

		if (!frag) {
			frag = net_pkt_get_frag(pkt, timeout);
			if (!frag) {
				goto error;
			}

			net_pkt_frag_add(pkt, frag);
		}
	} while (1);

error:
	*pos = 0xffff;

	return NULL;
}

static inline bool insert_data(struct net_pkt *pkt, struct net_buf *frag,
			       struct net_buf *temp, u16_t offset,
			       u16_t len, u8_t *data,
			       s32_t timeout)
{
	struct net_buf *insert;

	do {
		u16_t count = MIN(len, net_buf_tailroom(frag));

		if (data) {
			/* Copy insert data */
			memcpy(frag->data + offset, data, count);
		} else {
			/* If there is no data, just clear the area */
			(void)memset(frag->data + offset, 0, count);
		}

		net_buf_add(frag, count);

		len -= count;
		if (len == 0) {
			/* Once insertion is done, then add the data if
			 * there is anything after original insertion
			 * offset.
			 */
			if (temp) {
				net_buf_frag_insert(frag, temp);
			}

			/* As we are creating temporary buffers to cache,
			 * compact the fragments to save space.
			 */
			net_pkt_compact(pkt);

			return true;
		}

		if (data) {
			data += count;
		}

		offset = 0U;

		insert = net_pkt_get_frag(pkt, timeout);
		if (!insert) {
			return false;
		}

		net_buf_frag_insert(frag, insert);
		frag = insert;

	} while (1);

	return false;
}

static inline struct net_buf *adjust_insert_offset(struct net_buf *frag,
						   u16_t offset,
						   u16_t *pos)
{
	if (!frag) {
		NET_ERR("Invalid fragment");
		return NULL;
	}

	while (frag) {
		if (offset == frag->len) {
			*pos = 0U;

			return frag->frags;
		}

		if (offset < frag->len) {
			*pos = offset;

			return frag;
		}

		if (offset > frag->len) {
			if (frag->frags) {
				offset -= frag->len;
				frag = frag->frags;
			} else {
				return NULL;
			}
		}
	}

	NET_ERR("Invalid offset, failed to adjust");

	return NULL;
}

bool net_pkt_insert(struct net_pkt *pkt, struct net_buf *frag,
		    u16_t offset, u16_t len, u8_t *data,
		    s32_t timeout)
{
	struct net_buf *temp = NULL;
	u16_t bytes;

	if (!pkt) {
		return false;
	}

	frag = adjust_insert_offset(frag, offset, &offset);
	if (!frag) {
		return false;
	}

	/* If there is any data after offset, store in temp fragment and
	 * add it after insertion is completed.
	 */
	bytes = frag->len - offset;
	if (bytes) {
		temp = net_pkt_get_frag(pkt, timeout);
		if (!temp) {
			return false;
		}

		memcpy(net_buf_add(temp, bytes), frag->data + offset, bytes);

		frag->len -= bytes;
	}

	/* Insert data into current(frag) fragment from "offset". */
	return insert_data(pkt, frag, temp, offset, len, data, timeout);
}

void net_pkt_get_info(struct k_mem_slab **rx,
		      struct k_mem_slab **tx,
		      struct net_buf_pool **rx_data,
		      struct net_buf_pool **tx_data)
{
	if (rx) {
		*rx = &rx_pkts;
	}

	if (tx) {
		*tx = &tx_pkts;
	}

	if (rx_data) {
		*rx_data = &rx_bufs;
	}

	if (tx_data) {
		*tx_data = &tx_bufs;
	}
}

#if defined(CONFIG_NET_DEBUG_NET_PKT_ALLOC)
void net_pkt_print(void)
{
	NET_DBG("TX %u RX %u RDATA %d TDATA %d",
		k_mem_slab_num_free_get(&tx_pkts),
		k_mem_slab_num_free_get(&rx_pkts),
		get_frees(&rx_bufs), get_frees(&tx_bufs));
}
#endif /* CONFIG_NET_DEBUG_NET_PKT_ALLOC */

struct net_buf *net_frag_get_pos(struct net_pkt *pkt,
				 u16_t offset,
				 u16_t *pos)
{
	struct net_buf *frag;

	frag = net_frag_skip(pkt->frags, offset, pos, 0);
	if (!frag) {
		return NULL;
	}

	return frag;
}

/* New allocator and API starts here */

#if defined(CONFIG_NET_BUF_FIXED_DATA_SIZE)

#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
static struct net_buf *pkt_alloc_buffer(struct net_buf_pool *pool,
					size_t size, s32_t timeout,
					const char *caller, int line)
#else
static struct net_buf *pkt_alloc_buffer(struct net_buf_pool *pool,
					size_t size, s32_t timeout)
#endif
{
	u32_t alloc_start = k_uptime_get_32();
	struct net_buf *first = NULL;
	struct net_buf *current = NULL;

	while (size) {
		struct net_buf *new;

		new = net_buf_alloc_fixed(pool, timeout);
		if (!new) {
			goto error;
		}

		if (!first && !current) {
			first = new;
		} else {
			current->frags = new;
		}

		current = new;
		if (current->size > size) {
			current->size = size;
		}

		size -= current->size;

		if (timeout != K_NO_WAIT && timeout != K_FOREVER) {
			u32_t diff = k_uptime_get_32() - alloc_start;

			timeout -= MIN(timeout, diff);
		}

#if CONFIG_NET_PKT_LOG_LEVEL >= LOG_LEVEL_DBG
		NET_FRAG_CHECK_IF_NOT_IN_USE(new, new->ref + 1);

		net_pkt_alloc_add(new, false, caller, line);

		NET_DBG("%s (%s) [%d] frag %p ref %d (%s():%d)",
			pool2str(pool), get_name(pool), get_frees(pool),
			new, new->ref, caller, line);
#endif
	}

	return first;
error:
	if (first) {
		net_buf_unref(first);
	}

	return NULL;
}

#else /* !CONFIG_NET_BUF_FIXED_DATA_SIZE */

#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
static struct net_buf *pkt_alloc_buffer(struct net_buf_pool *pool,
					size_t size, s32_t timeout,
					const char *caller, int line)
#else
static struct net_buf *pkt_alloc_buffer(struct net_buf_pool *pool,
					size_t size, s32_t timeout)
#endif
{
	struct net_buf *buf;

	buf = net_buf_alloc_len(pool, size, timeout);

#if CONFIG_NET_PKT_LOG_LEVEL >= LOG_LEVEL_DBG
	NET_FRAG_CHECK_IF_NOT_IN_USE(buf, buf->ref + 1);

	net_pkt_alloc_add(buf, false, caller, line);

	NET_DBG("%s (%s) [%d] frag %p ref %d (%s():%d)",
		pool2str(pool), get_name(pool), get_frees(pool),
		buf, buf->ref, caller, line);
#endif

	return buf;
}

#endif /* CONFIG_NET_BUF_FIXED_DATA_SIZE */

static size_t pkt_buffer_length(struct net_pkt *pkt,
				size_t size,
				enum net_ip_protocol proto,
				size_t existing)
{
	size_t max_len = net_if_get_mtu(net_pkt_iface(pkt));
	sa_family_t family = net_pkt_family(pkt);

	/* Family vs iface MTU */
	if (IS_ENABLED(CONFIG_NET_IPV6) && family == AF_INET6) {
		max_len = MAX(max_len, NET_IPV6_MTU);
	} else if (IS_ENABLED(CONFIG_NET_IPV4) && family == AF_INET) {
		max_len = MAX(max_len, NET_IPV4_MTU);
	} else { /* family == AF_UNSPEC */
#if defined (CONFIG_NET_L2_ETHERNET)
		if (net_if_l2(net_pkt_iface(pkt)) ==
		    &NET_L2_GET_NAME(ETHERNET)) {
			max_len += sizeof(struct net_eth_hdr);
		} else
#endif /* CONFIG_NET_L2_ETHERNET */
		{
			/* Other L2 are not checked as the pkt MTU in this case
			 * is based on the IP layer (IPv6 most of the time).
			 */
			max_len = size;
		}
	}

	max_len -= existing;

	return MIN(size, max_len);
}

static size_t pkt_estimate_headers_length(struct net_pkt *pkt,
					  sa_family_t family,
					  enum net_ip_protocol proto)
{
	size_t hdr_len = 0;

	if (family == AF_UNSPEC) {
		return  0;
	}

	/* Family header */
	if (IS_ENABLED(CONFIG_NET_IPV6) && family == AF_INET6) {
		hdr_len += NET_IPV6H_LEN;
	} else if (IS_ENABLED(CONFIG_NET_IPV4) && family == AF_INET) {
		hdr_len += NET_IPV4H_LEN;
	}

	/* + protocol header */
	if (IS_ENABLED(CONFIG_NET_TCP) && proto == IPPROTO_TCP) {
		hdr_len += NET_TCPH_LEN + NET_TCP_MAX_OPT_SIZE;
	} else if (IS_ENABLED(CONFIG_NET_UDP) && proto == IPPROTO_UDP) {
		hdr_len += NET_UDPH_LEN;
	} else if (proto == IPPROTO_ICMP || proto == IPPROTO_ICMPV6) {
		hdr_len += NET_ICMPH_LEN;
	}

	NET_DBG("HDRs length estimation %zu", hdr_len);

	return hdr_len;
}

static size_t pkt_get_size(struct net_pkt *pkt)
{
	struct net_buf *buf = pkt->buffer;
	size_t size = 0;

	while (buf) {
		size += buf->size;
		buf = buf->frags;
	}

	return size;
}

size_t net_pkt_available_buffer(struct net_pkt *pkt)
{
	if (!pkt) {
		return 0;
	}

	return pkt_get_size(pkt) - net_pkt_get_len(pkt);
}

size_t net_pkt_available_payload_buffer(struct net_pkt *pkt,
					enum net_ip_protocol proto)
{
	size_t hdr_len = 0;
	size_t len;

	if (!pkt) {
		return 0;
	}

	hdr_len = pkt_estimate_headers_length(pkt, net_pkt_family(pkt), proto);
	len = net_pkt_get_len(pkt);

	hdr_len = hdr_len <= len ? 0 : hdr_len - len;

	len = net_pkt_available_buffer(pkt) - hdr_len;

	return len;
}

void net_pkt_trim_buffer(struct net_pkt *pkt)
{
	struct net_buf *buf, *prev;

	buf = pkt->buffer;
	prev = buf;

	while (buf) {
		struct net_buf *next = buf->frags;

		if (!buf->len) {
			if (buf == pkt->buffer) {
				pkt->buffer = next;
			} else if (buf == prev->frags) {
				prev->frags = next;
			}

			buf->frags = NULL;
			net_buf_unref(buf);
		} else {
			prev = buf;
		}

		buf = next;
	}
}

#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
int net_pkt_alloc_buffer_debug(struct net_pkt *pkt,
			       size_t size,
			       enum net_ip_protocol proto,
			       s32_t timeout,
			       const char *caller,
			       int line)
#else
int net_pkt_alloc_buffer(struct net_pkt *pkt,
			 size_t size,
			 enum net_ip_protocol proto,
			 s32_t timeout)
#endif
{
	u32_t alloc_start = k_uptime_get_32();
	size_t alloc_len = 0;
	size_t hdr_len = 0;
	struct net_buf *buf;

	if (!size && proto == 0 && net_pkt_family(pkt) == AF_UNSPEC) {
		return 0;
	}

	if (k_is_in_isr()) {
		timeout = K_NO_WAIT;
	}

	/* Verifying existing buffer and take into account free space there */
	alloc_len = pkt_get_size(pkt) - net_pkt_get_len(pkt);
	if (!alloc_len) {
		/* In case of no free space, it will account for header
		 * space estimation
		 */
		hdr_len = pkt_estimate_headers_length(pkt,
						      net_pkt_family(pkt),
						      proto);
	}

	/* Calculate the maximum that can be allocated depending on size */
	alloc_len = pkt_buffer_length(pkt, size + hdr_len, proto, alloc_len);

	NET_DBG("Data allocation maximum size %zu (requested %zu)",
		alloc_len, size);

	if (timeout != K_NO_WAIT && timeout != K_FOREVER) {
		u32_t diff = k_uptime_get_32() - alloc_start;

		timeout -= MIN(timeout, diff);
	}

#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
	buf = pkt_alloc_buffer(pkt->slab == &tx_pkts ?
			       &tx_bufs : &rx_bufs,
			       alloc_len, timeout,
			       caller, line);
#else
	buf = pkt_alloc_buffer(pkt->slab == &tx_pkts ?
			       &tx_bufs : &rx_bufs,
			       alloc_len, timeout);
#endif

	if (!buf) {
		NET_ERR("Data buffer allocation failed.");
		return -ENOMEM;
	}

	net_pkt_append_buffer(pkt, buf);

	return 0;
}

#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
static struct net_pkt *pkt_alloc(struct k_mem_slab *slab, s32_t timeout,
				 const char *caller, int line)
#else
static struct net_pkt *pkt_alloc(struct k_mem_slab *slab, s32_t timeout)
#endif
{
	struct net_pkt *pkt;
	int ret;

	if (k_is_in_isr()) {
		timeout = K_NO_WAIT;
	}

	ret = k_mem_slab_alloc(slab, (void **)&pkt, timeout);
	if (ret) {
		return NULL;
	}

	memset(pkt, 0, sizeof(struct net_pkt));

	pkt->atomic_ref = ATOMIC_INIT(1);
	pkt->slab = slab;

	if (IS_ENABLED(CONFIG_NET_IPV6)) {
		net_pkt_set_ipv6_next_hdr(pkt, 255);
	}

	net_pkt_set_priority(pkt, CONFIG_NET_TX_DEFAULT_PRIORITY);
	net_pkt_set_vlan_tag(pkt, NET_VLAN_TAG_UNSPEC);

#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
	net_pkt_alloc_add(pkt, true, caller, line);
#endif

	net_pkt_cursor_init(pkt);

	return pkt;
}


#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
struct net_pkt *net_pkt_alloc_debug(s32_t timeout,
				    const char *caller, int line)
#else
struct net_pkt *net_pkt_alloc(s32_t timeout)
#endif
{
#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
	return pkt_alloc(&tx_pkts, timeout, caller, line);
#else
	return pkt_alloc(&tx_pkts, timeout);
#endif
}

#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
struct net_pkt *net_pkt_rx_alloc_debug(s32_t timeout,
				       const char *caller, int line)
#else
struct net_pkt *net_pkt_rx_alloc(s32_t timeout)
#endif
{
#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
	return pkt_alloc(&rx_pkts, timeout, caller, line);
#else
	return pkt_alloc(&rx_pkts, timeout);
#endif
}

#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
static struct net_pkt *pkt_alloc_on_iface(struct k_mem_slab *slab,
					  struct net_if *iface, s32_t timeout,
					  const char *caller, int line)
#else
static struct net_pkt *pkt_alloc_on_iface(struct k_mem_slab *slab,
					  struct net_if *iface, s32_t timeout)

#endif
{
	struct net_pkt *pkt;

#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
	pkt = pkt_alloc(slab, timeout, caller, line);
#else
	pkt = pkt_alloc(slab, timeout);
#endif

	if (pkt) {
		net_pkt_set_iface(pkt, iface);
	}

	return pkt;
}

#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
struct net_pkt *net_pkt_alloc_on_iface_debug(struct net_if *iface,
					     s32_t timeout,
					     const char *caller,
					     int line)
#else
struct net_pkt *net_pkt_alloc_on_iface(struct net_if *iface, s32_t timeout)
#endif
{
#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
	return pkt_alloc_on_iface(&tx_pkts, iface, timeout, caller, line);
#else
	return pkt_alloc_on_iface(&tx_pkts, iface, timeout);
#endif
}

#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
struct net_pkt *net_pkt_rx_alloc_on_iface_debug(struct net_if *iface,
						s32_t timeout,
						const char *caller,
						int line)
#else
struct net_pkt *net_pkt_rx_alloc_on_iface(struct net_if *iface, s32_t timeout)
#endif
{
#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
	return pkt_alloc_on_iface(&rx_pkts, iface, timeout, caller, line);
#else
	return pkt_alloc_on_iface(&rx_pkts, iface, timeout);
#endif
}

#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
static struct net_pkt *
pkt_alloc_with_buffer(struct k_mem_slab *slab,
		      struct net_if *iface,
		      size_t size,
		      sa_family_t family,
		      enum net_ip_protocol proto,
		      s32_t timeout,
		      const char *caller,
		      int line)
#else
static struct net_pkt *
pkt_alloc_with_buffer(struct k_mem_slab *slab,
		      struct net_if *iface,
		      size_t size,
		      sa_family_t family,
		      enum net_ip_protocol proto,
		      s32_t timeout)
#endif
{
	u32_t alloc_start = k_uptime_get_32();
	struct net_pkt *pkt;
	int ret;

	NET_DBG("On iface %p size %zu", iface, size);

#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
	pkt = pkt_alloc_on_iface(slab, iface, timeout, caller, line);
#else
	pkt = pkt_alloc_on_iface(slab, iface, timeout);
#endif

	if (!pkt) {
		return NULL;
	}

	net_pkt_set_family(pkt, family);

	if (timeout != K_NO_WAIT && timeout != K_FOREVER) {
		u32_t diff = k_uptime_get_32() - alloc_start;

		timeout -= MIN(timeout, diff);
	}

#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
	ret = net_pkt_alloc_buffer_debug(pkt, size, proto, timeout,
					 caller, line);
#else
	ret = net_pkt_alloc_buffer(pkt, size, proto, timeout);
#endif

	if (ret) {
		net_pkt_unref(pkt);
		return NULL;
	}

	return pkt;
}

#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
struct net_pkt *net_pkt_alloc_with_buffer_debug(struct net_if *iface,
						size_t size,
						sa_family_t family,
						enum net_ip_protocol proto,
						s32_t timeout,
						const char *caller,
						int line)
#else
struct net_pkt *net_pkt_alloc_with_buffer(struct net_if *iface,
					  size_t size,
					  sa_family_t family,
					  enum net_ip_protocol proto,
					  s32_t timeout)
#endif
{
#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
	return pkt_alloc_with_buffer(&tx_pkts, iface, size, family,
				     proto, timeout, caller, line);
#else
	return pkt_alloc_with_buffer(&tx_pkts, iface, size, family,
				     proto, timeout);
#endif
}

#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
struct net_pkt *net_pkt_rx_alloc_with_buffer_debug(struct net_if *iface,
						   size_t size,
						   sa_family_t family,
						   enum net_ip_protocol proto,
						   s32_t timeout,
						   const char *caller,
						   int line)
#else
struct net_pkt *net_pkt_rx_alloc_with_buffer(struct net_if *iface,
					     size_t size,
					     sa_family_t family,
					     enum net_ip_protocol proto,
					     s32_t timeout)
#endif
{
#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
	return pkt_alloc_with_buffer(&rx_pkts, iface, size, family,
					proto, timeout, caller, line);
#else
	return pkt_alloc_with_buffer(&rx_pkts, iface, size, family,
					proto, timeout);
#endif
}

void net_pkt_append_buffer(struct net_pkt *pkt, struct net_buf *buffer)
{
	if (!pkt->buffer) {
		pkt->buffer = buffer;
		net_pkt_cursor_init(pkt);
	} else {
		net_buf_frag_insert(net_buf_frag_last(pkt->buffer), buffer);
	}
}

void net_pkt_cursor_init(struct net_pkt *pkt)
{
	pkt->cursor.buf = pkt->buffer;
	if (pkt->cursor.buf) {
		pkt->cursor.pos = pkt->cursor.buf->data;
	} else {
		pkt->cursor.pos = NULL;
	}
}

static void pkt_cursor_jump(struct net_pkt *pkt, bool write)
{
	struct net_pkt_cursor *cursor = &pkt->cursor;

	cursor->buf = cursor->buf->frags;
	while (cursor->buf) {
		size_t len = write ? cursor->buf->size : cursor->buf->len;

		if (!len) {
			cursor->buf = cursor->buf->frags;
		} else {
			break;
		}
	}

	if (cursor->buf) {
		cursor->pos = cursor->buf->data;
	} else {
		cursor->pos = NULL;
	}
}

static void pkt_cursor_advance(struct net_pkt *pkt, bool write)
{
	struct net_pkt_cursor *cursor = &pkt->cursor;
	size_t len;

	if (!cursor->buf) {
		return;
	}

	len = write ? cursor->buf->size : cursor->buf->len;
	if ((cursor->pos - cursor->buf->data) == len) {
		pkt_cursor_jump(pkt, write);
	}
}

static void pkt_cursor_update(struct net_pkt *pkt,
			      size_t length, bool write)
{
	struct net_pkt_cursor *cursor = &pkt->cursor;
	size_t len;

	if (net_pkt_is_being_overwritten(pkt)) {
		write = false;
	}

	len = write ? cursor->buf->size : cursor->buf->len;
	if (length + (cursor->pos - cursor->buf->data) == len &&
	    !(net_pkt_is_being_overwritten(pkt) && len < cursor->buf->size)) {
		pkt_cursor_jump(pkt, write);
	} else {
		cursor->pos += length;
	}
}

/* Internal function that does all operation (skip/read/write/memset) */
static int net_pkt_cursor_operate(struct net_pkt *pkt,
				  void *data, size_t length,
				  bool copy, bool write)
{
	/* We use such variable to avoid lengthy lines */
	struct net_pkt_cursor *c_op = &pkt->cursor;

	while (c_op->buf && length) {
		size_t d_len, len;

		pkt_cursor_advance(pkt, net_pkt_is_being_overwritten(pkt) ?
				   false : write);
		if (c_op->buf == NULL) {
			break;
		}

		if (write && !net_pkt_is_being_overwritten(pkt)) {
			d_len = c_op->buf->size - (c_op->pos - c_op->buf->data);
		} else {
			d_len = c_op->buf->len - (c_op->pos - c_op->buf->data);
		}

		if (!d_len) {
			break;
		}

		if (length < d_len) {
			len = length;
		} else {
			len = d_len;
		}

		if (copy) {
			memcpy(write ? c_op->pos : data,
			       write ? data : c_op->pos,
			       len);
		} else if (data) {
			memset(c_op->pos, *(int *)data, len);
		}

		if (write && !net_pkt_is_being_overwritten(pkt)) {
			net_buf_add(c_op->buf, len);
		}

		pkt_cursor_update(pkt, len, write);

		if (copy && data) {
			data = (u8_t *) data + len;
		}

		length -= len;
	}

	if (length) {
		NET_DBG("Still some length to go %zu", length);
		return -ENOBUFS;
	}

	return 0;
}

int net_pkt_skip(struct net_pkt *pkt, size_t skip)
{
	NET_DBG("pkt %p skip %zu", pkt, skip);

	return net_pkt_cursor_operate(pkt, NULL, skip, false, true);
}

int net_pkt_memset(struct net_pkt *pkt, int byte, size_t amount)
{
	NET_DBG("pkt %p byte %d amount %zu", pkt, byte, amount);

	return net_pkt_cursor_operate(pkt, &byte, amount, false, true);
}

int net_pkt_read_new(struct net_pkt *pkt, void *data, size_t length)
{
	NET_DBG("pkt %p data %p length %zu", pkt, data, length);

	return net_pkt_cursor_operate(pkt, data, length, true, false);
}

int net_pkt_read_be16_new(struct net_pkt *pkt, u16_t *data)
{
	u8_t d16[2];
	int ret;

	ret = net_pkt_read_new(pkt, d16, sizeof(u16_t));

	*data = d16[0] << 8 | d16[1];

	return ret;
}

int net_pkt_read_be32_new(struct net_pkt *pkt, u32_t *data)
{
	u8_t d32[4];
	int ret;

	ret = net_pkt_read_new(pkt, d32, sizeof(u32_t));

	*data = d32[0] << 24 | d32[1] << 16 | d32[2] << 8 | d32[3];

	return ret;
}

int net_pkt_write_new(struct net_pkt *pkt, const void *data, size_t length)
{
	NET_DBG("pkt %p data %p length %zu", pkt, data, length);

	if (data == pkt->cursor.pos && net_pkt_is_contiguous(pkt, length)) {
		return net_pkt_skip(pkt, length);
	}

	return net_pkt_cursor_operate(pkt, (void *)data, length, true, true);
}

int net_pkt_copy(struct net_pkt *pkt_dst,
		 struct net_pkt *pkt_src,
		 size_t length)
{
	struct net_pkt_cursor *c_dst = &pkt_dst->cursor;
	struct net_pkt_cursor *c_src = &pkt_src->cursor;

	while (c_dst->buf && c_src->buf && length) {
		size_t s_len, d_len, len;

		pkt_cursor_advance(pkt_dst, true);
		pkt_cursor_advance(pkt_src, false);

		if (!c_dst->buf || !c_src->buf) {
			break;
		}

		s_len = c_src->buf->len - (c_src->pos - c_src->buf->data);
		d_len = c_dst->buf->size - (c_dst->pos - c_dst->buf->data);
		if (length < s_len && length < d_len) {
			len = length;
		} else {
			if (d_len < s_len) {
				len = d_len;
			} else {
				len = s_len;
			}
		}

		if (!len) {
			break;
		}

		memcpy(c_dst->pos, c_src->pos, len);

		if (!net_pkt_is_being_overwritten(pkt_dst)) {
			net_buf_add(c_dst->buf, len);
		}

		pkt_cursor_update(pkt_dst, len, true);
		pkt_cursor_update(pkt_src, len, false);

		length -= len;
	}

	if (length) {
		NET_DBG("Still some length to go %zu", length);
		return -ENOBUFS;
	}

	return 0;
}

struct net_pkt *net_pkt_clone(struct net_pkt *pkt, s32_t timeout)
{
	struct net_pkt *clone_pkt;

	clone_pkt = net_pkt_alloc_with_buffer(net_pkt_iface(pkt),
					      net_pkt_get_len(pkt),
					      AF_UNSPEC, 0, timeout);
	if (!clone_pkt) {
		return NULL;
	}

	net_pkt_cursor_init(pkt);

	if (net_pkt_copy(clone_pkt, pkt, net_pkt_get_len(pkt))) {
		net_pkt_unref(clone_pkt);
		return NULL;
	}

	if (clone_pkt->buffer) {
		/* The link header pointers are only usable if there is
		 * a buffer that we copied because those pointers point
		 * to start of the fragment which we do not have right now.
		 */
		memcpy(&clone_pkt->lladdr_src, &pkt->lladdr_src,
		       sizeof(clone_pkt->lladdr_src));
		memcpy(&clone_pkt->lladdr_dst, &pkt->lladdr_dst,
		       sizeof(clone_pkt->lladdr_dst));
	}

	net_pkt_set_family(clone_pkt, net_pkt_family(pkt));
	net_pkt_set_context(clone_pkt, net_pkt_context(pkt));
	net_pkt_set_token(clone_pkt, net_pkt_token(pkt));
	net_pkt_set_ip_hdr_len(clone_pkt, net_pkt_ip_hdr_len(pkt));
	net_pkt_set_vlan_tag(clone_pkt, net_pkt_vlan_tag(pkt));
	net_pkt_set_timestamp(clone_pkt, net_pkt_timestamp(pkt));
	net_pkt_set_priority(clone_pkt, net_pkt_priority(pkt));
	net_pkt_set_orig_iface(clone_pkt, net_pkt_orig_iface(pkt));

	if (IS_ENABLED(CONFIG_NET_IPV4) && net_pkt_family(pkt) == AF_INET) {
		net_pkt_set_ipv4_ttl(clone_pkt, net_pkt_ipv4_ttl(pkt));
	} else if (IS_ENABLED(CONFIG_NET_IPV6) &&
		   net_pkt_family(pkt) == AF_INET6) {
		net_pkt_set_ipv6_hop_limit(clone_pkt,
					   net_pkt_ipv6_hop_limit(pkt));
		net_pkt_set_ipv6_ext_len(clone_pkt, net_pkt_ipv6_ext_len(pkt));
		net_pkt_set_ipv6_ext_opt_len(clone_pkt,
					     net_pkt_ipv6_ext_opt_len(pkt));
		net_pkt_set_ipv6_hdr_prev(clone_pkt,
					  net_pkt_ipv6_hdr_prev(pkt));
		net_pkt_set_ipv6_next_hdr(clone_pkt,
					  net_pkt_ipv6_next_hdr(pkt));
	}

	net_pkt_cursor_init(clone_pkt);

	NET_DBG("Cloned %p to %p", pkt, clone_pkt);

	return clone_pkt;
}

size_t net_pkt_remaining_data(struct net_pkt *pkt)
{
	struct net_buf *buf;
	size_t data_length;

	if (!pkt || !pkt->cursor.buf || !pkt->cursor.pos) {
		return 0;
	}

	buf = pkt->cursor.buf;
	data_length = buf->len - (pkt->cursor.pos - buf->data);

	buf = buf->frags;
	while (buf) {
		data_length += buf->len;
		buf = buf->frags;
	}

	return data_length;
}

int net_pkt_update_length(struct net_pkt *pkt, size_t length)
{
	struct net_buf *buf;

	for (buf = pkt->buffer; buf; buf = buf->frags) {
		if (buf->len < length) {
			length -= buf->len;
		} else {
			buf->len = length;
			length = 0;
		}
	}

	return !length ? 0 : -EINVAL;
}

int net_pkt_pull(struct net_pkt *pkt, size_t length)
{
	struct net_pkt_cursor *c_op = &pkt->cursor;
	struct net_pkt_cursor backup;

	net_pkt_cursor_backup(pkt, &backup);

	while (length) {
		u8_t left, rem;

		pkt_cursor_advance(pkt, false);

		left = c_op->buf->len - (c_op->pos - c_op->buf->data);
		if (!left) {
			break;
		}

		rem = left;
		if (rem > length) {
			rem = length;
		}

		c_op->buf->len -= rem;
		left -= rem;
		if (left) {
			memmove(c_op->pos, c_op->pos+rem, rem);
		}

		/* For now, empty buffer are not freed, and there is no
		 * compaction done either.
		 * net_pkt_pull() is currently used only in very specific
		 * places where such memory optimization would not make
		 * that much sense. Let's see in future if it's worth do to it.
		 */

		length -= rem;
	}

	net_pkt_cursor_restore(pkt, &backup);

	if (length) {
		NET_DBG("Still some length to go %zu", length);
		return -ENOBUFS;
	}

	return 0;
}

u16_t net_pkt_get_current_offset(struct net_pkt *pkt)
{
	struct net_buf *buf = pkt->buffer;
	u16_t offset;

	if (!pkt->cursor.buf || !pkt->cursor.pos) {
		return 0;
	}

	offset = 0;

	while (buf != pkt->cursor.buf) {
		offset += buf->len;
		buf = buf->frags;
	}

	offset += pkt->cursor.pos - buf->data;

	return offset;
}

bool net_pkt_is_contiguous(struct net_pkt *pkt, size_t size)
{
	if (pkt->cursor.buf && pkt->cursor.pos) {
		size_t len;

		len = net_pkt_is_being_overwritten(pkt) ?
			pkt->cursor.buf->len : pkt->cursor.buf->size;
		len -= pkt->cursor.pos - pkt->cursor.buf->data;
		if (len >= size) {
			return true;
		}
	}

	return false;
}

void *net_pkt_get_data_new(struct net_pkt *pkt,
			   struct net_pkt_data_access *access)
{
	if (IS_ENABLED(CONFIG_NET_HEADERS_ALWAYS_CONTIGUOUS)) {
		if (!net_pkt_is_contiguous(pkt, access->size)) {
			return NULL;
		}

		return pkt->cursor.pos;
	} else {
		if (net_pkt_is_contiguous(pkt, access->size)) {
			access->data = pkt->cursor.pos;
		} else if (net_pkt_is_being_overwritten(pkt)) {
			struct net_pkt_cursor backup;

			if (!access->data) {
				NET_ERR("Uncontiguous data"
					" cannot be linearized");
				return NULL;
			}

			net_pkt_cursor_backup(pkt, &backup);

			if (net_pkt_read_new(pkt, access->data, access->size)) {
				net_pkt_cursor_restore(pkt, &backup);
				return NULL;
			}

			net_pkt_cursor_restore(pkt, &backup);
		}

		return access->data;
	}

	return NULL;
}

int net_pkt_set_data(struct net_pkt *pkt,
		     struct net_pkt_data_access *access)
{
	if (IS_ENABLED(CONFIG_NET_HEADERS_ALWAYS_CONTIGUOUS)) {
		return net_pkt_skip(pkt, access->size);
	}

	return net_pkt_write_new(pkt, access->data, access->size);
}

void net_pkt_init(void)
{
#if CONFIG_NET_PKT_LOG_LEVEL >= LOG_LEVEL_DBG
	NET_DBG("Allocating %u RX (%zu bytes), %u TX (%zu bytes), "
		"%d RX data (%u bytes) and %d TX data (%u bytes) buffers",
		k_mem_slab_num_free_get(&rx_pkts),
		(size_t)(k_mem_slab_num_free_get(&rx_pkts) *
			 sizeof(struct net_pkt)),
		k_mem_slab_num_free_get(&tx_pkts),
		(size_t)(k_mem_slab_num_free_get(&tx_pkts) *
			 sizeof(struct net_pkt)),
		get_frees(&rx_bufs), get_size(&rx_bufs),
		get_frees(&tx_bufs), get_size(&tx_bufs));
#endif
}
