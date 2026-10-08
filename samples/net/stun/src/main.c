/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Kirill Shypachov
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_stun_sample, LOG_LEVEL_INF);

#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/stun_client.h>

#include "net_sample_common.h"

int main(void)
{
	char text[NET_IPV6_ADDR_LEN];
	struct net_sockaddr mapped;
	const void *addr;
	int rc;

	wait_for_network();

	LOG_INF("Asking %s port %d", CONFIG_NET_SAMPLE_STUN_SERVER,
		CONFIG_NET_SAMPLE_STUN_SERVER_PORT);

	rc = stun_client_simple(CONFIG_NET_SAMPLE_STUN_SERVER, CONFIG_NET_SAMPLE_STUN_SERVER_PORT,
				CONFIG_NET_SAMPLE_STUN_TIMEOUT_MS, &mapped);
	switch (rc) {
	case 0:
		break;
	case -EHOSTUNREACH:
		LOG_ERR("Cannot resolve the name of the server");
		return 0;
	case -ETIMEDOUT:
		LOG_ERR("No answer from the server");
		return 0;
	default:
		LOG_ERR("STUN query failed (%d)", rc);
		return 0;
	}

	addr = &net_sin(&mapped)->sin_addr;
#if defined(CONFIG_NET_IPV6)
	if (mapped.sa_family == NET_AF_INET6) {
		addr = &net_sin6(&mapped)->sin6_addr;
	}
#endif
	LOG_INF("Public address: %s", net_addr_ntop(mapped.sa_family, addr, text, sizeof(text)));

	return 0;
}
