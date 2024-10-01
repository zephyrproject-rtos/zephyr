/*
 * Copyright (c) 2018 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_socket_offload, CONFIG_NET_SOCKETS_LOG_LEVEL);

#include <zephyr/net/socket_offload.h>
#include <zephyr/net/socket.h>

#include "sockets_internal.h"

const struct socket_dns_offload *dns_offload;

void socket_offload_dns_register(const struct socket_dns_offload *ops)
{
	__ASSERT_NO_MSG(ops);
	__ASSERT_NO_MSG(dns_offload == NULL);

	dns_offload = ops;
}

int socket_offload_getaddrinfo(const char *node, const char *service,
			       const struct zsock_addrinfo *hints,
			       struct zsock_addrinfo **res)
{
	__ASSERT_NO_MSG(dns_offload);
	__ASSERT_NO_MSG(dns_offload->getaddrinfo);

	return dns_offload->getaddrinfo(node, service, hints, res);
}

void socket_offload_freeaddrinfo(struct zsock_addrinfo *res)
{
	__ASSERT_NO_MSG(dns_offload);
	__ASSERT_NO_MSG(dns_offload->freeaddrinfo);

	dns_offload->freeaddrinfo(res);
}
