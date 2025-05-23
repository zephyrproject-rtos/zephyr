/* Bosch BMA4xx 3-axis accelerometer driver
 *
 * Copyright (c) 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT bosch_bma4xx

#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>

#include "bma4xx.h"

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)

LOG_MODULE_DECLARE(bma4xx, CONFIG_SENSOR_LOG_LEVEL);

static int bma4xx_i2c_read_data(const struct device *dev, uint8_t reg_addr,
				 uint8_t *value, uint8_t len)
{
	const struct bma4xx_config *cfg = dev->config;

	return i2c_burst_read_dt(&cfg->bus_cfg.i2c, reg_addr, value,
				 len);
}

static int bma4xx_i2c_write_data(const struct device *dev, uint8_t reg_addr,
				  uint8_t *value, uint8_t len)
{
	const struct bma4xx_config *cfg = dev->config;
	const struct bma4xx_data *bma4xx = dev->data;
	int res = 0;

	res = i2c_burst_write_dt(&cfg->bus_cfg.i2c, reg_addr, value, len);
	if (res) {
		LOG_ERR("Could not perform i2c write data");
		return -ENOTSUP;
	}

	/* A 1.3us delay is required after write operation when device operates in
	 * power performance mode whereas 1000us is required when the device operates
	 * in low power mode.
	 */
	if (bma4xx->cfg.accel_pwr_mode) {
		k_sleep(K_NSEC(1300));
	} else {
		k_sleep(K_USEC(1000));
	}

	return 0;
}

static int bma4xx_i2c_read_reg(const struct device *dev, uint8_t reg_addr,
				uint8_t *value)
{
	const struct bma4xx_config *cfg = dev->config;

	return i2c_reg_read_byte_dt(&cfg->bus_cfg.i2c, reg_addr, value);
}

static int bma4xx_i2c_write_reg(const struct device *dev, uint8_t reg_addr,
				uint8_t value)
{
	const struct bma4xx_config *cfg = dev->config;
	const struct bma4xx_data *bma4xx = dev->data;
	int res = 0;

	res = i2c_reg_write_byte_dt(&cfg->bus_cfg.i2c, reg_addr, value);
	if (res) {
		LOG_ERR("Could not perform i2c write reg");
		return -ENOTSUP;
	}

	/* A 1.3us delay is required after write operation when device operates in
	 * power performance mode whereas 1000us is required when the device operates
	 * in low power mode.
	 */
	if (bma4xx->cfg.accel_pwr_mode) {
		k_sleep(K_NSEC(1300));
	} else {
		k_sleep(K_USEC(1000));
	}

	return 0;
}

static int bma4xx_i2c_update_reg(const struct device *dev, uint8_t reg_addr,
				  uint8_t mask, uint8_t value)
{
	const struct bma4xx_config *cfg = dev->config;
	const struct bma4xx_data *bma4xx = dev->data;
	int res = 0;

	res = i2c_reg_update_byte_dt(&cfg->bus_cfg.i2c, reg_addr, mask, value);
	if (res) {
		LOG_ERR("Could not perform i2c update data");
		return -ENOTSUP;
	}

	/* A 1.3us delay is required after write operation when device operates in
	 * power performance mode whereas 1000us is required when the device operates
	 * in low power mode.
	 */
	if (bma4xx->cfg.accel_pwr_mode) {
		k_sleep(K_NSEC(1300));
	} else {
		k_sleep(K_USEC(1000));
	}
	return 0;
}

static const struct bma4xx_hw_operations i2c_ops = {
	.read_data = bma4xx_i2c_read_data,
	.write_data = bma4xx_i2c_write_data,
	.read_reg  = bma4xx_i2c_read_reg,
	.write_reg  = bma4xx_i2c_write_reg,
	.update_reg = bma4xx_i2c_update_reg,
};

int bma4xx_i2c_init(const struct device *dev)
{
	struct bma4xx_data *data = dev->data;
	const struct bma4xx_config *cfg = dev->config;

	if (!device_is_ready(cfg->bus_cfg.i2c.bus)) {
		LOG_ERR("Bus device is not ready");
		return -ENODEV;
	}

	data->hw_ops = &i2c_ops;

	return 0;
}
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c) */
