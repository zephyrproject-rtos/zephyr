/** @file
 * @brief L2 buffer API
 *
 * L2 (layer 2 or MAC layer) data is passed between application and
 * IP stack via a l2_buf struct. Currently L2 buffers are only used
 * in IEEE 802.15.4 code.
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

#ifndef __L2_BUF_H
#define __L2_BUF_H

#include <stdint.h>

#include <net/ip_buf.h>

#include "contiki/ip/uipopt.h"
#include "contiki/ip/uip.h"
#include "contiki/packetbuf.h"
#include "contiki/os/lib/list.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_L2_BUFFERS)

#ifdef CONFIG_NETWORKING_WITH_LOGGING
#undef DEBUG_L2_BUFS
#define DEBUG_L2_BUFS
#endif

/** @cond ignore */
void l2_buf_init(void);
/* @endcond */

/** For the MAC/L2 layer (after the IPv6 packet is fragmented to smaller
 * chunks), we can use much smaller buffers (depending on used radio
 * technology). For 802.15.4 we use the 128 bytes long buffers.
 */
#ifndef NET_L2_BUF_MAX_SIZE
#define NET_L2_BUF_MAX_SIZE (PACKETBUF_SIZE + PACKETBUF_HDR_SIZE)
#endif

struct l2_buf {
	/** @cond ignore */
	/* 6LoWPAN pointers */
	uint8_t *packetbuf_ptr;
	uint8_t packetbuf_hdr_len;
	uint8_t packetbuf_payload_len;
	uint8_t uncomp_hdr_len;
	int last_tx_status;
#if defined(CONFIG_NETWORKING_WITH_15_4)
	LIST_STRUCT(neighbor_list);
#endif

	struct packetbuf_attr pkt_packetbuf_attrs[PACKETBUF_NUM_ATTRS];
	struct packetbuf_addr pkt_packetbuf_addrs[PACKETBUF_NUM_ADDRS];
	uint16_t pkt_buflen, pkt_bufptr;
	uint8_t pkt_hdrptr;
	uint8_t *pkt_packetbufptr;
	/* @endcond */
};

/** @cond ignore */
#define uip_packetbuf_ptr(buf) \
	(((struct l2_buf *)net_buf_user_data((buf)))->packetbuf_ptr)
#define uip_packetbuf_hdr_len(buf) \
	(((struct l2_buf *)net_buf_user_data((buf)))->packetbuf_hdr_len)
#define uip_packetbuf_payload_len(buf) \
	(((struct l2_buf *)net_buf_user_data((buf)))->packetbuf_payload_len)
#define uip_uncomp_hdr_len(buf) \
	(((struct l2_buf *)net_buf_user_data((buf)))->uncomp_hdr_len)
#define uip_last_tx_status(buf) \
	(((struct l2_buf *)net_buf_user_data((buf)))->last_tx_status)
#if defined(CONFIG_NETWORKING_WITH_15_4)
#define uip_neighbor_list(buf) \
	(((struct l2_buf *)net_buf_user_data((buf)))->neighbor_list)
#endif
#define uip_pkt_buflen(buf) \
	(((struct l2_buf *)net_buf_user_data((buf)))->pkt_buflen)
#define uip_pkt_bufptr(buf) \
	(((struct l2_buf *)net_buf_user_data((buf)))->pkt_bufptr)
#define uip_pkt_hdrptr(buf) \
	(((struct l2_buf *)net_buf_user_data((buf)))->pkt_hdrptr)
#define uip_pkt_packetbufptr(buf) \
	(((struct l2_buf *)net_buf_user_data((buf)))->pkt_packetbufptr)
#define uip_pkt_packetbuf_attrs(buf) \
	(((struct l2_buf *)net_buf_user_data((buf)))->pkt_packetbuf_attrs)
#define uip_pkt_packetbuf_addrs(buf) \
	(((struct l2_buf *)net_buf_user_data((buf)))->pkt_packetbuf_addrs)

/* Note that we do not reserve extra space for the header when the packetbuf
 * is converted to use net_buf, so the packet starts directly from the
 * data pointer. This is done in order to simplify the 802.15.4 packet
 * handling. So the L2 buffer should only be allocated by calling
 * reserve function like this: l2_buf_get_reserve(0);
 */
#define uip_pkt_packetbuf(ptr) ((ptr)->data)
/* @endcond */

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
#ifdef DEBUG_L2_BUFS
#define l2_buf_get_reserve(res) l2_buf_get_reserve_debug(res, \
							 __func__, __LINE__)
struct net_buf *l2_buf_get_reserve_debug(uint16_t reserve_head,
					 const char *caller, int line);
#else
struct net_buf *l2_buf_get_reserve(uint16_t reserve_head);
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
#ifdef DEBUG_L2_BUFS
#define l2_buf_unref(buf) l2_buf_unref_debug(buf, __func__, __LINE__)
void l2_buf_unref_debug(struct net_buf *buf, const char *caller, int line);
#else
void l2_buf_unref(struct net_buf *buf);
#endif

#else /* defined(CONFIG_L2_BUFFERS) */

#define l2_buf_init(...)

#endif /* defined(CONFIG_L2_BUFFERS) */

#ifdef __cplusplus
}
#endif

#endif /* __L2_BUF_H */
