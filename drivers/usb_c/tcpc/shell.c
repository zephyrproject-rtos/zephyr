/*
 * Copyright 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/usb_c/usbc.h>
#include <zephyr/shell/shell.h>

/** Macro used to call the dump_std_reg function from the TCPC device pointer */
#define TCPC_DUMP_DEV(dev) ret |= tcpc_dump_std_reg(dev);

/** Macro used to call the dump_std_reg function from the USB-C connector node */
#define TCPC_DUMP_CONN_NODE(node) TCPC_DUMP_DEV(DEVICE_DT_GET(DT_PROP(node, tcpc)))

/**
 * @brief Shell command that dumps standard registers of TCPCs for all available USB-C ports
 *
 * @param sh Shell structure
 * @param argc Arguments count
 * @param argv Device name
 * @return int ORed return values of all the functions executed, 0 in case of success
 */
static int cmd_tcpc_dump(const struct shell *sh, size_t argc, char **argv)
{
	int ret = 0;

	if (argc <= 1) {
		DT_FOREACH_STATUS_OKAY(usb_c_connector, TCPC_DUMP_CONN_NODE);
	} else {
		const struct device *dev = device_get_binding(argv[1]);

		if (dev != NULL) {
			TCPC_DUMP_DEV(dev);
		} else {
			ret = -ENODEV;
		}
	}

	return ret;
}

/**
 * @brief Function used to create subcommands with devices names
 *
 * @param idx counter of devices
 * @param entry shell structure that will be filled
 */
static void device_name_get(size_t idx, struct shell_static_entry *entry)
{
	const struct device *dev = shell_device_lookup(idx, NULL);

	entry->syntax = (dev != NULL) ? dev->name : NULL;
	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = NULL;
}

SHELL_DYNAMIC_CMD_CREATE(list_device_names, device_name_get);

SHELL_STATIC_SUBCMD_SET_CREATE(sub_tcpc_cmds,
			       SHELL_CMD_ARG(dump, &list_device_names,
					     "Dump TCPC registers\n"
					     "Usage: tcpc dump [<tcpc device>]",
					     cmd_tcpc_dump, 1, 1),
			       SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(tcpc, &sub_tcpc_cmds, "TCPC (USB-C PD) diagnostics", NULL);
