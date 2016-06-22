/** @file
 @brief Generic connection handling.

 This is not to be included by the application.
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

#ifndef __CONNECTION_H
#define __CONNECTION_H

#include <stdint.h>

#include <misc/util.h>

#include <net/net_core.h>
#include <net/net_ip.h>
#include <net/nbuf.h>

#ifdef __cplusplus
extern "C" {
#endif

struct net_conn;

/**
 * @brief Function that is called by connection subsystem when UDP/TCP
 * packet is received and which matches local and remote IP address
 * and port.
 */
typedef enum net_verdict (*net_conn_cb_t)(struct net_buf *buf,
					  void *user_data);

/**
 * @brief Information about a connection in the system.
 *
 * Stores the connection information.
 *
 */
struct net_conn {
	/** Remote IP address */
	struct sockaddr remote_addr;

	/** Local IP address */
	struct sockaddr local_addr;

	/** Callback to be called when matching UDP packet is received */
	net_conn_cb_t cb;

	/** Possible user to pass to the callback */
	void *user_data;

	/** Connection protocol */
	uint8_t proto;

	/** Flags for the connection */
	uint8_t flags;

	/** Rank of this connection. Higher rank means more specific
	 * connection.
	 * Value is constructed like this:
	 *   bit 0  local port, bit set if specific value
	 *   bit 1  remote port, bit set if specific value
	 *   bit 2  local address, bit set if unspecified address
	 *   bit 3  remote address, bit set if unspecified address
	 *   bit 4  local address, bit set if specific address
	 *   bit 5  remote address, bit set if specific address
	 */
	uint8_t rank;
};

/**
 * @brief Register a callback to be called when UDP/TCP packet
 * is received corresponding to received packet.
 *
 * @param proto Protocol for the connection (UDP or TCP)
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
int net_conn_register(enum ip_protocol proto,
		      const struct sockaddr *remote_addr,
		      const struct sockaddr *local_addr,
		      uint16_t remote_port,
		      uint16_t local_port,
		      net_conn_cb_t cb,
		      void *user_data,
		      void **handle);

/**
 * @brief Unregister connection handler.
 *
 * @param handle Handle from registering.
 *
 * @return Return 0 if the unregistration succeed, <0 otherwise.
 */
int net_conn_unregister(void *handle);

/**
 * @brief Called by net_core.c when a network packet is received.
 *
 * @param buf Network buffer holding received data
 *
 * @return NET_OK if the packet was consumed, NET_DROP if
 * the packet parsing failed and the caller should handle
 * the received packet. If corresponding IP protocol support is
 * disabled, the function will always return NET_DROP.
 */
#if defined(CONFIG_NET_UDP) || defined(CONFIG_NET_TCP)
enum net_verdict net_conn_input(enum ip_protocol proto, struct net_buf *buf);
#else
static inline enum net_verdict net_conn_input(enum ip_protocol proto,
					      struct net_buf *buf)
{
	return NET_DROP;
}
#endif /* CONFIG_NET_UDP || CONFIG_NET_TCP */

void net_conn_init(void);

#ifdef __cplusplus
}
#endif

#endif /* __CONNECTION_H */
