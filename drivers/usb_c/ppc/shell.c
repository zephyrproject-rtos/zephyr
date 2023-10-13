/*
 * Copyright 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/shell/shell.h>
#include <zephyr/drivers/usb_c/usbc_ppc.h>

/** Macro used to iterate over USB-C connector and call a function if the node has PPC property */
#define CALL_IF_HAS_PPC(usb_node, func)                                                            \
	COND_CODE_1(DT_NODE_HAS_PROP(usb_node, ppc),                                               \
		    (ret |= func(DEVICE_DT_GET(DT_PHANDLE_BY_IDX(usb_node, ppc, 0)));), ())

/**
 * @brief Command that dumps registers of one or all of the PPCs
 *
 * @param sh Shell structure
 * @param argc Arguments count
 * @param argv Device name
 * @return int ORed return values of all the functions executed, 0 in case of success
 */
static int cmd_ppc_dump(const struct shell *sh, size_t argc, char **argv)
{
	int ret = 0;

	if (argc <= 1) {
		DT_FOREACH_STATUS_OKAY_VARGS(usb_c_connector, CALL_IF_HAS_PPC, ppc_dump_regs);
	} else {
		const struct device *dev = device_get_binding(argv[1]);

		ret = ppc_dump_regs(dev);
	}

	return ret;
}

/**
 * @brief Function used to pretty print status of the PPC
 *
 * @param dev Pointer to the PPC device structure
 */
static int print_status(const struct device *dev)
{
	printk("PPC %s:\n", dev->name);
	printk("  Dead battery:    %d\n", ppc_is_dead_battery_mode(dev));
	printk("  Is sourcing:     %d\n", ppc_is_vbus_source(dev));
	printk("  Is sinking:      %d\n", ppc_is_vbus_sink(dev));
	printk("  Is VBUS present: %d\n", ppc_is_vbus_present(dev));

	return 0;
}

/**
 * @brief Command that prints the status of one or all of the PPCs
 *
 * @param sh Shell structure
 * @param argc Arguments count
 * @param argv Device name
 * @return int ORed return values of all the functions executed, 0 in case of success
 */
static int cmd_ppc_status(const struct shell *sh, size_t argc, char **argv)
{
	int ret = 0;

	if (argc <= 1) {
		DT_FOREACH_STATUS_OKAY_VARGS(usb_c_connector, CALL_IF_HAS_PPC, print_status);
	} else {
		const struct device *dev = device_get_binding(argv[1]);

		ret = print_status(dev);
	}

	return ret;
}

/**
 * @brief Command that requests one or all of the PPCs to try exiting the dead battery mode
 *
 * @param sh Shell structure
 * @param argc Arguments count
 * @param argv Device name
 * @return int ORed return values of all the functions executed, 0 in case of success
 */
static int cmd_ppc_exit_db(const struct shell *sh, size_t argc, char **argv)
{
	int ret = 0;

	if (argc <= 1) {
		DT_FOREACH_STATUS_OKAY_VARGS(usb_c_connector, CALL_IF_HAS_PPC,
					     ppc_exit_dead_battery_mode);
	} else {
		const struct device *dev = device_get_binding(argv[1]);

		ret = ppc_exit_dead_battery_mode(dev);
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

SHELL_STATIC_SUBCMD_SET_CREATE(sub_ppc_cmds,
			       SHELL_CMD_ARG(dump, &list_device_names,
					     "Dump PPC registers\n"
					     "Usage: ppc dump [<ppc device>]",
					     cmd_ppc_dump, 1, 1),
			       SHELL_CMD_ARG(status, &list_device_names,
					     "Write PPC power status\n"
					     "Usage: ppc statuc [<ppc device>]",
					     cmd_ppc_status, 1, 1),
			       SHELL_CMD_ARG(exitdb, &list_device_names,
					     "Exit from the dead battery mode\n"
					     "Usage: ppc exitdb [<ppc device>]",
					     cmd_ppc_exit_db, 1, 1),
			       SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(ppc, &sub_ppc_cmds, "PPC (USB-C PD) diagnostics", NULL);
