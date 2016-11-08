/** @file
 * @brief IP buffer API
 *
 * IP data is passed between application and IP stack via a ip_buf struct.
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

/* Data buffer API - used for all data to/from net */

#ifndef __IP_BUF_H
#define __IP_BUF_H

#include <stdint.h>
#include <stdbool.h>

#include <net/net_core.h>

#include "contiki/ip/uipopt.h"
#include "contiki/ip/uip.h"
#include "contiki/packetbuf.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_NETWORKING_WITH_LOGGING
#undef DEBUG_IP_BUFS
#define DEBUG_IP_BUFS
#endif

#ifdef DEBUG_IP_BUFS
#define NET_BUF_CHECK_IF_IN_USE(buf)					\
	do {								\
		if (buf->ref) {						\
			NET_ERR("**ERROR** buf %p in use (%s:%s():%d)\n", \
				buf, __FILE__, __func__, __LINE__);	\
		}							\
	} while (0)

#define NET_BUF_CHECK_IF_NOT_IN_USE(buf)				\
	do {								\
		if (!buf->ref) {					\
			NET_ERR("**ERROR** buf %p not in use (%s:%s():%d)\n",\
				buf, __FILE__, __func__, __LINE__);	\
		}							\
	} while (0)
#else
#define NET_BUF_CHECK_IF_IN_USE(buf)
#define NET_BUF_CHECK_IF_NOT_IN_USE(buf)
#endif

struct net_context;

/** @cond ignore */
enum ip_buf_type {
	IP_BUF_RX = 0,
	IP_BUF_TX = 1,
};
/** @endcond */

/** The default MTU is 1280 (minimum IPv6 packet size) + LL header
 * In Contiki terms this is UIP_LINK_MTU + UIP_LLH_LEN = UIP_BUFSIZE
 *
 * Contiki assumes that this value is UIP_BUFSIZE so do not change it
 * without changing the value of UIP_BUFSIZE!
 */
#define IP_BUF_MAX_DATA UIP_BUFSIZE

struct ip_buf {
	/** @cond ignore */
	enum ip_buf_type type;
	uint16_t reserve; /* length of the protocol headers */
	/* @endcond */

	/** Network connection context */
	struct net_context *context;

	/** @cond ignore */
	/* uIP stack specific data */
	uint16_t len; /* Contiki will set this to 0 if packet is discarded */
	uint8_t uip_ext_len;
	uint8_t uip_ext_bitmap;
	uint8_t uip_ext_opt_offset;
	uint8_t uip_flags;
	uint16_t uip_slen;
	uint16_t uip_appdatalen;
	uint8_t *uip_next_hdr;
	void *uip_appdata;  /* application data */
	void *uip_sappdata; /* app data to be sent */
	void *uip_conn;
	void *uip_udp_conn;
	linkaddr_t dest;
	linkaddr_t src;
#if defined(CONFIG_NETWORKING_WITH_TCP)
	int8_t sent_status; /* tells if the TCP packet was sent ok or not */
	uint8_t tcp_retry_count;
#endif

	/* Neighbor discovery vars. Note that we are using void pointers here
	 * so that we do not need to include Contiki headers in this file.
	 */
	void *nd6_opt_prefix_info;
	void *nd6_prefix;
	void *nd6_nbr;
	void *nd6_defrt;
	void *nd6_ifaddr;
	uint8_t *nd6_opt_llao;
	uip_ipaddr_t ipaddr;
	uint8_t nd6_opt_offset;

	/* 6lowpan specific vars */
	uint8_t compressed_hdr_len;
	uint8_t uncompressed_hdr_len;
	uint8_t first_frag_len;
	uint8_t uncompressed;
	/* @endcond */
};

/** @cond ignore */
/* Value returned by ip_buf_len() contains length of all the protocol headers,
 * like IP and UDP, and the length of the user payload.
 */
#define ip_buf_len(buf) ((buf)->len)

/* Macros to access net_buf when inside Contiki stack */
#define uip_buf(buf) ((buf)->data)
#define uip_len(buf) (((struct ip_buf *)net_buf_user_data((buf)))->len)
#define uip_slen(buf) (((struct ip_buf *)net_buf_user_data((buf)))->uip_slen)
#define uip_ext_len(buf) \
	(((struct ip_buf *)net_buf_user_data((buf)))->uip_ext_len)
#define uip_ext_bitmap(buf) \
	(((struct ip_buf *)net_buf_user_data((buf)))->uip_ext_bitmap)
#define uip_ext_opt_offset(buf) \
	(((struct ip_buf *)net_buf_user_data((buf)))->uip_ext_opt_offset)
#define uip_nd6_opt_offset(buf) \
	(((struct ip_buf *)net_buf_user_data((buf)))->nd6_opt_offset)
#define uip_next_hdr(buf) \
	(((struct ip_buf *)net_buf_user_data((buf)))->uip_next_hdr)
#define uip_appdata(buf) \
	(((struct ip_buf *)net_buf_user_data((buf)))->uip_appdata)
#define uip_appdatalen(buf) \
	(((struct ip_buf *)net_buf_user_data((buf)))->uip_appdatalen)
#define uip_sappdata(buf) \
	(((struct ip_buf *)net_buf_user_data((buf)))->uip_sappdata)
#define uip_flags(buf) \
	(((struct ip_buf *)net_buf_user_data((buf)))->uip_flags)
#define uip_conn(buf) \
	((struct uip_conn *) \
	 (((struct ip_buf *)net_buf_user_data((buf)))->uip_conn))
#define uip_set_conn(buf) \
	(((struct ip_buf *)net_buf_user_data((buf)))->uip_conn)
#define uip_udp_conn(buf) \
	((struct uip_udp_conn *) \
	 (((struct ip_buf *)net_buf_user_data((buf)))->uip_udp_conn))
#define uip_set_udp_conn(buf) \
	 (((struct ip_buf *)net_buf_user_data((buf)))->uip_udp_conn)
#define uip_nd6_opt_prefix_info(buf) \
	((uip_nd6_opt_prefix_info *) \
	 (((struct ip_buf *)net_buf_user_data((buf)))->nd6_opt_prefix_info))
#define uip_set_nd6_opt_prefix_info(buf) \
	(((struct ip_buf *)net_buf_user_data((buf)))->nd6_opt_prefix_info)
#define uip_prefix(buf) \
	((uip_ds6_prefix_t *) \
	 (((struct ip_buf *)net_buf_user_data((buf)))->nd6_prefix))
#define uip_set_prefix(buf) \
	(((struct ip_buf *)net_buf_user_data((buf)))->nd6_prefix)
#define uip_nbr(buf) \
	((uip_ds6_nbr_t *) \
	 (((struct ip_buf *)net_buf_user_data((buf)))->nd6_nbr))
#define uip_set_nbr(buf) \
	(((struct ip_buf *)net_buf_user_data((buf)))->nd6_nbr)
#define uip_defrt(buf) \
	((uip_ds6_defrt_t *) \
	 (((struct ip_buf *)net_buf_user_data((buf)))->nd6_defrt))
#define uip_set_defrt(buf) \
	(((struct ip_buf *)net_buf_user_data((buf)))->nd6_defrt)
#define uip_addr(buf) \
	((uip_ds6_addr_t *) \
	 (((struct ip_buf *)net_buf_user_data((buf)))->nd6_ifaddr))
#define uip_set_addr(buf) \
	(((struct ip_buf *)net_buf_user_data((buf)))->nd6_ifaddr)
#define uip_nd6_opt_llao(buf) \
	(((struct ip_buf *)net_buf_user_data((buf)))->nd6_opt_llao)
#define uip_set_nd6_opt_llao(buf) \
	(((struct ip_buf *)net_buf_user_data((buf)))->nd6_opt_llao)
#define uip_nd6_ipaddr(buf) \
	(((struct ip_buf *)net_buf_user_data((buf)))->ipaddr)
#define uip_compressed_hdr_len(buf) \
	(((struct ip_buf *)net_buf_user_data((buf)))->compressed_hdr_len)
#define uip_uncompressed_hdr_len(buf) \
	(((struct ip_buf *)net_buf_user_data((buf)))->uncompressed_hdr_len)
#define uip_first_frag_len(buf) \
	(((struct ip_buf *)net_buf_user_data((buf)))->first_frag_len)
#define uip_uncompressed(buf) \
	(((struct ip_buf *)net_buf_user_data((buf)))->uncompressed)

/* These two return only the application data and length without
 * IP and UDP header length.
 */
#define ip_buf_appdata(buf) uip_appdata(buf)
#define ip_buf_appdatalen(buf) uip_appdatalen(buf)
#define ip_buf_reserve(buf) (((struct ip_buf *) \
			      net_buf_user_data((buf)))->reserve)

#define ip_buf_ll_src(buf) (((struct ip_buf *)net_buf_user_data((buf)))->src)
#define ip_buf_ll_dest(buf) (((struct ip_buf *)net_buf_user_data((buf)))->dest)
#define ip_buf_context(buf) (((struct ip_buf *)net_buf_user_data((buf)))->context)
#define ip_buf_type(ptr) (((struct ip_buf *)net_buf_user_data((ptr)))->type)
#define ip_buf_sent_status(ptr) (((struct ip_buf *)net_buf_user_data((ptr)))->sent_status)
#define ip_buf_tcp_retry_count(ptr) (((struct ip_buf *)net_buf_user_data((ptr)))->tcp_retry_count)
/* @endcond */

/** NET_BUF_IP
 *
 * @brief This macro returns IP header information struct stored in net_buf.
 *
 * @details The macro returns pointer to uip_ip_hdr struct which
 * contains IP header information.
 *
 * @param buf Network buffer.
 *
 * @return Pointer to uip_ip_hdr.
 */
#define NET_BUF_IP(buf)   ((struct uip_ip_hdr *)&uip_buf(buf)[UIP_LLH_LEN])

/** NET_BUF_UDP
 *
 * @brief This macro returns UDP header information struct stored in net_buf.
 *
 * @details The macro returns pointer to uip_udp_hdr struct which
 * contains UDP header information.
 *
 * @param buf Network buffer.
 *
 * @return Pointer to uip_ip_hdr.
 */
#define NET_BUF_UDP(buf)  ((struct uip_udp_hdr *)&uip_buf(buf)[UIP_LLIPH_LEN])

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
#ifdef DEBUG_IP_BUFS
#define ip_buf_get_rx(context) ip_buf_get_rx_debug(context, __func__, __LINE__)
#define ip_buf_get_tx(context) ip_buf_get_tx_debug(context, __func__, __LINE__)
struct net_buf *ip_buf_get_rx_debug(struct net_context *context,
				    const char *caller, int line);
struct net_buf *ip_buf_get_tx_debug(struct net_context *context,
				    const char *caller, int line);
#else
struct net_buf *ip_buf_get_rx(struct net_context *context);
struct net_buf *ip_buf_get_tx(struct net_context *context);
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
#ifdef DEBUG_IP_BUFS
#define ip_buf_get_reserve_rx(res) ip_buf_get_reserve_rx_debug(res,	\
							       __func__, \
							       __LINE__)
#define ip_buf_get_reserve_tx(res) ip_buf_get_reserve_tx_debug(res,	\
							       __func__, \
							       __LINE__)
struct net_buf *ip_buf_get_reserve_rx_debug(uint16_t reserve_head,
					    const char *caller, int line);
struct net_buf *ip_buf_get_reserve_tx_debug(uint16_t reserve_head,
					    const char *caller, int line);
#else
struct net_buf *ip_buf_get_reserve_rx(uint16_t reserve_head);
struct net_buf *ip_buf_get_reserve_tx(uint16_t reserve_head);
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
#ifdef DEBUG_IP_BUFS
#define ip_buf_unref(buf) ip_buf_unref_debug(buf, __func__, __LINE__)
void ip_buf_unref_debug(struct net_buf *buf, const char *caller, int line);
#else
void ip_buf_unref(struct net_buf *buf);
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
#ifdef DEBUG_IP_BUFS
#define ip_buf_ref(buf) ip_buf_ref_debug(buf, __func__, __LINE__)
struct net_buf *ip_buf_ref_debug(struct net_buf *buf, const char *caller, int line);
#else
struct net_buf *ip_buf_ref(struct net_buf *buf);
#endif

/** @cond ignore */
void ip_buf_init(void);
/* @endcond */

/** @cond ignore */
#if defined(CONFIG_INIT_STACKS) && defined(CONFIG_PRINTK)
#include <offsets.h>
#include <misc/printk.h>

static inline void net_analyze_stack(const char *name,
				     unsigned char *stack,
				     size_t size)
{
	unsigned i, stack_offset, pcnt, unused = 0;

	/* The TCS is always placed on a 4-byte aligned boundary - if
	 * the stack beginning doesn't match that there will be some
	 * unused bytes in the beginning.
	 */
	stack_offset = K_THREAD_SIZEOF + ((4 - ((unsigned)stack % 4)) % 4);

/* TODO
 * Currently all supported platforms have stack growth down and there is no
 * Kconfig option to configure it so this always build "else" branch.
 * When support for platform with stack direction up (or configurable direction)
 * is added this check should be confirmed that correct Kconfig option is used.
 */
#if defined(CONFIG_STACK_GROWS_UP)
	for (i = size - 1; i >= stack_offset; i--) {
		if ((unsigned char)stack[i] == 0xaa) {
			unused++;
		} else {
			break;
		}
	}
#else
	for (i = stack_offset; i < size; i++) {
		if ((unsigned char)stack[i] == 0xaa) {
			unused++;
		} else {
			break;
		}
	}
#endif

	/* Calculate the real size reserved for the stack */
	size -= stack_offset;
	pcnt = ((size - unused) * 100) / size;

	printk("net (%p): %s stack real size %zu "
	       "unused %u usage %zu/%zu (%u %%)\n",
	       sys_thread_self_get(), name, size + stack_offset,
	       unused, size - unused, size, pcnt);
}
#else
#define net_analyze_stack(...)
#endif
/* @endcond */

#ifdef __cplusplus
}
#endif

#endif /* __IP_BUF_H */
