/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2024 Arunmani Alagarsamy
 * Author: Arunmani Alagarsamy  <arunmani.a@capgemini.com>
 */

#define DT_DRV_COMPAT st_m24xxx

#include <zephyr/device.h>
#include <zephyr/drivers/eeprom.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(EEPROM_M24XXX, CONFIG_EEPROM_LOG_LEVEL);

struct m24xxx_config {
	struct i2c_dt_spec i2c;
	struct gpio_dt_spec wp_gpio;
	size_t size;
	size_t pagesize;
        uint8_t addr_width;
	uint16_t timeout;
	bool readonly;
};

struct m24xxx_data {
	struct k_mutex lock;
};

static size_t m24xxx_get_size(const struct device *dev)
{
	const struct m24xxx_config *cfg = dev->config;

	return cfg->size;
}

static int m24xxx_write_protect_set(const struct device *dev, int value)
{
	const struct m24xxx_config *cfg = dev->config;

	if (!cfg->wp_gpio.port) {
		return 0;
	}

	return gpio_pin_set_dt(&cfg->wp_gpio, value);
}

static int m24xxx_read(const struct device *dev, off_t offset,
					void *read_buf, size_t len)
{
	const struct m24xxx_config *cfg = dev->config;
	struct m24xxx_data *data = dev->data;
	uint8_t reg_addr[2];
	int ret;

	if (offset + len > cfg->size) {
		LOG_ERR("attempt to read past device boundary");
		return -EINVAL;
	}

	sys_put_be16((uint16_t)offset, reg_addr);

	k_msleep(cfg->timeout);
	k_mutex_lock(&data->lock, K_FOREVER);

	ret = i2c_write_read(cfg->i2c.bus, cfg->i2c.addr, reg_addr, sizeof(reg_addr),
								read_buf, len);
	if (ret) {
		LOG_ERR("Failed to read (err %d)", ret);
		return ret;
	}

	k_mutex_unlock(&data->lock);

	return 0;
}

void get_reg_addr(uint16_t offset, const void *buf, uint8_t *reg_addr, uint32_t len, uint8_t addr_width)
{
	if (addr_width > 8) {
		sys_put_be16(offset, reg_addr);
		memcpy(&reg_addr[2], buf, len);
	} else {
		reg_addr[0] = (uint8_t)offset;
		memcpy(&reg_addr[1], buf, len);
	}
}

static int write_aligned_pages(const struct m24xxx_config *cfg, uint16_t offset, const void *buf,
										uint32_t len)
{
	int ret;
	uint32_t numofpages = len / cfg->pagesize;
	uint32_t i;
	uint8_t reg_addr[len + 2];

	get_reg_addr(offset, buf, reg_addr, len, cfg->addr_width);

	for (i = 0; i < numofpages; ++i) {
		ret = i2c_write_dt(&cfg->i2c, reg_addr, cfg->pagesize);
		if (ret) {
			LOG_ERR("Failed to write (err %d)", ret);
			return ret;
		}

		offset += (uint16_t)cfg->pagesize;
		*(uint8_t *)&buf += cfg->pagesize;
	}

	k_msleep(cfg->timeout);

	if (len % cfg->pagesize != 0) {
		get_reg_addr(offset, buf, reg_addr, len % cfg->pagesize, cfg->addr_width);

		ret = i2c_write_dt(&cfg->i2c, reg_addr, len % cfg->pagesize);
		if (ret) {
			LOG_ERR("Failed to write (err %d)", ret);
			return ret;
		}
	}

	return ret;
}

static int write_unaligned_pages(const struct m24xxx_config *cfg, uint16_t offset, const void *buf,
							uint32_t len, uint16_t page_alignment)
{
	int ret;
	uint8_t reg_addr[len + 2];

	if (len > page_alignment) {
		get_reg_addr(offset, buf, reg_addr, page_alignment, cfg->addr_width);

		ret = i2c_write_dt(&cfg->i2c, reg_addr, (page_alignment + 2));
		if (ret) {
			LOG_ERR("Failed to write (err %d)", ret);
			return ret;
		}

		k_msleep(cfg->timeout);

		offset += page_alignment;
		*(uint8_t *)&buf += page_alignment;

		get_reg_addr(offset, buf, reg_addr, (len - page_alignment), cfg->addr_width);

		ret = i2c_write_dt(&cfg->i2c, reg_addr, (len - page_alignment)+2);
		if (ret) {
			LOG_ERR("Failed to write (err %d)", ret);
			return ret;
		}
	} else {
		get_reg_addr(offset, buf, reg_addr, len, cfg->addr_width);

		ret = i2c_write_dt(&cfg->i2c, reg_addr, len + 2);
		if (ret) {
			LOG_ERR("Failed to write (err %d)", ret);
			return ret;
		}
	}

	return ret;
}

static int m24xxx_write(const struct device *dev, off_t offset, const void *buf, size_t len)
{
	int ret;
	const struct m24xxx_config *cfg = dev->config;
	struct m24xxx_data *data = dev->data;
	uint16_t page_alignment;

	if (cfg->readonly) {
		LOG_ERR("attempt to write to read-only device");
		return -EACCES;
	}

	if (offset + len > cfg->size) {
		LOG_ERR("attempt to write past device boundary");
		return -EINVAL;
	}

	ret = m24xxx_write_protect_set(dev, 0);
	if (ret) {
		LOG_ERR("failed to write-enable (err %d)", ret);
		return ret;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	page_alignment = cfg->pagesize - (offset % cfg->pagesize);

	if ((offset % cfg->pagesize) == 0) {
		ret = write_aligned_pages(cfg, offset, buf, len);
	} else {
		ret = write_unaligned_pages(cfg, offset, buf, len, page_alignment);
	}

	k_mutex_unlock(&data->lock);

	m24xxx_write_protect_set(dev, 1);

	return ret;
}

static const struct eeprom_driver_api m24xxx_driver_api = {
	.read = &m24xxx_read,
	.write = &m24xxx_write,
	.size = &m24xxx_get_size,
};

static int m24xxx_init(const struct device *dev)
{
	const struct m24xxx_config *cfg = dev->config;
	struct m24xxx_data *data = dev->data;
	int err;

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

		err = gpio_pin_configure_dt(&cfg->wp_gpio, GPIO_OUTPUT_ACTIVE);
		if (err) {
			LOG_ERR("failed to configure WP GPIO pin (err %d)", err);
			return err;
		}
	}

	return 0;
}

#define M24XXX_DEFINE(inst)									\
	static struct m24xxx_data m24xxx_data_##inst;						\
												\
	static const struct m24xxx_config m24xxx_config_##inst = {				\
		.i2c = I2C_DT_SPEC_INST_GET(inst),						\
		IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, wp_gpios),				\
			(.wp_gpio = GPIO_DT_SPEC_INST_GET(inst, wp_gpios),))			\
		.size = DT_INST_PROP(inst, size),						\
		.readonly = DT_INST_PROP(inst, read_only),					\
		.addr_width = DT_INST_PROP(inst, address_width),				\
		.pagesize = DT_INST_PROP(inst, pagesize),					\
		.timeout = DT_INST_PROP(inst, timeout)};					\
												\
	DEVICE_DT_INST_DEFINE(inst, m24xxx_init, NULL, &m24xxx_data_##inst,			\
			&m24xxx_config_##inst, POST_KERNEL, CONFIG_EEPROM_INIT_PRIORITY,	\
				&m24xxx_driver_api);

DT_INST_FOREACH_STATUS_OKAY(M24XXX_DEFINE)
