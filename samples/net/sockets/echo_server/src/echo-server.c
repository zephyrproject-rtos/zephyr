/* echo-server.c - Networking echo server */

/*
 * Copyright (c) 2016 Intel Corporation.
 * Copyright (c) 2018 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(net_echo_server_sample, LOG_LEVEL_DBG);

#include <zephyr.h>
#include <linker/sections.h>
#include <errno.h>

#include <net/net_core.h>
#include <net/tls_credentials.h>

#include "common.h"
#include "certificate.h"

#define APP_BANNER "Run echo server"

static struct k_sem quit_lock;

struct configs conf = {
	.ipv4 = {
		.proto = "IPv4",
	},
	.ipv6 = {
		.proto = "IPv6",
	},
};

void quit(void)
{
	k_sem_give(&quit_lock);
}

static void init_app(void)
{
	k_sem_init(&quit_lock, 0, UINT_MAX);

	LOG_INF(APP_BANNER);

#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
	int err = tls_credential_add(SERVER_CERTIFICATE_TAG,
				     TLS_CREDENTIAL_SERVER_CERTIFICATE,
				     server_certificate,
				     sizeof(server_certificate));
	if (err < 0) {
		LOG_ERR("Failed to register public certificate: %d", err);
	}


	err = tls_credential_add(SERVER_CERTIFICATE_TAG,
				 TLS_CREDENTIAL_PRIVATE_KEY,
				 private_key, sizeof(private_key));
	if (err < 0) {
		LOG_ERR("Failed to register private key: %d", err);
	}
#endif

#if defined(CONFIG_MBEDTLS_KEY_EXCHANGE_PSK_ENABLED)
	err = tls_credential_add(PSK_TAG,
				TLS_CREDENTIAL_PSK,
				psk,
				sizeof(psk));
	if (err < 0) {
		LOG_ERR("Failed to register PSK: %d", err);
	}
	err = tls_credential_add(PSK_TAG,
				TLS_CREDENTIAL_PSK_ID,
				psk_id,
				sizeof(psk_id) - 1);
	if (err < 0) {
		LOG_ERR("Failed to register PSK ID: %d", err);
	}
#endif

	init_vlan();
}

void main(void)
{
	init_app();

	if (IS_ENABLED(CONFIG_NET_TCP)) {
		start_tcp();
	}

	if (IS_ENABLED(CONFIG_NET_UDP)) {
		start_udp();
	}

	k_sem_take(&quit_lock, K_FOREVER);

	LOG_INF("Stopping...");

	if (IS_ENABLED(CONFIG_NET_TCP)) {
		stop_tcp();
	}

	if (IS_ENABLED(CONFIG_NET_UDP)) {
		stop_udp();
	}
}
