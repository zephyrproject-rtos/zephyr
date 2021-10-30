/* Kionix KX022 3-axis accelerometer driver
 *
 * Copyright (c) 2021 G-Technologies Sdn. Bhd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT kionix_kx022

#include <string.h>
#include <drivers/i2c.h>
#include <logging/log.h>

#include "kx022.h"

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)

static uint16_t kx022_i2c_slave_addr = DT_INST_REG_ADDR(0);

LOG_MODULE_DECLARE(KX022, CONFIG_SENSOR_LOG_LEVEL);

static int kx022_i2c_read_data(struct kx022_data *data, uint8_t reg_addr,
			       uint8_t *value, uint8_t len)
{
	return i2c_burst_read(data->comm_master, kx022_i2c_slave_addr, reg_addr,
			      value, len);
}

static int kx022_i2c_write_data(struct kx022_data *data, uint8_t reg_addr,
				uint8_t *value, uint8_t len)
{
	return i2c_burst_write(data->comm_master, kx022_i2c_slave_addr,
			       reg_addr, value, len);
}

static int kx022_i2c_read_reg(struct kx022_data *data, uint8_t reg_addr,
			      uint8_t *value)
{
	return i2c_reg_read_byte(data->comm_master, kx022_i2c_slave_addr,
				 reg_addr, value);
}

static int kx022_i2c_write_reg(struct kx022_data *data, uint8_t reg_addr,
			       uint8_t value)
{
	return i2c_reg_write_byte(data->comm_master, kx022_i2c_slave_addr,
				  reg_addr, value);
}

static int kx022_i2c_update_reg(struct kx022_data *data, uint8_t reg_addr,
				uint8_t mask, uint8_t value)
{
	return i2c_reg_update_byte(data->comm_master, kx022_i2c_slave_addr,
				   reg_addr, mask, value);
}

static const struct kx022_transfer_function kx022_i2c_transfer_fn = {
	.read_data = kx022_i2c_read_data,
	.write_data = kx022_i2c_write_data,
	.read_reg = kx022_i2c_read_reg,
	.write_reg = kx022_i2c_write_reg,
	.update_reg = kx022_i2c_update_reg,
};

int kx022_i2c_init(const struct device *dev)
{
	struct kx022_data *data = dev->data;

	data->hw_tf = &kx022_i2c_transfer_fn;

	return 0;
}
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c) */
