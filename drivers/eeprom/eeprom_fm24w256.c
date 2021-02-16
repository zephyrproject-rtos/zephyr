/*
 * Copyright (c) 2021 Caspar Friedrich <c.s.w.friedrich@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <devicetree.h>
#include <devicetree/gpio.h>
#include <drivers/eeprom.h>
#include <drivers/gpio.h>
#include <drivers/i2c.h>
#include <logging/log.h>

#define DT_DRV_COMPAT cypress_fm24w256

/*
 * 256-Kbit ferroelectric random access memory logically organized as 32K × 8 bit
 */
#define SIZE KB(32)

/*
 * Get the upper seven address bits and mask them properly.
 */
#define OFFSET_HI(offset) ((offset >> 8) & BIT_MASK(7))

/*
 * Get the lower eight address bits and mask them properly.
 * Redundant shift is kept for consistency.
 */
#define OFFSET_LO(offset) ((offset >> 0) & BIT_MASK(8))

LOG_MODULE_REGISTER(fm24w256, CONFIG_EEPROM_LOG_LEVEL);

struct fm24w256_config {
	struct i2c_dt_spec i2c_spec;
	struct gpio_dt_spec wp_spec;
	bool read_only;
};

struct fm24w256_data {
	struct k_mutex lock;
};

static inline const struct fm24w256_config *get_config(const struct device *dev)
{
	return dev->config;
}

static inline struct fm24w256_data *get_data(const struct device *dev)
{
	return dev->data;
}

static inline int enable_write_protect(const struct device *dev)
{
	if (!get_config(dev)->wp_spec.port) {
		return 0;
	}

	return gpio_pin_set_dt(&get_config(dev)->wp_spec, 1);
}

static inline int disable_write_protect(const struct device *dev)
{
	if (!get_config(dev)->wp_spec.port) {
		return 0;
	}

	return gpio_pin_set_dt(&get_config(dev)->wp_spec, 0);
}

static int fm24w256_memory_operation(const struct device *dev, off_t offset, const void *buf,
				     size_t buf_len, bool is_write)
{
	int ret;
	int ret_transfer;

	uint8_t addr_buf[] = {
		OFFSET_HI(offset),
		OFFSET_LO(offset),
	};

	struct i2c_msg msg[] = {
		{
			.buf = addr_buf,
			.len = ARRAY_SIZE(addr_buf),
			.flags = I2C_MSG_WRITE,
		},
		{
			.buf = (uint8_t *)buf,
			.len = buf_len,
			.flags = (is_write ? I2C_MSG_WRITE : I2C_MSG_RESTART | I2C_MSG_READ) |
				 I2C_MSG_STOP,
		},
	};

	if (is_write && get_config(dev)->read_only) {
		LOG_ERR("Device is configured read only");
		return -EACCES;
	}

	if (offset < 0 || offset > SIZE) {
		LOG_ERR("Offset out of range: %d", offset);
		return -EINVAL;
	}

	if (offset + buf_len > SIZE) {
		LOG_ERR("Memory roll over");
		return -EINVAL;
	}

	k_mutex_lock(&get_data(dev)->lock, K_FOREVER);

	ret = disable_write_protect(dev);
	if (ret) {
		LOG_ERR("Unable to disable write protection: %d", ret);
		k_mutex_unlock(&get_data(dev)->lock);
		return ret;
	}

	ret_transfer = i2c_transfer_dt(&get_config(dev)->i2c_spec, msg, ARRAY_SIZE(msg));
	if (ret_transfer) {
		LOG_ERR("I2C transfer failed: %d", ret);
	}

	ret = enable_write_protect(dev);
	if (ret) {
		LOG_ERR("Unable to enable write protection: %d", ret);
		k_mutex_unlock(&get_data(dev)->lock);
		return ret;
	}

	k_mutex_unlock(&get_data(dev)->lock);

	return ret_transfer;
}

static int fm24w256_init(const struct device *dev)
{
	int ret;

	ret = k_mutex_init(&get_data(dev)->lock);
	if (ret) {
		LOG_ERR("Unable initialize mutex: %d", ret);
		return ret;
	}

	/*
	 * Write protect via GPIO is optional. Only configure if set.
	 */
	if (get_config(dev)->wp_spec.port) {
		if (!device_is_ready(get_config(dev)->wp_spec.port)) {
			LOG_ERR("write protect port not ready");
			return -EINVAL;
		}

		ret = gpio_pin_configure_dt(&get_config(dev)->wp_spec, GPIO_OUTPUT_ACTIVE);
		if (ret) {
			LOG_ERR("Unable to configure write protect pin: %d", ret);
			return ret;
		}
	}

	return 0;
}

static int fm24w256_read(const struct device *dev, off_t offset, void *buf, size_t buf_len)
{
	LOG_DBG("About to read %u bytes from 0x%04x", buf_len, offset);
	return fm24w256_memory_operation(dev, offset, buf, buf_len, false);
}

static int fm24w256_write(const struct device *dev, off_t offset, const void *buf, size_t buf_len)
{
	LOG_DBG("About to write %u bytes to 0x%04x", buf_len, offset);
	return fm24w256_memory_operation(dev, offset, buf, buf_len, true);
}

static size_t fm24w256_size(const struct device *dev)
{
	return SIZE;
}

static const struct eeprom_driver_api api = {
	.read = fm24w256_read,
	.write = fm24w256_write,
	.size = fm24w256_size,
};

#define INIT(n)                                                                                    \
	static const struct fm24w256_config inst_##n##_config = {                                  \
		.i2c_spec = I2C_DT_SPEC_INST_GET(n),                                               \
		.wp_spec = GPIO_DT_SPEC_INST_GET_OR(n, wp_gpios, { 0 }),                           \
		.read_only = DT_INST_PROP(n, read_only),                                           \
	};                                                                                         \
	static struct fm24w256_data inst_##n##_data = { 0 };                                       \
	DEVICE_DT_INST_DEFINE(n, &fm24w256_init, NULL, &inst_##n##_data, &inst_##n##_config,       \
			      POST_KERNEL, CONFIG_I2C_INIT_PRIORITY, &api);

DT_INST_FOREACH_STATUS_OKAY(INIT)
