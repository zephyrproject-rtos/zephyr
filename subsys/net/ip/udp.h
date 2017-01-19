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

#include <stdint.h>

#include <net/net_core.h>
#include <net/net_ip.h>
#include <net/nbuf.h>
#include <net/net_context.h>

#include "connection.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_NET_UDP)
/**
 * @brief Append UDP packet into net_buf
 *
 * @param buf Network buffer
 * @param src_port Source port in host byte order.
 * @param dst_port Destination port in host byte order.
 *
 * @return Return network buffer that contains the UDP packet.
 */
static inline struct net_buf *net_udp_append_raw(struct net_buf *buf,
						 uint16_t src_port,
						 uint16_t dst_port)
{
	NET_UDP_BUF(buf)->src_port = htons(src_port);
	NET_UDP_BUF(buf)->dst_port = htons(dst_port);

	net_buf_add(buf->frags, sizeof(struct net_udp_hdr));

	NET_UDP_BUF(buf)->len = htons(net_buf_frags_len(buf) -
				      net_nbuf_ip_hdr_len(buf));

	net_nbuf_set_appdata(buf, net_nbuf_udp_data(buf) +
			     sizeof(struct net_udp_hdr));

	return buf;
}

/**
 * @brief Append UDP packet into net_buf
 *
 * @param context Network context for a connection
 * @param buf Network buffer
 * @param port Destination port in host byte order.
 *
 * @return Return network buffer that contains the UDP packet.
 */
static inline struct net_buf *net_udp_append(struct net_context *context,
					     struct net_buf *buf,
					     uint16_t port)
{
	return net_udp_append_raw(buf,
				  ntohs(net_sin((struct sockaddr *)
						&context->local)->sin_port),
				  port);
}
#else
#define net_udp_append_raw(buf, src_port, dst_port) (buf)
#define net_udp_append(context, buf, port) (buf)
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
				   uint16_t remote_port,
				   uint16_t local_port,
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
