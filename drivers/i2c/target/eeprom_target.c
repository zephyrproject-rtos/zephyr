/*
 * Copyright (c) 2017 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_i2c_target_eeprom

#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <errno.h>
#include <zephyr/drivers/i2c.h>
#include <string.h>
#include <zephyr/drivers/i2c/target/eeprom.h>

#define LOG_LEVEL CONFIG_I2C_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(i2c_target);

struct i2c_eeprom_target_data {
	struct i2c_target_config config;
	uint32_t buffer_size;
	uint8_t *buffer;
	uint32_t buffer_idx;
	uint32_t idx_write_cnt;
	uint8_t address_width;
};

struct i2c_eeprom_target_config {
	struct i2c_dt_spec bus;
	uint32_t buffer_size;
	uint8_t *buffer;
};

int eeprom_target_program(const struct device *dev, const uint8_t *eeprom_data,
			 unsigned int length)
{
	struct i2c_eeprom_target_data *data = dev->data;

	if (length > data->buffer_size) {
		return -EINVAL;
	}

	memcpy(data->buffer, eeprom_data, length);

	return 0;
}

int eeprom_target_read(const struct device *dev, uint8_t *eeprom_data,
		      unsigned int offset)
{
	struct i2c_eeprom_target_data *data = dev->data;

	if (!data || offset >= data->buffer_size) {
		return -EINVAL;
	}

	*eeprom_data = data->buffer[offset];

	return 0;
}

#ifdef CONFIG_I2C_EEPROM_TARGET_RUNTIME_ADDR
int eeprom_target_set_addr(const struct device *dev, uint8_t addr)
{
	const struct i2c_eeprom_target_config *cfg = dev->config;
	struct i2c_eeprom_target_data *data = dev->data;
	int ret;

	ret = i2c_target_unregister(cfg->bus.bus, &data->config);
	if (ret) {
		LOG_DBG("eeprom target failed to unregister");
		return ret;
	}

	data->config.address = addr;

	return i2c_target_register(cfg->bus.bus, &data->config);
}
#endif /* CONFIG_I2C_EEPROM_TARGET_RUNTIME_ADDR */

static int eeprom_target_write_requested(struct i2c_target_config *config)
{
	struct i2c_eeprom_target_data *data = CONTAINER_OF(config,
						struct i2c_eeprom_target_data,
						config);

	LOG_DBG("eeprom: write req");

	data->idx_write_cnt = 0;

	return 0;
}

static int eeprom_target_read_requested(struct i2c_target_config *config,
				       uint8_t *val)
{
	struct i2c_eeprom_target_data *data = CONTAINER_OF(config,
						struct i2c_eeprom_target_data,
						config);

	*val = data->buffer[data->buffer_idx];

	LOG_DBG("eeprom: read req, val=0x%x", *val);

	/* Increment will be done in the read_processed callback */

	return 0;
}

static int eeprom_target_write_received(struct i2c_target_config *config,
				       uint8_t val)
{
	struct i2c_eeprom_target_data *data = CONTAINER_OF(config,
						struct i2c_eeprom_target_data,
						config);

	LOG_DBG("eeprom: write done, val=0x%x", val);

	/* In case EEPROM wants to be R/O, return !0 here could trigger
	 * a NACK to the I2C controller, support depends on the
	 * I2C controller support
	 */

	if (data->idx_write_cnt < (data->address_width >> 3)) {
		if (data->idx_write_cnt == 0) {
			data->buffer_idx = 0;
		}

		data->buffer_idx = val | (data->buffer_idx << 8);
		data->idx_write_cnt++;
	} else {
		data->buffer[data->buffer_idx++] = val;
	}

	data->buffer_idx = data->buffer_idx % data->buffer_size;

	return 0;
}

static int eeprom_target_read_processed(struct i2c_target_config *config,
				       uint8_t *val)
{
	struct i2c_eeprom_target_data *data = CONTAINER_OF(config,
						struct i2c_eeprom_target_data,
						config);

	/* Increment here */
	data->buffer_idx = (data->buffer_idx + 1) % data->buffer_size;

	*val = data->buffer[data->buffer_idx];

	LOG_DBG("eeprom: read done, val=0x%x", *val);

	/* Increment will be done in the next read_processed callback
	 * In case of STOP, the byte won't be taken in account
	 */

	return 0;
}

static int eeprom_target_stop(struct i2c_target_config *config)
{
	struct i2c_eeprom_target_data *data = CONTAINER_OF(config,
						struct i2c_eeprom_target_data,
						config);

	LOG_DBG("eeprom: stop");

	data->idx_write_cnt = 0;

	return 0;
}

#ifdef CONFIG_I2C_TARGET_BUFFER_MODE
static void eeprom_target_buf_write_received(struct i2c_target_config *config,
					     uint8_t *ptr, uint32_t len)
{
	struct i2c_eeprom_target_data *data = CONTAINER_OF(config,
						struct i2c_eeprom_target_data,
						config);
	/* The first byte is offset */
	data->buffer_idx = *ptr;
	memcpy(&data->buffer[data->buffer_idx], ptr + 1, len - 1);
}

static int eeprom_target_buf_read_requested(struct i2c_target_config *config,
					    uint8_t **ptr, uint32_t *len)
{
	struct i2c_eeprom_target_data *data = CONTAINER_OF(config,
						struct i2c_eeprom_target_data,
						config);

	*ptr = &data->buffer[data->buffer_idx];
	*len = data->buffer_size;

	return 0;
}
#endif

static int eeprom_target_register(const struct device *dev)
{
	const struct i2c_eeprom_target_config *cfg = dev->config;
	struct i2c_eeprom_target_data *data = dev->data;

	return i2c_target_register(cfg->bus.bus, &data->config);
}

static int eeprom_target_unregister(const struct device *dev)
{
	const struct i2c_eeprom_target_config *cfg = dev->config;
	struct i2c_eeprom_target_data *data = dev->data;

	return i2c_target_unregister(cfg->bus.bus, &data->config);
}

static const struct i2c_target_driver_api api_funcs = {
	.driver_register = eeprom_target_register,
	.driver_unregister = eeprom_target_unregister,
};

static const struct i2c_target_callbacks eeprom_callbacks = {
	.write_requested = eeprom_target_write_requested,
	.read_requested = eeprom_target_read_requested,
	.write_received = eeprom_target_write_received,
	.read_processed = eeprom_target_read_processed,
#ifdef CONFIG_I2C_TARGET_BUFFER_MODE
	.buf_write_received = eeprom_target_buf_write_received,
	.buf_read_requested = eeprom_target_buf_read_requested,
#endif
	.stop = eeprom_target_stop,
};

static int i2c_eeprom_target_init(const struct device *dev)
{
	struct i2c_eeprom_target_data *data = dev->data;
	const struct i2c_eeprom_target_config *cfg = dev->config;

	if (!device_is_ready(cfg->bus.bus)) {
		LOG_ERR("I2C controller device not ready");
		return -ENODEV;
	}

	data->buffer_size = cfg->buffer_size;
	data->buffer = cfg->buffer;
	data->config.address = cfg->bus.addr;
	data->config.callbacks = &eeprom_callbacks;

	return 0;
}

#define I2C_EEPROM_INIT(inst)						\
	static struct i2c_eeprom_target_data				\
		i2c_eeprom_target_##inst##_dev_data = {			\
			.address_width = DT_INST_PROP_OR(inst,		\
					address_width, 8),		\
		};							\
									\
	static uint8_t							\
	i2c_eeprom_target_##inst##_buffer[(DT_INST_PROP(inst, size))];	\
									\
	BUILD_ASSERT(DT_INST_PROP(inst, size) <=			\
			(1 << DT_INST_PROP_OR(inst, address_width, 8)), \
			"size must be <= than 2^address_width");	\
									\
	static const struct i2c_eeprom_target_config			\
		i2c_eeprom_target_##inst##_cfg = {			\
		.bus = I2C_DT_SPEC_INST_GET(inst),			\
		.buffer_size = DT_INST_PROP(inst, size),		\
		.buffer = i2c_eeprom_target_##inst##_buffer		\
	};								\
									\
	DEVICE_DT_INST_DEFINE(inst,					\
			    &i2c_eeprom_target_init,			\
			    NULL,			\
			    &i2c_eeprom_target_##inst##_dev_data,	\
			    &i2c_eeprom_target_##inst##_cfg,		\
			    POST_KERNEL,				\
			    CONFIG_I2C_TARGET_INIT_PRIORITY,		\
			    &api_funcs);

DT_INST_FOREACH_STATUS_OKAY(I2C_EEPROM_INIT)
