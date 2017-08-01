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

static struct shell_cmd br_commands[] = {
	/* Keep the commands in alphabetical order */
	{ "repair", br_repair,
		"\n\tGlobal repair RPL network" },
	{ NULL, NULL, NULL }
};

SHELL_REGISTER(BR_SHELL_MODULE, br_commands);
