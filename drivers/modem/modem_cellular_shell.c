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

#if defined(CONFIG_MODEM_CELLULAR_STATS)
static int cmd_modem_cellular_stats(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	const struct cellular_stats *stats;

	ARG_UNUSED(argc);

	dev = modem_cellular_shell_get_device(sh, argv[1]);
	if (dev == NULL) {
		return -ENODEV;
	}

	stats = cellular_get_stats(dev);
	if (stats == NULL) {
		shell_error(sh, "cellular_get_stats: not supported");
		return -ENOSYS;
	}

	shell_print(sh, "outage_ms:             %u", stats->outage_ms);
	shell_print(sh, "deregistrations:       %u", stats->deregistrations);
	shell_print(sh, "link_drops:            %u", stats->link_drops);
	shell_print(sh, "command_failures:      %u", stats->command_failures);
	shell_print(sh, "command_errors_cme:    %u", stats->command_errors_cme);
	shell_print(sh, "recoveries:            %u", stats->recoveries);
	shell_print(sh, "cmux_disconnects:      %u", stats->cmux_disconnects);
	shell_print(sh, "uart_rx_dropped_bytes: %u", stats->uart_rx_dropped_bytes);
	shell_print(sh, "cmux_frames_dropped:   %u", stats->cmux_frames_dropped);
	shell_print(sh, "cmux_rx_dropped_bytes: %u", stats->cmux_rx_dropped_bytes);

	return 0;
}
#endif

SHELL_STATIC_SUBCMD_SET_CREATE(modem_cellular_cmds,
			       SHELL_CMD_ARG(pause, &dsub_device_name,
					     SHELL_HELP("Pause periodic chat script", "<device>"),
					     cmd_modem_cellular_pause, 2, 0),
			       SHELL_CMD_ARG(resume, &dsub_device_name,
					     SHELL_HELP("Resume periodic chat script", "<device>"),
					     cmd_modem_cellular_resume, 2, 0),
#if defined(CONFIG_MODEM_CELLULAR_STATS)
			       SHELL_CMD_ARG(stats, &dsub_device_name,
					     SHELL_HELP("Print operational statistics", "<device>"),
					     cmd_modem_cellular_stats, 2, 0),
#endif
			       SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(modem_cellular, &modem_cellular_cmds,
		   "modem_cellular driver shell commands", NULL);
