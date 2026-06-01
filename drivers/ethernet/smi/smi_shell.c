/*
 * Copyright (c) 2021 IP-Logix Inc.
 * Copyright 2023 NXP
 * Copyright 2026 Bas van Loon
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/shell/shell.h>
#include <stdlib.h>
#include <zephyr/drivers/smi.h>
#include <string.h>
#include <zephyr/sys/util.h>
#include <zephyr/devicetree.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(smi_shell, CONFIG_LOG_DEFAULT_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(microchip_smi_gpio)
#define DT_DRV_COMPAT microchip_smi_gpio
#else
#error "No known devicetree compatible match for SMI shell"
#endif

#define SMI_NODE_ID DT_COMPAT_GET_ANY_STATUS_OKAY(DT_DRV_COMPAT)

/* smi write <port_addr> <reg_addr> <data> */
static int cmd_smi_write(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	uint16_t data;
	uint16_t reg_addr;
	uint16_t port_addr;

	dev = DEVICE_DT_GET(SMI_NODE_ID);
	if (!device_is_ready(dev)) {
		shell_error(sh, "SMI: Device driver %s is not ready.", dev->name);

		return -ENODEV;
	}

	port_addr = strtol(argv[1], NULL, 16);
	reg_addr = strtol(argv[2], NULL, 16);
	data = strtol(argv[3], NULL, 16);

	smi_bus_enable(dev);

	if (smi_write(dev, port_addr, reg_addr, data) < 0) {
		shell_error(sh, "Failed to write to device: %s", dev->name);
		smi_bus_disable(dev);

		return -EIO;
	}

	smi_bus_disable(dev);

	return 0;
}

/* smi read <port_addr> <reg_addr> */
static int cmd_smi_read(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	uint16_t data;
	uint16_t reg_addr;
	uint16_t port_addr;

	dev = DEVICE_DT_GET(SMI_NODE_ID);
	if (!device_is_ready(dev)) {
		shell_error(sh, "SMI: Device driver %s is not ready.", dev->name);

		return -ENODEV;
	}

	port_addr = strtol(argv[1], NULL, 16);
	reg_addr = strtol(argv[2], NULL, 16);

	smi_bus_enable(dev);

	if (smi_read(dev, port_addr, reg_addr, &data) < 0) {
		shell_error(sh, "Failed to read from device: %s", dev->name);
		smi_bus_disable(dev);

		return -EIO;
	}

	smi_bus_disable(dev);

	shell_print(sh, "%x[%x]: 0x%x", port_addr, reg_addr, data);

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_smi_cmds,
	SHELL_CMD_ARG(read, NULL,
		"Read from SMI device: read <phy_addr> <reg_addr>",
		cmd_smi_read, 3, 0),
	SHELL_CMD_ARG(write, NULL,
		"Write to SMI device: write <phy_addr> <reg_addr> <value>",
		cmd_smi_write, 4, 0),
	SHELL_SUBCMD_SET_END     /* Array terminated. */
);

SHELL_CMD_REGISTER(smi, &sub_smi_cmds, "SMI commands", NULL);
