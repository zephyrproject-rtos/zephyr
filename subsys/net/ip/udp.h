/** @file
 @brief UDP data handler

 This is not to be included by the application.
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __UDP_H
#define __UDP_H

#include <zephyr/types.h>

#include <net/net_core.h>
#include <net/net_ip.h>
#include <net/net_pkt.h>
#include <net/net_context.h>

#include "connection.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_NET_UDP)
/**
 * @brief Append UDP packet into net_pkt
 *
 * @param pkt Network packet
 * @param src_port Source port in host byte order.
 * @param dst_port Destination port in host byte order.
 *
 * @return Return network packet that contains the UDP packet.
 */
static inline struct net_pkt *net_udp_append_raw(struct net_pkt *pkt,
						 u16_t src_port,
						 u16_t dst_port)
{
	net_pkt_append_be16(pkt, src_port);
	net_pkt_append_be16(pkt, dst_port);
	net_pkt_append_be16(pkt, net_pkt_get_len(pkt) -
			    net_pkt_ip_hdr_len(pkt) -
			    net_pkt_ipv6_ext_len(pkt));

	net_pkt_set_appdata(pkt, net_pkt_udp_data(pkt) +
			    sizeof(struct net_udp_hdr));

	return pkt;
}

#ifndef NET_UDP_PKT_WAIT_TIME
#define NET_UDP_PKT_WAIT_TIME K_SECONDS(1)
#endif

static inline struct net_buf *net_udp_set_chksum(struct net_pkt *pkt,
						 struct net_buf *frag)
{
	u16_t chksum;
	u16_t pos;

	frag = net_pkt_write_be16(pkt, pkt->frags,
				  net_pkt_ip_hdr_len(pkt) +
				  net_pkt_ipv6_ext_len(pkt) +
				  2 + 2 + 2 /* src + dst + len */,
				  &pos, 0);

	chksum = ~net_calc_chksum_udp(pkt);

	frag = net_pkt_write(pkt, frag, pos - 2, &pos, sizeof(chksum),
			     (u8_t *)&chksum, NET_UDP_PKT_WAIT_TIME);

	NET_ASSERT(frag);

	return frag;
}

static inline u16_t net_udp_get_chksum(struct net_pkt *pkt,
				       struct net_buf *frag)
{
	u16_t chksum;
	u16_t pos;

	frag = net_frag_read_be16(pkt->frags,
				  net_pkt_ip_hdr_len(pkt) +
				  net_pkt_ipv6_ext_len(pkt) +
				  2 + 2 + 2 /* src + dst + len */,
				  &pos, &chksum);
	NET_ASSERT(frag);

	return chksum;
}

static inline struct net_udp_hdr *net_udp_get_hdr(struct net_pkt *pkt,
						  struct net_udp_udp *hdr)
{
	struct net_buf *frag = pkt->frags;
	u16_t pos;

	frag = net_frag_read_be16(frag, net_pkt_ip_hdr_len(pkt) +
				  net_pkt_ipv6_ext_len(pkt),
				  &pos, &hdr->src_port);
	frag = net_frag_read_be16(frag, pos, &pos, &hdr->dst_port);
	frag = net_frag_read_be16(frag, pos, &pos, &hdr->len);
	frag = net_frag_read_be16(frag, pos, &pos, &hdr->chksum);
	if (!frag) {
		NET_ASSERT(frag);
		return NULL;
	}

	return hdr;
}

/**
 * @brief Append UDP packet into net_pkt
 *
 * @param context Network context for a connection
 * @param pkt Network packet
 * @param port Destination port in host byte order.
 *
 * @return Return network packet that contains the UDP packet.
 */
static inline struct net_pkt *net_udp_append(struct net_context *context,
					     struct net_pkt *pkt,
					     u16_t port)
{
	return net_udp_append_raw(pkt,
				  ntohs(net_sin((struct sockaddr *)
						&context->local)->sin_port),
				  port);
}
#else
#define net_udp_append_raw(pkt, src_port, dst_port) (pkt)
#define net_udp_append(context, pkt, port) (pkt)
#endif /* CONFIG_NET_UDP */

/**
 * @brief Register a callback to be called when UDP packet
 * is received corresponding to received packet.
 *
 * @param remote_addr Remote address of the connection end point.
 * @param local_addr Local address of the connection end point.
 * @param remote_port Remote port of the connection end point.
 * @param local_port Local port of the connection end point.
 * @param cb Callback to be called
 * @param user_data User data supplied by caller.
 * @param handle UDP handle that can be used when unregistering
 *
 * @return Return 0 if the registration succeed, <0 otherwise.
 */
static inline int net_udp_register(const struct sockaddr *remote_addr,
				   const struct sockaddr *local_addr,
				   u16_t remote_port,
				   u16_t local_port,
				   net_conn_cb_t cb,
				   void *user_data,
				   struct net_conn_handle **handle)
{
	return net_conn_register(IPPROTO_UDP, remote_addr, local_addr,
				 remote_port, local_port, cb, user_data,
				 handle);
}

/**
 * @brief Unregister UDP handler.
 *
 * @param handle Handle from registering.
 *
 * @return Return 0 if the unregistration succeed, <0 otherwise.
 */
static inline int net_udp_unregister(struct net_conn_handle *handle)
{
	return net_conn_unregister(handle);
}

#if defined(CONFIG_NET_UDP)
static inline void net_udp_init(void)
{
	return;
}
#else
#define net_udp_init(...)
#endif

#ifdef __cplusplus
}
#endif

#endif /* __UDP_H */
