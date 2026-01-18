/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_shell);

#include <stdlib.h>

#include "net_shell_private.h"

#include <zephyr/net/capture.h>

#if defined(CONFIG_NET_CAPTURE)
#define DEFAULT_DEV_NAME "NET_CAPTURE0"
static const struct device *capture_dev;

static void get_address_str(const struct net_sockaddr *addr,
			    char *str, int str_len)
{
	if (IS_ENABLED(CONFIG_NET_IPV6) && addr->sa_family == NET_AF_INET6) {
		snprintk(str, str_len, "[%s]:%u",
			 net_sprint_ipv6_addr(&net_sin6(addr)->sin6_addr),
			 net_ntohs(net_sin6(addr)->sin6_port));

	} else if (IS_ENABLED(CONFIG_NET_IPV4) && addr->sa_family == NET_AF_INET) {
		snprintk(str, str_len, "%s:%d",
			 net_sprint_ipv4_addr(&net_sin(addr)->sin_addr),
			 net_ntohs(net_sin(addr)->sin_port));

	} else if (IS_ENABLED(CONFIG_NET_SOCKETS_PACKET) &&
		   addr->sa_family == NET_AF_PACKET) {
		snprintk(str, str_len, "AF_PACKET");
	} else if (IS_ENABLED(CONFIG_NET_SOCKETS_CAN) &&
		   addr->sa_family == NET_AF_CAN) {
		snprintk(str, str_len, "AF_CAN");
	} else if (addr->sa_family == NET_AF_UNSPEC) {
		snprintk(str, str_len, "AF_UNSPEC");
	} else {
		snprintk(str, str_len, "AF_UNK(%d)", addr->sa_family);
	}
}

static void capture_cb(struct net_capture_info *info, void *user_data)
{
	struct net_shell_user_data *data = user_data;
	const struct shell *sh = data->sh;
	int *count = data->user_data;
	char addr_local[ADDR_LEN + 7];
	char addr_peer[ADDR_LEN + 7];

	if (*count == 0) {
		PR("      \t\tCapture  Tunnel\n");
		PR("Device\t\tiface    iface   Local\t\t\tPeer\n");
	}

	get_address_str(info->local, addr_local, sizeof(addr_local));
	get_address_str(info->peer, addr_peer, sizeof(addr_peer));

	PR("%s\t%c        %d      %s\t%s\n", info->capture_dev->name,
	   info->is_enabled ?
	   (net_if_get_by_iface(info->capture_iface) + '0') : '-',
	   net_if_get_by_iface(info->tunnel_iface),
	   addr_local, addr_peer);

	(*count)++;
}
#endif

static int cmd_net_capture(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_NET_CAPTURE)
	bool ret;

	if (capture_dev == NULL) {
		capture_dev = device_get_binding(DEFAULT_DEV_NAME);
	}

	if (capture_dev == NULL) {
		PR_INFO("Network packet capture %s\n", "not configured");
	} else {
		struct net_shell_user_data user_data;
		int count = 0;

		ret = net_capture_is_enabled(capture_dev);
		PR_INFO("Network packet capture %s\n",
			ret ? "enabled" : "disabled");

		user_data.sh = sh;
		user_data.user_data = &count;

		net_capture_foreach(capture_cb, &user_data);
	}
#else
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	PR_INFO("Set %s to enable %s support.\n",
		"CONFIG_NET_CAPTURE", "network packet capture");
#endif
	return 0;
}

static int cmd_net_capture_setup(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_NET_CAPTURE)
	int ret, arg = 1;
	const char *remote, *local, *peer;

	remote = argv[arg++];
	if (!remote) {
		PR_WARNING("Remote IP address not specified.\n");
		return -ENOEXEC;
	}

	local = argv[arg++];
	if (!local) {
		PR_WARNING("Local IP address not specified.\n");
		return -ENOEXEC;
	}

	peer = argv[arg];
	if (!peer) {
		PR_WARNING("Peer IP address not specified.\n");
		return -ENOEXEC;
	}

	if (capture_dev != NULL) {
		PR_INFO("Capture already setup, cleaning up settings.\n");
		net_capture_cleanup(capture_dev);
		capture_dev = NULL;
	}

	ret = net_capture_setup(remote, local, peer, &capture_dev);
	if (ret < 0) {
		PR_WARNING("Capture cannot be setup (%d)\n", ret);
		return -ENOEXEC;
	}

	PR_INFO("Capture setup done, next enable it by "
		"\"net capture enable <idx>\"\n");
#else
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	PR_INFO("Set %s to enable %s support.\n",
		"CONFIG_NET_CAPTURE", "network packet capture");
#endif

	return 0;
}

static int cmd_net_capture_cleanup(const struct shell *sh, size_t argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

#if defined(CONFIG_NET_CAPTURE)
	int ret;

	if (capture_dev == NULL) {
		return 0;
	}

	ret = net_capture_cleanup(capture_dev);
	if (ret < 0) {
		PR_WARNING("Capture %s failed (%d)\n", "cleanup", ret);
		return -ENOEXEC;
	}

	capture_dev = NULL;
#else
	PR_INFO("Set %s to enable %s support.\n",
		"CONFIG_NET_CAPTURE", "network packet capture");
#endif

	return 0;
}

static int cmd_net_capture_enable(const struct shell *sh, size_t argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

#if defined(CONFIG_NET_CAPTURE)
	int ret, arg = 1, if_index;
	struct net_if *iface;

	if (capture_dev == NULL) {
		return 0;
	}

	if (argv[arg] == NULL) {
		PR_WARNING("Interface index is missing. Please give interface "
			   "what you want to monitor\n");
		return -ENOEXEC;
	}

	if_index = atoi(argv[arg++]);
	if (if_index == 0) {
		PR_WARNING("Interface index %d is invalid.\n", if_index);
		return -ENOEXEC;
	}

	iface = net_if_get_by_index(if_index);
	if (iface == NULL) {
		PR_WARNING("No such interface with index %d\n", if_index);
		return -ENOEXEC;
	}

	ret = net_capture_enable(capture_dev, iface);
	if (ret < 0) {
		PR_WARNING("Capture %s failed (%d)\n", "enable", ret);
		return -ENOEXEC;
	}
#else
	PR_INFO("Set %s to enable %s support.\n",
		"CONFIG_NET_CAPTURE", "network packet capture");
#endif

	return 0;
}

static int cmd_net_capture_disable(const struct shell *sh, size_t argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

#if defined(CONFIG_NET_CAPTURE)
	int ret;

	if (capture_dev == NULL) {
		return 0;
	}

	ret = net_capture_disable(capture_dev);
	if (ret < 0) {
		PR_WARNING("Capture %s failed (%d)\n", "disable", ret);
		return -ENOEXEC;
	}
#else
	PR_INFO("Set %s to enable %s support.\n",
		"CONFIG_NET_CAPTURE", "network packet capture");
#endif

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(net_cmd_capture,
	SHELL_CMD(setup, NULL, "Setup network packet capture.\n"
		  "'net capture setup <remote-ip-addr> <local-addr> <peer-addr>'\n"
		  "<remote> is the (outer) endpoint IP address,\n"
		  "<local> is the (inner) local IP address,\n"
		  "<peer> is the (inner) peer IP address\n"
		  "Local and Peer addresses can have UDP port number in them (optional)\n"
		  "like 198.0.51.2:9000 or [2001:db8:100::2]:4242",
		  cmd_net_capture_setup),
	SHELL_CMD(cleanup, NULL, "Cleanup network packet capture.",
		  cmd_net_capture_cleanup),
	SHELL_CMD(enable, NULL, "Enable network packet capture for a given "
		  "network interface.\n"
		  "'net capture enable <interface index>'",
		  cmd_net_capture_enable),
	SHELL_CMD(disable, NULL, "Disable network packet capture.",
		  cmd_net_capture_disable),
	SHELL_SUBCMD_SET_END
);

SHELL_SUBCMD_ADD((net), capture, &net_cmd_capture,
		 "Configure network packet capture.", cmd_net_capture, 1, 0);
