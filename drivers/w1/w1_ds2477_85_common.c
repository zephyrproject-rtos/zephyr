/*
 * Copyright (c) 2022 Thomas Stranger
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Common functions for Analog Devices DS2477,DS2485 1-Wire Masters
 */

#include "w1_ds2477_85_common.h"

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/w1.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>

LOG_MODULE_REGISTER(w1_ds2477_85, CONFIG_W1_LOG_LEVEL);

int ds2477_85_write_port_config(const struct device *dev, uint8_t reg, uint16_t value)
{
	const struct w1_ds2477_85_config *cfg = dev->config;
	uint8_t buf[5] = {CMD_WR_W1_PORT_CFG, CMD_WR_W1_PORT_CFG_LEN, reg};
	int ret;

	__ASSERT_NO_MSG(reg <= PORT_REG_COUNT);

	sys_put_le16(value, &buf[3]);
	ret = i2c_write_dt(&cfg->i2c_spec, buf, (CMD_WR_W1_PORT_CFG_LEN + CMD_OVERHEAD_LEN));
	if (ret < 0) {
		return ret;
	}

	k_usleep(cfg->t_op_us);

	ret = i2c_read_dt(&cfg->i2c_spec, buf, 2);
	if (ret < 0) {
		return ret;
	}
	if ((buf[0] != 1) || (buf[1] != DS2477_88_RES_SUCCESS)) {
		return -EIO;
	}

	return 0;
}

int ds2477_85_read_port_config(const struct device *dev, uint8_t reg, uint16_t *value)
{
	const struct w1_ds2477_85_config *cfg = dev->config;
	uint8_t buf[4] = {CMD_RD_W1_PORT_CFG, CMD_RD_W1_PORT_CFG_LEN, reg};
	int ret;

	__ASSERT_NO_MSG(value != NULL && reg <= PORT_REG_COUNT);

	ret = i2c_write_dt(&cfg->i2c_spec, buf, (CMD_RD_W1_PORT_CFG_LEN + CMD_OVERHEAD_LEN));
	if (ret < 0) {
		return ret;
	}

	k_usleep(cfg->t_op_us);

	ret = i2c_read_dt(&cfg->i2c_spec, buf, 4);
	if (ret < 0) {
		return ret;
	}
	if ((buf[0] != 3) || (buf[1] != DS2477_88_RES_SUCCESS)) {
		return -EIO;
	}

	*value = sys_get_le16(&buf[2]);

	return 0;
}

int ds2477_85_reset_master(const struct device *dev)
{
	const struct w1_ds2477_85_config *cfg = dev->config;
	uint8_t buf[2] = {CMD_MASTER_RESET};
	int ret;

	ret = i2c_write_dt(&cfg->i2c_spec, buf, 1);
	if (ret < 0) {
		LOG_ERR("initiate reset failed");
		return ret;
	}

	k_usleep(cfg->t_op_us);

	ret = i2c_read_dt(&cfg->i2c_spec, buf, 2);
	if (ret < 0) {
		return ret;
	}
	if ((buf[0] != 1) || (buf[1] != DS2477_88_RES_SUCCESS)) {
		return -EIO;
	}

	return 0;
}

int ds2477_85_reset_bus(const struct device *dev)
{
	const struct w1_ds2477_85_config *cfg = dev->config;
	struct w1_ds2477_85_data *data = dev->data;
	uint16_t delay_us = cfg->mode_timing[data->master_reg.od_active].t_reset;
	uint8_t tx_data;
	uint8_t rx_data;
	int ret;

	tx_data = data->master_reg.od_active ? BIT(3) : BIT(7);
	ret = cfg->w1_script_cmd(dev, delay_us, SCRIPT_OW_RESET, &tx_data, 1, &rx_data, 1);

	switch (ret) {
	case DS2477_88_RES_COMM_FAILURE:
		/* presence not detected */
		return 0;
	case DS2477_88_RES_SUCCESS:
		/* at least 1 device present */
		return 1;
	default:
		return -EIO;
	}
}

int ds2477_85_read_bit(const struct device *dev)
{
	const struct w1_ds2477_85_config *cfg = dev->config;
	struct w1_ds2477_85_data *data = dev->data;
	uint16_t delay_us = cfg->mode_timing[data->master_reg.od_active].t_slot;
	uint8_t rx_data;
	int ret;

	ret = cfg->w1_script_cmd(dev, delay_us, SCRIPT_OW_READ_BIT, NULL, 0, &rx_data, 1);
	if (ret != DS2477_88_RES_SUCCESS) {
		return -EIO;
	}

	return (rx_data & BIT(5)) ? 1 : 0;
}

int ds2477_85_write_bit(const struct device *dev, const bool bit)
{
	const struct w1_ds2477_85_config *cfg = dev->config;
	struct w1_ds2477_85_data *data = dev->data;
	uint16_t delay_us = cfg->mode_timing[data->master_reg.od_active].t_slot;
	uint8_t tx_data = (uint8_t)bit;
	uint8_t rx_data;
	int ret;

	ret = cfg->w1_script_cmd(dev, delay_us, SCRIPT_OW_WRITE_BIT, &tx_data, 1, &rx_data, 1);
	if (ret != DS2477_88_RES_SUCCESS) {
		return -EIO;
	}

	return 0;
}

int ds2477_85_read_byte(const struct device *dev)
{
	const struct w1_ds2477_85_config *cfg = dev->config;
	struct w1_ds2477_85_data *data = dev->data;
	uint16_t delay_us = cfg->mode_timing[data->master_reg.od_active].t_slot * 8;
	uint8_t rx_data;
	int ret;

	ret = cfg->w1_script_cmd(dev, delay_us, SCRIPT_OW_READ_BYTE, NULL, 0, &rx_data, 1);
	if (ret != DS2477_88_RES_SUCCESS) {
		return -EIO;
	}

	return rx_data;
}

int ds2477_85_write_byte(const struct device *dev, const uint8_t tx_byte)
{
	const struct w1_ds2477_85_config *cfg = dev->config;
	struct w1_ds2477_85_data *data = dev->data;
	uint16_t delay_us = cfg->mode_timing[data->master_reg.od_active].t_slot * 8;
	uint8_t rx_data;
	int ret;

	ret = cfg->w1_script_cmd(dev, delay_us, SCRIPT_OW_WRITE_BYTE, &tx_byte, 1, &rx_data, 1);
	if (ret != DS2477_88_RES_SUCCESS) {
		return -EIO;
	}

	return 0;
}

int ds2477_85_write_block(const struct device *dev, const uint8_t *buffer, size_t tx_len)
{
	const struct w1_ds2477_85_config *cfg = dev->config;
	struct w1_ds2477_85_data *data = dev->data;
	int w1_timing = cfg->mode_timing[data->master_reg.od_active].t_slot * 8 * tx_len;
	uint8_t buf[3] = {CMD_WR_BLOCK, (tx_len + CMD_WR_BLOCK_LEN), 0};
	struct i2c_msg tx_msg[2] = {
		{.buf = buf, .len = (CMD_WR_BLOCK_LEN + CMD_OVERHEAD_LEN), .flags = I2C_MSG_WRITE},
		{.buf = (uint8_t *)buffer, .len = tx_len, .flags = (I2C_MSG_WRITE | I2C_MSG_STOP)},
	};
	int ret;

	__ASSERT_NO_MSG(tx_len <= MAX_BLOCK_LEN);

	if (tx_len == 0) {
		return 0;
	}

	ret = i2c_transfer_dt(&cfg->i2c_spec, tx_msg, 2);
	if (ret < 0) {
		LOG_ERR("write block fail: %x", ret);
		return ret;
	}

	k_usleep(cfg->t_op_us + (cfg->t_seq_us * tx_len) +  w1_timing);

	ret = i2c_read_dt(&cfg->i2c_spec, buf, 2);
	if (ret < 0) {
		return ret;
	}
	if ((buf[0] != 1) || (buf[1] != DS2477_88_RES_SUCCESS)) {
		return -EIO;
	}

	return 0;
}

int ds2477_85_read_block(const struct device *dev, uint8_t *buffer, size_t rx_len)
{
	const struct w1_ds2477_85_config *cfg = dev->config;
	struct w1_ds2477_85_data *data = dev->data;
	int w1_timing = cfg->mode_timing[data->master_reg.od_active].t_slot * 8 * rx_len;
	uint8_t buf[3] = {CMD_RD_BLOCK, CMD_RD_BLOCK_LEN, rx_len};
	struct i2c_msg rx_msg[2] = {
		{.buf = buf, .len = 2, .flags = I2C_MSG_READ},
		{.buf = buffer, .len = rx_len, .flags = (I2C_MSG_READ | I2C_MSG_STOP)}
	};
	int ret;

	__ASSERT_NO_MSG(rx_len <= MAX_BLOCK_LEN);

	if (rx_len == 0) {
		return 0;
	}

	ret = i2c_write_dt(&cfg->i2c_spec, buf, (CMD_RD_BLOCK_LEN + CMD_OVERHEAD_LEN));
	if (ret < 0) {
		LOG_ERR("read block fail: %x", ret);
		return ret;
	}

	k_usleep(cfg->t_op_us + (cfg->t_seq_us * rx_len) +  w1_timing);

	ret = i2c_transfer_dt(&cfg->i2c_spec, rx_msg, 2);
	if (ret < 0) {
		return ret;
	}
	if (buf[1] != DS2477_88_RES_SUCCESS) {
		return -EIO;
	}

	return 0;
}

int ds2477_85_configure(const struct device *dev, enum w1_settings_type type, uint32_t value)
{
	struct w1_ds2477_85_data *data = dev->data;

	switch (type) {
	case W1_SETTING_SPEED:
		__ASSERT_NO_MSG(value <= 1);
		data->master_reg.od_active = value;
		break;
	default:
		return -EINVAL;
	}

	return ds2477_85_write_port_config(dev, PORT_REG_MASTER_CONFIGURATION,
					   data->master_reg.value);
}

int w1_ds2477_85_init(const struct device *dev)
{
	const struct w1_ds2477_85_config *cfg = dev->config;
	struct w1_ds2477_85_data *data = dev->data;

	data->master_reg.apu = cfg->apu;

	if (ds2477_85_write_port_config(dev, PORT_REG_MASTER_CONFIGURATION,
					data->master_reg.value) < 0) {
		return -EIO;
	}

	if (ds2477_85_write_port_config(dev, PORT_REG_RPUP_BUF, cfg->rpup_buf) < 0) {
		return -EIO;
	}

	if (ds2477_85_write_port_config(dev, PORT_REG_PDSLEW, cfg->pdslew) < 0) {
		return -EIO;
	}

	LOG_DBG("cfg: rpup_buf=%02x, pdslew:%02x", cfg->rpup_buf, cfg->pdslew);

	/* RPUP/BUF configuration is applied after a bus reset */
	(void)ds2477_85_reset_bus(dev);

	LOG_DBG("w1-ds2477/85 init; %d slave devices",
		cfg->master_config.slave_count);

	return 0;
}
