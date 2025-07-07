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

static const struct socket_dns_offload *dns_offload;
static bool dns_offload_enabled;

void socket_offload_dns_register(const struct socket_dns_offload *ops)
{
	__ASSERT_NO_MSG(ops);
	__ASSERT_NO_MSG(dns_offload == NULL);

	dns_offload = ops;

	socket_offload_dns_enable(true);
}

int socket_offload_dns_deregister(const struct socket_dns_offload *ops)
{
	__ASSERT_NO_MSG(ops != NULL);

	if (dns_offload != ops) {
		return -EINVAL;
	}

	dns_offload = NULL;

	socket_offload_dns_enable(false);

	return 0;
}

void socket_offload_dns_enable(bool enable)
{
	dns_offload_enabled = enable;
}

bool socket_offload_dns_is_enabled(void)
{
	return (dns_offload != NULL) && dns_offload_enabled;
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
