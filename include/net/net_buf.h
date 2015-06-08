/*! @file
 @brief Network buffer API

 Network data is passed between application and IP stack via
 a net_buf struct.
 */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Intel Corporation nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/* Data buffer API - used for all data to/from net */

#ifndef __NET_BUF_H
#define __NET_BUF_H

#include <stdint.h>

#include "contiki/ip/uipopt.h"
#include "contiki/ip/uip.h"
#include "contiki/packetbuf.h"

struct net_context;

/*! The default MTU is 1280 (minimum IPv6 packet size) + LL header
 * In Contiki terms this is UIP_LINK_MTU + UIP_LLH_LEN = UIP_BUFSIZE
 *
 * Contiki assumes that this value is UIP_BUFSIZE so do not change it
 * without changing the value of UIP_BUFSIZE!
 */
#define NET_BUF_MAX_DATA UIP_BUFSIZE

struct net_buf {
	/*! @cond ignore */
	/* FIFO uses first 4 bytes itself, reserve space */
	int __unused;
	/* @endcond */

	/*! Network connection context */
	struct net_context *context;

	/*! @cond ignore */
	/* uIP stack specific data */
	uint8_t uip_ext_len;
	uint8_t uip_ext_bitmap;
	uint8_t uip_ext_opt_offset;
	uint8_t uip_flags;
	uint16_t uip_slen;
	uint8_t *uip_next_hdr;
	void *uip_appdata;  /* application data */
	void *uip_sappdata; /* app data to be sent */
	void *uip_conn;
	void *uip_udp_conn;

	/* Neighbor discovery vars */
	void *nd6_opt_prefix_info;
	void *nd6_prefix;
	void *nd6_nbr;
	void *nd6_defrt;
	void *nd6_ifaddr;
	uint8_t *nd6_opt_llao;
	uip_ipaddr_t ipaddr;
	uint8_t nd6_opt_offset;

	/* 6LoWPAN pointers */
	uint8_t *packetbuf_ptr;
	uint8_t packetbuf_hdr_len;
	int packetbuf_payload_len;
	uint8_t uncomp_hdr_len;
	int last_tx_status;

	/* 802.15.4 specific stuff */
	/* FIXME - put behind #ifdef
	 *       - have a separate buffers for these packets
	 */
	struct packetbuf_attr pkt_packetbuf_attrs[PACKETBUF_NUM_ATTRS];
	struct packetbuf_addr pkt_packetbuf_addrs[PACKETBUF_NUM_ADDRS];
	uint16_t pkt_buflen, pkt_bufptr;
	uint8_t pkt_hdrptr;
	uint8_t pkt_packetbuf[PACKETBUF_SIZE + PACKETBUF_HDR_SIZE];
	uint8_t *pkt_packetbufptr;
	/* @endcond */

	/*! Buffer data length */
	uint16_t len;
	/*! Buffer head pointer */
	uint8_t *data;
	/*! Actual network buffer storage */
	uint8_t buf[NET_BUF_MAX_DATA];
};

/*! @cond ignore */
/* Macros to access net_buf when inside Contiki stack */
#define uip_buf(buf) ((buf)->buf)
#define uip_len(buf) ((buf)->len)
#define uip_slen(buf) ((buf)->uip_slen)
#define uip_ext_len(buf) ((buf)->uip_ext_len)
#define uip_ext_bitmap(buf) ((buf)->uip_ext_bitmap)
#define uip_ext_opt_offset(buf) ((buf)->uip_ext_opt_offset)
#define uip_nd6_opt_offset(buf) ((buf)->nd6_opt_offset)
#define uip_next_hdr(buf) ((buf)->uip_next_hdr)
#define uip_appdata(buf) ((buf)->uip_appdata)
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

/*!
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
struct net_buf *net_buf_get(struct net_context *context);

/*!
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
struct net_buf *net_buf_get_reserve(uint16_t reserve_head);

/*!
 * @brief Place buffer back into the available buffers pool.
 *
 * @details Releases the buffer to other use. This needs to be
 * called by application after it has finished with
 * the buffer.
 *
 * @param buf Network buffer to release.
 *
 */
void net_buf_put(struct net_buf *buf);

/*!
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

/*!
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

/*!
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

/*! @def net_buf_tail
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

/*! @cond ignore */
void net_buf_init(void);
/* @endcond */

#endif /* __NET_BUF_H */
