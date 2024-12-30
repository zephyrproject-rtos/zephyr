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

/** Macro used to call the vbus_measure function from the VBUS device pointer */
#define TCPC_VBUS_DEV(dev)                                                                         \
	{                                                                                          \
		int val;                                                                           \
		ret |= usbc_vbus_measure(dev, &val);                                               \
		shell_print(sh, "%s vbus: %d mV", dev->name, val);                                 \
	}

/** Macro used to call the vbus_measure function from the USB-C connector node */
#define TCPC_VBUS_CONN_NODE(node) TCPC_VBUS_DEV(DEVICE_DT_GET(DT_PROP(node, vbus)))

/** Macro used to call the get_chip function from the TCPC device pointer */
#define TCPC_GET_CHIP_DEV(dev)                                                                     \
	{                                                                                          \
		ret |= tcpc_get_chip_info(dev, &chip_info);                                        \
		shell_print(sh, "Chip: %s", dev->name);                                            \
		shell_print(sh, "\tVendor:   %04x", chip_info.vendor_id);                          \
		shell_print(sh, "\tProduct:  %04x", chip_info.product_id);                         \
		shell_print(sh, "\tDevice:   %04x", chip_info.device_id);                          \
		shell_print(sh, "\tFirmware: %llx", chip_info.fw_version_number);                  \
	}

/** Macro used to call the get_chip function from the USB-C connector node */
#define TCPC_GET_CHIP_CONN_NODE(node) TCPC_GET_CHIP_DEV(DEVICE_DT_GET(DT_PROP(node, tcpc)))

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
		const struct device *dev = shell_device_get_binding(argv[1]);

		if (dev != NULL) {
			TCPC_DUMP_DEV(dev);
		} else {
			ret = -ENODEV;
		}
	}

	return ret;
}

/**
 * @brief Shell command that prints the vbus measures for all available USB-C ports
 *
 * @param sh Shell structure
 * @param argc Arguments count
 * @param argv Device name
 * @return int ORed return values of all the functions executed, 0 in case of success
 */
static int cmd_tcpc_vbus(const struct shell *sh, size_t argc, char **argv)
{
	int ret = 0;

	if (argc <= 1) {
		DT_FOREACH_STATUS_OKAY(usb_c_connector, TCPC_VBUS_CONN_NODE);
	} else {
		const struct device *dev = shell_device_get_binding(argv[1]);

		if (dev != NULL) {
			TCPC_VBUS_DEV(dev);
		} else {
			ret = -ENODEV;
		}
	}

	return ret;
}

/**
 * @brief Shell command that prints the TCPCs chips information for all available USB-C ports
 *
 * @param sh Shell structure
 * @param argc Arguments count
 * @param argv Device name
 * @return int ORed return values of all the functions executed, 0 in case of success
 */
static int cmd_tcpc_chip_info(const struct shell *sh, size_t argc, char **argv)
{
	struct tcpc_chip_info chip_info;
	int ret = 0;

	if (argc <= 1) {
		DT_FOREACH_STATUS_OKAY(usb_c_connector, TCPC_GET_CHIP_CONN_NODE);
	} else {
		const struct device *dev = shell_device_get_binding(argv[1]);

		if (dev != NULL) {
			TCPC_GET_CHIP_DEV(dev);
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
			       SHELL_CMD_ARG(vbus, &list_device_names,
					     "Display VBUS voltage\n"
					     "Usage: tcpc vbus [<vbus device>]",
					     cmd_tcpc_vbus, 1, 1),
			       SHELL_CMD_ARG(chip, &list_device_names,
					     "Display chip information\n"
					     "Usage: tcpc chip [<tcpc device>]",
					     cmd_tcpc_chip_info, 1, 1),
			       SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(tcpc, &sub_tcpc_cmds, "TCPC (USB-C PD) diagnostics", NULL);
