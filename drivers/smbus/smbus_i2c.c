/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_smbus_i2c

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/smbus.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

#include "smbus_utils.h"

LOG_MODULE_REGISTER(smbus_i2c, CONFIG_SMBUS_LOG_LEVEL);

#define SMBUS_I2C_SUPPORTED_MODES (SMBUS_MODE_CONTROLLER | SMBUS_MODE_PEC)

struct smbus_i2c_config {
	const struct device *i2c_dev;
};

struct smbus_i2c_data {
	uint32_t config;
};

/*
 * The PEC byte is sent within the data message, as some I2C controllers
 * cannot chain write messages without a repeated START.
 */
static int smbus_i2c_write(const struct device *dev, uint16_t addr, uint8_t *buf, size_t len)
{
	const struct smbus_i2c_config *config = dev->config;
	struct smbus_i2c_data *data = dev->data;
	struct i2c_msg msg = {.buf = buf, .len = len, .flags = I2C_MSG_WRITE};

	if ((data->config & SMBUS_MODE_PEC) != 0U) {
		buf[len] = smbus_pec(addr, &msg, 1);
		len++;
	}

	return i2c_write(config->i2c_dev, buf, len, addr);
}

static int smbus_i2c_read(const struct device *dev, uint16_t addr, uint8_t *wr, size_t wr_len,
			  uint8_t *rd, size_t rd_len)
{
	const struct smbus_i2c_config *config = dev->config;
	struct smbus_i2c_data *data = dev->data;
	bool pec = (data->config & SMBUS_MODE_PEC) != 0U;
	size_t len = rd_len + (pec ? 1U : 0U);
	struct i2c_msg msgs[] = {
		{.buf = wr, .len = wr_len, .flags = I2C_MSG_WRITE},
		{.buf = rd, .len = rd_len, .flags = I2C_MSG_READ},
	};
	int ret;

	if (wr_len == 0U) {
		ret = i2c_read(config->i2c_dev, rd, len, addr);
	} else {
		ret = i2c_write_read(config->i2c_dev, addr, wr, wr_len, rd, len);
	}

	if (ret < 0 || !pec) {
		return ret;
	}

	if (smbus_pec(addr, (wr_len == 0U) ? &msgs[1] : msgs, (wr_len == 0U) ? 1 : 2) !=
	    rd[rd_len]) {
		LOG_DBG("%s: PEC mismatch from 0x%02x", dev->name, addr);
		return -EIO;
	}

	return 0;
}

static int smbus_i2c_configure(const struct device *dev, uint32_t dev_config)
{
	struct smbus_i2c_data *data = dev->data;

	if ((dev_config & ~SMBUS_I2C_SUPPORTED_MODES) != 0U ||
	    (dev_config & SMBUS_MODE_CONTROLLER) == 0U) {
		LOG_ERR("%s: unsupported config 0x%x", dev->name, dev_config);
		return -ENOTSUP;
	}

	data->config = dev_config;

	return 0;
}

static int smbus_i2c_get_config(const struct device *dev, uint32_t *dev_config)
{
	struct smbus_i2c_data *data = dev->data;

	*dev_config = data->config;

	return 0;
}

static int smbus_i2c_quick(const struct device *dev, uint16_t addr, enum smbus_direction direction)
{
	const struct smbus_i2c_config *config = dev->config;
	struct i2c_msg msg = {
		.buf = NULL,
		.len = 0U,
		.flags = (direction == SMBUS_MSG_READ) ? I2C_MSG_READ : I2C_MSG_WRITE,
	};

	if (direction != SMBUS_MSG_READ && direction != SMBUS_MSG_WRITE) {
		return -EINVAL;
	}

	return i2c_transfer(config->i2c_dev, &msg, 1, addr);
}

static int smbus_i2c_byte_write(const struct device *dev, uint16_t addr, uint8_t byte)
{
	uint8_t buf[2] = {byte};

	return smbus_i2c_write(dev, addr, buf, 1);
}

static int smbus_i2c_byte_read(const struct device *dev, uint16_t addr, uint8_t *byte)
{
	uint8_t buf[2];
	int ret;

	ret = smbus_i2c_read(dev, addr, NULL, 0, buf, 1);
	if (ret < 0) {
		return ret;
	}

	*byte = buf[0];

	return 0;
}

static int smbus_i2c_byte_data_write(const struct device *dev, uint16_t addr, uint8_t cmd,
				     uint8_t byte)
{
	uint8_t buf[3] = {cmd, byte};

	return smbus_i2c_write(dev, addr, buf, 2);
}

static int smbus_i2c_byte_data_read(const struct device *dev, uint16_t addr, uint8_t cmd,
				    uint8_t *byte)
{
	uint8_t buf[2];
	int ret;

	ret = smbus_i2c_read(dev, addr, &cmd, 1, buf, 1);
	if (ret < 0) {
		return ret;
	}

	*byte = buf[0];

	return 0;
}

static int smbus_i2c_word_data_write(const struct device *dev, uint16_t addr, uint8_t cmd,
				     uint16_t word)
{
	uint8_t buf[4] = {cmd};

	sys_put_le16(word, &buf[1]);

	return smbus_i2c_write(dev, addr, buf, 3);
}

static int smbus_i2c_word_data_read(const struct device *dev, uint16_t addr, uint8_t cmd,
				    uint16_t *word)
{
	uint8_t buf[3];
	int ret;

	ret = smbus_i2c_read(dev, addr, &cmd, 1, buf, 2);
	if (ret < 0) {
		return ret;
	}

	*word = sys_get_le16(buf);

	return 0;
}

static int smbus_i2c_pcall(const struct device *dev, uint16_t addr, uint8_t cmd, uint16_t send_word,
			   uint16_t *recv_word)
{
	uint8_t tx[3] = {cmd};
	uint8_t rx[3];
	int ret;

	sys_put_le16(send_word, &tx[1]);

	ret = smbus_i2c_read(dev, addr, tx, sizeof(tx), rx, 2);
	if (ret < 0) {
		return ret;
	}

	*recv_word = sys_get_le16(rx);

	return 0;
}

static int smbus_i2c_block_write(const struct device *dev, uint16_t addr, uint8_t cmd,
				 uint8_t count, uint8_t *buf)
{
	uint8_t tx[SMBUS_BLOCK_BYTES_MAX + 3];

	tx[0] = cmd;
	tx[1] = count;
	memcpy(&tx[2], buf, count);

	return smbus_i2c_write(dev, addr, tx, count + 2U);
}

static int smbus_i2c_init(const struct device *dev)
{
	const struct smbus_i2c_config *config = dev->config;
	struct smbus_i2c_data *data = dev->data;

	if (!device_is_ready(config->i2c_dev)) {
		LOG_ERR("%s: I2C device not ready", dev->name);
		return -ENODEV;
	}

	data->config = SMBUS_MODE_CONTROLLER;

	return 0;
}

/*
 * Block read and block process call are not provided: the I2C API cannot
 * size a read from the count byte sent by the target, and reading past the
 * block is a fault for SMBus targets.
 */
static DEVICE_API(smbus, smbus_i2c_api) = {
	.configure = smbus_i2c_configure,
	.get_config = smbus_i2c_get_config,
	.smbus_quick = smbus_i2c_quick,
	.smbus_byte_write = smbus_i2c_byte_write,
	.smbus_byte_read = smbus_i2c_byte_read,
	.smbus_byte_data_write = smbus_i2c_byte_data_write,
	.smbus_byte_data_read = smbus_i2c_byte_data_read,
	.smbus_word_data_write = smbus_i2c_word_data_write,
	.smbus_word_data_read = smbus_i2c_word_data_read,
	.smbus_pcall = smbus_i2c_pcall,
	.smbus_block_write = smbus_i2c_block_write,
};

#define SMBUS_I2C_DEFINE(n)                                                                        \
	static const struct smbus_i2c_config smbus_i2c_config_##n = {                              \
		.i2c_dev = DEVICE_DT_GET(DT_INST_PHANDLE(n, i2c)),                                 \
	};                                                                                         \
                                                                                                   \
	static struct smbus_i2c_data smbus_i2c_data_##n;                                           \
                                                                                                   \
	SMBUS_DEVICE_DT_INST_DEFINE(n, smbus_i2c_init, NULL, &smbus_i2c_data_##n,                  \
				    &smbus_i2c_config_##n, POST_KERNEL,                            \
				    CONFIG_SMBUS_INIT_PRIORITY, &smbus_i2c_api);

DT_INST_FOREACH_STATUS_OKAY(SMBUS_I2C_DEFINE)
