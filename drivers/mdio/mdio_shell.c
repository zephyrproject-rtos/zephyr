/*
 * Copyright (c) 2021 IP-Logix Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/shell/shell.h>
#include <stdlib.h>
#include <zephyr/drivers/mdio.h>
#include <string.h>
#include <zephyr/sys/util.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mdio_shell, CONFIG_LOG_DEFAULT_LEVEL);

#define MDIO_DEVICE "MDIO"

/*
 * Scan the entire 5-bit address space of the MDIO bus
 *
 * scan [<dev_addr>]
 */
static int cmd_mdio_scan(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	int cnt;
	uint16_t data;
	uint16_t dev_addr;

	dev = device_get_binding(MDIO_DEVICE);
	if (!dev) {
		shell_error(sh, "MDIO: Device driver %s not found.",
			    MDIO_DEVICE);

		return -ENODEV;
	}

	if (argc >= 2) {
		dev_addr = strtol(argv[1], NULL, 16);
	} else {
		dev_addr = 0;
	}

	shell_print(sh,
		    "Scanning bus for devices. Reading register 0x%x",
		    dev_addr);
	cnt = 0;

	mdio_bus_enable(dev);

	for (int i = 0; i < 32; i++) {
		data = 0;
		if (mdio_read(dev, i, dev_addr, &data) >= 0 &&
			data != UINT16_MAX) {
			cnt++;
			shell_print(sh, "Found MDIO device @ 0x%x", i);
		}
	}

	mdio_bus_disable(dev);

	shell_print(sh, "%u devices found on %s", cnt, MDIO_DEVICE);

	return 0;
}

/* mdio write <port_addr> <dev_addr> <data> */
static int cmd_mdio_write(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	uint16_t data;
	uint16_t dev_addr;
	uint16_t port_addr;

	dev = device_get_binding(MDIO_DEVICE);
	if (!dev) {
		shell_error(sh, "MDIO: Device driver %s not found.",
			    MDIO_DEVICE);

		return -ENODEV;
	}

	port_addr = strtol(argv[1], NULL, 16);
	dev_addr = strtol(argv[2], NULL, 16);
	data = strtol(argv[3], NULL, 16);

	mdio_bus_enable(dev);

	if (mdio_write(dev, port_addr, dev_addr, data) < 0) {
		shell_error(sh, "Failed to write to device: %s", MDIO_DEVICE);
		mdio_bus_disable(dev);

		return -EIO;
	}

	mdio_bus_disable(dev);

	return 0;
}

/* mdio read <port_addr> <dev_addr> */
static int cmd_mdio_read(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	uint16_t data;
	uint16_t dev_addr;
	uint16_t port_addr;

	dev = device_get_binding(MDIO_DEVICE);
	if (!dev) {
		shell_error(sh, "MDIO: Device driver %s not found.",
			    MDIO_DEVICE);

		return -ENODEV;
	}

	port_addr = strtol(argv[1], NULL, 16);
	dev_addr = strtol(argv[2], NULL, 16);

	mdio_bus_enable(dev);

	if (mdio_read(dev, port_addr, dev_addr, &data) < 0) {
		shell_error(sh, "Failed to read from device: %s", MDIO_DEVICE);
		mdio_bus_disable(dev);

		return -EIO;
	}

	mdio_bus_disable(dev);

	shell_print(sh, "%x[%x]: 0x%x", port_addr, dev_addr, data);

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_mdio_cmds,
	SHELL_CMD_ARG(scan, NULL,
		"Scan MDIO bus for devices: scan [<dev_addr>]",
		cmd_mdio_scan, 0, 1),
	SHELL_CMD_ARG(read, NULL,
		"Read from MDIO device: read <phy_addr> <dev_addr>",
		cmd_mdio_read, 3, 0),
	SHELL_CMD_ARG(write, NULL,
		"Write to MDIO device: write <phy_addr> <dev_addr> <value>",
		cmd_mdio_write, 4, 0),
	SHELL_SUBCMD_SET_END     /* Array terminated. */
);

SHELL_CMD_REGISTER(mdio, &sub_mdio_cmds, "MDIO commands", NULL);
