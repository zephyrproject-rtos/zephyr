/*
 * Copyright (c) 2021 IP-Logix Inc.
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/shell/shell.h>
#include <stdlib.h>
#include <zephyr/drivers/mdio.h>
#include <string.h>
#include <zephyr/sys/util.h>
#include <zephyr/devicetree.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mdio_shell, CONFIG_LOG_DEFAULT_LEVEL);

static bool device_is_mdio(const struct device *dev)
{
	return DEVICE_API_IS(mdio, dev);
}

static void device_name_get(size_t idx, struct shell_static_entry *entry)
{
	const struct device *dev = shell_device_filter(idx, device_is_mdio);

	entry->syntax = (dev != NULL) ? dev->name : NULL;
	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = NULL;
}

SHELL_DYNAMIC_CMD_CREATE(dsub_device_name, device_name_get);

static int parse_device_arg(const struct shell *sh, char **argv, const struct device **dev)
{
	*dev = shell_device_get_binding(argv[1]);
	if (!*dev) {
		shell_error(sh, "device %s not found", argv[1]);
		return -ENODEV;
	}
	return 0;
}

/*
 * Scan the entire 5-bit address space of the MDIO bus
 *
 * scan <device> [<reg_addr>]
 */
static int cmd_mdio_scan(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	int cnt;
	uint16_t data;
	uint16_t reg_addr;
	int ret;

	ret = parse_device_arg(sh, argv, &dev);
	if (ret < 0) {
		return ret;
	}

	if (argc >= 2) {
		reg_addr = strtol(argv[2], NULL, 16);
	} else {
		reg_addr = 0;
	}

	shell_print(sh,
		    "Scanning bus for devices. Reading register 0x%x",
		    reg_addr);
	cnt = 0;

	mdio_bus_enable(dev);

	for (int i = 0; i < 32; i++) {
		data = 0;
		if (mdio_read(dev, i, reg_addr, &data) >= 0 &&
			data != UINT16_MAX) {
			cnt++;
			shell_print(sh, "Found MDIO device @ 0x%x", i);
		}
	}

	mdio_bus_disable(dev);

	shell_print(sh, "%u devices found on %s", cnt, dev->name);

	return 0;
}

/* mdio write <device> <port_addr> <reg_addr> <data> */
static int cmd_mdio_write(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	uint16_t data;
	uint16_t reg_addr;
	uint16_t port_addr;
	int ret;

	ret = parse_device_arg(sh, argv, &dev);
	if (ret < 0) {
		return ret;
	}

	port_addr = strtol(argv[2], NULL, 16);
	reg_addr = strtol(argv[3], NULL, 16);
	data = strtol(argv[4], NULL, 16);

	mdio_bus_enable(dev);

	if (mdio_write(dev, port_addr, reg_addr, data) < 0) {
		shell_error(sh, "Failed to write to device: %s", dev->name);
		mdio_bus_disable(dev);

		return -EIO;
	}

	mdio_bus_disable(dev);

	return 0;
}

/* mdio read <device> <port_addr> <reg_addr> */
static int cmd_mdio_read(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	uint16_t data;
	uint16_t reg_addr;
	uint16_t port_addr;
	int ret;

	ret = parse_device_arg(sh, argv, &dev);
	if (ret < 0) {
		return ret;
	}

	port_addr = strtol(argv[2], NULL, 16);
	reg_addr = strtol(argv[3], NULL, 16);

	mdio_bus_enable(dev);

	if (mdio_read(dev, port_addr, reg_addr, &data) < 0) {
		shell_error(sh, "Failed to read from device: %s", dev->name);
		mdio_bus_disable(dev);

		return -EIO;
	}

	mdio_bus_disable(dev);

	shell_print(sh, "%x[%x]: 0x%x", port_addr, reg_addr, data);

	return 0;
}

/* mdio write_c45 <device> <port_addr> <dev_addr> <reg_addr> <value> */
static int cmd_mdio_write_45(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	uint16_t data;
	uint16_t reg_addr;
	uint8_t dev_addr;
	uint8_t port_addr;
	int ret;

	ret = parse_device_arg(sh, argv, &dev);
	if (ret < 0) {
		return ret;
	}

	port_addr = strtol(argv[2], NULL, 16);
	dev_addr = strtol(argv[3], NULL, 16);
	reg_addr = strtol(argv[4], NULL, 16);
	data = strtol(argv[5], NULL, 16);

	mdio_bus_enable(dev);

	if (mdio_write_c45(dev, port_addr, dev_addr, reg_addr, data) < 0) {
		shell_error(sh, "Failed to write to device: %s", dev->name);
		mdio_bus_disable(dev);

		return -EIO;
	}

	mdio_bus_disable(dev);

	return 0;
}

/* mdio read_c45 <device> <port_addr> <dev_addr> <reg_addr> */
static int cmd_mdio_read_c45(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	uint16_t data;
	uint16_t reg_addr;
	uint8_t dev_addr;
	uint8_t port_addr;
	int ret;

	ret = parse_device_arg(sh, argv, &dev);
	if (ret < 0) {
		return ret;
	}

	port_addr = strtol(argv[2], NULL, 16);
	dev_addr = strtol(argv[3], NULL, 16);
	reg_addr = strtol(argv[4], NULL, 16);

	mdio_bus_enable(dev);

	if (mdio_read_c45(dev, port_addr, dev_addr, reg_addr, &data) < 0) {
		shell_error(sh, "Failed to read from device: %s", dev->name);
		mdio_bus_disable(dev);

		return -EIO;
	}

	mdio_bus_disable(dev);

	shell_print(sh, "%x[%x:%x]: 0x%x", port_addr, dev_addr, reg_addr, data);

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_mdio_cmds,
	SHELL_CMD_ARG(scan, &dsub_device_name,
		"Scan MDIO bus for devices: scan <device> [<reg_addr>]",
		cmd_mdio_scan, 1, 1),
	SHELL_CMD_ARG(read, &dsub_device_name,
		"Read from MDIO device: read <device> <phy_addr> <reg_addr>",
		cmd_mdio_read, 4, 0),
	SHELL_CMD_ARG(write, &dsub_device_name,
		"Write to MDIO device: write <device> <phy_addr> <reg_addr> <value>",
		cmd_mdio_write, 5, 0),
	SHELL_CMD_ARG(read_c45, &dsub_device_name,
		"Read from MDIO Clause 45 device: "
		"read_c45 <device> <port_addr> <dev_addr> <reg_addr>",
		cmd_mdio_read_c45, 5, 0),
	SHELL_CMD_ARG(write_c45, &dsub_device_name,
		"Write to MDIO Clause 45 device: "
		"write_c45 <device> <port_addr> <dev_addr> <reg_addr> <value>",
		cmd_mdio_write_45, 6, 0),
	SHELL_SUBCMD_SET_END     /* Array terminated. */
);

SHELL_CMD_REGISTER(mdio, &sub_mdio_cmds, "MDIO commands", NULL);
