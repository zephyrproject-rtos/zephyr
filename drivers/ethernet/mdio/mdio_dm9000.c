/*
 * Copyright (c) 2026 Fuyu Fei
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT davicom_dm9000_mdio

#include <errno.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/drivers/mdio.h>
#include <zephyr/logging/log.h>

#include "../eth_dm9000_priv.h"

LOG_MODULE_REGISTER(mdio_dm9000, CONFIG_MDIO_LOG_LEVEL);

static int mdio_dm9000_read(const struct device *dev, uint8_t prtad, uint8_t regad,
			    uint16_t *data)
{
	const struct device *mac = dev->config;

	return dm9000_mdio_c22_read(mac, prtad, regad, data);
}

static int mdio_dm9000_write(const struct device *dev, uint8_t prtad, uint8_t regad,
			     uint16_t data)
{
	const struct device *mac = dev->config;

	return dm9000_mdio_c22_write(mac, prtad, regad, data);
}

static int mdio_dm9000_init(const struct device *dev)
{
	const struct device *mac = dev->config;

	if (!device_is_ready(mac)) {
		LOG_ERR_DEVICE_NOT_READY(mac);
		return -ENODEV;
	}

	return 0;
}

static DEVICE_API(mdio, mdio_dm9000_api) = {
	.read = mdio_dm9000_read,
	.write = mdio_dm9000_write,
};

#define MDIO_DM9000_INIT(n)									\
	DEVICE_DT_INST_DEFINE(n, mdio_dm9000_init, NULL, NULL,					\
			      DEVICE_DT_GET(DT_INST_PARENT(n)), POST_KERNEL,			\
			      CONFIG_MDIO_INIT_PRIORITY, &mdio_dm9000_api);

DT_INST_FOREACH_STATUS_OKAY(MDIO_DM9000_INIT)
