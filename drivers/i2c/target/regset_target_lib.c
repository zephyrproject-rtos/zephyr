/*
 * SPDX-FileCopyrightText: Copyright (c) 2017 BayLibre, SAS
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c/target/regset_target_lib.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(regset, CONFIG_I2C_LOG_LEVEL);

void regset_target_lib_set_changed_callback(const struct device *dev,
					    regset_target_lib_changed_handler_t handler,
					    void *user_data)
{
	struct regset_target_lib_data *data = dev->data;

	data->changed_handler = handler;
	data->changed_handler_data = user_data;
}

size_t regset_target_lib_get_size(const struct device *dev)
{
	const struct regset_target_lib_config *cfg = dev->config;

	return cfg->buffer_size;
}

int regset_target_lib_read_data(const struct device *dev, off_t offset,
				void *data, size_t len)
{
	const struct regset_target_lib_config *cfg = dev->config;

	if ((offset + len) > cfg->buffer_size) {
		LOG_WRN("attempt to read past device boundary");
		return -EINVAL;
	}

	memcpy(data, cfg->buffer + offset, len);
	return 0;
}

int regset_target_lib_write_data(const struct device *dev, off_t offset,
				 const void *data, size_t len)
{
	const struct regset_target_lib_config *cfg = dev->config;

	if ((offset + len) > cfg->buffer_size) {
		LOG_WRN("attempt to write past device boundary");
		return -EINVAL;
	}

	memcpy(cfg->buffer + offset, data, len);
	return 0;
}

int regset_target_lib_set_addr(const struct device *dev, uint8_t addr)
{
	const struct regset_target_lib_config *cfg = dev->config;
	struct regset_target_lib_data *data = dev->data;
	int ret;

	ret = i2c_target_unregister(cfg->bus.bus, &data->config);
	if (ret) {
		LOG_DBG("i2c target failed to unregister");
		return ret;
	}

	data->config.address = addr;

	return i2c_target_register(cfg->bus.bus, &data->config);
}

static int regset_target_lib_write_requested(struct i2c_target_config *config)
{
	struct regset_target_lib_data *data = CONTAINER_OF(
			config, struct regset_target_lib_data, config);

	LOG_DBG("i2c target: write req");

	data->idx_write_cnt = 0;

	return 0;
}

static int regset_target_lib_read_requested(struct i2c_target_config *config,
					     uint8_t *val)
{
	struct regset_target_lib_data *data = CONTAINER_OF(
			config, struct regset_target_lib_data, config);
	const struct device *dev = data->dev;
	const struct regset_target_lib_config *cfg = dev->config;

	*val = cfg->buffer[data->buffer_idx];

	LOG_DBG("i2c target: read req, val=0x%x", *val);

	/* Increment will be done in the read_processed callback */

	return 0;
}

static int regset_target_lib_write_received(struct i2c_target_config *config,
					    uint8_t val)
{
	struct regset_target_lib_data *data = CONTAINER_OF(
			config, struct regset_target_lib_data, config);
	const struct device *dev = data->dev;
	const struct regset_target_lib_config *cfg = dev->config;

	LOG_DBG("i2c target: write done, val=0x%x", val);

	/* In case EEPROM wants to be R/O, return !0 here could trigger
	 * a NACK to the I2C controller, support depends on the
	 * I2C controller support
	 */

	if (data->idx_write_cnt < (cfg->address_width >> 3)) {
		if (data->idx_write_cnt == 0) {
			data->buffer_idx = 0;
		}

		data->buffer_idx = val | (data->buffer_idx << 8);
		data->idx_write_cnt++;
	} else {
		cfg->buffer[data->buffer_idx++] = val;
		data->changed = true;
	}

	data->buffer_idx = data->buffer_idx % cfg->buffer_size;

	return 0;
}

static int regset_target_lib_read_processed(struct i2c_target_config *config,
					    uint8_t *val)
{
	struct regset_target_lib_data *data = CONTAINER_OF(
			config, struct regset_target_lib_data, config);
	const struct device *dev = data->dev;
	const struct regset_target_lib_config *cfg = dev->config;

	/* Increment here */
	data->buffer_idx = (data->buffer_idx + 1) % cfg->buffer_size;

	*val = cfg->buffer[data->buffer_idx];

	LOG_DBG("i2c target: read done, val=0x%x", *val);

	/* Increment will be done in the next read_processed callback
	 * In case of STOP, the byte won't be taken in account
	 */

	return 0;
}

static int regset_target_lib_stop(struct i2c_target_config *config)
{
	struct regset_target_lib_data *data = CONTAINER_OF(
			config, struct regset_target_lib_data, config);
	regset_target_lib_changed_handler_t handler = data->changed_handler;

	LOG_DBG("i2c target: stop");

	data->idx_write_cnt = 0;

	if (data->changed && handler != NULL) {
		handler(data->dev, data->changed_handler_data);
	}
	data->changed = false;

	return 0;
}

#ifdef CONFIG_I2C_TARGET_BUFFER_MODE
static void regset_target_lib_buf_write_received(struct i2c_target_config *config,
						 uint8_t *ptr, uint32_t len)
{
	struct regset_target_lib_data *data = CONTAINER_OF(
			config, struct regset_target_lib_data, config);
	const struct device *dev = data->dev;
	const struct regset_target_lib_config *cfg = dev->config;

	/* The first byte(s) is offset */
	uint32_t idx_write_cnt = 0;

	data->buffer_idx = 0;
	while (idx_write_cnt < (cfg->address_width >> 3)) {
		data->buffer_idx = (data->buffer_idx << 8) | *ptr++;
		len--;
		idx_write_cnt++;
	}

	if (len > 0) {
		memcpy(&cfg->buffer[data->buffer_idx], ptr, len);
		data->changed = true;
	}
}

static int regset_target_lib_buf_read_requested(struct i2c_target_config *config,
						uint8_t **ptr, uint32_t *len)
{
	struct regset_target_lib_data *data = CONTAINER_OF(
			config, struct regset_target_lib_data, config);
	const struct device *dev = data->dev;
	const struct regset_target_lib_config *cfg = dev->config;

	*ptr = &cfg->buffer[data->buffer_idx];
	*len = cfg->buffer_size;

	return 0;
}
#endif

static int regset_target_lib_register(const struct device *dev)
{
	const struct regset_target_lib_config *cfg = dev->config;
	struct regset_target_lib_data *data = dev->data;

	return i2c_target_register(cfg->bus.bus, &data->config);
}

static int regset_target_lib_unregister(const struct device *dev)
{
	const struct regset_target_lib_config *cfg = dev->config;
	struct regset_target_lib_data *data = dev->data;

	return i2c_target_unregister(cfg->bus.bus, &data->config);
}

static const struct i2c_target_callbacks regset_target_lib_callbacks = {
	.write_requested = regset_target_lib_write_requested,
	.read_requested = regset_target_lib_read_requested,
	.write_received = regset_target_lib_write_received,
	.read_processed = regset_target_lib_read_processed,
#ifdef CONFIG_I2C_TARGET_BUFFER_MODE
	.buf_write_received = regset_target_lib_buf_write_received,
	.buf_read_requested = regset_target_lib_buf_read_requested,
#endif
	.stop = regset_target_lib_stop,
};

DEVICE_API(i2c_target, regset_target_api) = {
	.driver_register = regset_target_lib_register,
	.driver_unregister = regset_target_lib_unregister,
};

int regset_target_lib_init(const struct device *dev)
{
	struct regset_target_lib_data *data = dev->data;
	const struct regset_target_lib_config *cfg = dev->config;

	if (!device_is_ready(cfg->bus.bus)) {
		LOG_ERR("I2C controller device not ready");
		return -ENODEV;
	}

	data->dev = dev;
	data->config.address = cfg->bus.addr;
	data->config.callbacks = &regset_target_lib_callbacks;

	return 0;
}
