/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if 1
#define SYS_LOG_DOMAIN "rpl-br/shell"
#define NET_SYS_LOG_LEVEL SYS_LOG_LEVEL_DEBUG
#define NET_LOG_ENABLED 1
#endif

#include <zephyr.h>
#include <shell/shell.h>
#include <stdio.h>

#include "../../../subsys/net/ip/rpl.h"

#include "config.h"

#define BR_SHELL_MODULE "br"

int br_repair(int argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	NET_INFO("Starting global repair...");

	net_rpl_repair_root(CONFIG_NET_RPL_DEFAULT_INSTANCE);

	return 0;
}

int coap_send(int argc, char *argv[])
{
	struct in6_addr peer_addr;
	enum coap_request_type type;
	int r;

	if (argc != 3 || !argv[1] || !argv[2]) {
		NET_INFO("Invalid arguments");
		return -EINVAL;
	}

	r = net_addr_pton(AF_INET6, argv[2], &peer_addr);
	if (r < 0) {
		return -EINVAL;
	}

	if (strcmp(argv[1], "toggle") == 0) {
		type = COAP_REQ_TOGGLE_LED;
	} else if (strcmp(argv[1], "rpl_obs") == 0) {
		type = COAP_REQ_RPL_OBS;
	} else {
		NET_INFO("Invalid arguments");
		return -EINVAL;
	}

	coap_send_request(&peer_addr, type, NULL, NULL);

	return 0;
}

static struct shell_cmd br_commands[] = {
	/* Keep the commands in alphabetical order */
	{ "coap", coap_send, "\n\tSend CoAP commands to Node\n"
			"toggle <host>\n\tToggle the LED on Node\n"
			"rpl_obs <host>\n\tSet RPL observer on Node\n"
	},
	{ "repair", br_repair,
		"\n\tGlobal repair RPL network" },
	{ NULL, NULL, NULL }
};

SHELL_REGISTER(BR_SHELL_MODULE, br_commands);
