/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc. (AMD)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/shell/shell.h>
#include <zephyr/net/net_if.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include "eth_xilinx_axienet_bd.h"

static void print_help(const struct shell *sh)
{
	shell_print(sh,
		"Usage:\n"
		"  axienet_bd_config help\n"
		"      Show this help message.\n"
		"\n"
		"  axienet_bd_config <iface_idx>\n"
		"      Get current TX/RX BD counts\n"
		"\n"
		"  axienet_bd_config <iface_idx> <num_tx> <num_rx>\n"
		"      Set new TX/RX BD counts\n"
		"\n"
		"Workflow:\n"
		"  1. net iface down <iface_idx>\n"
		"  2. axienet_bd_config <iface_idx> <num_tx> <num_rx>\n"
		"  3. net iface up <iface_idx>\n"
		"\n"
		"Examples:\n"
		"  axienet_bd_config 1        -- get current TX/RX BD counts\n"
		"  axienet_bd_config 1 8 8    -- set 8 TX + 8 RX BDs\n"
		"\n"
		"NOTE: Counts must be >= 1 and <= the compile-time Kconfig maxima\n"
		"      CONFIG_ETH_XILINX_AXIENET_BUFFER_NUM_TX/RX (default 16).\n"
		"      Rebuild with larger Kconfig values to allow higher counts.");
}

static int cmd_axienet_bd_config(const struct shell *sh, size_t argc, char **argv)
{
	/* No args or "help": print help */
	if (argc < 2 || (argc == 2 && strcmp(argv[1], "help") == 0)) {
		print_help(sh);
		return 0;
	}

	int err;
	long iface_idx = shell_strtol(argv[1], 10, &err);

	if (err) {
		shell_error(sh, "Invalid interface index: %s", argv[1]);
		return -EINVAL;
	}

	struct net_if *iface = net_if_get_by_index((int)iface_idx);

	if (!iface) {
		shell_error(sh, "Invalid interface index %ld", iface_idx);
		return -EINVAL;
	}

	const struct device *dev = net_if_get_device(iface);

	/* Query mode: one positional arg (iface_idx only) */
	if (argc == 2) {
		size_t num_tx, num_rx;

		xilinx_axienet_get_bd_counts(dev, &num_tx, &num_rx);
		shell_print(sh,
			"Interface %ld (%s): TX BDs = %zu, RX BDs = %zu  [%s]",
			iface_idx, dev->name, num_tx, num_rx,
			net_if_is_up(iface) ? "UP" : "DOWN");
		return 0;
	}

	/* Set mode: requires iface_idx + num_tx + num_rx */
	if (argc < 4) {
		shell_error(sh, "Usage: axienet_bd_config <iface_idx> <num_tx> <num_rx>");
		shell_error(sh, "Run 'axienet_bd_config help' for details.");
		return -EINVAL;
	}

	if (net_if_is_up(iface)) {
		shell_warn(sh, "Interface %ld is UP. Bring it down first:", iface_idx);
		shell_warn(sh, "  net iface down %ld", iface_idx);
		shell_warn(sh, "Then: axienet_bd_config %ld <num_tx> <num_rx>", iface_idx);
		return -EBUSY;
	}

	unsigned long num_tx = shell_strtoul(argv[2], 10, &err);

	if (err) {
		shell_error(sh, "Invalid num_tx: %s", argv[2]);
		return -EINVAL;
	}

	unsigned long num_rx = shell_strtoul(argv[3], 10, &err);

	if (err) {
		shell_error(sh, "Invalid num_rx: %s", argv[3]);
		return -EINVAL;
	}

	int ret = xilinx_axienet_set_bd_counts(dev, (size_t)num_tx, (size_t)num_rx);

	if (ret) {
		shell_error(sh, "Failed to set BD counts: %d", ret);
		return ret;
	}

	shell_print(sh,
		"BD counts set: tx=%lu rx=%lu. Run 'net iface up %ld' to apply.",
		num_tx, num_rx, iface_idx);
	return 0;
}

SHELL_CMD_REGISTER(axienet_bd_config, NULL,
	"Configure Xilinx AXI Ethernet TX/RX BD ring counts",
	cmd_axienet_bd_config);
