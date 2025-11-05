/*
 * Copyright 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_test, CONFIG_QUIC_LOG_LEVEL);

#include <zephyr/ztest_assert.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/quic.h>
#include <zephyr/net/tls_credentials.h>

#include "quic_test.h"

void prepare_quic_socket(int *sock,
			 const struct net_sockaddr *remote_addr,
			 const struct net_sockaddr *local_addr)
{
	int ret;

	zassert_not_null(sock, "null socket pointer");
	zassert_not_null(local_addr, "null sockaddr pointer");

	if (remote_addr != NULL && net_sin(remote_addr)->sin_port == 0) {
		net_sin(remote_addr)->sin_port = net_htons(DEFAULT_QUIC_PORT);
	}

	if (local_addr != NULL && net_sin(local_addr)->sin_port == 0) {
		net_sin(local_addr)->sin_port = net_htons(DEFAULT_QUIC_PORT);
	}

	ret = quic_connection_open(remote_addr, local_addr);
	zassert_true(ret >= 0, "Failed to open QUIC connection socket (%d)", ret);

	*sock = ret;
}

void setup_quic_certs(int sock, sec_tag_t sec_tag_list[], size_t sec_tag_list_size)
{
	/* Currently not set, update this if needed */
}

void setup_alpn(int sock, const char * const alpn_list[], size_t alpn_list_size)
{
	int ret;

	ret = zsock_setsockopt(sock, ZSOCK_SOL_TLS, ZSOCK_TLS_ALPN_LIST,
			       alpn_list, sizeof(const char *) * alpn_list_size);
	zassert_equal(ret, 0, "Failed to set ALPN list (%d)", -errno);
}
