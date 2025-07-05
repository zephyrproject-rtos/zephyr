/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mdio_i2c, CONFIG_MDIO_LOG_LEVEL);

#define DT_DRV_COMPAT zephyr_mdio_i2c

#include <stdint.h>
#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/mdio.h>
#include <zephyr/drivers/i2c.h>

struct mdio_i2c_config {
	const struct device *bus;
};

static bool mdio_i2c_valid_phy_id(uint8_t prtad)
{
	/* on SFP modules 0x50 and 0x51 are normally an EEPROM. */
	return prtad != 0x10 && prtad != 0x11;
}

static inline uint16_t mdio_i2c_phy_addr(uint8_t prtad)
{
	return (uint16_t)prtad + 0x40;
}

static int mdio_i2c_read_c22(const struct device *dev, uint8_t prtad, uint8_t regad, uint16_t *data)
{
	const struct mdio_i2c_config *cfg = dev->config;
	uint16_t dev_addr;

	if (!mdio_i2c_valid_phy_id(prtad)) {
		return -EINVAL;
	}

	dev_addr = mdio_i2c_phy_addr(prtad);

	return i2c_write_read(cfg->bus, dev_addr, &regad, sizeof(regad), data, sizeof(*data));
}

static int mdio_i2c_write_c22(const struct device *dev, uint8_t prtad, uint8_t regad, uint16_t data)
{
	const struct mdio_i2c_config *cfg = dev->config;
	uint16_t dev_addr;
	uint8_t tx_buf[3];

	if (!mdio_i2c_valid_phy_id(prtad)) {
		return -EINVAL;
	}

	dev_addr = mdio_i2c_phy_addr(prtad);

	tx_buf[0] = regad;
	tx_buf[1] = data >> 8;
	tx_buf[2] = data & 0xFF;

	return i2c_write(cfg->bus, tx_buf, 3, dev_addr);
}

static int mdio_i2c_initialize(const struct device *dev)
{
	const struct mdio_i2c_config *cfg = dev->config;

	if (!device_is_ready(cfg->bus)) {
		LOG_ERR("%s is not ready", cfg->bus->name);
		return -ENODEV;
	}

	return 0;
}

static DEVICE_API(mdio, mdio_i2c_driver_api) = {
	.read = mdio_i2c_read_c22,
	.write = mdio_i2c_write_c22,
};

#define MDIO_I2C_DEVICE(inst)                                                                      \
	static struct mdio_i2c_config mdio_i2c_dev_config_##inst = {                               \
		.bus = DEVICE_DT_GET(DT_INST_BUS(inst)),                                           \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, &mdio_i2c_initialize, NULL, NULL, &mdio_i2c_dev_config_##inst, \
			      POST_KERNEL, CONFIG_MDIO_INIT_PRIORITY, &mdio_i2c_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MDIO_I2C_DEVICE)
