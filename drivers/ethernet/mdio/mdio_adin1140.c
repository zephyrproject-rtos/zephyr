/*
 * Copyright (c) 2024-2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief MDIO bus driver for the ADIN1140 MAC-PHY.
 *
 * Thin wrapper that delegates MDIO Clause 22 and Clause 45
 * register access to the parent ethernet driver's OA TC6
 * register interface.
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mdio_adin1140, CONFIG_MDIO_LOG_LEVEL);

#define DT_DRV_COMPAT adi_adin1140_mdio

#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/mdio.h>
#include <zephyr/drivers/ethernet/eth_adin1140.h>

/**
 * Read a PHY register using MDIO Clause 45.
 */
static int mdio_adin1140_read_c45(const struct device *dev, uint8_t prtad, uint8_t devad,
				  uint16_t regad, uint16_t *data)
{
	const struct device *adin = dev->config;

	return eth_adin1140_mdio_c45_read(adin, prtad, devad, regad, data);
}

/**
 * Write a PHY register using MDIO Clause 45.
 */
static int mdio_adin1140_write_c45(const struct device *dev, uint8_t prtad, uint8_t devad,
				   uint16_t regad, uint16_t data)
{
	const struct device *adin = dev->config;

	return eth_adin1140_mdio_c45_write(adin, prtad, devad, regad, data);
}

/**
 * Read a PHY register using MDIO Clause 22.
 */
static int mdio_adin1140_read(const struct device *dev, uint8_t prtad, uint8_t regad,
			      uint16_t *data)
{
	const struct device *adin = dev->config;

	return eth_adin1140_mdio_c22_read(adin, prtad, regad, data);
}

/**
 * Write a PHY register using MDIO Clause 22.
 */
static int mdio_adin1140_write(const struct device *dev, uint8_t prtad, uint8_t regad,
			       uint16_t data)
{
	const struct device *adin = dev->config;

	return eth_adin1140_mdio_c22_write(adin, prtad, regad, data);
}

static DEVICE_API(mdio, mdio_adin1140_api) = {
	.read = mdio_adin1140_read,
	.write = mdio_adin1140_write,
	.read_c45 = mdio_adin1140_read_c45,
	.write_c45 = mdio_adin1140_write_c45,
};

#define ADIN1140_MDIO_INIT(n)                                                                      \
	DEVICE_DT_INST_DEFINE(n, NULL, NULL, NULL, DEVICE_DT_GET(DT_INST_PARENT(n)), POST_KERNEL,  \
			      CONFIG_MDIO_INIT_PRIORITY, &mdio_adin1140_api);

DT_INST_FOREACH_STATUS_OKAY(ADIN1140_MDIO_INIT)
