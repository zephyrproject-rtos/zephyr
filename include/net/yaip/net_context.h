/** @file
 * @brief Network context definitions
 *
 * An API for applications to define a network connection.
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

#if defined(CONFIG_NET_YAIP)

#ifndef __NET_CONTEXT_H
#define __NET_CONTEXT_H

#include <nanokernel.h>

#include <net/net_ip.h>
#include <net/net_if.h>

#ifdef __cplusplus
extern "C" {
#endif

struct net_context {
	/* Connection tuple identifies the connection */
	struct net_tuple tuple;

	/* Application receives data via this fifo */
	struct nano_fifo rx_queue;

	/* Network interface assigned to this context */
	struct net_if *iface;
};

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
 * @brief Get network tuple for this context.
 *
 * @details This function returns the used connection tuple.
 *
 * @param context Network context.
 *
 * @return Network tuple if successful, NULL otherwise.
 */
static inline
struct net_tuple *net_context_get_tuple(struct net_context *context)
{
	return &context->tuple;
}

/**
 * @brief Get network queue for this context.
 *
 * @details This function returns the used network queue.
 *
 * @param context Network context.
 *
 * @return Context RX queue if successful, NULL otherwise.
 */
static inline
struct nano_fifo *net_context_get_queue(struct net_context *context)
{
	return &context->rx_queue;
}

/**
 * @brief Get network interface for this context.
 *
 * @details This function returns the used network interface.
 *
 * @param context Network context.
 *
 * @return Context network interface if context is bind to interface,
 * NULL otherwise.
 */
static inline
struct net_if *net_context_get_iface(struct net_context *context)
{
	return context->iface;
}

/**
 * @brief Set network interface for this context.
 *
 * @details This function binds network interface to this context.
 *
 * @param context Network context.
 * @param iface Network interface.
 */
static inline void net_context_set_iface(struct net_context *context,
					 struct net_if *iface)
{
	context->iface = iface;
}

#ifdef __cplusplus
}
#endif

#endif /* __NET_CONTEXT_H */

#endif /* CONFIG_NET_YAIP */
