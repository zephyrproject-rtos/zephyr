/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_MODULE_NAME net_rpl_br_shell
#define NET_LOG_LEVEL LOG_LEVEL_DBG

#include <zephyr.h>
#include <shell/shell.h>
#include <shell/shell_uart.h>
#include <stdio.h>

#include "../../../subsys/net/ip/rpl.h"

#include "config.h"

#define BR_SHELL_MODULE "br"

static int cmd_br_repair(const struct shell *shell,
			 size_t argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (shell_help_requested(shell)) {
		shell_help_print(shell, NULL, 0);
		return -ENOEXEC;
	}

	shell_fprintf(shell, SHELL_INFO, "Starting global repair...\n");

	net_rpl_repair_root(CONFIG_NET_RPL_DEFAULT_INSTANCE);

	return 0;
}

static int cmd_coap_send(const struct shell *shell,
			 size_t argc, char *argv[])
{
	struct in6_addr peer_addr;
	enum coap_request_type type;
	int r;

	if (shell_help_requested(shell)) {
		shell_help_print(shell, NULL, 0);
		return -ENOEXEC;
	}

	if (argc != 3 || !argv[1] || !argv[2]) {
		shell_fprintf(shell, SHELL_ERROR, "Invalid arguments.\n");
		return -ENOEXEC;
	}

	r = net_addr_pton(AF_INET6, argv[2], &peer_addr);
	if (r < 0) {
		return -ENOEXEC;
	}

	if (strcmp(argv[1], "toggle") == 0) {
		type = COAP_REQ_TOGGLE_LED;
	} else if (strcmp(argv[1], "rpl_obs") == 0) {
		type = COAP_REQ_RPL_OBS;
	} else {
		shell_fprintf(shell, SHELL_ERROR, "Invalid arguments.\n");
		return -ENOEXEC;
	}

	coap_send_request(&peer_addr, type, NULL, NULL);

	return 0;
}

SHELL_CREATE_STATIC_SUBCMD_SET(br_commands)
{
	SHELL_CMD(coap, NULL,
		  "Send CoAP commands to Node\n"
		  "toggle <host>   Toggle the LED on Node\n"
		  "rpl_obs <host>  Set RPL observer on Node",
		  cmd_coap_send),
	SHELL_CMD(repair, NULL,
		  "Global repair RPL network",
		  cmd_br_repair),
	SHELL_SUBCMD_SET_END
};

SHELL_CMD_REGISTER(br, &br_commands, "Border router commands", NULL);

void br_shell_init(void)
{
}
