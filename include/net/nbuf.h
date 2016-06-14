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

#if defined(CONFIG_NETWORK_IP_STACK_DEBUG_NET_BUF)
#undef NET_DEBUG_NBUFS
#define NET_DEBUG_NBUFS 1
#endif

#include <net/net_core.h>
#include <net/net_linkaddr.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(NET_DEBUG_NBUFS)
#define NET_BUF_CHECK_IF_IN_USE(buf, ref)				\
	do {								\
		if (ref) {						\
			NET_ERR("**ERROR** buf %p in use (%s:%s():%d)", \
				buf, __FILE__, __func__, __LINE__);	\
		}							\
	} while (0)

#define NET_BUF_CHECK_IF_NOT_IN_USE(buf, ref)				\
	do {								\
		if (!(ref)) {						\
			NET_ERR("**ERROR** buf %p not in use (%s:%s():%d)", \
				buf, __FILE__, __func__, __LINE__);	\
		}							\
	} while (0)
#else
#define NET_BUF_CHECK_IF_IN_USE(buf, ref)
#define NET_BUF_CHECK_IF_NOT_IN_USE(buf, ref)
#endif

struct net_context;

/** @cond ignore */
enum net_nbuf_type {
	NET_NBUF_RX = 0,
	NET_NBUF_TX = 1,
	NET_NBUF_DATA = 2,
};
/** @endcond */

/** Note that the buf len is usually smaller that the minimum IPv6 MTU 1280
 * bytes. The default MTU is 1280 (minimum IPv6 packet size) + LL header
 * In Contiki terms this is UIP_LINK_MTU + UIP_LLH_LEN = UIP_BUFSIZE
 *
 * Contiki assumes that this value is UIP_BUFSIZE so do not change it
 * without changing the value of UIP_BUFSIZE!
 */
#if defined(CONFIG_NET_BUF_MAX_DATA)
#define NBUF_MAX_DATA CONFIG_NET_BUF_MAX_DATA
#else
#define NBUF_MAX_DATA 128
#endif

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

	/** Network interface */
	struct net_if *iface;

	/** @cond ignore */
	uint8_t *appdata;  /* application data */
	uint16_t appdatalen;
	/* @endcond */
};

/** @cond ignore */

/* Value returned by nbuf_len() contains length of all the protocol headers,
 * like IP and UDP, and the length of the user payload.
 */
#define net_nbuf_len(buf) ((buf)->len)

/* This returns pointer to start of the protocol IP header */
#define net_nbuf_ip_data(buf) ((buf)->frags->data)
#define net_nbuf_udp_data(buf) (&(buf)->frags->data[net_nbuf_ip_hdr_len(buf)])
#define net_nbuf_tcp_data(buf) (&(buf)->frags->data[net_nbuf_ip_hdr_len(buf)])
#define net_nbuf_icmp_data(buf) (&(buf)->frags->data[net_nbuf_ip_hdr_len(buf) +\
						    net_nbuf_ext_len(buf)])

/* These two return only the application data length without
 * IP and other protocol header length.
 */
#define net_nbuf_appdata(buf) (((struct net_nbuf *) \
			    net_buf_user_data((buf)))->appdata)
#define net_nbuf_appdatalen(buf) (((struct net_nbuf *) \
			       net_buf_user_data((buf)))->appdatalen)
#define net_nbuf_reserve(buf) (((struct net_nbuf *) \
			    net_buf_user_data((buf)))->reserve)
#define net_nbuf_ll_reserve(buf) (((struct net_nbuf *) \
			       net_buf_user_data((buf)))->ll_reserve)
#define net_nbuf_ll(buf) (net_nbuf_ip_data(buf) - net_nbuf_ll_reserve(buf))

#define net_nbuf_ll_src(buf) \
	(&(((struct net_nbuf *)net_buf_user_data((buf)))->lladdr_src))
#define net_nbuf_ll_dst(buf) \
	(&(((struct net_nbuf *)net_buf_user_data((buf)))->lladdr_dst))

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

/* The interface real ll address */
#define net_nbuf_ll_if(buf)						\
	net_if_get_link_addr(((struct net_nbuf *)net_buf_user_data(buf))->iface)

#define net_nbuf_context(buf) \
	(((struct net_nbuf *)net_buf_user_data((buf)))->context)
#define net_nbuf_iface(buf) \
	(((struct net_nbuf *)net_buf_user_data((buf)))->iface)
#define net_nbuf_type(ptr) (((struct net_nbuf *)net_buf_user_data((ptr)))->type)
#define net_nbuf_family(ptr) (((struct net_nbuf *) \
			       net_buf_user_data((ptr)))->family)
#define net_nbuf_ip_hdr_len(buf) (((struct net_nbuf *) \
				   net_buf_user_data((buf)))->ip_hdr_len)
#define net_nbuf_ext_len(buf) \
	(((struct net_nbuf *)net_buf_user_data((buf)))->ext_len)
#define net_nbuf_ext_bitmap(buf) \
	(((struct net_nbuf *)net_buf_user_data((buf)))->ext_bitmap)
#define net_nbuf_next_hdr(buf) \
	(((struct net_nbuf *)net_buf_user_data((buf)))->next_hdr)
#define net_nbuf_ext_opt_len(buf) \
	(((struct net_nbuf *)net_buf_user_data((buf)))->ext_opt_len)

#define NET_IPV6_BUF(buf) ((struct net_ipv6_hdr *)net_nbuf_ip_data(buf))
#define NET_IPV4_BUF(buf) ((struct net_ipv4_hdr *)net_nbuf_ip_data(buf))
#define NET_ICMP_BUF(buf) ((struct net_icmp_hdr *)net_nbuf_icmp_data(buf))
#define NET_UDP_BUF(buf)  ((struct net_udp_hdr *)(net_nbuf_udp_data(buf)))

#define net_nbuf_set_src_ipv6_addr(buf)				 \
	net_if_select_src(net_context_get_if(nbuf_context(buf)), \
			  &NET_IPV6_BUF(buf)->src,		 \
			  &NET_IPV6_BUF(buf)->dst)
/* @endcond */

/**
 * @brief Get buffer from the available buffers pool.
 *
 * @details Get network buffer from buffer pool. You must have
 * network context before able to use this function.
 *
 * @param context Network context that will be related to
 * this buffer.
 *
 * @return Network buffer if successful, NULL otherwise.
 */
/* Get buffer from the available buffers pool */
#if defined(NET_DEBUG_NBUFS)
#define net_nbuf_get_rx(context) net_nbuf_get_rx_debug(context, __func__, __LINE__)
#define net_nbuf_get_tx(context) net_nbuf_get_tx_debug(context, __func__, __LINE__)
#define net_nbuf_get_data(context) net_nbuf_get_data_debug(context, __func__, __LINE__)
struct net_buf *net_nbuf_get_rx_debug(struct net_context *context,
				  const char *caller, int line);
struct net_buf *net_nbuf_get_tx_debug(struct net_context *context,
				  const char *caller, int line);
struct net_buf *net_nbuf_get_data_debug(struct net_context *context,
				  const char *caller, int line);
#else
struct net_buf *net_nbuf_get_rx(struct net_context *context);
struct net_buf *net_nbuf_get_tx(struct net_context *context);
struct net_buf *net_nbuf_get_data(struct net_context *context);
#endif

/**
 * @brief Get buffer from pool but also reserve headroom for
 * potential headers.
 *
 * @details Normally this version is not useful for applications
 * but is mainly used by network fragmentation code.
 *
 * @param reserve_head How many bytes to reserve for headroom.
 *
 * @return Network buffer if successful, NULL otherwise.
 */
/* Same as net_buf_get, but also reserve headroom for potential headers */
#if defined(NET_DEBUG_NBUFS)
#define net_nbuf_get_reserve_rx(res) net_nbuf_get_reserve_rx_debug(res,	\
							       __func__, \
							       __LINE__)
#define net_nbuf_get_reserve_tx(res) net_nbuf_get_reserve_tx_debug(res,	\
							       __func__, \
							       __LINE__)
#define net_nbuf_get_reserve_data(res) net_nbuf_get_reserve_data_debug(res,	\
							       __func__, \
							       __LINE__)
struct net_buf *net_nbuf_get_reserve_rx_debug(uint16_t reserve_head,
					    const char *caller, int line);
struct net_buf *net_nbuf_get_reserve_tx_debug(uint16_t reserve_head,
					    const char *caller, int line);
struct net_buf *net_nbuf_get_reserve_data_debug(uint16_t reserve_head,
					    const char *caller, int line);
#else
struct net_buf *net_nbuf_get_reserve_rx(uint16_t reserve_head);
struct net_buf *net_nbuf_get_reserve_tx(uint16_t reserve_head);
struct net_buf *net_nbuf_get_reserve_data(uint16_t reserve_head);
#endif

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
#ifdef NET_DEBUG_NBUFS
#define net_nbuf_unref(buf) net_nbuf_unref_debug(buf, __func__, __LINE__)
void net_nbuf_unref_debug(struct net_buf *buf, const char *caller, int line);
#else
void net_nbuf_unref(struct net_buf *buf);
#endif

/**
 * @brief Increase the ref count
 *
 * @details Mark the buffer to be used still.
 *
 * @param buf Network buffer to ref.
 *
 * @return Network buffer if successful, NULL otherwise.
 */
#ifdef NET_DEBUG_NBUFS
#define net_nbuf_ref(buf) net_nbuf_ref_debug(buf, __func__, __LINE__)
struct net_buf *net_nbuf_ref_debug(struct net_buf *buf, const char *caller, int line);
#else
struct net_buf *net_nbuf_ref(struct net_buf *buf);
#endif

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

#ifdef __cplusplus
}
#endif

#endif /* __NBUF_H */
