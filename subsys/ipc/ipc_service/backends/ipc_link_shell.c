/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Basalte bv
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/ipc/ipc_link.h>
#include <zephyr/ipc/ipc_service_backend.h>
#include <zephyr/shell/shell.h>

static bool ipc_link_shell_is_link(const struct device *dev)
{
	return dev->api == &ipc_link_backend_ops;
}

static void ipc_link_shell_print(const struct shell *sh, const struct device *dev)
{
	const struct ipc_link_config *cfg = dev->config;
	struct ipc_link_stats stats;
	int ret;

	ret = ipc_link_stats_get(dev, &stats);
	if (ret < 0) {
		shell_error(sh, "%s: failed to get the counters (%d)", dev->name, ret);
		return;
	}

	shell_print(sh, "%s, %u B frames", dev->name, cfg->mtu);
	shell_print(sh, "rx frames:       %u", stats.rx_frames);
	shell_print(sh, "tx payloads:     %u", stats.tx_payloads);
	shell_print(sh, "retransmits:     %u", stats.retransmits);
	shell_print(sh, "rx payloads:     %u", stats.rx_payloads);
	shell_print(sh, "header errors:   %u", stats.header_errors);
	shell_print(sh, "peer mismatches: %u", stats.peer_mismatches);
	shell_print(sh, "payload errors:  %u", stats.payload_errors);
	shell_print(sh, "rx dropped:      %u", stats.rx_dropped);
	shell_print(sh, "peer resets:     %u", stats.peer_resets);
	shell_print(sh, "disconnects:     %u", stats.disconnects);
}

static void ipc_link_shell_reset(const struct shell *sh, const struct device *dev)
{
	int ret;

	ret = ipc_link_stats_reset(dev);
	if (ret < 0) {
		shell_error(sh, "%s: failed to clear the counters (%d)", dev->name, ret);
	}
}

/* Runs fn for the instance named in argv[1], or for every instance without one */
static int ipc_link_shell_foreach(const struct shell *sh, size_t argc, char **argv,
				  void (*fn)(const struct shell *sh, const struct device *dev))
{
	const struct device *dev;

	if (argc > 1) {
		dev = shell_device_get_binding(argv[1]);
		if (dev == NULL || !ipc_link_shell_is_link(dev)) {
			shell_error(sh, "%s: not an IPC link instance", argv[1]);
			return -ENODEV;
		}

		fn(sh, dev);
		return 0;
	}

	for (size_t i = 0; (dev = shell_device_filter(i, ipc_link_shell_is_link)) != NULL; i++) {
		fn(sh, dev);
	}

	return 0;
}

static int cmd_stats(const struct shell *sh, size_t argc, char **argv)
{
	return ipc_link_shell_foreach(sh, argc, argv, ipc_link_shell_print);
}

static int cmd_reset(const struct shell *sh, size_t argc, char **argv)
{
	return ipc_link_shell_foreach(sh, argc, argv, ipc_link_shell_reset);
}

static void device_name_get(size_t idx, struct shell_static_entry *entry)
{
	const struct device *dev = shell_device_filter(idx, ipc_link_shell_is_link);

	entry->syntax = (dev != NULL) ? dev->name : NULL;
	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = NULL;
}

SHELL_DYNAMIC_CMD_CREATE(dsub_device_name, device_name_get);

SHELL_STATIC_SUBCMD_SET_CREATE(sub_ipc_link,
			       SHELL_CMD_ARG(stats, &dsub_device_name,
					     "Show the counters of one or all instances [device]",
					     cmd_stats, 1, 1),
			       SHELL_CMD_ARG(reset, &dsub_device_name,
					     "Clear the counters of one or all instances [device]",
					     cmd_reset, 1, 1),
			       SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(ipc_link, &sub_ipc_link, "IPC link commands", NULL);
