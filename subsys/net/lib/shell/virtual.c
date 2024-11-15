/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_shell);

#include <stdlib.h>

#if defined(CONFIG_NET_L2_VIRTUAL)
#include <zephyr/net/virtual.h>
#endif

#include "net_shell_private.h"

#if defined(CONFIG_NET_L2_VIRTUAL)
static void virtual_iface_cb(struct net_if *iface, void *user_data)
{
	struct net_shell_user_data *data = user_data;
	const struct shell *sh = data->sh;
	int *count = data->user_data;
	char *name, buf[CONFIG_NET_L2_VIRTUAL_MAX_NAME_LEN];
	struct net_if *orig_iface;

	if (net_if_l2(iface) != &NET_L2_GET_NAME(VIRTUAL)) {
		return;
	}

	if (*count == 0) {
		PR("Interface  Attached-To  Description\n");
		(*count)++;
	}

	orig_iface = net_virtual_get_iface(iface);

	name = net_virtual_get_name(iface, buf, sizeof(buf));

	PR("%d          %c            %s\n",
	   net_if_get_by_iface(iface),
	   orig_iface ? net_if_get_by_iface(orig_iface) + '0' : '-',
	   name);

	(*count)++;
}

static void attached_iface_cb(struct net_if *iface, void *user_data)
{
	struct net_shell_user_data *data = user_data;
	const struct shell *sh = data->sh;
	int *count = data->user_data;
	char buf[CONFIG_NET_L2_VIRTUAL_MAX_NAME_LEN];
	const char *name;
	struct virtual_interface_context *ctx, *tmp;

	if (sys_slist_is_empty(&iface->config.virtual_interfaces)) {
		return;
	}

	if (*count == 0) {
		PR("Interface  Below-of  Description\n");
		(*count)++;
	}

	PR("%d          ", net_if_get_by_iface(iface));

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&iface->config.virtual_interfaces,
					  ctx, tmp, node) {
		if (ctx->virtual_iface == iface) {
			continue;
		}

		PR("%d ", net_if_get_by_iface(ctx->virtual_iface));
	}

	name = net_virtual_get_name(iface, buf, sizeof(buf));
	if (name == NULL) {
		name = iface2str(iface, NULL);
	}

	PR("        %s\n", name);

	(*count)++;
}
#endif /* CONFIG_NET_L2_VIRTUAL */

static int cmd_virtual_show(const struct shell *sh, size_t argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

#if defined(CONFIG_NET_L2_VIRTUAL)
	struct net_shell_user_data user_data;
	int count = 0;

	user_data.sh = sh;
	user_data.user_data = &count;

	net_if_foreach(virtual_iface_cb, &user_data);

	if (count == 0) {
		PR("No virtual interfaces found.");
	}

	count = 0;
	PR("\n");

	net_if_foreach(attached_iface_cb, &user_data);
#else
	PR_INFO("Set %s to enable %s support.\n", "CONFIG_NET_L2_VIRTUAL",
		"virtual network interface");
#endif
	return 0;
}

static int cmd_virtual_attach(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_NET_L2_VIRTUAL)
	struct net_if *virtual_iface, *lower_iface;
	int ret;

	virtual_iface = net_if_get_by_index(atoi(argv[1]));
	if (virtual_iface == NULL) {
		PR("No %s interface %s found.\n", "virtual", argv[1]);
		return -ENOENT;
	}

	lower_iface = net_if_get_by_index(atoi(argv[2]));
	if (lower_iface == NULL) {
		PR("No %s interface %s found.\n", "such", argv[2]);
		return -ENOENT;
	}

	ret = net_virtual_interface_attach(virtual_iface, lower_iface);
	if (ret < 0) {
		PR("Cannot attach interface %s to %s (%d)\n", argv[1], argv[2], ret);
		return -ENOENT;
	}
#else
	PR_INFO("Set %s to enable %s support.\n", "CONFIG_NET_L2_VIRTUAL",
		"virtual network interface");
#endif
	return 0;
}

static int cmd_virtual_detach(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_NET_L2_VIRTUAL)
	struct net_if *virtual_iface;
	int ret;

	virtual_iface = net_if_get_by_index(atoi(argv[1]));
	if (virtual_iface == NULL) {
		PR("No %s interface %s found.\n", "virtual", argv[1]);
		return -ENOENT;
	}

	ret = net_virtual_interface_attach(virtual_iface, NULL);
	if (ret < 0) {
		PR("Cannot detach interface %s (%d)\n", argv[1], ret);
		return -ENOENT;
	}
#else
	PR_INFO("Set %s to enable %s support.\n", "CONFIG_NET_L2_VIRTUAL",
		"virtual network interface");
#endif
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(virtual_commands,
	SHELL_CMD_ARG(attach, NULL,
		  "Attach a network interface to another interface.\n"
		  "'virtual attach <upper virtual iface index> <lower iface index>'",
		  cmd_virtual_attach, 3, 0),
	SHELL_CMD_ARG(detach, NULL,
		  "Detach a network interface from another interface.\n"
		  "'virtual detach <upper virtual iface index>'",
		  cmd_virtual_detach, 2, 0),
	SHELL_CMD_ARG(show, NULL,
		  "Show virtual interface information.\n"
		  "'virtual show'",
		  cmd_virtual_show, 1, 1),
	SHELL_SUBCMD_SET_END
);

SHELL_SUBCMD_ADD((net), virtual, &virtual_commands,
		 "Show/manipulate virtual network interfaces.",
		 cmd_virtual_show, 1, 1);
