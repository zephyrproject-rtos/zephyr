/*
 * Copyright (c) 2026 Leica Geosystems AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/modem/modem_cellular.h>
#include <zephyr/shell/shell.h>

static bool device_is_modem_cellular(const struct device *dev)
{
	return dev->api == &modem_cellular_api;
}

static void device_name_get(size_t idx, struct shell_static_entry *entry)
{
	const struct device *dev = shell_device_filter(idx, device_is_modem_cellular);

	entry->syntax = (dev != NULL) ? dev->name : NULL;
	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = NULL;
}

SHELL_DYNAMIC_CMD_CREATE(dsub_device_name, device_name_get);

static const struct device *modem_cellular_shell_get_device(const struct shell *sh,
							    const char *name)
{
	const struct device *dev = shell_device_get_binding(name);

	if (dev == NULL) {
		shell_error(sh, "device %s not found", name);
		return NULL;
	}

	if (!device_is_modem_cellular(dev)) {
		shell_error(sh, "device %s is not a modem_cellular driver instance", name);
		return NULL;
	}

	return dev;
}

static int cmd_modem_cellular_pause(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	int ret;

	dev = modem_cellular_shell_get_device(sh, argv[1]);
	if (dev == NULL) {
		return -ENODEV;
	}

	ret = cellular_modem_pause_periodic_script(dev);
	if (ret < 0) {
		shell_error(sh, "cellular_modem_pause_periodic_script: %d", ret);
		return ret;
	}

	return 0;
}

static int cmd_modem_cellular_resume(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	int ret;

	dev = modem_cellular_shell_get_device(sh, argv[1]);
	if (dev == NULL) {
		return -ENODEV;
	}

	ret = cellular_modem_resume_periodic_script(dev);
	if (ret < 0) {
		shell_error(sh, "cellular_modem_resume_periodic_script: %d", ret);
		return ret;
	}

	return 0;
}

/**
 * @brief Show the SIM slots of a modem, or select its active SIM slot.
 *
 * Usage: @c sim @c <device> @c [slot]. Without a slot, prints the number of SIM slots
 * and the active one. With a slot, selects it; the modem must be suspended.
 *
 * @param sh Shell instance.
 * @param argc Number of arguments, 2 or 3.
 * @param argv Arguments: command name, device name and optional slot index.
 *
 * @retval 0 Success.
 * @retval -ENODEV Device not found, or not a modem_cellular instance.
 * @retval -EINVAL Slot index is not a number from 0 to 255.
 * @retval -errno Error from cellular_get_sim_slots() or cellular_select_sim().
 */
static int cmd_modem_cellular_sim(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	unsigned long slot;
	uint8_t count;
	uint8_t active;
	int err = 0;
	int ret;

	dev = modem_cellular_shell_get_device(sh, argv[1]);
	if (dev == NULL) {
		return -ENODEV;
	}

	if (argc < 3) {
		ret = cellular_get_sim_slots(dev, &count, &active);
		if (ret < 0) {
			shell_error(sh, "cellular_get_sim_slots: %d", ret);
			return ret;
		}

		shell_print(sh, "SIM slots: %u, active: %u", count, active);
		return 0;
	}

	slot = shell_strtoul(argv[2], 10, &err);
	if ((err != 0) || (slot > UINT8_MAX)) {
		shell_error(sh, "invalid SIM slot %s", argv[2]);
		return -EINVAL;
	}

	ret = cellular_select_sim(dev, (uint8_t)slot);
	if (ret == -EBUSY) {
		shell_error(sh,
			    "modem must be suspended to switch the SIM slot, "
			    "for example with \"pm suspend %s\"",
			    dev->name);
		return ret;
	}

	if (ret < 0) {
		shell_error(sh, "cellular_select_sim: %d", ret);
		return ret;
	}

	shell_print(sh, "SIM slot %lu selected", slot);
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(modem_cellular_cmds,
			       SHELL_CMD_ARG(pause, &dsub_device_name,
					     SHELL_HELP("Pause periodic chat script", "<device>"),
					     cmd_modem_cellular_pause, 2, 0),
			       SHELL_CMD_ARG(resume, &dsub_device_name,
					     SHELL_HELP("Resume periodic chat script", "<device>"),
					     cmd_modem_cellular_resume, 2, 0),
			       SHELL_CMD_ARG(sim, &dsub_device_name,
					     SHELL_HELP("Show or select the active SIM slot",
							"<device> [slot]"),
					     cmd_modem_cellular_sim, 2, 1),
			       SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(modem_cellular, &modem_cellular_cmds,
		   "modem_cellular driver shell commands", NULL);
