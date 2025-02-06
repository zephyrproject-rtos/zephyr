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

#if DT_HAS_COMPAT_STATUS_OKAY(atmel_sam_mdio)
#define DT_DRV_COMPAT atmel_sam_mdio
#elif DT_HAS_COMPAT_STATUS_OKAY(espressif_esp32_mdio)
#define DT_DRV_COMPAT espressif_esp32_mdio
#elif DT_HAS_COMPAT_STATUS_OKAY(nxp_imx_netc_emdio)
#define DT_DRV_COMPAT nxp_imx_netc_emdio
#elif DT_HAS_COMPAT_STATUS_OKAY(nxp_s32_netc_emdio)
#define DT_DRV_COMPAT nxp_s32_netc_emdio
#elif DT_HAS_COMPAT_STATUS_OKAY(nxp_s32_gmac_mdio)
#define DT_DRV_COMPAT nxp_s32_gmac_mdio
#elif DT_HAS_COMPAT_STATUS_OKAY(adi_adin2111_mdio)
#define DT_DRV_COMPAT adi_adin2111_mdio
#elif DT_HAS_COMPAT_STATUS_OKAY(smsc_lan91c111_mdio)
#define DT_DRV_COMPAT smsc_lan91c111_mdio
#elif DT_HAS_COMPAT_STATUS_OKAY(zephyr_mdio_gpio)
#define DT_DRV_COMPAT zephyr_mdio_gpio
#elif DT_HAS_COMPAT_STATUS_OKAY(nxp_enet_mdio)
#define DT_DRV_COMPAT nxp_enet_mdio
#elif DT_HAS_COMPAT_STATUS_OKAY(infineon_xmc4xxx_mdio)
#define DT_DRV_COMPAT infineon_xmc4xxx_mdio
#elif DT_HAS_COMPAT_STATUS_OKAY(nxp_enet_qos_mdio)
#define DT_DRV_COMPAT nxp_enet_qos_mdio
#elif DT_HAS_COMPAT_STATUS_OKAY(litex_liteeth_mdio)
#define DT_DRV_COMPAT litex_liteeth_mdio
#elif DT_HAS_COMPAT_STATUS_OKAY(st_stm32_mdio)
#define DT_DRV_COMPAT st_stm32_mdio
#elif DT_HAS_COMPAT_STATUS_OKAY(snps_dwcxgmac_mdio)
#define DT_DRV_COMPAT snps_dwcxgmac_mdio
#elif DT_HAS_COMPAT_STATUS_OKAY(sensry_sy1xx_mdio)
#define DT_DRV_COMPAT sensry_sy1xx_mdio
#else
#error "No known devicetree compatible match for MDIO shell"
#endif

#define MDIO_NODE_ID DT_COMPAT_GET_ANY_STATUS_OKAY(DT_DRV_COMPAT)

/*
 * Scan the entire 5-bit address space of the MDIO bus
 *
 * scan [<reg_addr>]
 */
static int cmd_mdio_scan(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	int cnt;
	uint16_t data;
	uint16_t reg_addr;

	dev = DEVICE_DT_GET(MDIO_NODE_ID);
	if (!device_is_ready(dev)) {
		shell_error(sh, "MDIO: Device driver %s is not ready.", dev->name);

		return -ENODEV;
	}

	if (argc >= 2) {
		reg_addr = strtol(argv[1], NULL, 16);
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

/* mdio write <port_addr> <reg_addr> <data> */
static int cmd_mdio_write(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	uint16_t data;
	uint16_t reg_addr;
	uint16_t port_addr;

	dev = DEVICE_DT_GET(MDIO_NODE_ID);
	if (!device_is_ready(dev)) {
		shell_error(sh, "MDIO: Device driver %s is not ready.", dev->name);

		return -ENODEV;
	}

	port_addr = strtol(argv[1], NULL, 16);
	reg_addr = strtol(argv[2], NULL, 16);
	data = strtol(argv[3], NULL, 16);

	mdio_bus_enable(dev);

	if (mdio_write(dev, port_addr, reg_addr, data) < 0) {
		shell_error(sh, "Failed to write to device: %s", dev->name);
		mdio_bus_disable(dev);

		return -EIO;
	}

	mdio_bus_disable(dev);

	return 0;
}

/* mdio read <port_addr> <reg_addr> */
static int cmd_mdio_read(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	uint16_t data;
	uint16_t reg_addr;
	uint16_t port_addr;

	dev = DEVICE_DT_GET(MDIO_NODE_ID);
	if (!device_is_ready(dev)) {
		shell_error(sh, "MDIO: Device driver %s is not ready.", dev->name);

		return -ENODEV;
	}

	port_addr = strtol(argv[1], NULL, 16);
	reg_addr = strtol(argv[2], NULL, 16);

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

/* mdio write_c45 <port_addr> <dev_addr> <reg_addr> <value> */
static int cmd_mdio_write_45(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	uint16_t data;
	uint16_t reg_addr;
	uint8_t dev_addr;
	uint8_t port_addr;

	dev = DEVICE_DT_GET(MDIO_NODE_ID);
	if (!device_is_ready(dev)) {
		shell_error(sh, "MDIO: Device driver %s is not ready.", dev->name);

		return -ENODEV;
	}

	port_addr = strtol(argv[1], NULL, 16);
	dev_addr = strtol(argv[2], NULL, 16);
	reg_addr = strtol(argv[3], NULL, 16);
	data = strtol(argv[4], NULL, 16);

	mdio_bus_enable(dev);

	if (mdio_write_c45(dev, port_addr, dev_addr, reg_addr, data) < 0) {
		shell_error(sh, "Failed to write to device: %s", dev->name);
		mdio_bus_disable(dev);

		return -EIO;
	}

	mdio_bus_disable(dev);

	return 0;
}

/* mdio read_c45 <port_addr> <dev_addr> <reg_addr> */
static int cmd_mdio_read_c45(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	uint16_t data;
	uint16_t reg_addr;
	uint8_t dev_addr;
	uint8_t port_addr;

	dev = DEVICE_DT_GET(MDIO_NODE_ID);
	if (!device_is_ready(dev)) {
		shell_error(sh, "MDIO: Device driver %s is not ready.", dev->name);

		return -ENODEV;
	}

	port_addr = strtol(argv[1], NULL, 16);
	dev_addr = strtol(argv[2], NULL, 16);
	reg_addr = strtol(argv[3], NULL, 16);

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
	SHELL_CMD_ARG(scan, NULL,
		"Scan MDIO bus for devices: scan [<reg_addr>]",
		cmd_mdio_scan, 0, 1),
	SHELL_CMD_ARG(read, NULL,
		"Read from MDIO device: read <phy_addr> <reg_addr>",
		cmd_mdio_read, 3, 0),
	SHELL_CMD_ARG(write, NULL,
		"Write to MDIO device: write <phy_addr> <reg_addr> <value>",
		cmd_mdio_write, 4, 0),
	SHELL_CMD_ARG(read_c45, NULL,
		"Read from MDIO Clause 45 device: "
		"read_c45 <port_addr> <dev_addr> <reg_addr>",
		cmd_mdio_read_c45, 4, 0),
	SHELL_CMD_ARG(write_c45, NULL,
		"Write to MDIO Clause 45 device: "
		"write_c45 <port_addr> <dev_addr> <reg_addr> <value>",
		cmd_mdio_write_45, 5, 0),
	SHELL_SUBCMD_SET_END     /* Array terminated. */
);

SHELL_CMD_REGISTER(mdio, &sub_mdio_cmds, "MDIO commands", NULL);
