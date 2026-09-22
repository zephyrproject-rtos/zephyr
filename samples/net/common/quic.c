/*
 * Copyright (c) 2026 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_samples_common, LOG_LEVEL_DBG);

#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/net/tls_credentials.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/quic.h>

#include "net_sample_common.h"

#define DEFAULT_QUIC_PORT 4422

int setup_quic(const struct net_sockaddr *remote_addr,
	       const struct net_sockaddr *local_addr,
	       enum quic_stream_direction direction,
	       sec_tag_t sec_tag_list[],
	       size_t sec_tag_list_size,
	       const char *alpn_list[],
	       size_t alpn_list_size)
{
	int quic_sock;
	int ret;

	ARG_UNUSED(direction);

	if (remote_addr != NULL && net_sin(remote_addr)->sin_port == 0) {
		net_sin(remote_addr)->sin_port = net_htons(DEFAULT_QUIC_PORT);
	}

	if (local_addr != NULL && net_sin(local_addr)->sin_port == 0) {
		net_sin(local_addr)->sin_port = net_htons(DEFAULT_QUIC_PORT);
	}

	quic_sock = quic_connection_open(remote_addr, local_addr);
	if (quic_sock < 0) {
		LOG_ERR("Failed to open QUIC connection socket (%d)", quic_sock);
		return quic_sock;
	}

	if (remote_addr != NULL) {
		LOG_INF("QUIC %s remote connection socket %d opened successfully",
			remote_addr->sa_family == NET_AF_INET6 ? "IPv6" : "IPv4",
			quic_sock);
	} else {
		LOG_INF("QUIC %s local connection socket %d opened successfully",
			local_addr->sa_family == NET_AF_INET6 ? "IPv6" : "IPv4",
			quic_sock);
	}

	ret = zsock_setsockopt(quic_sock, ZSOCK_SOL_TLS, ZSOCK_TLS_SEC_TAG_LIST,
			       sec_tag_list, sec_tag_list_size);
	if (ret < 0) {
		LOG_ERR("Failed to set TLS certs (%d)", -errno);
		ret = -errno;
		close_quic(quic_sock);
		goto out;
	}

	ret = zsock_setsockopt(quic_sock, ZSOCK_SOL_TLS, ZSOCK_TLS_ALPN_LIST,
			       alpn_list, alpn_list_size);
	if (ret < 0) {
		LOG_ERR("Failed to set ALPN (%d)", -errno);
		ret = -errno;
		close_quic(quic_sock);
		goto out;
	}

	ret = quic_sock;

out:
	return ret;
}

void close_quic(int quic_sock)
{
	int ret;

	ret = quic_connection_close(quic_sock);
	if (ret < 0) {
		LOG_ERR("Failed to close QUIC socket %d (%d)", quic_sock, ret);
		return;
	}

	LOG_INF("QUIC socket %d closed successfully", quic_sock);
}
