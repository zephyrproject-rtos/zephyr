/*
 * Copyright (c) 2024 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mdio_lan865x, CONFIG_MDIO_LOG_LEVEL);

#define DT_DRV_COMPAT microchip_lan865x_mdio

#include <stdint.h>
#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/mdio.h>
#include <zephyr/drivers/ethernet/eth_lan865x.h>

struct mdio_lan865x_config {
	const struct device *dev;
};

static void lan865x_mdio_bus_enable(const struct device *dev)
{
	ARG_UNUSED(dev);
}

static void lan865x_mdio_bus_disable(const struct device *dev)
{
	ARG_UNUSED(dev);
}

static int lan865x_mdio_c22_read(const struct device *dev, uint8_t prtad, uint8_t regad,
				 uint16_t *data)
{
	const struct mdio_lan865x_config *const cfg = dev->config;

	return eth_lan865x_mdio_c22_read(cfg->dev, prtad, regad, data);
}

static int lan865x_mdio_c22_write(const struct device *dev, uint8_t prtad, uint8_t regad,
				  uint16_t data)
{
	const struct mdio_lan865x_config *const cfg = dev->config;

	return eth_lan865x_mdio_c22_write(cfg->dev, prtad, regad, data);
}

static int lan865x_mdio_c45_read(const struct device *dev, uint8_t prtad, uint8_t devad,
				 uint16_t regad, uint16_t *data)
{
	const struct mdio_lan865x_config *const cfg = dev->config;

	return eth_lan865x_mdio_c45_read(cfg->dev, prtad, devad, regad, data);
}

static int lan865x_mdio_c45_write(const struct device *dev, uint8_t prtad, uint8_t devad,
				  uint16_t regad, uint16_t data)
{
	const struct mdio_lan865x_config *const cfg = dev->config;

	return eth_lan865x_mdio_c45_write(cfg->dev, prtad, devad, regad, data);
}

static DEVICE_API(mdio, mdio_lan865x_api) = {
	.read = lan865x_mdio_c22_read,
	.write = lan865x_mdio_c22_write,
	.read_c45 = lan865x_mdio_c45_read,
	.write_c45 = lan865x_mdio_c45_write,
	.bus_enable = lan865x_mdio_bus_enable,
	.bus_disable = lan865x_mdio_bus_disable,
};

#define MICROCHIP_LAN865X_MDIO_INIT(n)                                                             \
	static const struct mdio_lan865x_config mdio_lan865x_config_##n = {                        \
		.dev = DEVICE_DT_GET(DT_INST_PARENT(n)),                                           \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, NULL, NULL, NULL, &mdio_lan865x_config_##n, POST_KERNEL,          \
			      CONFIG_MDIO_LAN865X_INIT_PRIORITY, &mdio_lan865x_api);

DT_INST_FOREACH_STATUS_OKAY(MICROCHIP_LAN865X_MDIO_INIT)
