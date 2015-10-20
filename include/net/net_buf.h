/** @file
 * @brief Network buffer API
 *
 * Network data is passed between application and IP stack via a net_buf struct.
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

#ifndef __NET_BUF_H
#define __NET_BUF_H

#include <stdint.h>
#include <stdbool.h>

#include <net/net_core.h>

#include "contiki/ip/uipopt.h"
#include "contiki/ip/uip.h"
#include "contiki/packetbuf.h"

#if defined(CONFIG_NETWORKING_WITH_LOGGING)
#undef DEBUG_NET_BUFS
#define DEBUG_NET_BUFS
#endif

#ifdef DEBUG_NET_BUFS
#define NET_BUF_CHECK_IF_IN_USE(buf)					\
	do {								\
		if (buf->in_use) {					\
			NET_ERR("**ERROR** buf %p in use (%s:%s():%d)\n", \
				buf, __FILE__, __func__, __LINE__);	\
		}							\
	} while (0)

#define NET_BUF_CHECK_IF_NOT_IN_USE(buf)				\
	do {								\
		if (!buf->in_use) {					\
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
enum net_buf_type {
	NET_BUF_RX = 0,
	NET_BUF_TX = 1,
};
/** @endcond */

/** The default MTU is 1280 (minimum IPv6 packet size) + LL header
 * In Contiki terms this is UIP_LINK_MTU + UIP_LLH_LEN = UIP_BUFSIZE
 *
 * Contiki assumes that this value is UIP_BUFSIZE so do not change it
 * without changing the value of UIP_BUFSIZE!
 */
#define NET_BUF_MAX_DATA UIP_BUFSIZE

struct net_buf {
	/** @cond ignore */
	/* FIFO uses first 4 bytes itself, reserve space */
	int __unused;
	bool in_use;
	enum net_buf_type type;
	/* @endcond */

	/** Network connection context */
	struct net_context *context;

	/** @cond ignore */
	/* uIP stack specific data */
	uint16_t len;
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

	/* Neighbor discovery vars */
	void *nd6_opt_prefix_info;
	void *nd6_prefix;
	void *nd6_nbr;
	void *nd6_defrt;
	void *nd6_ifaddr;
	uint8_t *nd6_opt_llao;
	uip_ipaddr_t ipaddr;
	uint8_t nd6_opt_offset;
	/* @endcond */

	/** Buffer data length */
	uint16_t datalen;
	/** Buffer head pointer */
	uint8_t *data;
	/** Actual network buffer storage */
	uint8_t buf[NET_BUF_MAX_DATA];
};

#define net_buf_data(buf) ((buf)->data)
#define net_buf_datalen(buf) ((buf)->datalen)

/** @cond ignore */
/* Macros to access net_buf when inside Contiki stack */
#define uip_buf(ptr) ((ptr)->buf)
#define uip_len(buf) ((buf)->len)
#define uip_slen(buf) ((buf)->uip_slen)
#define uip_ext_len(buf) ((buf)->uip_ext_len)
#define uip_ext_bitmap(buf) ((buf)->uip_ext_bitmap)
#define uip_ext_opt_offset(buf) ((buf)->uip_ext_opt_offset)
#define uip_nd6_opt_offset(buf) ((buf)->nd6_opt_offset)
#define uip_next_hdr(buf) ((buf)->uip_next_hdr)
#define uip_appdata(buf) ((buf)->uip_appdata)
#define uip_appdatalen(buf) ((buf)->uip_appdatalen)
#define uip_sappdata(buf) ((buf)->uip_sappdata)
#define uip_flags(buf) ((buf)->uip_flags)
#define uip_conn(buf) ((struct uip_conn *)((buf)->uip_conn))
#define uip_set_conn(buf) ((buf)->uip_conn)
#define uip_udp_conn(buf) ((struct uip_udp_conn *)((buf)->uip_udp_conn))
#define uip_set_udp_conn(buf) ((buf)->uip_udp_conn)
#define uip_nd6_opt_prefix_info(buf) ((uip_nd6_opt_prefix_info *)((buf)->nd6_opt_prefix_info))
#define uip_set_nd6_opt_prefix_info(buf) ((buf)->nd6_opt_prefix_info)
#define uip_prefix(buf) ((uip_ds6_prefix_t *)((buf)->nd6_prefix))
#define uip_set_prefix(buf) ((buf)->nd6_prefix)
#define uip_nbr(buf) ((uip_ds6_nbr_t *)((buf)->nd6_nbr))
#define uip_set_nbr(buf) ((buf)->nd6_nbr)
#define uip_defrt(buf) ((uip_ds6_defrt_t *)((buf)->nd6_defrt))
#define uip_set_defrt(buf) ((buf)->nd6_defrt)
#define uip_addr(buf) ((uip_ds6_addr_t *)((buf)->nd6_ifaddr))
#define uip_set_addr(buf) ((buf)->nd6_ifaddr)
#define uip_nd6_opt_llao(buf) ((buf)->nd6_opt_llao)
#define uip_set_nd6_opt_llao(buf) ((buf)->nd6_opt_llao)
#define uip_nd6_ipaddr(buf) ((buf)->ipaddr)
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
#ifdef DEBUG_NET_BUFS
#define net_buf_get_rx(context) net_buf_get_rx_debug(context, __func__, __LINE__)
#define net_buf_get_tx(context) net_buf_get_tx_debug(context, __func__, __LINE__)
struct net_buf *net_buf_get_rx_debug(struct net_context *context, const char *caller, int line);
struct net_buf *net_buf_get_tx_debug(struct net_context *context, const char *caller, int line);
#else
struct net_buf *net_buf_get_rx(struct net_context *context);
struct net_buf *net_buf_get_tx(struct net_context *context);
#endif

/**
 * @brief Get buffer from pool but also reserve headroom for
 * potential headers.
 *
 * @details Normally this version is not useful for applications
 * but is mainly used by network fragmentation code.
 *
 * @param reserve How many bytes to reserve for headroom.
 *
 * @return Network buffer if successful, NULL otherwise.
 */
/* Same as net_buf_get, but also reserve headroom for potential headers */
#ifdef DEBUG_NET_BUFS
#define net_buf_get_reserve_rx(res) net_buf_get_reserve_rx_debug(res, __func__, __LINE__)
#define net_buf_get_reserve_tx(res) net_buf_get_reserve_tx_debug(res, __func__, __LINE__)
struct net_buf *net_buf_get_reserve_rx_debug(uint16_t reserve_head, const char *caller, int line);
struct net_buf *net_buf_get_reserve_tx_debug(uint16_t reserve_head, const char *caller, int line);
#else
struct net_buf *net_buf_get_reserve_rx(uint16_t reserve_head);
struct net_buf *net_buf_get_reserve_tx(uint16_t reserve_head);
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
/* Place buffer back into the available buffers pool */
#ifdef DEBUG_NET_BUFS
#define net_buf_put(buf) net_buf_put_debug(buf, __func__, __LINE__)
void net_buf_put_debug(struct net_buf *buf, const char *caller, int line);
#else
void net_buf_put(struct net_buf *buf);
#endif

/**
 * @brief Prepare data to be added at the end of the buffer.
 *
 * @details Move the tail pointer forward.
 *
 * @param buf Network buffer.
 * @param len Size of data to be added.
 *
 * @return Pointer to new tail.
 */
uint8_t *net_buf_add(struct net_buf *buf, uint16_t len);

/**
 * @brief Push data to the beginning of the buffer.
 *
 * @details Move the data pointer backwards.
 *
 * @param buf Network buffer.
 * @param len Size of data to be added.
 *
 * @return Pointer to new head.
 */
uint8_t *net_buf_push(struct net_buf *buf, uint16_t len);

/**
 * @brief Remove data from the beginning of the buffer.
 *
 * @details Move the data pointer forward.
 *
 * @param buf Network buffer.
 * @param len Size of data to be removed.
 *
 * @return Pointer to new head.
 */
uint8_t *net_buf_pull(struct net_buf *buf, uint16_t len);

/** @def net_buf_tail
 *
 * @brief Return pointer to the end of the data in the buffer.
 *
 * @details This macro returns the tail of the buffer.
 *
 * @param buf Network buffer.
 *
 * @return Pointer to tail.
 */
#define net_buf_tail(buf) ((buf)->data + (buf)->len)

/** @cond ignore */
void net_buf_init(void);
/* @endcond */

/** For the MAC layer (after the IPv6 packet is fragmented to smaller
 * chunks), we can use much smaller buffers (depending on used radio
 * technology). For 802.15.4 we use the 128 bytes long buffers.
 */
#ifndef NET_MAC_BUF_MAX_SIZE
#define NET_MAC_BUF_MAX_SIZE PACKETBUF_SIZE
#endif

struct net_mbuf {
	/** @cond ignore */
	/* FIFO uses first 4 bytes itself, reserve space */
	int __unused;
	bool in_use;
	/* @endcond */

	/** @cond ignore */
	/* 6LoWPAN pointers */
	uint8_t *packetbuf_ptr;
	uint8_t packetbuf_hdr_len;
	int packetbuf_payload_len;
	uint8_t uncomp_hdr_len;
	int last_tx_status;

	struct packetbuf_attr pkt_packetbuf_attrs[PACKETBUF_NUM_ATTRS];
	struct packetbuf_addr pkt_packetbuf_addrs[PACKETBUF_NUM_ADDRS];
	uint16_t pkt_buflen, pkt_bufptr;
	uint8_t pkt_hdrptr;
	uint8_t pkt_packetbuf[PACKETBUF_SIZE + PACKETBUF_HDR_SIZE];
	uint8_t *pkt_packetbufptr;
	/* @endcond */
};

/**
 * @brief Get buffer from the available buffers pool
 * and also reserve headroom for potential headers.
 *
 * @details Normally this version is not useful for applications
 * but is mainly used by network fragmentation code.
 *
 * @param reserve How many bytes to reserve for headroom.
 *
 * @return Network buffer if successful, NULL otherwise.
 */
#ifdef DEBUG_NET_BUFS
#define net_mbuf_get_reserve(res) net_mbuf_get_reserve_debug(res, __func__, __LINE__)
struct net_mbuf *net_mbuf_get_reserve_debug(uint16_t reserve_head, const char *caller, int line);
#else
struct net_mbuf *net_mbuf_get_reserve(uint16_t reserve_head);
#endif

/**
 * @brief Place buffer back into the available buffers pool.
 *
 * @details Releases the buffer to other use. This needs to be
 * called by application after it has finished with
 * the buffer.
 *
 * @param buf Network buffer to release.
 */
#ifdef DEBUG_NET_BUFS
#define net_mbuf_put(buf) net_mbuf_put_debug(buf, __func__, __LINE__)
void net_mbuf_put_debug(struct net_mbuf *buf, const char *caller, int line);
#else
void net_mbuf_put(struct net_mbuf *buf);
#endif

/** @cond ignore */
#define uip_packetbuf_ptr(buf) ((buf)->packetbuf_ptr)
#define uip_packetbuf_hdr_len(buf) ((buf)->packetbuf_hdr_len)
#define uip_packetbuf_payload_len(buf) ((buf)->packetbuf_payload_len)
#define uip_uncomp_hdr_len(buf) ((buf)->uncomp_hdr_len)
#define uip_last_tx_status(buf) ((buf)->last_tx_status)
#define uip_pkt_buflen(buf) ((buf)->pkt_buflen)
#define uip_pkt_bufptr(buf) ((buf)->pkt_bufptr)
#define uip_pkt_hdrptr(buf) ((buf)->pkt_hdrptr)
#define uip_pkt_packetbuf(buf) ((buf)->pkt_packetbuf)
#define uip_pkt_packetbufptr(buf) ((buf)->pkt_packetbufptr)
#define uip_pkt_packetbuf_attrs(buf) ((buf)->pkt_packetbuf_attrs)
#define uip_pkt_packetbuf_addrs(buf) ((buf)->pkt_packetbuf_addrs)
/* @endcond */

/** @cond ignore */
#if defined(CONFIG_INIT_STACKS) && defined(CONFIG_PRINTK)
#include <offsets.h>
#include <misc/printk.h>

enum {
	STACK_DIRECTION_UP,
	STACK_DIRECTION_DOWN,
};

static inline unsigned net_calculate_unused(const char *stack, unsigned size,
					    int stack_growth)
{
	unsigned i, unused = 0;

	if (stack_growth == STACK_DIRECTION_DOWN) {
		for (i = __tTCS_SIZEOF; i < size; i++) {
			if ((unsigned char)stack[i] == 0xaa) {
				unused++;
			} else {
				break;
			}
		}
	} else {
		for (i = size - 1; i >= __tTCS_SIZEOF; i--) {
			if ((unsigned char)stack[i] == 0xaa) {
				unused++;
			} else {
				break;
			}
		}
	}

	return unused;
}

static inline unsigned net_get_stack_dir(struct net_buf *buf,
					 struct net_buf **ref)
{
	if (buf > *ref) {
		return 1;
	} else {
		return 0;
	}
}

static inline void net_analyze_stack(const char *name,
				     unsigned char *stack,
				     size_t size)
{
	unsigned unused;
	int stack_growth;
	char *dir;
	struct net_buf *buf = NULL;

	if (net_get_stack_dir(buf, &buf)) {
		dir = "up";
		stack_growth = STACK_DIRECTION_UP;
	} else {
		dir = "down";
		stack_growth = STACK_DIRECTION_DOWN;
	}

	unused = net_calculate_unused(stack, size, stack_growth);

	printk("net: ip: %s stack grows %s, "
	       "stack(%p/%u): unused %u bytes\n",
	       name, dir, stack, size, unused);
}
#else
#define net_analyze_stack(...)
#endif
/* @endcond */

#endif /* __NET_BUF_H */
