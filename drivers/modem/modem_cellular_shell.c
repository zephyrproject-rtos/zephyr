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

/* Rows the shell reports from one measurement. */
#define MODEM_CELLULAR_SHELL_NEIGHBOR_CELLS 16

static int cmd_modem_cellular_neighbor_scan(const struct shell *sh, size_t argc, char **argv)
{
	struct cellular_neighbor_cell cells[MODEM_CELLULAR_SHELL_NEIGHBOR_CELLS];
	const struct device *dev;
	uint8_t count = ARRAY_SIZE(cells);
	int ret;
	int i;

	dev = modem_cellular_shell_get_device(sh, argv[1]);
	if (dev == NULL) {
		return -ENODEV;
	}

	ret = cellular_scan_neighbor_cells(dev, cells, &count);
	if (ret < 0 && ret != -ENOMEM) {
		shell_error(sh, "cellular_scan_neighbor_cells: %d", ret);
		return ret;
	}

	if (ret == -ENOMEM) {
		shell_warn(sh, "buffer full, reporting %u of the neighbours measured", count);
	}

	for (i = 0; i < count; i++) {
		shell_print(sh, "act %u earfcn %u pci %u rsrp %d rsrq %d", cells[i].access_tech,
			    cells[i].cell.lte.earfcn, cells[i].cell.lte.phys_cell_id,
			    cells[i].cell.lte.rsrp, cells[i].cell.lte.rsrq);
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(modem_cellular_cmds,
			       SHELL_CMD_ARG(pause, &dsub_device_name,
					     SHELL_HELP("Pause periodic chat script", "<device>"),
					     cmd_modem_cellular_pause, 2, 0),
			       SHELL_CMD_ARG(resume, &dsub_device_name,
					     SHELL_HELP("Resume periodic chat script", "<device>"),
					     cmd_modem_cellular_resume, 2, 0),
			       SHELL_CMD_ARG(neighbor_scan, &dsub_device_name,
					     SHELL_HELP("Measure neighbour cells", "<device>"),
					     cmd_modem_cellular_neighbor_scan, 2, 0),
			       SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(modem_cellular, &modem_cellular_cmds,
		   "modem_cellular driver shell commands", NULL);
