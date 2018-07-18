/* echo-server.c - Networking echo server */

/*
 * Copyright (c) 2016 Intel Corporation.
 * Copyright (c) 2018 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if 1
#define SYS_LOG_DOMAIN "echo-server"
#define NET_SYS_LOG_LEVEL SYS_LOG_LEVEL_DEBUG
#define NET_LOG_ENABLED 1
#endif

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

	NET_INFO(APP_BANNER);

#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
	int err = tls_credential_add(SERVER_CERTIFICATE_TAG,
				     TLS_CREDENTIAL_SERVER_CERTIFICATE,
				     server_certificate,
				     sizeof(server_certificate));
	if (err < 0) {
		NET_ERR("Failed to register public certificate: %d", err);
	}


	err = tls_credential_add(SERVER_CERTIFICATE_TAG,
				 TLS_CREDENTIAL_PRIVATE_KEY,
				 private_key, sizeof(private_key));
	if (err < 0) {
		NET_ERR("Failed to register private key: %d", err);
	}
#endif
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

	NET_INFO("Stopping...");

	if (IS_ENABLED(CONFIG_NET_TCP)) {
		stop_tcp();
	}

	if (IS_ENABLED(CONFIG_NET_UDP)) {
		stop_udp();
	}
}
