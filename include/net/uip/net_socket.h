/** @file
 * @brief Simple socket API
 *
 * Simple socket API for applications to connection establishment and
 * disconnection.
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

#ifndef __NET_SOCKET_H
#define __NET_SOCKET_H

#include <stdint.h>
#include <net/net_ip.h>
#include <net/buf.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get network context.
 *
 * @details Network context is used to define the connection
 * 5-tuple (protocol, remote address, remote port, source
 * address and source port).
 *
 * @param ip_proto Protocol to use. Currently only UDP is supported.
 * @param remote_addr Remote IPv6/IPv4 address.
 * @param remote_port Remote UDP/TCP port.
 * @param local_addr Local IPv6/IPv4 address. If the local address is
 * set to be anyaddr (all zeros), the IP stack will use the link
 * local address defined for the system.
 * @param local_port Local UDP/TCP port. If the local port is 0,
 * then a random port will be allocated.
 *
 * @return Network context if successful, NULL otherwise.
 */
struct net_context *net_context_get(enum ip_protocol ip_proto,
				    const struct net_addr *remote_addr,
				    uint16_t remote_port,
				    struct net_addr *local_addr,
				    uint16_t local_port);

/**
 * @brief Get network tuple.
 *
 * @details This function returns the used connection tuple.
 *
 * @param context Network context.
 *
 * @return Network tuple if successful, NULL otherwise.
 */
struct net_tuple *net_context_get_tuple(struct net_context *context);

/**
 * @brief Release network context.
 *
 * @details Free the resources allocated for the context.
 * All network listeners tied to this context are removed.
 *
 * @param context Network context.
 *
 */
void net_context_put(struct net_context *context);

/**
 * @brief Send data to network.
 *
 * @details Send user specified data to network. This
 * requires that net_buf is tied to context. This means
 * that the net_buf was allocated using net_buf_get().
 * This function will yield in thread and fiber contexts.
 *
 * @param buf Network buffer.
 *
 * @return 0 if ok (do not unref the buf),
 *        <0 if error (unref the buf).
 */
int net_send(struct net_buf *buf);

/**
 * @brief Receive data from network.
 *
 * @details Application uses this to get data from network
 * connection. Caller can specify a timeout, if there is no
 * data to return after a timeout, a NULL will be returned.
 * Caller is responsible to release the returned net_buf.
 *
 * @param context Network context.
 * @param timeout Timeout to wait. The value is in ticks.
 * If TICKS_UNLIMITED (-1), wait forever.
 * If TICKS_NONE (0), do not wait.
 * If > 0, wait amount of ticks.
 * The timeout is only available if kernel is compiled
 * with CONFIG_NANO_TIMEOUTS. If CONFIG_NANO_TIMEOUT is not
 * defined, then value > 0 means not to wait.
 *
 * @return Network buffer if successful, NULL otherwise.
 */
struct net_buf *net_receive(struct net_context *context,
			    int32_t timeout);

/**
 * @brief Get the UDP connection pointer from net_context.
 *
 * @details This is only needed in special occacions when
 * there is an existing UDP connection that application
 * wants to attach to. Normally the connection information
 * is not needed by application.
 *
 * @return UDP connection if it exists, NULL otherwise.
 */
struct simple_udp_connection *
	net_context_get_udp_connection(struct net_context *context);

#ifdef __cplusplus
}
#endif

#endif /* __NET_SOCKET_H */
