/** @file
 @brief Network buffers for IP stack

 Network data is passed between components using nbuf.
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_NET_DEBUG_NET_BUF)
#define SYS_LOG_DOMAIN "net/nbuf"
#define NET_LOG_ENABLED 1
#endif

#include <kernel.h>
#include <toolchain.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>

#include <misc/util.h>

#include <net/net_core.h>
#include <net/net_ip.h>
#include <net/buf.h>
#include <net/nbuf.h>

#include "net_private.h"

/* Available (free) buffers queue */
#define NBUF_RX_COUNT		CONFIG_NET_NBUF_RX_COUNT
#define NBUF_TX_COUNT		CONFIG_NET_NBUF_TX_COUNT
#define NBUF_RX_DATA_COUNT	CONFIG_NET_NBUF_RX_DATA_COUNT
#define NBUF_TX_DATA_COUNT	CONFIG_NET_NBUF_TX_DATA_COUNT
#define NBUF_DATA_LEN		CONFIG_NET_NBUF_DATA_SIZE
#define NBUF_USER_DATA_LEN	CONFIG_NET_NBUF_USER_DATA_SIZE

#if defined(CONFIG_NET_TCP)
#define APP_PROTO_LEN NET_TCPH_LEN
#else
#if defined(CONFIG_NET_UDP)
#define APP_PROTO_LEN NET_UDPH_LEN
#else
#define APP_PROTO_LEN 0
#endif /* UDP */
#endif /* TCP */

#if defined(CONFIG_NET_IPV6) || defined(CONFIG_NET_L2_RAW_CHANNEL)
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

NET_BUF_POOL_DEFINE(rx_bufs, NBUF_RX_COUNT, 0, sizeof(struct net_nbuf),
		    NULL);
NET_BUF_POOL_DEFINE(tx_bufs, NBUF_TX_COUNT, 0, sizeof(struct net_nbuf),
		    NULL);

/* The data fragment pool is for storing network data. */
NET_BUF_POOL_DEFINE(data_rx_bufs, NBUF_RX_DATA_COUNT, NBUF_DATA_LEN,
		    NBUF_USER_DATA_LEN, NULL);

NET_BUF_POOL_DEFINE(data_tx_bufs, NBUF_TX_DATA_COUNT, NBUF_DATA_LEN,
		    NBUF_USER_DATA_LEN, NULL);

#if defined(CONFIG_NET_DEBUG_NET_BUF)

#define NET_BUF_CHECK_IF_IN_USE(buf, ref)				\
	do {								\
		if (ref) {						\
			NET_ERR("**ERROR** buf %p in use (%s:%s():%d)", \
				buf, __FILE__, __func__, __LINE__);     \
		}                                                       \
	} while (0)

#define NET_BUF_CHECK_IF_NOT_IN_USE(buf, ref)				\
	do {								\
		if (!(ref)) {                                           \
			NET_ERR("**ERROR** buf %p not in use (%s:%s():%d)", \
				buf, __FILE__, __func__, __LINE__);     \
		}                                                       \
	} while (0)

#else /* CONFIG_NET_DEBUG_NET_BUF */
#define NET_BUF_CHECK_IF_IN_USE(buf, ref)
#define NET_BUF_CHECK_IF_NOT_IN_USE(buf, ref)
#endif /* CONFIG_NET_DEBUG_NET_BUF */

static inline bool is_data_pool(struct net_buf_pool *pool)
{
	/* The user data can only be found in TX/RX pool and it
	 * is always the size of struct net_nbuf.
	 */
	if (pool->user_data_size != sizeof(struct net_nbuf)) {
		return true;
	}

	return false;
}

static inline bool is_from_data_pool(struct net_buf *buf)
{
	return is_data_pool(buf->pool);
}

#if defined(CONFIG_NET_DEBUG_NET_BUF)
struct nbuf_alloc {
	struct net_buf *buf;
	const char *func_alloc;
	const char *func_free;
	uint16_t line_alloc;
	uint16_t line_free;
	uint8_t in_use;
};

#define MAX_NBUF_ALLOCS (NBUF_RX_COUNT + NBUF_TX_COUNT + \
			 NBUF_RX_DATA_COUNT + NBUF_TX_DATA_COUNT + \
			 CONFIG_NET_DEBUG_NET_BUF_EXTERNALS)

static struct nbuf_alloc nbuf_allocs[MAX_NBUF_ALLOCS];

static bool nbuf_alloc_add(struct net_buf *buf,
			   const char *func,
			   int line)
{
	int i;

	for (i = 0; i < MAX_NBUF_ALLOCS; i++) {
		if (nbuf_allocs[i].in_use) {
			continue;
		}

		nbuf_allocs[i].in_use = true;
		nbuf_allocs[i].buf = buf;
		nbuf_allocs[i].func_alloc = func;
		nbuf_allocs[i].line_alloc = line;

		return true;
	}

	return false;
}

static bool nbuf_alloc_del(struct net_buf *buf,
			   const char *func,
			   int line)
{
	int i;

	for (i = 0; i < MAX_NBUF_ALLOCS; i++) {
		if (nbuf_allocs[i].in_use && nbuf_allocs[i].buf == buf) {
			nbuf_allocs[i].func_free = func;
			nbuf_allocs[i].line_free = line;
			nbuf_allocs[i].in_use = false;

			return true;
		}
	}

	return false;
}

static bool nbuf_alloc_find(struct net_buf *buf,
			    const char **func_free,
			    int *line_free)
{
	int i;

	for (i = 0; i < MAX_NBUF_ALLOCS; i++) {
		if (!nbuf_allocs[i].in_use && nbuf_allocs[i].buf == buf) {
			*func_free = nbuf_allocs[i].func_free;
			*line_free = nbuf_allocs[i].line_free;

			return true;
		}
	}

	return false;
}

void net_nbuf_allocs_foreach(net_nbuf_allocs_cb_t cb, void *user_data)
{
	int i;

	for (i = 0; i < MAX_NBUF_ALLOCS; i++) {
		if (nbuf_allocs[i].in_use) {
			cb(nbuf_allocs[i].buf,
			   nbuf_allocs[i].func_alloc,
			   nbuf_allocs[i].line_alloc,
			   nbuf_allocs[i].func_free,
			   nbuf_allocs[i].line_free,
			   nbuf_allocs[i].in_use,
			   user_data);
		}
	}

	for (i = 0; i < MAX_NBUF_ALLOCS; i++) {
		if (!nbuf_allocs[i].in_use) {
			cb(nbuf_allocs[i].buf,
			   nbuf_allocs[i].func_alloc,
			   nbuf_allocs[i].line_alloc,
			   nbuf_allocs[i].func_free,
			   nbuf_allocs[i].line_free,
			   nbuf_allocs[i].in_use,
			   user_data);
		}
	}
}

const char *net_nbuf_pool2str(struct net_buf_pool *pool)
{
	if (pool == &rx_bufs) {
		return "RX";
	} else if (pool == &tx_bufs) {
		return "TX";
	} else if (pool == &data_rx_bufs) {
		return "RDATA";
	} else if (pool == &data_tx_bufs) {
		return "TDATA";
	} else if (is_data_pool(pool)) {
		return "EDATA";
	}

	return "EXT";
}

static inline int16_t get_frees(struct net_buf_pool *pool)
{
	return pool->avail_count;
}
#endif

#if defined(CONFIG_NET_CONTEXT_NBUF_POOL)
static inline struct net_buf_pool *get_tx_pool(struct net_context *context)
{
	if (context->tx_pool) {
		return context->tx_pool();
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
#define get_tx_pool(context) NULL
#define get_data_pool(context) NULL
#endif /* CONFIG_NET_CONTEXT_NBUF_POOL */

static inline bool is_external_pool(struct net_buf_pool *pool)
{
	if (pool != &rx_bufs && pool != &tx_bufs &&
	    pool != &data_rx_bufs && pool != &data_tx_bufs) {
		return true;
	}

	return false;
}

#if defined(CONFIG_NET_DEBUG_NET_BUF)
static inline const char *pool2str(struct net_buf_pool *pool)
{
	return net_nbuf_pool2str(pool);
}

void net_nbuf_print_frags(struct net_buf *buf)
{
	struct net_buf *frag;
	size_t total = 0;
	int count = -1, frag_size = 0, ll_overhead = 0;

	if (!buf) {
		NET_INFO("Buf %p", buf);
		return;
	}

	NET_INFO("Buf %p frags %p", buf, buf->frags);

	NET_ASSERT(buf->frags);

	frag = buf;

	while (frag) {
		total += frag->len;

		frag_size = frag->size;
		ll_overhead = net_buf_headroom(frag);

		NET_INFO("[%d] frag %p len %d size %d reserve %d "
			 "pool %p [sz %d ud_sz %d]",
			 count, frag, frag->len, frag_size, ll_overhead,
			 frag->pool, frag->pool->buf_size,
			 frag->pool->user_data_size);

		count++;

		frag = frag->frags;
	}

	NET_INFO("Total data size %zu, occupied %d bytes, ll overhead %d, "
		"utilization %zu%%",
		total, count * frag_size - count * ll_overhead,
		count * ll_overhead, (total * 100) / (count * frag_size));
}

struct net_buf *net_nbuf_get_reserve_debug(struct net_buf_pool *pool,
					   uint16_t reserve_head,
					   int32_t timeout,
					   const char *caller,
					   int line)
#else /* CONFIG_NET_DEBUG_NET_BUF */
struct net_buf *net_nbuf_get_reserve(struct net_buf_pool *pool,
				     uint16_t reserve_head,
				     int32_t timeout)
#endif /* CONFIG_NET_DEBUG_NET_BUF */
{
	struct net_buf *buf;

	/*
	 * The reserve_head variable in the function will tell
	 * the size of the link layer headers if there are any.
	 */

	if (k_is_in_isr()) {
		buf = net_buf_alloc(pool, K_NO_WAIT);
	} else {
		buf = net_buf_alloc(pool, timeout);
	}

	if (!buf) {
		return NULL;
	}

	if (is_data_pool(pool)) {
		/* The buf->data will point to the start of the L3
		 * header (like IPv4 or IPv6 packet header).
		 */
		net_buf_reserve(buf, reserve_head);
	} else {
		memset(net_buf_user_data(buf), 0, sizeof(struct net_nbuf));

		net_nbuf_set_ll_reserve(buf, reserve_head);

		/* Remember the RX vs. TX so that the fragments can be
		 * allocated from correct DATA pool.
		 */
		if (pool == &rx_bufs) {
			net_nbuf_set_dir(buf, NET_RX);
		}
	}

	NET_BUF_CHECK_IF_NOT_IN_USE(buf, buf->ref + 1);

#if defined(CONFIG_NET_DEBUG_NET_BUF)
	nbuf_alloc_add(buf, caller, line);

	NET_DBG("%s (%s) [%d] buf %p reserve %u ref %d (%s():%d)",
		pool2str(pool), pool->name, get_frees(pool),
		buf, reserve_head, buf->ref, caller, line);
#endif

	return buf;
}

/* Get a fragment, try to figure out the pool from where to get
 * the data.
 */
#if defined(CONFIG_NET_DEBUG_NET_BUF)
struct net_buf *net_nbuf_get_frag_debug(struct net_buf *buf,
					int32_t timeout,
					const char *caller, int line)
#else
struct net_buf *net_nbuf_get_frag(struct net_buf *buf,
				  int32_t timeout)
#endif
{
#if defined(CONFIG_NET_CONTEXT_NBUF_POOL)
	struct net_context *context;
#endif

	if (is_from_data_pool(buf)) {
		/* We cannot know the correct data pool in this case (because
		 * we do not know the context). Return error to the caller.
		 */
		return NULL;
	}

#if defined(CONFIG_NET_CONTEXT_NBUF_POOL)
	context = net_nbuf_context(buf);
	if (context && context->data_pool) {
#if defined(CONFIG_NET_DEBUG_NET_BUF)
		return net_nbuf_get_reserve_debug(context->data_pool(),
						  net_nbuf_ll_reserve(buf),
						  timeout, caller, line);
#else
		return net_nbuf_get_reserve(context->data_pool(),
					    net_nbuf_ll_reserve(buf), timeout);
#endif /* CONFIG_NET_DEBUG_NET_BUF */
	}
#endif /* CONFIG_NET_CONTEXT_NBUF_POOL */

	if (net_nbuf_dir(buf) == NET_RX) {
#if defined(CONFIG_NET_DEBUG_NET_BUF)
		return net_nbuf_get_reserve_rx_data_debug(
			net_nbuf_ll_reserve(buf), timeout, caller, line);
#else
		return net_nbuf_get_reserve_rx_data(net_nbuf_ll_reserve(buf),
						    timeout);
#endif
	}

#if defined(CONFIG_NET_DEBUG_NET_BUF)
	return net_nbuf_get_reserve_tx_data_debug(net_nbuf_ll_reserve(buf),
						  timeout, caller, line);
#else
	return net_nbuf_get_reserve_tx_data(net_nbuf_ll_reserve(buf),
					    timeout);
#endif
}

#if defined(CONFIG_NET_DEBUG_NET_BUF)
struct net_buf *net_nbuf_get_reserve_rx_debug(uint16_t reserve_head,
					      int32_t timeout,
					      const char *caller, int line)
{
	return net_nbuf_get_reserve_debug(&rx_bufs, reserve_head, timeout,
					  caller, line);
}

struct net_buf *net_nbuf_get_reserve_tx_debug(uint16_t reserve_head,
					      int32_t timeout,
					      const char *caller, int line)
{
	return net_nbuf_get_reserve_debug(&tx_bufs, reserve_head, timeout,
					  caller, line);
}

struct net_buf *net_nbuf_get_reserve_rx_data_debug(uint16_t reserve_head,
						   int32_t timeout,
						   const char *caller, int line)
{
	return net_nbuf_get_reserve_debug(&data_rx_bufs, reserve_head,
					  timeout, caller, line);
}

struct net_buf *net_nbuf_get_reserve_tx_data_debug(uint16_t reserve_head,
						   int32_t timeout,
						   const char *caller, int line)
{
	return net_nbuf_get_reserve_debug(&data_tx_bufs, reserve_head,
					  timeout, caller, line);
}

#else /* CONFIG_NET_DEBUG_NET_BUF */

struct net_buf *net_nbuf_get_reserve_rx(uint16_t reserve_head,
					int32_t timeout)
{
	return net_nbuf_get_reserve(&rx_bufs, reserve_head, timeout);
}

struct net_buf *net_nbuf_get_reserve_tx(uint16_t reserve_head,
					int32_t timeout)
{
	return net_nbuf_get_reserve(&tx_bufs, reserve_head, timeout);
}

struct net_buf *net_nbuf_get_reserve_rx_data(uint16_t reserve_head,
					     int32_t timeout)
{
	return net_nbuf_get_reserve(&data_rx_bufs, reserve_head, timeout);
}

struct net_buf *net_nbuf_get_reserve_tx_data(uint16_t reserve_head,
					     int32_t timeout)
{
	return net_nbuf_get_reserve(&data_tx_bufs, reserve_head, timeout);
}

#endif /* CONFIG_NET_DEBUG_NET_BUF */


#if defined(CONFIG_NET_DEBUG_NET_BUF)
static struct net_buf *net_nbuf_get_debug(struct net_buf_pool *pool,
					  struct net_context *context,
					  int32_t timeout,
					  const char *caller, int line)
#else
static struct net_buf *net_nbuf_get(struct net_buf_pool *pool,
				    struct net_context *context,
				    int32_t timeout)
#endif /* CONFIG_NET_DEBUG_NET_BUF */
{
	struct in6_addr *addr6 = NULL;
	struct net_if *iface;
	struct net_buf *buf;

	if (!context) {
		return NULL;
	}

	iface = net_context_get_iface(context);

	NET_ASSERT(iface);

	if (net_context_get_family(context) == AF_INET6) {
		addr6 = &((struct sockaddr_in6 *) &context->remote)->sin6_addr;
	}

#if defined(CONFIG_NET_DEBUG_NET_BUF)
	buf = net_nbuf_get_reserve_debug(pool,
					 net_if_get_ll_reserve(iface, addr6),
					 timeout, caller, line);
#else
	buf = net_nbuf_get_reserve(pool, net_if_get_ll_reserve(iface, addr6),
				   timeout);
#endif
	if (!buf) {
		return buf;
	}

	if (!is_data_pool(pool)) {
		net_nbuf_set_context(buf, context);
		net_nbuf_set_iface(buf, iface);

		if (context) {
			net_nbuf_set_family(buf,
					    net_context_get_family(context));
		}
	}

	return buf;
}

#if defined(CONFIG_NET_DEBUG_NET_BUF)
struct net_buf *net_nbuf_get_rx_debug(struct net_context *context,
				      int32_t timeout,
				      const char *caller, int line)
{
	return net_nbuf_get_debug(&rx_bufs, context, timeout, caller, line);
}

struct net_buf *net_nbuf_get_tx_debug(struct net_context *context,
				      int32_t timeout,
				      const char *caller, int line)
{
	struct net_buf_pool *pool = get_tx_pool(context);

	return net_nbuf_get_debug(pool ? pool : &tx_bufs, context,
				  timeout, caller, line);
}

struct net_buf *net_nbuf_get_data_debug(struct net_context *context,
					int32_t timeout,
					const char *caller, int line)
{
	struct net_buf_pool *pool = get_data_pool(context);

	return net_nbuf_get_debug(pool ? pool : &data_tx_bufs, context,
				  timeout, caller, line);
}

#else /* CONFIG_NET_DEBUG_NET_BUF */

struct net_buf *net_nbuf_get_rx(struct net_context *context, int32_t timeout)
{
	NET_ASSERT_INFO(context, "RX context not set");

	return net_nbuf_get(&rx_bufs, context, timeout);
}

struct net_buf *net_nbuf_get_tx(struct net_context *context, int32_t timeout)
{
	struct net_buf_pool *pool;

	NET_ASSERT_INFO(context, "TX context not set");

	pool = get_tx_pool(context);

	return net_nbuf_get(pool ? pool : &tx_bufs, context, timeout);
}

struct net_buf *net_nbuf_get_data(struct net_context *context, int32_t timeout)
{
	struct net_buf_pool *pool;

	NET_ASSERT_INFO(context, "Data context not set");

	pool = get_data_pool(context);

	/* The context is not known in RX path so we can only have TX
	 * data here.
	 */
	return net_nbuf_get(pool ? pool : &data_tx_bufs, context, timeout);
}

#endif /* CONFIG_NET_DEBUG_NET_BUF */


#if defined(CONFIG_NET_DEBUG_NET_BUF)
void net_nbuf_unref_debug(struct net_buf *buf, const char *caller, int line)
{
	struct net_buf *frag;

#else
void net_nbuf_unref(struct net_buf *buf)
{
#endif /* CONFIG_NET_DEBUG_NET_BUF */
	if (!buf) {
		NET_DBG("*** ERROR *** buf %p (%s():%d)", buf, caller, line);
		return;
	}

	if (!buf->ref) {
#if defined(CONFIG_NET_DEBUG_NET_BUF)
		const char *func_freed;
		int line_freed;

		if (nbuf_alloc_find(buf, &func_freed, &line_freed)) {
			NET_DBG("*** ERROR *** buf %p is freed already by "
				"%s():%d (%s():%d)",
				buf, func_freed, line_freed, caller, line);
		} else {
			NET_DBG("*** ERROR *** buf %p is freed already "
				"(%s():%d)", buf, caller, line);
		}
#endif
		return;
	}

#if defined(CONFIG_NET_DEBUG_NET_BUF)
	NET_DBG("%s (%s) [%d] buf %p ref %d frags %p (%s():%d)",
		pool2str(buf->pool), buf->pool->name, get_frees(buf->pool),
		buf, buf->ref - 1, buf->frags, caller, line);

	if (buf->ref > 1) {
		goto done;
	}

	frag = buf->frags;
	while (frag) {
		NET_DBG("%s (%s) [%d] buf %p ref %d frags %p (%s():%d)",
			pool2str(frag->pool), frag->pool->name,
			get_frees(frag->pool), frag, frag->ref - 1,
			frag->frags, caller, line);

		if (!frag->ref) {
			const char *func_freed;
			int line_freed;

			if (nbuf_alloc_find(frag, &func_freed, &line_freed)) {
				NET_DBG("*** ERROR *** frag %p is freed "
					"already by %s():%d (%s():%d)",
					frag, func_freed, line_freed,
					caller, line);
			} else {
				NET_DBG("*** ERROR *** frag %p is freed "
					"already (%s():%d)",
					frag, caller, line);
			}
		}

		nbuf_alloc_del(frag, caller, line);

		frag = frag->frags;
	}

	nbuf_alloc_del(buf, caller, line);

done:
#endif /* CONFIG_NET_DEBUG_NET_BUF */
	net_buf_unref(buf);
}


#if defined(CONFIG_NET_DEBUG_NET_BUF)
struct net_buf *net_nbuf_ref_debug(struct net_buf *buf, const char *caller,
				   int line)
#else
struct net_buf *net_nbuf_ref(struct net_buf *buf)
#endif /* CONFIG_NET_DEBUG_NET_BUF */
{
	if (!buf) {
		NET_DBG("*** ERROR *** buf %p (%s():%d)", buf, caller, line);
		return NULL;
	}

#if defined(CONFIG_NET_DEBUG_NET_BUF)
	NET_DBG("%s (%s) [%d] buf %p ref %d (%s():%d)",
		pool2str(buf->pool), buf->pool->name, get_frees(buf->pool),
		buf, buf->ref + 1, caller, line);
#endif

	return net_buf_ref(buf);
}

#if defined(CONFIG_NET_DEBUG_NET_BUF)
struct net_buf *net_nbuf_frag_del_debug(struct net_buf *parent,
					struct net_buf *frag,
					const char *caller, int line)
#else
struct net_buf *net_nbuf_frag_del(struct net_buf *parent,
				  struct net_buf *frag)
#endif
{
#if defined(CONFIG_NET_DEBUG_NET_BUF)
	if (frag->ref == 1) {
		nbuf_alloc_del(frag, caller, line);
	}
#endif

	return net_buf_frag_del(parent, frag);
}

struct net_buf *net_nbuf_copy(struct net_buf *buf, size_t amount,
			      size_t reserve, int32_t timeout)
{
	struct net_buf *frag, *first, *orig;
	uint8_t *orig_data;
	size_t orig_len;

	if (is_from_data_pool(buf)) {
		NET_ERR("Buffer %p should not be a data fragment", buf);
		return NULL;
	}

	orig = buf->frags;

	frag = net_nbuf_get_frag(buf, timeout);
	if (!frag) {
		return NULL;
	}

	if (reserve > net_buf_tailroom(frag)) {
		NET_ERR("Reserve %zu is too long, max is %zu",
			reserve, net_buf_tailroom(frag));
		net_nbuf_unref(frag);
		return NULL;
	}

	net_buf_add(frag, reserve);

	first = frag;

	NET_DBG("Copying frag %p with %zu bytes and reserving %zu bytes",
		first, amount, reserve);

	if (!orig->len) {
		/* No data in the first fragment in the original message */
		NET_DBG("Original buffer empty!");
		return frag;
	}

	orig_len = orig->len;
	orig_data = orig->data;

	while (orig && amount) {
		int left_len = net_buf_tailroom(frag);
		int copy_len;

		if (amount > orig_len) {
			copy_len = orig_len;
		} else {
			copy_len = amount;
		}

		if ((copy_len - left_len) >= 0) {
			/* Just copy the data from original fragment
			 * to new fragment. The old data will fit the
			 * new fragment and there could be some space
			 * left in the new fragment.
			 */
			amount -= left_len;

			memcpy(net_buf_add(frag, left_len), orig_data,
			       left_len);

			if (!net_buf_tailroom(frag)) {
				/* There is no space left in copy fragment.
				 * We must allocate a new one.
				 */
				struct net_buf *new_frag =
					net_nbuf_get_frag(buf, timeout);
				if (!new_frag) {
					net_nbuf_unref(first);
					return NULL;
				}

				net_buf_frag_add(frag, new_frag);

				frag = new_frag;
			}

			orig_len -= left_len;
			orig_data += left_len;

			continue;
		} else {
			/* We should be at the end of the original buf
			 * fragment list.
			 */
			amount -= copy_len;

			memcpy(net_buf_add(frag, copy_len), orig_data,
			       copy_len);
		}

		orig = orig->frags;
		if (orig) {
			orig_len = orig->len;
			orig_data = orig->data;
		}
	}

	return first;
}

int net_nbuf_linear_copy(struct net_buf *dst, struct net_buf *src,
			 uint16_t offset, uint16_t len)
{
	uint16_t to_copy;
	uint16_t copied;

	if (dst->size < len) {
		return -ENOMEM;
	}

	/* find the right fragment to start copying from */
	while (src && offset >= src->len) {
		offset -= src->len;
		src = src->frags;
	}

	/* traverse the fragment chain until len bytes are copied */
	copied = 0;
	while (src && len > 0) {
		to_copy = min(len, src->len - offset);
		memcpy(dst->data + copied, src->data + offset, to_copy);

		copied += to_copy;
		/* to_copy is always <= len */
		len -= to_copy;
		src = src->frags;
		/* after the first iteration, this value will be 0 */
		offset = 0;
	}

	if (len > 0) {
		return -ENOMEM;
	}

	dst->len = copied;

	return 0;
}

bool net_nbuf_is_compact(struct net_buf *buf)
{
	struct net_buf *last;
	size_t total = 0, calc;
	int count = 0;

	last = NULL;

	if (!is_from_data_pool(buf)) {
		/* Skip the first element that does not contain any data.
		 */
		buf = buf->frags;
	}

	while (buf) {
		total += buf->len;
		count++;

		last = buf;
		buf = buf->frags;
	}

	NET_ASSERT(last);

	if (!last) {
		return false;
	}

	calc = count * last->size - net_buf_tailroom(last) -
		count * net_buf_headroom(last);

	if (total == calc) {
		return true;
	}

	NET_DBG("Not compacted total %zu real %zu", total, calc);

	return false;
}

bool net_nbuf_compact(struct net_buf *buf)
{
	struct net_buf *prev;

	if (is_from_data_pool(buf)) {
		NET_DBG("Buffer %p is a data fragment", buf);
		return false;
	}

	NET_DBG("Compacting data to buf %p", buf);

	buf = buf->frags;
	prev = NULL;

	while (buf) {
		if (buf->frags) {
			/* Copy amount of data from next fragment to this
			 * fragment.
			 */
			size_t copy_len;

			copy_len = buf->frags->len;
			if (copy_len > net_buf_tailroom(buf)) {
				copy_len = net_buf_tailroom(buf);
			}

			memcpy(net_buf_tail(buf), buf->frags->data, copy_len);
			net_buf_add(buf, copy_len);

			memmove(buf->frags->data,
				buf->frags->data + copy_len,
				buf->frags->len - copy_len);

			buf->frags->len -= copy_len;

			/* Is there any more space in this fragment */
			if (net_buf_tailroom(buf)) {
				/* There is. This also means that the next
				 * fragment is empty as otherwise we could
				 * not have copied all data. Remove next
				 * fragment as there is no data in it any more.
				 */
				net_nbuf_frag_del(buf, buf->frags);

				/* Then check next fragment */
				continue;
			}
		} else {
			if (!buf->len) {
				/* Remove the last fragment because there is no
				 * data in it.
				 */
				net_nbuf_frag_del(prev, buf);

				break;
			}
		}

		prev = buf;
		buf = buf->frags;
	}

	return true;
}

struct net_buf *net_nbuf_pull(struct net_buf *buf, size_t amount)
{
	struct net_buf *first;
	ssize_t count = amount;

	if (amount == 0) {
		NET_DBG("No data to remove.");
		return buf;
	}

	first = buf;

	if (!is_from_data_pool(buf)) {
		NET_DBG("Buffer %p is not a data fragment", buf);
		buf = buf->frags;
	}

	NET_DBG("Removing first %zd bytes from the fragments (%zu bytes)",
		count, net_buf_frags_len(buf));

	while (buf && count > 0) {
		if (count < buf->len) {
			/* We can remove the data from this single fragment */
			net_buf_pull(buf, count);
			return first;
		}

		if (buf->len == count) {
			if (buf == first) {
				struct net_buf tmp;

				tmp.frags = buf;
				first = buf->frags;
				net_nbuf_frag_del(&tmp, buf);
			} else {
				net_nbuf_frag_del(first, buf);
			}

			return first;
		}

		count -= buf->len;

		if (buf == first) {
			struct net_buf tmp;

			tmp.frags = buf;
			first = buf->frags;
			net_nbuf_frag_del(&tmp, buf);
			buf = first;
		} else {
			net_nbuf_frag_del(first, buf);
			buf = first->frags;
		}
	}

	if (count > 0) {
		NET_ERR("Not enough data in the fragments");
	}

	return first;
}

/* This helper routine will append multiple bytes, if there is no place for
 * the data in current fragment then create new fragment and add it to
 * the buffer. It assumes that the buffer has at least one fragment.
 */
static inline bool net_nbuf_append_bytes(struct net_buf *buf,
					 const uint8_t *value,
					 uint16_t len, int32_t timeout)
{
	struct net_buf *frag = net_buf_frag_last(buf);

	do {
		uint16_t count = min(len, net_buf_tailroom(frag));
		void *data = net_buf_add(frag, count);

		memcpy(data, value, count);
		len -= count;
		value += count;

		if (len == 0) {
			return true;
		}

		frag = net_nbuf_get_frag(buf, timeout);
		if (!frag) {
			return false;
		}

		net_buf_frag_add(buf, frag);
	} while (1);

	return false;
}

bool net_nbuf_append(struct net_buf *buf, uint16_t len, const uint8_t *data,
		     int32_t timeout)
{
	struct net_buf *frag;

	if (!buf || !data) {
		return false;
	}

	if (is_from_data_pool(buf)) {
		/* The buf needs to be a net_buf that has user data
		 * part. Otherwise we cannot use the net_nbuf_ll_reserve()
		 * function to figure out the reserve amount.
		 */
		NET_DBG("Buffer %p is a data fragment", buf);
		return false;
	}

	if (!buf->frags) {
		frag = net_nbuf_get_frag(buf, timeout);
		if (!frag) {
			return false;
		}

		net_buf_frag_add(buf, frag);
	}

	return net_nbuf_append_bytes(buf, data, len, timeout);
}

/* Helper routine to retrieve single byte from fragment and move
 * offset. If required byte is last byte in framgent then return
 * next fragment and set offset = 0.
 */
static inline struct net_buf *net_nbuf_read_byte(struct net_buf *buf,
						 uint16_t offset,
						 uint16_t *pos,
						 uint8_t *data)
{
	if (data) {
		*data = buf->data[offset];
	}

	*pos = offset + 1;

	if (*pos >= buf->len) {
		*pos = 0;

		return buf->frags;
	}

	return buf;
}

/* Helper function to adjust offset in net_nbuf_read() call
 * if given offset is more than current fragment length.
 */
static inline struct net_buf *adjust_offset(struct net_buf *buf,
					    uint16_t offset, uint16_t *pos)
{
	if (!buf || !is_from_data_pool(buf)) {
		NET_ERR("Invalid buffer or buffer is not a fragment");
		return NULL;
	}

	while (buf) {
		if (offset == buf->len) {
			*pos = 0;

			return buf->frags;
		} else if (offset < buf->len) {
			*pos = offset;

			return buf;
		}

		offset -= buf->len;
		buf = buf->frags;
	}

	NET_ERR("Invalid offset (%u), failed to adjust", offset);

	return NULL;
}

struct net_buf *net_nbuf_read(struct net_buf *buf, uint16_t offset,
			      uint16_t *pos, uint16_t len, uint8_t *data)
{
	uint16_t copy = 0;

	buf = adjust_offset(buf, offset, pos);
	if (!buf) {
		goto error;
	}

	while (len-- > 0 && buf) {
		if (data) {
			buf = net_nbuf_read_byte(buf, *pos, pos, data + copy++);
		} else {
			buf = net_nbuf_read_byte(buf, *pos, pos, NULL);
		}

		/* Error: Still reamining length to be read, but no data. */
		if (!buf && len) {
			NET_ERR("Not enough data to read");
			goto error;
		}
	}

	return buf;

error:
	*pos = 0xffff;

	return NULL;
}

struct net_buf *net_nbuf_read_be16(struct net_buf *buf, uint16_t offset,
				   uint16_t *pos, uint16_t *value)
{
	struct net_buf *retbuf;
	uint8_t v16[2];

	retbuf = net_nbuf_read(buf, offset, pos, sizeof(uint16_t), v16);

	*value = v16[0] << 8 | v16[1];

	return retbuf;
}

struct net_buf *net_nbuf_read_be32(struct net_buf *buf, uint16_t offset,
				   uint16_t *pos, uint32_t *value)
{
	struct net_buf *retbuf;
	uint8_t v32[4];

	retbuf = net_nbuf_read(buf, offset, pos, sizeof(uint32_t), v32);

	*value = v32[0] << 24 | v32[1] << 16 | v32[2] << 8 | v32[3];

	return retbuf;
}

static inline struct net_buf *check_and_create_data(struct net_buf *buf,
						    struct net_buf *data,
						    int32_t timeout)
{
	struct net_buf *frag;

	if (data) {
		return data;
	}

	frag = net_nbuf_get_frag(buf, timeout);
	if (!frag) {
		return NULL;
	}

	net_buf_frag_add(buf, frag);

	return frag;
}

static inline struct net_buf *adjust_write_offset(struct net_buf *buf,
						  struct net_buf *frag,
						  uint16_t offset,
						  uint16_t *pos,
						  int32_t timeout)
{
	uint16_t tailroom;

	do {
		frag = check_and_create_data(buf, frag, timeout);
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

		/* Offset is equal to fragment length. If some tailtoom exists,
		 * offset start from same fragment otherwise offset starts from
		 * beginning of next fragment.
		 */
		if (offset == frag->len) {
			if (net_buf_tailroom(frag)) {
				*pos = offset;

				return frag;
			}

			*pos = 0;

			return check_and_create_data(buf, frag->frags,
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

				*pos = 0;

				return check_and_create_data(buf,
							     frag->frags,
							     timeout);
			}

			if (offset > tailroom) {
				/* Creating empty space */
				net_buf_add(frag, tailroom);
				offset -= tailroom;

				frag = check_and_create_data(buf,
							     frag->frags,
							     timeout);
			}
		}

	} while (1);

	return NULL;
}

struct net_buf *net_nbuf_write(struct net_buf *buf, struct net_buf *frag,
			       uint16_t offset, uint16_t *pos,
			       uint16_t len, uint8_t *data,
			       int32_t timeout)
{
	if (!buf || is_from_data_pool(buf)) {
		NET_ERR("Invalid buffer or it is data fragment");
		goto error;
	}

	frag = adjust_write_offset(buf, frag, offset, &offset, timeout);
	if (!frag) {
		NET_DBG("Failed to adjust offset (%u)", offset);
		goto error;
	}

	do {
		uint16_t space = frag->size - net_buf_headroom(frag) - offset;
		uint16_t count = min(len, space);
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
		offset = 0;
		frag = frag->frags;

		if (!frag) {
			frag = net_nbuf_get_frag(buf, timeout);
			if (!frag) {
				goto error;
			}

			net_buf_frag_add(buf, frag);
		}
	} while (1);

error:
	*pos = 0xffff;

	return NULL;
}

static inline bool insert_data(struct net_buf *buf, struct net_buf *frag,
			       struct net_buf *temp, uint16_t offset,
			       uint16_t len, uint8_t *data,
			       int32_t timeout)
{
	struct net_buf *insert;

	do {
		uint16_t count = min(len, net_buf_tailroom(frag));

		/* Copy insert data */
		memcpy(frag->data + offset, data, count);
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
			net_nbuf_compact(buf->frags);

			return true;
		}

		data += count;
		offset = 0;

		insert = net_nbuf_get_frag(buf, timeout);
		if (!insert) {
			return false;
		}

		net_buf_frag_insert(frag, insert);
		frag = insert;

	} while (1);

	return false;
}

static inline struct net_buf *adjust_insert_offset(struct net_buf *buf,
						   uint16_t offset,
						   uint16_t *pos)
{
	if (!buf || !is_from_data_pool(buf)) {
		NET_ERR("Invalid buffer or buffer is not a fragment");

		return NULL;
	}

	while (buf) {
		if (offset == buf->len) {
			*pos = 0;

			return buf->frags;
		}

		if (offset < buf->len) {
			*pos = offset;

			return buf;
		}

		if (offset > buf->len) {
			if (buf->frags) {
				offset -= buf->len;
				buf = buf->frags;
			} else {
				return NULL;
			}
		}
	}

	NET_ERR("Invalid offset, failed to adjust");

	return NULL;
}

bool net_nbuf_insert(struct net_buf *buf, struct net_buf *frag,
		     uint16_t offset, uint16_t len, uint8_t *data,
		     int32_t timeout)
{
	struct net_buf *temp = NULL;
	uint16_t bytes;

	if (!buf || is_from_data_pool(buf)) {
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
		temp = net_nbuf_get_frag(buf, timeout);
		if (!temp) {
			return false;
		}

		memcpy(net_buf_add(temp, bytes), frag->data + offset, bytes);

		frag->len -= bytes;
	}

	/* Insert data into current(frag) fragment from "offset". */
	return insert_data(buf, frag, temp, offset, len, data, timeout);
}

void net_nbuf_get_info(struct net_buf_pool **rx,
		       struct net_buf_pool **tx,
		       struct net_buf_pool **rx_data,
		       struct net_buf_pool **tx_data)
{
	if (rx) {
		*rx = &rx_bufs;
	}

	if (tx) {
		*tx = &tx_bufs;
	}

	if (rx_data) {
		*rx_data = &data_rx_bufs;
	}

	if (tx_data) {
		*tx_data = &data_tx_bufs;
	}
}

#if defined(CONFIG_NET_DEBUG_NET_BUF)
void net_nbuf_print(void)
{
	NET_DBG("TX %d RX %d RDATA %d TDATA %d", tx_bufs.avail_count,
		rx_bufs.avail_count, data_rx_bufs.avail_count,
		data_tx_bufs.avail_count);
}
#endif /* CONFIG_NET_DEBUG_NET_BUF */

void net_nbuf_init(void)
{
	NET_DBG("Allocating %d RX (%u bytes), %d TX (%u bytes), "
		"%d RX data (%u bytes) and %d TX data (%u bytes) buffers",
		get_frees(&rx_bufs), rx_bufs.pool_size,
		get_frees(&tx_bufs), tx_bufs.pool_size,
		get_frees(&data_rx_bufs), data_rx_bufs.pool_size,
		get_frees(&data_tx_bufs), data_tx_bufs.pool_size);
}
