/*
 * Copyright (c) 2023 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT fujitsu_mb85rcxx

#include <zephyr/device.h>
#include <zephyr/drivers/eeprom.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(EEPROM_MB85RCXX, CONFIG_EEPROM_LOG_LEVEL);

struct mb85rcxx_config {
	struct i2c_dt_spec i2c;
	struct gpio_dt_spec wp_gpio;
	size_t size;
	size_t pagesize;
	uint8_t addr_width;
	bool readonly;
};

struct mb85rcxx_data {
	struct k_mutex lock;
};

static int mb85rcxx_write_protect_set(const struct device *dev, int value)
{
	const struct mb85rcxx_config *cfg = dev->config;

	if (!cfg->wp_gpio.port) {
		return 0;
	}

	return gpio_pin_set_dt(&cfg->wp_gpio, value);
}

static uint16_t mb85rcxx_translate_address(const struct device *dev, off_t offset, uint8_t *addr)
{
	const struct mb85rcxx_config *cfg = dev->config;

	off_t page_offset = offset % cfg->pagesize;

	if (cfg->addr_width > 8) {
		sys_put_be16(page_offset, addr);
		addr[0] &= BIT_MASK(cfg->addr_width - 8);
	} else {
		addr[0] = page_offset & BIT_MASK(cfg->addr_width);
	}

	return cfg->i2c.addr + (offset >> cfg->addr_width);
}

static size_t mb85rcxx_remaining_len_in_page(const struct device *dev, off_t offset, size_t len)
{
	const struct mb85rcxx_config *cfg = dev->config;
	off_t page_offset = offset % cfg->pagesize;
	size_t rem = cfg->pagesize - page_offset;

	if (rem > len) {
		rem = len;
	}

	return rem;
}

static int mb85rcxx_init(const struct device *dev)
{
	const struct mb85rcxx_config *cfg = dev->config;
	struct mb85rcxx_data *data = dev->data;

	k_mutex_init(&data->lock);

	if (!i2c_is_ready_dt(&cfg->i2c)) {
		LOG_ERR("i2c bus device not ready");
		return -EINVAL;
	}

	if (cfg->wp_gpio.port) {
		if (!gpio_is_ready_dt(&cfg->wp_gpio)) {
			LOG_ERR("wp gpio device not ready");
			return -EINVAL;
		}

		int err = gpio_pin_configure_dt(&cfg->wp_gpio, GPIO_OUTPUT_ACTIVE);

		if (err) {
			LOG_ERR("failed to configure WP GPIO pin (err %d)", err);
			return err;
		}
	}
	return 0;
}

static int mb85rcxx_read(const struct device *dev, off_t offset, void *buf, size_t len)
{
	const struct mb85rcxx_config *cfg = dev->config;
	struct mb85rcxx_data *data = dev->data;
	uint8_t addr[2];
	uint16_t i2c_addr;
	size_t len_in_page;
	int ret;

	if (offset + len > cfg->size) {
		LOG_ERR("attempt to read past device boundary");
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	while (len) {
		i2c_addr = mb85rcxx_translate_address(dev, offset, addr);
		len_in_page = mb85rcxx_remaining_len_in_page(dev, offset, len);

		ret = i2c_write_read(cfg->i2c.bus, i2c_addr, addr, DIV_ROUND_UP(cfg->addr_width, 8),
				     buf, len_in_page);
		if (ret < 0) {
			LOG_ERR("failed to read FRAM (err %d)", ret);
			k_mutex_unlock(&data->lock);
			return ret;
		}

		len -= len_in_page;
		*(char *)&buf += len_in_page;
		offset += len_in_page;
	}

	k_mutex_unlock(&data->lock);
	return 0;
}

static int mb85rcxx_i2c_write(const struct device *dev, uint16_t i2c_addr, uint8_t *addr,
			      const void *buf, size_t len)
{
	const struct mb85rcxx_config *cfg = dev->config;
	struct i2c_msg msgs[2];

	msgs[0].buf = addr;
	msgs[0].len = DIV_ROUND_UP(cfg->addr_width, 8);
	msgs[0].flags = I2C_MSG_WRITE;

	msgs[1].buf = (void *)buf;
	msgs[1].len = len;
	msgs[1].flags = I2C_MSG_WRITE | I2C_MSG_STOP;

	return i2c_transfer(cfg->i2c.bus, &msgs[0], 2, i2c_addr);
}

static int mb85rcxx_write(const struct device *dev, off_t offset, const void *buf, size_t len)
{
	const struct mb85rcxx_config *cfg = dev->config;
	struct mb85rcxx_data *data = dev->data;
	uint8_t addr[2];
	uint16_t i2c_addr;
	size_t len_in_page;
	int ret;

	if (cfg->readonly) {
		LOG_ERR("attempt to write to read-only device");
		return -EACCES;
	}

	if (offset + len > cfg->size) {
		LOG_ERR("attempt to write past device boundary");
		return -EINVAL;
	}

	ret = mb85rcxx_write_protect_set(dev, 0);
	if (ret) {
		LOG_ERR("failed to write-enable FRAM (err %d)", ret);
		return ret;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	while (len) {
		i2c_addr = mb85rcxx_translate_address(dev, offset, addr);
		len_in_page = mb85rcxx_remaining_len_in_page(dev, offset, len);

		ret = mb85rcxx_i2c_write(dev, i2c_addr, addr, buf, len);
		if (ret < 0) {
			LOG_ERR("failed to write to FRAM (err %d)", ret);
			k_mutex_unlock(&data->lock);
			return ret;
		}

		len -= len_in_page;
		*(char *)&buf += len_in_page;
		offset += len_in_page;
	}

	k_mutex_unlock(&data->lock);
	mb85rcxx_write_protect_set(dev, 1);
	return ret;
}

static size_t mb85rcxx_get_size(const struct device *dev)
{
	const struct mb85rcxx_config *cfg = dev->config;

	return cfg->size;
}

static DEVICE_API(eeprom, mb85rcxx_driver_api) = {
	.read = &mb85rcxx_read,
	.write = &mb85rcxx_write,
	.size = &mb85rcxx_get_size,
};

#define MB85RCXX_DEFINE(inst)                                                                      \
	static struct mb85rcxx_data mb85rcxx_data_##inst;                                          \
                                                                                                   \
	static const struct mb85rcxx_config mb85rcxx_config_##inst = {                             \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, wp_gpios),                                  \
			   (.wp_gpio = GPIO_DT_SPEC_INST_GET(inst, wp_gpios),))                    \
			.size = DT_INST_PROP(inst, size),                                          \
		.pagesize =                                                                        \
			COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, pagesize),                         \
				    (DT_INST_PROP(inst, pagesize)), (DT_INST_PROP(inst, size))),   \
		.addr_width = DT_INST_PROP(inst, address_width),                                   \
		.readonly = DT_INST_PROP(inst, read_only)};                                        \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, mb85rcxx_init, NULL, &mb85rcxx_data_##inst,                    \
			      &mb85rcxx_config_##inst, POST_KERNEL, CONFIG_EEPROM_INIT_PRIORITY,   \
			      &mb85rcxx_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MB85RCXX_DEFINE)
