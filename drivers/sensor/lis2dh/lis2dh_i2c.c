/* ST Microelectronics LIS2DH 3-axis accelerometer driver
 *
 * Copyright (c) 2020 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/lis2dh.pdf
 */

#define DT_DRV_COMPAT st_lis2dh

#include <string.h>
#include <drivers/i2c.h>
#include <logging/log.h>

#include "lis2dh.h"

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)

LOG_MODULE_DECLARE(lis2dh, CONFIG_SENSOR_LOG_LEVEL);

static int lis2dh_i2c_read_data(const struct device *dev, uint8_t reg_addr,
				 uint8_t *value, uint8_t len)
{
	struct lis2dh_data *data = dev->data;
	const struct lis2dh_config *cfg = dev->config;

	return i2c_burst_read(data->bus, cfg->bus_cfg.i2c_slv_addr,
			      reg_addr | LIS2DH_AUTOINCREMENT_ADDR,
			      value, len);
}

static int lis2dh_i2c_write_data(const struct device *dev, uint8_t reg_addr,
				  uint8_t *value, uint8_t len)
{
	struct lis2dh_data *data = dev->data;
	const struct lis2dh_config *cfg = dev->config;

	return i2c_burst_write(data->bus, cfg->bus_cfg.i2c_slv_addr,
			       reg_addr | LIS2DH_AUTOINCREMENT_ADDR,
			       value, len);
}

static int lis2dh_i2c_read_reg(const struct device *dev, uint8_t reg_addr,
				uint8_t *value)
{
	struct lis2dh_data *data = dev->data;
	const struct lis2dh_config *cfg = dev->config;

	return i2c_reg_read_byte(data->bus,
				 cfg->bus_cfg.i2c_slv_addr,
				 reg_addr, value);
}

static int lis2dh_i2c_write_reg(const struct device *dev, uint8_t reg_addr,
				uint8_t value)
{
	struct lis2dh_data *data = dev->data;
	const struct lis2dh_config *cfg = dev->config;

	return i2c_reg_write_byte(data->bus,
				  cfg->bus_cfg.i2c_slv_addr,
				  reg_addr, value);
}

static int lis2dh_i2c_update_reg(const struct device *dev, uint8_t reg_addr,
				  uint8_t mask, uint8_t value)
{
	struct lis2dh_data *data = dev->data;
	const struct lis2dh_config *cfg = dev->config;

	return i2c_reg_update_byte(data->bus,
				   cfg->bus_cfg.i2c_slv_addr,
				   reg_addr, mask, value);
}

static const struct lis2dh_transfer_function lis2dh_i2c_transfer_fn = {
	.read_data = lis2dh_i2c_read_data,
	.write_data = lis2dh_i2c_write_data,
	.read_reg  = lis2dh_i2c_read_reg,
	.write_reg  = lis2dh_i2c_write_reg,
	.update_reg = lis2dh_i2c_update_reg,
};

int lis2dh_i2c_init(const struct device *dev)
{
	struct lis2dh_data *data = dev->data;

	data->hw_tf = &lis2dh_i2c_transfer_fn;

	return 0;
}
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c) */
