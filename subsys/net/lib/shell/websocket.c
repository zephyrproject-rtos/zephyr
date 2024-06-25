/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_shell);

#if defined(CONFIG_NET_L2_VIRTUAL)
#include <zephyr/net/virtual.h>
#endif

#include "net_shell_private.h"

#include "websocket/websocket_internal.h"

#include <zephyr/sys/fdtable.h>

#if defined(CONFIG_WEBSOCKET_CLIENT)
static void websocket_context_cb(struct websocket_context *context,
				 void *user_data)
{
	struct net_shell_user_data *data = user_data;
	const struct shell *sh = data->sh;
	struct net_context *net_ctx;
	int *count = data->user_data;
	/* +7 for []:port */
	char addr_local[ADDR_LEN + 7];
	char addr_remote[ADDR_LEN + 7] = "";

	net_ctx = zvfs_get_fd_obj(context->real_sock, NULL, 0);
	if (net_ctx == NULL) {
		PR_ERROR("Invalid fd %d", context->real_sock);
		return;
	}

	if ((*count) == 0) {
		PR("     websocket/net_ctx  \tIface\t"
		   "%-16s\t%-16s\n", "Local", "Remote");
	}

	get_addresses(net_ctx, addr_local, sizeof(addr_local),
		      addr_remote, sizeof(addr_remote));

	PR("[%2d] %p/%p\t%d\t%-16s\t%-16s\n",
	   (*count) + 1, context, net_ctx,
	   net_if_get_by_iface(net_context_get_iface(net_ctx)),
	   addr_local, addr_remote);

	(*count)++;
}
#endif /* CONFIG_WEBSOCKET_CLIENT */

static int cmd_net_websocket(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_WEBSOCKET_CLIENT)
	struct net_shell_user_data user_data;
	int count = 0;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	user_data.sh = sh;
	user_data.user_data = &count;

	websocket_context_foreach(websocket_context_cb, &user_data);

	if (count == 0) {
		PR("No connections\n");
	}
#else
	PR_INFO("Set %s to enable %s support.\n", "CONFIG_WEBSOCKET_CLIENT",
		"Websocket");
#endif /* CONFIG_WEBSOCKET_CLIENT */

	return 0;
}

SHELL_SUBCMD_ADD((net), websocket, NULL,
		 "Print information about WebSocket connections.",
		 cmd_net_websocket, 1, 0);
