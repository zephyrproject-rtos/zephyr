/** @file
 @brief Simple socket API

 Simple socket API for applications to connection establishment and
 disconnection.
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

#ifndef __NET_SOCKET_H
#define __NET_SOCKET_H

#include <stdint.h>
#include <net/net_ip.h>
#include <net/net_buf.h>

/**
 * @brief Get network context.
 *
 * @details Network context is used to define the connection
 * 5-tuple (protocol, remote address, remote port, source
 * address and source port).
 *
 * @param protocol Protocol to use. Currently only UDP is supported.
 * @param remote_addr Remote IPv6/IPv4 address.
 * @param remote_port Remote UDP/TCP port.
 * @param local_addr Local IPv6/IPv4 address. If the local addres is NULL
 * or set to be anyaddr (all zeros), the IP stack will use the link
 * local address defined for the system.
 * @param local_port Local UDP/TCP port. If the local port is 0,
 * then a random port will be allocated.
 *
 * @return Network context if successful, NULL otherwise.
 */
struct net_context *net_context_get(enum ip_protocol ip_proto,
				    const struct net_addr *remote_addr,
				    uint16_t remote_port,
				    const struct net_addr *local_addr,
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
 *
 * @param buf Network buffer.
 *
 * @return 0 if ok, <0 if error.
 */
int net_send(struct net_buf *buf);

/**
 * @brief Receive data from network.
 *
 * @details Application uses this to get data from network
 * connection. This function will not wait so if there is
 * no data to return, then NULL is returned. Caller is
 * responsible to release the returned net_buf.
 *
 * @param context Network context.
 * @param timeout Timeout to wait. The value is in ticks.
 * If TICKS_UNLIMITED (-1), wait forever.
 * If TICKS_NONE (0), do not wait.
 * If > 0, wait amount of ticks.
 * The timeout is only available if kernel is compiled
 * with CONFIG_NANO_TIMEOUTS. If CONFIG_NANO_TIMEOUT is not
 * defined, then value > 0 means to not wait.
 *
 * @return Network buffer if successful, NULL otherwise.
 */
struct net_buf *net_receive(struct net_context *context,
			    int32_t timeout);

#endif /* __NET_SOCKET_H */
