/** @file
 * @brief Network buffer API
 *
 * Network data is passed between different parts of the stack via
 * net_buf struct.
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

/* Data buffer API - used for all data to/from net */

#ifndef __NBUF_H
#define __NBUF_H

#include <stdint.h>
#include <stdbool.h>

#include <net/buf.h>

#include <net/net_core.h>
#include <net/net_linkaddr.h>
#include <net/net_ip.h>
#include <net/net_if.h>
#include <net/net_context.h>

#ifdef __cplusplus
extern "C" {
#endif

struct net_context;

/** @cond ignore */
enum net_nbuf_type {
	NET_NBUF_RX = 0,
	NET_NBUF_TX = 1,
	NET_NBUF_DATA = 2,
};
/** @endcond */

struct net_nbuf {
	/** @cond ignore */
	enum net_nbuf_type type;
	uint16_t reserve; /* length of the protocol headers */
	uint8_t ll_reserve; /* link layer header length */
	uint8_t family; /* IPv4 vs IPv6 */
	uint8_t ip_hdr_len; /* pre-filled in order to avoid func call */
	uint8_t ext_len; /* length of extension headers */
	uint8_t ext_bitmap;
	uint8_t *next_hdr;

	/* Filled by layer 2 when network packet is received. */
	struct net_linkaddr lladdr_src;
	struct net_linkaddr lladdr_dst;

#if defined(CONFIG_NET_IPV6)
	uint8_t ext_opt_len; /* IPv6 ND option length */
#endif
	/* @endcond */

	/** Network connection context */
	struct net_context *context;

	/** Network context token that user can set. This is passed
	 * to user callback when data has been sent.
	 */
	void *token;

	/** Network interface */
	struct net_if *iface;

	/** @cond ignore */
	uint8_t *appdata;  /* application data */
	uint16_t appdatalen;
	/* @endcond */
};

/** @cond ignore */


/* The interface real ll address */
static inline struct net_linkaddr *net_nbuf_ll_if(struct net_buf *buf)
{
	return net_if_get_link_addr(
		((struct net_nbuf *)net_buf_user_data(buf))->iface);
}

static inline struct net_context *net_nbuf_context(struct net_buf *buf)
{
	return ((struct net_nbuf *)net_buf_user_data(buf))->context;
}

static inline void net_nbuf_set_context(struct net_buf *buf,
					struct net_context *ctx)
{
	((struct net_nbuf *)net_buf_user_data(buf))->context = ctx;
}

static inline void *net_nbuf_token(struct net_buf *buf)
{
	return ((struct net_nbuf *)net_buf_user_data(buf))->token;
}

static inline void net_nbuf_set_token(struct net_buf *buf, void *token)
{
	((struct net_nbuf *)net_buf_user_data(buf))->token = token;
}

static inline struct net_if *net_nbuf_iface(struct net_buf *buf)
{
	return ((struct net_nbuf *)net_buf_user_data(buf))->iface;
}

static inline void net_nbuf_set_iface(struct net_buf *buf, struct net_if *iface)
{
	((struct net_nbuf *)net_buf_user_data(buf))->iface = iface;
}

static inline enum net_nbuf_type net_nbuf_type(struct net_buf *buf)
{
	return ((struct net_nbuf *)net_buf_user_data(buf))->type;
}

static inline void net_nbuf_set_type(struct net_buf *buf, uint8_t type)
{
	((struct net_nbuf *)net_buf_user_data(buf))->type = type;
}

static inline uint8_t net_nbuf_family(struct net_buf *buf)
{
	return ((struct net_nbuf *)net_buf_user_data(buf))->family;
}

static inline void net_nbuf_set_family(struct net_buf *buf, uint8_t family)
{
	((struct net_nbuf *)net_buf_user_data(buf))->family = family;
}

static inline uint8_t net_nbuf_ip_hdr_len(struct net_buf *buf)
{
	return ((struct net_nbuf *) net_buf_user_data(buf))->ip_hdr_len;
}

static inline void net_nbuf_set_ip_hdr_len(struct net_buf *buf, uint8_t len)
{
	((struct net_nbuf *) net_buf_user_data(buf))->ip_hdr_len = len;
}

static inline uint8_t net_nbuf_ext_len(struct net_buf *buf)
{
	return ((struct net_nbuf *)net_buf_user_data(buf))->ext_len;
}

static inline void net_nbuf_set_ext_len(struct net_buf *buf, uint8_t len)
{
	((struct net_nbuf *)net_buf_user_data(buf))->ext_len = len;
}

static inline uint8_t net_nbuf_ext_bitmap(struct net_buf *buf)
{
	return ((struct net_nbuf *)net_buf_user_data(buf))->ext_bitmap;
}

static inline void net_nbuf_set_ext_bitmap(struct net_buf *buf, uint8_t bm)
{
	((struct net_nbuf *)net_buf_user_data(buf))->ext_bitmap = bm;
}

static inline uint8_t *net_nbuf_next_hdr(struct net_buf *buf)
{
	return ((struct net_nbuf *)net_buf_user_data(buf))->next_hdr;
}

static inline void net_nbuf_set_next_hdr(struct net_buf *buf, uint8_t *hdr)
{
	((struct net_nbuf *)net_buf_user_data(buf))->next_hdr = hdr;
}

#if defined(CONFIG_NET_IPV6)
static inline uint8_t net_nbuf_ext_opt_len(struct net_buf *buf)
{
	return ((struct net_nbuf *)net_buf_user_data(buf))->ext_opt_len;
}

static inline void net_nbuf_set_ext_opt_len(struct net_buf *buf, uint8_t len)
{
	((struct net_nbuf *)net_buf_user_data(buf))->ext_opt_len = len;
}
#endif

static inline uint16_t net_nbuf_get_len(struct net_buf *buf)
{
	return buf->len;
}

static inline void net_nbuf_set_len(struct net_buf *buf, uint16_t len)
{
	buf->len = len;
}

static inline uint8_t *net_nbuf_ip_data(struct net_buf *buf)
{
	return buf->frags->data;
}

static inline uint8_t *net_nbuf_udp_data(struct net_buf *buf)
{
	return &buf->frags->data[net_nbuf_ip_hdr_len(buf)];
}

static inline uint8_t *net_nbuf_tcp_data(struct net_buf *buf)
{
	return &buf->frags->data[net_nbuf_ip_hdr_len(buf)];
}

static inline uint8_t *net_nbuf_icmp_data(struct net_buf *buf)
{
	return &buf->frags->data[net_nbuf_ip_hdr_len(buf) +
				 net_nbuf_ext_len(buf)];
}

static inline uint8_t *net_nbuf_appdata(struct net_buf *buf)
{
	return ((struct net_nbuf *)net_buf_user_data(buf))->appdata;
}

static inline void net_nbuf_set_appdata(struct net_buf *buf, uint8_t *data)
{
	((struct net_nbuf *)net_buf_user_data(buf))->appdata = data;
}

static inline uint16_t net_nbuf_appdatalen(struct net_buf *buf)
{
	return ((struct net_nbuf *)net_buf_user_data(buf))->appdatalen;
}

static inline void net_nbuf_set_appdatalen(struct net_buf *buf, uint16_t len)
{
	((struct net_nbuf *)net_buf_user_data(buf))->appdatalen = len;
}

static inline uint16_t net_nbuf_reserve(struct net_buf *buf)
{
	return ((struct net_nbuf *)net_buf_user_data(buf))->reserve;
}

static inline uint8_t net_nbuf_ll_reserve(struct net_buf *buf)
{
	return ((struct net_nbuf *) net_buf_user_data(buf))->ll_reserve;
}

static inline void net_nbuf_set_ll_reserve(struct net_buf *buf, uint8_t len)
{
	((struct net_nbuf *) net_buf_user_data(buf))->ll_reserve = len;
}

static inline uint8_t *net_nbuf_ll(struct net_buf *buf)
{
	return net_nbuf_ip_data(buf) - net_nbuf_ll_reserve(buf);
}

static inline struct net_linkaddr *net_nbuf_ll_src(struct net_buf *buf)
{
	return &((struct net_nbuf *)net_buf_user_data(buf))->lladdr_src;
}

static inline struct net_linkaddr *net_nbuf_ll_dst(struct net_buf *buf)
{
	return &((struct net_nbuf *)net_buf_user_data(buf))->lladdr_dst;
}

static inline void net_nbuf_ll_clear(struct net_buf *buf)
{
	memset(net_nbuf_ll(buf), 0, net_nbuf_ll_reserve(buf));
	net_nbuf_ll_src(buf)->addr = NULL;
	net_nbuf_ll_src(buf)->len = 0;
}

static inline void net_nbuf_ll_swap(struct net_buf *buf)
{
	uint8_t *addr = net_nbuf_ll_src(buf)->addr;

	net_nbuf_ll_src(buf)->addr = net_nbuf_ll_dst(buf)->addr;
	net_nbuf_ll_dst(buf)->addr = addr;
}

#define NET_IPV6_BUF(buf) ((struct net_ipv6_hdr *)net_nbuf_ip_data(buf))
#define NET_IPV4_BUF(buf) ((struct net_ipv4_hdr *)net_nbuf_ip_data(buf))
#define NET_ICMP_BUF(buf) ((struct net_icmp_hdr *)net_nbuf_icmp_data(buf))
#define NET_UDP_BUF(buf)  ((struct net_udp_hdr *)(net_nbuf_udp_data(buf)))

static inline void net_nbuf_set_src_ipv6_addr(struct net_buf *buf)
{
	net_if_ipv6_select_src_addr(net_context_get_iface(
					    net_nbuf_context(buf)),
				    &NET_IPV6_BUF(buf)->src);
}

/* @endcond */

#if defined(CONFIG_NET_DEBUG_NET_BUF)

/* Debug versions of the nbuf functions that are used when tracking
 * buffer usage.
 */

struct net_buf *net_nbuf_get_rx_debug(struct net_context *context,
				      const char *caller, int line);
#define net_nbuf_get_rx(context) \
	net_nbuf_get_rx_debug(context, __func__, __LINE__)

struct net_buf *net_nbuf_get_tx_debug(struct net_context *context,
				      const char *caller, int line);
#define net_nbuf_get_tx(context) \
	net_nbuf_get_tx_debug(context, __func__, __LINE__)

struct net_buf *net_nbuf_get_data_debug(struct net_context *context,
					const char *caller, int line);
#define net_nbuf_get_data(context) \
	net_nbuf_get_data_debug(context, __func__, __LINE__)

struct net_buf *net_nbuf_get_reserve_rx_debug(uint16_t reserve_head,
					      const char *caller, int line);
#define net_nbuf_get_reserve_rx(res) \
	net_nbuf_get_reserve_rx_debug(res, __func__, __LINE__)

struct net_buf *net_nbuf_get_reserve_tx_debug(uint16_t reserve_head,
					      const char *caller, int line);
#define net_nbuf_get_reserve_tx(res) \
	net_nbuf_get_reserve_tx_debug(res, __func__, __LINE__)

struct net_buf *net_nbuf_get_reserve_data_debug(uint16_t reserve_head,
						const char *caller, int line);
#define net_nbuf_get_reserve_data(res) \
	net_nbuf_get_reserve_data_debug(res, __func__, __LINE__)

void net_nbuf_unref_debug(struct net_buf *buf, const char *caller, int line);
#define net_nbuf_unref(buf) net_nbuf_unref_debug(buf, __func__, __LINE__)

struct net_buf *net_nbuf_ref_debug(struct net_buf *buf, const char *caller,
				   int line);
#define net_nbuf_ref(buf) net_nbuf_ref_debug(buf, __func__, __LINE__)

/**
 * @brief Print fragment list and the fragment sizes
 *
 * @details Only available if debugging is activated.
 *
 * @param buf Network buffer fragment. This should be the first fragment (data)
 * in the fragment list.
 */
void net_nbuf_print_frags(struct net_buf *buf);

#else /* CONFIG_NET_DEBUG_NET_BUF */

#define net_nbuf_print_frags(...)

/**
 * @brief Get buffer from the RX buffers pool.
 *
 * @details Get network buffer from RX buffer pool. You must have
 * network context before able to use this function.
 *
 * @param context Network context that will be related to
 * this buffer.
 *
 * @return Network buffer if successful, NULL otherwise.
 */
struct net_buf *net_nbuf_get_rx(struct net_context *context);

/**
 * @brief Get buffer from the TX buffers pool.
 *
 * @details Get network buffer from TX buffer pool. You must have
 * network context before able to use this function.
 *
 * @param context Network context that will be related to
 * this buffer.
 *
 * @return Network buffer if successful, NULL otherwise.
 */
struct net_buf *net_nbuf_get_tx(struct net_context *context);

/**
 * @brief Get buffer from the DATA buffers pool.
 *
 * @details Get network buffer from DATA buffer pool. You must have
 * network context before able to use this function.
 *
 * @param context Network context that will be related to
 * this buffer.
 *
 * @return Network buffer if successful, NULL otherwise.
 */
struct net_buf *net_nbuf_get_data(struct net_context *context);

/**
 * @brief Get RX buffer from pool but also reserve headroom for
 * potential headers.
 *
 * @details Normally this version is not useful for applications
 * but is mainly used by network fragmentation code.
 *
 * @param reserve_head How many bytes to reserve for headroom.
 *
 * @return Network buffer if successful, NULL otherwise.
 */
struct net_buf *net_nbuf_get_reserve_rx(uint16_t reserve_head);

/**
 * @brief Get TX buffer from pool but also reserve headroom for
 * potential headers.
 *
 * @details Normally this version is not useful for applications
 * but is mainly used by network fragmentation code.
 *
 * @param reserve_head How many bytes to reserve for headroom.
 *
 * @return Network buffer if successful, NULL otherwise.
 */
struct net_buf *net_nbuf_get_reserve_tx(uint16_t reserve_head);

/**
 * @brief Get DATA buffer from pool but also reserve headroom for
 * potential headers.
 *
 * @details Normally this version is not useful for applications
 * but is mainly used by network fragmentation code.
 *
 * @param reserve_head How many bytes to reserve for headroom.
 *
 * @return Network buffer if successful, NULL otherwise.
 */
struct net_buf *net_nbuf_get_reserve_data(uint16_t reserve_head);

/**
 * @brief Place buffer back into the available buffers pool.
 *
 * @details Releases the buffer to other use. This needs to be
 * called by application after it has finished with
 * the buffer.
 *
 * @param buf Network buffer to release.
 *
 */
void net_nbuf_unref(struct net_buf *buf);

/**
 * @brief Increase the ref count
 *
 * @details Mark the buffer to be used still.
 *
 * @param buf Network buffer to ref.
 *
 * @return Network buffer if successful, NULL otherwise.
 */
struct net_buf *net_nbuf_ref(struct net_buf *buf);

#endif /* CONFIG_NETWORK_IP_STACK_DEBUG_NET_BUF */

/**
 * @brief Copy a buffer with fragments while reserving some extra space
 * in destination buffer before a copy.
 *
 * @details Note that the original buffer is not really usable after the copy
 * as the function will call net_buf_pull() internally and should be discarded.
 *
 * @param buf Network buffer fragment. This should be the first fragment (data)
 * in the fragment list.
 * @param amount Max amount of data to be copied.
 * @param reserve Amount of extra data (this is not link layer header) in the
 * first data fragment that is returned. The function will copy the original
 * buffer right after the reserved bytes in the first destination fragment.
 *
 * @return New fragment list if successful, NULL otherwise.
 */
struct net_buf *net_nbuf_copy(struct net_buf *buf, size_t amount,
			      size_t reserve);

/**
 * @brief Copy a buffer with fragments while reserving some extra space
 * in destination buffer before a copy.
 *
 * @param buf Network buffer fragment. This should be the first fragment (data)
 * in the fragment list.
 * @param reserve Amount of extra data (this is not link layer header) in the
 * first data fragment that is returned. The function will copy the original
 * buffer right after the reserved bytes in the first destination fragment.
 *
 * @return New fragment list if successful, NULL otherwise.
 */
static inline struct net_buf *net_nbuf_copy_all(struct net_buf *buf,
						size_t reserve)
{
	return net_nbuf_copy(buf, net_buf_frags_len(buf), reserve);
}

/**
 * @brief Compact the fragment list.
 *
 * @details After this there is no more any free space in individual fragments.
 * @param buf Network buffer fragment. This should be the first fragment (data)
 * in the fragment list.
 *
 * @return Pointer to the start of the fragment list if ok, NULL otherwise.
 */
struct net_buf *net_nbuf_compact(struct net_buf *buf);

/**
 * @brief Check if the buffer chain is compact or not.
 *
 * @details The compact here means that is there any free space in the
 * fragments. Only the last fragment can have some free space if the fragment
 * list is compact.
 *
 * @param buf Network buffer.
 *
 * @return True if there is no free space in the fragment list,
 * false otherwise.
 */
bool net_nbuf_is_compact(struct net_buf *buf);

/**
 * @brief Create some more space in front of the fragment list.
 *
 * @details After this there is more space available before the first
 * fragment. The existing data needs to be moved "down" which will
 * cause a cascading effect on fragment list because fragments are fixed
 * size.
 *
 * @param parent Pointer to parent of the network buffer. If there is
 * no parent, then set this parameter NULL.
 * @param buf Network buffer
 * @param amount Amount of data that is needed in front of the fragment list.
 *
 * @return Pointer to the start of the fragment list if ok, NULL otherwise.
 */
struct net_buf *net_nbuf_push(struct net_buf *parent, struct net_buf *buf,
			      size_t amount);

#if defined(CONFIG_NET_DEBUG_NET_BUF)
/**
 * @brief Debug helper to print out the buffer allocations
 */
void net_nbuf_print(void);
#else
#define net_nbuf_print(...)
#endif /* CONFIG_NET_DEBUG_NET_BUF */

#ifdef __cplusplus
}
#endif

#endif /* __NBUF_H */
