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

#include <sys/util.h>

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
    defined(CONFIG_NET_SOCKETS_PACKET) || defined(CONFIG_NET_SOCKETS_OFFLOAD)
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
	uint16_t line_alloc;
	uint16_t line_free;
	uint8_t in_use;
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
static inline int16_t get_frees(struct net_buf_pool *pool)
{
#if defined(CONFIG_NET_BUF_POOL_USAGE)
	return atomic_get(&pool->avail_count);
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

static inline int16_t get_size(struct net_buf_pool *pool)
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
struct net_buf *net_pkt_get_reserve_data_debug(struct net_buf_pool *pool,
					       k_timeout_t timeout,
					       const char *caller,
					       int line)
#else /* NET_LOG_LEVEL >= LOG_LEVEL_DBG */
struct net_buf *net_pkt_get_reserve_data(struct net_buf_pool *pool,
					 k_timeout_t timeout)
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
	NET_FRAG_CHECK_IF_NOT_IN_USE(frag, frag->ref + 1U);
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
				       k_timeout_t timeout,
				       const char *caller, int line)
#else
struct net_buf *net_pkt_get_frag(struct net_pkt *pkt,
				 k_timeout_t timeout)
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
struct net_buf *net_pkt_get_reserve_rx_data_debug(k_timeout_t timeout,
						  const char *caller, int line)
{
	return net_pkt_get_reserve_data_debug(&rx_bufs, timeout, caller, line);
}

struct net_buf *net_pkt_get_reserve_tx_data_debug(k_timeout_t timeout,
						  const char *caller, int line)
{
	return net_pkt_get_reserve_data_debug(&tx_bufs, timeout, caller, line);
}

#else /* NET_LOG_LEVEL >= LOG_LEVEL_DBG */

struct net_buf *net_pkt_get_reserve_rx_data(k_timeout_t timeout)
{
	return net_pkt_get_reserve_data(&rx_bufs, timeout);
}

struct net_buf *net_pkt_get_reserve_tx_data(k_timeout_t timeout)
{
	return net_pkt_get_reserve_data(&tx_bufs, timeout);
}

#endif /* NET_LOG_LEVEL >= LOG_LEVEL_DBG */


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
			frag->ref - 1U, frag->frags, caller, line);
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
		frag, frag->ref + 1U, caller, line);
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
		frag, frag->ref - 1U, caller, line);
#endif

	if (frag->ref == 1U) {
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

		if (frag->ref == 1U) {
			net_pkt_alloc_del(frag, caller, line);
		}

		tmp = net_buf_frag_del(NULL, frag);
		pkt->frags = tmp;

		return tmp;
	}

	if (frag->ref == 1U) {
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

/* New allocator and API starts here */

#if defined(CONFIG_NET_BUF_FIXED_DATA_SIZE)

#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
static struct net_buf *pkt_alloc_buffer(struct net_buf_pool *pool,
					size_t size, k_timeout_t timeout,
					const char *caller, int line)
#else
static struct net_buf *pkt_alloc_buffer(struct net_buf_pool *pool,
					size_t size, k_timeout_t timeout)
#endif
{
	uint64_t end = z_timeout_end_calc(timeout);
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

		if (!K_TIMEOUT_EQ(timeout, K_NO_WAIT) &&
		    !K_TIMEOUT_EQ(timeout, K_FOREVER)) {
			int64_t remaining = end - z_tick_get();

			if (remaining <= 0) {
				break;
			}

			timeout = Z_TIMEOUT_TICKS(remaining);
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
					size_t size, k_timeout_t timeout,
					const char *caller, int line)
#else
static struct net_buf *pkt_alloc_buffer(struct net_buf_pool *pool,
					size_t size, k_timeout_t timeout)
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
	sa_family_t family = net_pkt_family(pkt);
	size_t max_len;

	if (net_pkt_iface(pkt)) {
		max_len = net_if_get_mtu(net_pkt_iface(pkt));
	} else {
		max_len = 0;
	}

	/* Family vs iface MTU */
	if (IS_ENABLED(CONFIG_NET_IPV6) && family == AF_INET6) {
		if (IS_ENABLED(CONFIG_NET_IPV6_FRAGMENT) && (size > max_len)) {
			/* We support larger packets if IPv6 fragmentation is
			 * enabled.
			 */
			max_len = size;
		}

		max_len = MAX(max_len, NET_IPV6_MTU);
	} else if (IS_ENABLED(CONFIG_NET_IPV4) && family == AF_INET) {
		max_len = MAX(max_len, NET_IPV4_MTU);
	} else { /* family == AF_UNSPEC */
#if defined (CONFIG_NET_L2_ETHERNET)
		if (net_if_l2(net_pkt_iface(pkt)) ==
		    &NET_L2_GET_NAME(ETHERNET)) {
			max_len += NET_ETH_MAX_HDR_SIZE;
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
			       k_timeout_t timeout,
			       const char *caller,
			       int line)
#else
int net_pkt_alloc_buffer(struct net_pkt *pkt,
			 size_t size,
			 enum net_ip_protocol proto,
			 k_timeout_t timeout)
#endif
{
	uint64_t end = z_timeout_end_calc(timeout);
	struct net_buf_pool *pool = NULL;
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

	if (pkt->context) {
		pool = get_data_pool(pkt->context);
	}

	if (!pool) {
		pool = pkt->slab == &tx_pkts ? &tx_bufs : &rx_bufs;
	}

	if (!K_TIMEOUT_EQ(timeout, K_NO_WAIT) &&
	    !K_TIMEOUT_EQ(timeout, K_FOREVER)) {
		int64_t remaining = end - z_tick_get();

		if (remaining <= 0) {
			timeout = K_NO_WAIT;
		} else {
			timeout = Z_TIMEOUT_TICKS(remaining);
		}
	}

#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
	buf = pkt_alloc_buffer(pool, alloc_len, timeout, caller, line);
#else
	buf = pkt_alloc_buffer(pool, alloc_len, timeout);
#endif

	if (!buf) {
#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
		NET_ERR("Data buffer (%zd) allocation failed (%s:%d)",
			alloc_len, caller, line);
#else
		NET_ERR("Data buffer (%zd) allocation failed.", alloc_len);
#endif
		return -ENOMEM;
	}

	net_pkt_append_buffer(pkt, buf);

	return 0;
}

#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
static struct net_pkt *pkt_alloc(struct k_mem_slab *slab, k_timeout_t timeout,
				 const char *caller, int line)
#else
static struct net_pkt *pkt_alloc(struct k_mem_slab *slab, k_timeout_t timeout)
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

#if IS_ENABLED(CONFIG_NET_TX_DEFAULT_PRIORITY)
#define TX_DEFAULT_PRIORITY CONFIG_NET_TX_DEFAULT_PRIORITY
#else
#define TX_DEFAULT_PRIORITY 0
#endif

#if IS_ENABLED(CONFIG_NET_RX_DEFAULT_PRIORITY)
#define RX_DEFAULT_PRIORITY CONFIG_NET_RX_DEFAULT_PRIORITY
#else
#define RX_DEFAULT_PRIORITY 0
#endif

	if (&tx_pkts == slab) {
		net_pkt_set_priority(pkt, TX_DEFAULT_PRIORITY);
	} else if (&rx_pkts == slab) {
		net_pkt_set_priority(pkt, RX_DEFAULT_PRIORITY);
	}

	if (IS_ENABLED(CONFIG_NET_PKT_RXTIME_STATS) ||
	    IS_ENABLED(CONFIG_NET_PKT_TXTIME_STATS)) {
		struct net_ptp_time tp = {
			/* Use the nanosecond field to temporarily
			 * store the cycle count as it is a 32-bit
			 * variable. The net_pkt timestamp field is used
			 * to calculate how long it takes the packet to travel
			 * between network device driver and application.
			 */
			.nanosecond = k_cycle_get_32(),
		};

		net_pkt_set_timestamp(pkt, &tp);
	}

	net_pkt_set_vlan_tag(pkt, NET_VLAN_TAG_UNSPEC);

#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
	net_pkt_alloc_add(pkt, true, caller, line);
#endif

	net_pkt_cursor_init(pkt);

	return pkt;
}

#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
struct net_pkt *net_pkt_alloc_debug(k_timeout_t timeout,
				    const char *caller, int line)
#else
struct net_pkt *net_pkt_alloc(k_timeout_t timeout)
#endif
{
#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
	return pkt_alloc(&tx_pkts, timeout, caller, line);
#else
	return pkt_alloc(&tx_pkts, timeout);
#endif
}

#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
struct net_pkt *net_pkt_alloc_from_slab_debug(struct k_mem_slab *slab,
					      k_timeout_t timeout,
					      const char *caller, int line)
#else
struct net_pkt *net_pkt_alloc_from_slab(struct k_mem_slab *slab,
					k_timeout_t timeout)
#endif
{
	if (!slab) {
		return NULL;
	}

#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
	return pkt_alloc(slab, timeout, caller, line);
#else
	return pkt_alloc(slab, timeout);
#endif
}

#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
struct net_pkt *net_pkt_rx_alloc_debug(k_timeout_t timeout,
				       const char *caller, int line)
#else
struct net_pkt *net_pkt_rx_alloc(k_timeout_t timeout)
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
					  struct net_if *iface,
					  k_timeout_t timeout,
					  const char *caller, int line)
#else
static struct net_pkt *pkt_alloc_on_iface(struct k_mem_slab *slab,
					  struct net_if *iface,
					  k_timeout_t timeout)

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
					     k_timeout_t timeout,
					     const char *caller,
					     int line)
#else
struct net_pkt *net_pkt_alloc_on_iface(struct net_if *iface,
				       k_timeout_t timeout)
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
						k_timeout_t timeout,
						const char *caller,
						int line)
#else
struct net_pkt *net_pkt_rx_alloc_on_iface(struct net_if *iface,
					  k_timeout_t timeout)
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
		      k_timeout_t timeout,
		      const char *caller,
		      int line)
#else
static struct net_pkt *
pkt_alloc_with_buffer(struct k_mem_slab *slab,
		      struct net_if *iface,
		      size_t size,
		      sa_family_t family,
		      enum net_ip_protocol proto,
		      k_timeout_t timeout)
#endif
{
	uint64_t end = z_timeout_end_calc(timeout);
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

	if (!K_TIMEOUT_EQ(timeout, K_NO_WAIT) &&
	    !K_TIMEOUT_EQ(timeout, K_FOREVER)) {
		int64_t remaining = end - z_tick_get();

		if (remaining <= 0) {
			timeout = K_NO_WAIT;
		} else {
			timeout = Z_TIMEOUT_TICKS(remaining);
		}
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
						k_timeout_t timeout,
						const char *caller,
						int line)
#else
struct net_pkt *net_pkt_alloc_with_buffer(struct net_if *iface,
					  size_t size,
					  sa_family_t family,
					  enum net_ip_protocol proto,
					  k_timeout_t timeout)
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
						   k_timeout_t timeout,
						   const char *caller,
						   int line)
#else
struct net_pkt *net_pkt_rx_alloc_with_buffer(struct net_if *iface,
					     size_t size,
					     sa_family_t family,
					     enum net_ip_protocol proto,
					     k_timeout_t timeout)
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
			data = (uint8_t *) data + len;
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

int net_pkt_read(struct net_pkt *pkt, void *data, size_t length)
{
	NET_DBG("pkt %p data %p length %zu", pkt, data, length);

	return net_pkt_cursor_operate(pkt, data, length, true, false);
}

int net_pkt_read_be16(struct net_pkt *pkt, uint16_t *data)
{
	uint8_t d16[2];
	int ret;

	ret = net_pkt_read(pkt, d16, sizeof(uint16_t));

	*data = d16[0] << 8 | d16[1];

	return ret;
}

int net_pkt_read_le16(struct net_pkt *pkt, uint16_t *data)
{
	uint8_t d16[2];
	int ret;

	ret = net_pkt_read(pkt, d16, sizeof(uint16_t));

	*data = d16[1] << 8 | d16[0];

	return ret;
}

int net_pkt_read_be32(struct net_pkt *pkt, uint32_t *data)
{
	uint8_t d32[4];
	int ret;

	ret = net_pkt_read(pkt, d32, sizeof(uint32_t));

	*data = d32[0] << 24 | d32[1] << 16 | d32[2] << 8 | d32[3];

	return ret;
}

int net_pkt_write(struct net_pkt *pkt, const void *data, size_t length)
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

static void clone_pkt_attributes(struct net_pkt *pkt, struct net_pkt *clone_pkt)
{
	net_pkt_set_family(clone_pkt, net_pkt_family(pkt));
	net_pkt_set_context(clone_pkt, net_pkt_context(pkt));
	net_pkt_set_ip_hdr_len(clone_pkt, net_pkt_ip_hdr_len(pkt));
	net_pkt_set_vlan_tag(clone_pkt, net_pkt_vlan_tag(pkt));
	net_pkt_set_timestamp(clone_pkt, net_pkt_timestamp(pkt));
	net_pkt_set_priority(clone_pkt, net_pkt_priority(pkt));
	net_pkt_set_orig_iface(clone_pkt, net_pkt_orig_iface(pkt));

	if (IS_ENABLED(CONFIG_NET_IPV4) && net_pkt_family(pkt) == AF_INET) {
		net_pkt_set_ipv4_ttl(clone_pkt, net_pkt_ipv4_ttl(pkt));
		net_pkt_set_ipv4_opts_len(clone_pkt,
					  net_pkt_ipv4_opts_len(pkt));
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
}

struct net_pkt *net_pkt_clone(struct net_pkt *pkt, k_timeout_t timeout)
{
	size_t cursor_offset = net_pkt_get_current_offset(pkt);
	struct net_pkt *clone_pkt;
	struct net_pkt_cursor backup;

	clone_pkt = net_pkt_alloc_with_buffer(net_pkt_iface(pkt),
					      net_pkt_get_len(pkt),
					      AF_UNSPEC, 0, timeout);
	if (!clone_pkt) {
		return NULL;
	}

	net_pkt_cursor_backup(pkt, &backup);
	net_pkt_cursor_init(pkt);

	if (net_pkt_copy(clone_pkt, pkt, net_pkt_get_len(pkt))) {
		net_pkt_unref(clone_pkt);
		net_pkt_cursor_restore(pkt, &backup);
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

	clone_pkt_attributes(pkt, clone_pkt);

	net_pkt_cursor_init(clone_pkt);

	if (cursor_offset) {
		net_pkt_set_overwrite(clone_pkt, true);
		net_pkt_skip(clone_pkt, cursor_offset);
	}

	net_pkt_cursor_restore(pkt, &backup);

	NET_DBG("Cloned %p to %p", pkt, clone_pkt);

	return clone_pkt;
}

struct net_pkt *net_pkt_shallow_clone(struct net_pkt *pkt, k_timeout_t timeout)
{
	struct net_pkt *clone_pkt;
	struct net_buf *buf;

	clone_pkt = net_pkt_alloc(timeout);
	if (!clone_pkt) {
		return NULL;
	}

	net_pkt_set_iface(clone_pkt, net_pkt_iface(pkt));
	clone_pkt->buffer = pkt->buffer;
	buf = pkt->buffer;

	while (buf) {
		net_pkt_frag_ref(buf);
		buf = buf->frags;
	}

	if (pkt->buffer) {
		/* The link header pointers are only usable if there is
		 * a buffer that we copied because those pointers point
		 * to start of the fragment which we do not have right now.
		 */
		memcpy(&clone_pkt->lladdr_src, &pkt->lladdr_src,
		       sizeof(clone_pkt->lladdr_src));
		memcpy(&clone_pkt->lladdr_dst, &pkt->lladdr_dst,
		       sizeof(clone_pkt->lladdr_dst));
	}

	clone_pkt_attributes(pkt, clone_pkt);

	net_pkt_cursor_restore(clone_pkt, &pkt->cursor);

	NET_DBG("Shallow cloned %p to %p", pkt, clone_pkt);

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

	while (length) {
		size_t left, rem;

		pkt_cursor_advance(pkt, false);

		if (!c_op->buf) {
			break;
		}

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
			memmove(c_op->pos, c_op->pos+rem, left);
		} else {
			struct net_buf *buf = pkt->buffer;

			if (buf) {
				pkt->buffer = buf->frags;
				buf->frags = NULL;
				net_buf_unref(buf);
			}

			net_pkt_cursor_init(pkt);
		}

		length -= rem;
	}

	net_pkt_cursor_init(pkt);

	if (length) {
		NET_DBG("Still some length to go %zu", length);
		return -ENOBUFS;
	}

	return 0;
}

uint16_t net_pkt_get_current_offset(struct net_pkt *pkt)
{
	struct net_buf *buf = pkt->buffer;
	uint16_t offset;

	if (!pkt->cursor.buf || !pkt->cursor.pos) {
		return 0;
	}

	offset = 0U;

	while (buf != pkt->cursor.buf) {
		offset += buf->len;
		buf = buf->frags;
	}

	offset += pkt->cursor.pos - buf->data;

	return offset;
}

bool net_pkt_is_contiguous(struct net_pkt *pkt, size_t size)
{
	pkt_cursor_advance(pkt, !net_pkt_is_being_overwritten(pkt));

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

void *net_pkt_get_data(struct net_pkt *pkt,
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

			if (net_pkt_read(pkt, access->data, access->size)) {
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

	return net_pkt_write(pkt, access->data, access->size);
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
