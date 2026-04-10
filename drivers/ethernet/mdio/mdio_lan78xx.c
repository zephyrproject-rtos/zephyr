/*
 * Copyright (c) 2026 Narek Aydinyan
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/ethernet/eth_lan78xx.h>
#include <zephyr/drivers/mdio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(mdio_lan78xx, CONFIG_MDIO_LOG_LEVEL);

#define DT_DRV_COMPAT microchip_lan78xx_mdio

struct mdio_lan78xx_config {
	const struct device *dev;
};

static int lan78xx_mdio_c22_read(const struct device *dev, uint8_t prtad, uint8_t regad,
				 uint16_t *data)
{
	const struct mdio_lan78xx_config *const cfg = dev->config;

	return eth_lan78xx_mdio_c22_read(cfg->dev, prtad, regad, data);
}

static int lan78xx_mdio_c22_write(const struct device *dev, uint8_t prtad, uint8_t regad,
				  uint16_t data)
{
	const struct mdio_lan78xx_config *const cfg = dev->config;

	return eth_lan78xx_mdio_c22_write(cfg->dev, prtad, regad, data);
}

static DEVICE_API(mdio, mdio_lan78xx_api) = {
	.read = lan78xx_mdio_c22_read,
	.write = lan78xx_mdio_c22_write,
};

#define MICROCHIP_LAN78XX_MDIO_INIT(n)                                                             \
	static const struct mdio_lan78xx_config mdio_lan78xx_config_##n = {                        \
		.dev = DEVICE_DT_GET(DT_INST_PARENT(n)),                                           \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, NULL, NULL, NULL, &mdio_lan78xx_config_##n, POST_KERNEL,          \
			      CONFIG_MDIO_INIT_PRIORITY, &mdio_lan78xx_api);

DT_INST_FOREACH_STATUS_OKAY(MICROCHIP_LAN78XX_MDIO_INIT)
