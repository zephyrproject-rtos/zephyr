/*
 * Copyright (c) 2022 Thomas Stranger
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT maxim_ds2485

/**
 * @brief Driver for the Analog Devices DS2485 1-Wire Master
 */

#include "w1_ds2477_85_common.h"

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/w1.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(w1_ds2485, CONFIG_W1_LOG_LEVEL);

/* upper limits; guaranteed over operating temperature range */
#define DS2485_T_OSCWUP_us 1000U
#define DS2485_T_OP_us	   400U
#define DS2485_T_SEQ_us	   10U

int ds2485_w1_script_cmd(const struct device *dev, int w1_delay_us, uint8_t w1_cmd,
			 const uint8_t *tx_buf, const uint8_t tx_len,
			 uint8_t *rx_buf, uint8_t rx_len)
{
	const struct w1_ds2477_85_config *cfg = dev->config;
	uint8_t i2c_len = 3 + tx_len;
	uint8_t tx_bytes[3 + SCRIPT_WR_LEN] = {
		CMD_W1_SCRIPT, tx_len + CMD_W1_SCRIPT_LEN, w1_cmd
	};
	uint8_t rx_bytes[3];
	struct i2c_msg rx_msg[2] = {
		{
			.buf = rx_bytes,
			.len = (CMD_W1_SCRIPT_LEN + CMD_OVERHEAD_LEN),
			.flags = I2C_MSG_READ,
		},
		{
			.buf = rx_buf,
			.len = rx_len,
			.flags = (I2C_MSG_READ | I2C_MSG_STOP),
		},
	};
	int ret;

	__ASSERT_NO_MSG(tx_len <= SCRIPT_WR_LEN);
	memcpy(&tx_bytes[3], tx_buf, tx_len);
	ret = i2c_write_dt(&cfg->i2c_spec, tx_bytes, i2c_len);
	if (ret < 0) {
		return ret;
	}

	k_usleep(DS2485_T_OP_us + DS2485_T_SEQ_us + w1_delay_us);

	ret = i2c_transfer_dt(&cfg->i2c_spec, rx_msg, 2);
	if (ret < 0) {
		LOG_ERR("scripts_cmd fail: ret: %x", ret);
		return ret;
	}

	if ((rx_bytes[0] != (rx_len + 2)) || (rx_bytes[2] != w1_cmd)) {
		LOG_ERR("scripts_cmd fail: response: %x,%x:",
			rx_bytes[0], rx_bytes[2]);
		return -EIO;
	}

	return rx_bytes[1];
}

static int w1_ds2485_init(const struct device *dev)
{
	const struct w1_ds2477_85_config *cfg = dev->config;

	if (!device_is_ready(cfg->i2c_spec.bus)) {
		LOG_ERR("%s is not ready", cfg->i2c_spec.bus->name);
		return -ENODEV;
	}

	if (ds2477_85_reset_master(dev)) {
		return -EIO;
	}
	k_usleep(DS2485_T_OSCWUP_us);

	return w1_ds2477_85_init(dev);
}

static const struct w1_driver_api w1_ds2485_driver_api = {
	.reset_bus = ds2477_85_reset_bus,
	.read_bit = ds2477_85_read_bit,
	.write_bit = ds2477_85_write_bit,
	.read_byte = ds2477_85_read_byte,
	.write_byte = ds2477_85_write_byte,
	.read_block = ds2477_85_read_block,
	.write_block = ds2477_85_write_block,
	.configure = ds2477_85_configure,
};

#define W1_DS2485_INIT(inst)                                                   \
	static const struct w1_ds2477_85_config w1_ds2477_85_cfg_##inst =      \
		W1_DS2477_85_DT_CONFIG_INST_GET(inst, DS2485_T_OP_us,          \
						DS2485_T_SEQ_us,               \
						ds2485_w1_script_cmd);         \
	                                                                       \
	static struct w1_ds2477_85_data w1_ds2477_85_data_##inst = {};         \
	DEVICE_DT_INST_DEFINE(inst, &w1_ds2485_init, NULL,                     \
			      &w1_ds2477_85_data_##inst,                       \
			      &w1_ds2477_85_cfg_##inst, POST_KERNEL,           \
			      CONFIG_W1_INIT_PRIORITY, &w1_ds2485_driver_api);

DT_INST_FOREACH_STATUS_OKAY(W1_DS2485_INIT)
