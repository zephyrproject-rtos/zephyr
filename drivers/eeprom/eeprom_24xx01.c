/*
 * MICROCHIP EEPROM 24XX01 driver
 *
 * Copyright (c) 2024 Basalte bv
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_24xx01_eeprom

#include <zephyr/device.h>
#include <zephyr/drivers/eeprom.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(eeprom_24xx01, CONFIG_EEPROM_LOG_LEVEL);

struct eeprom_24xx01_config {
	struct i2c_dt_spec i2c;
	struct gpio_dt_spec wp_gpio;
	size_t size;
	size_t pagesize;
	uint16_t write_cycle_time;
	bool readonly;
};

struct eeprom_24xx01_data {
	struct k_mutex lock;
	k_timepoint_t next_read_write_timepoint;
};

static inline void eeprom_24xx01_wait_ready(const struct device *dev)
{
	struct eeprom_24xx01_data *data = dev->data;

	k_sleep(sys_timepoint_timeout(data->next_read_write_timepoint));
}

static int eeprom_24xx01_write_protect_set(const struct device *dev, int value)
{
	const struct eeprom_24xx01_config *cfg = dev->config;

	if (!gpio_is_ready_dt(&cfg->wp_gpio)) {
		return 0;
	}

	return gpio_pin_set_dt(&cfg->wp_gpio, value);
}

static size_t eeprom_24xx01_remaining_len_in_page(const struct device *dev, off_t offset,
						  size_t len)
{
	const struct eeprom_24xx01_config *cfg = dev->config;
	off_t page_offset = offset % cfg->pagesize;
	size_t remaining_len = cfg->pagesize - page_offset;

	if (remaining_len > len) {
		remaining_len = len;
	}

	return remaining_len;
}

static int eeprom_24xx01_read(const struct device *dev, off_t offset, void *buf, size_t len)
{
	const struct eeprom_24xx01_config *cfg = dev->config;
	struct eeprom_24xx01_data *data = dev->data;
	uint8_t *data_buf = (uint8_t *)buf;
	int ret;

	if (offset + len > cfg->size) {
		LOG_ERR("attempt to read past device boundary");
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	while (len) {
		size_t len_in_page = eeprom_24xx01_remaining_len_in_page(dev, offset, len);

		eeprom_24xx01_wait_ready(dev);

		ret = i2c_write_read_dt(&cfg->i2c, &offset, 1, data_buf, len_in_page);
		if (ret < 0) {
			LOG_ERR("Failed to read EEPROM (%d)", ret);
			k_mutex_unlock(&data->lock);
			return ret;
		}

		len -= len_in_page;
		data_buf += len_in_page;
		offset += len_in_page;
	}

	k_mutex_unlock(&data->lock);
	return 0;
}

static int eeprom_24xx01_i2c_write(const struct device *dev, uint16_t i2c_addr, uint8_t *addr,
				   uint8_t *buf, size_t len)
{
	const struct eeprom_24xx01_config *cfg = dev->config;
	struct i2c_msg msgs[2];

	msgs[0].buf = addr;
	msgs[0].len = 1;
	msgs[0].flags = I2C_MSG_WRITE;

	msgs[1].buf = buf;
	msgs[1].len = len;
	msgs[1].flags = I2C_MSG_WRITE | I2C_MSG_STOP;

	return i2c_transfer_dt(&cfg->i2c, &msgs[0], 2);
}

static int eeprom_24xx01_write(const struct device *dev, off_t offset, const void *buf, size_t len)
{
	const struct eeprom_24xx01_config *cfg = dev->config;
	struct eeprom_24xx01_data *data = dev->data;
	uint8_t *data_buf = (uint8_t *)buf;
	int ret;

	if (cfg->readonly) {
		LOG_ERR("attempt to write to read-only device");
		return -EACCES;
	}

	if (offset + len > cfg->size) {
		LOG_ERR("attempt to write past device boundary");
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	ret = eeprom_24xx01_write_protect_set(dev, 0);
	if (ret < 0) {
		LOG_ERR("failed to write-enable EEPROM (%d)", ret);
		goto end;
	}

	while (len) {
		size_t len_in_page = eeprom_24xx01_remaining_len_in_page(dev, offset, len);

		eeprom_24xx01_wait_ready(dev);

		ret = eeprom_24xx01_i2c_write(dev, cfg->i2c.addr, (uint8_t *)&offset, data_buf,
					      len_in_page);
		if (ret < 0) {
			LOG_ERR("failed to write to EEPROM (%d)", ret);
			goto end;
		}

		data->next_read_write_timepoint = sys_timepoint_calc(K_MSEC(cfg->write_cycle_time));

		len -= len_in_page;
		data_buf += len_in_page;
		offset += len_in_page;
	}

end:
	eeprom_24xx01_write_protect_set(dev, 1);
	k_mutex_unlock(&data->lock);

	return ret;
}

static size_t eeprom_24xx01_get_size(const struct device *dev)
{
	const struct eeprom_24xx01_config *cfg = dev->config;

	return cfg->size;
}

static const struct eeprom_driver_api eeprom_24xx01_driver_api = {
	.read = eeprom_24xx01_read,
	.write = eeprom_24xx01_write,
	.size = eeprom_24xx01_get_size,
};

static int eeprom_24xx01_init(const struct device *dev)
{
	const struct eeprom_24xx01_config *cfg = dev->config;
	struct eeprom_24xx01_data *data = dev->data;

	k_mutex_init(&data->lock);

	if (!i2c_is_ready_dt(&cfg->i2c)) {
		LOG_ERR("i2c bus device not ready");
		return -ENODEV;
	}

	if (cfg->wp_gpio.port) {
		int ret;

		if (!gpio_is_ready_dt(&cfg->wp_gpio)) {
			LOG_ERR("wp gpio device not ready");
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(&cfg->wp_gpio, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			LOG_ERR("failed to configure WP GPIO pin (%d)", ret);
			return ret;
		}
	}
	return 0;
}

#define EEPROM_24XX01_DEFINE(inst)                                                                 \
	static struct eeprom_24xx01_data eeprom_24xx01_data_##inst;                                \
                                                                                                   \
	static const struct eeprom_24xx01_config eeprom_24xx01_config_##inst = {                   \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.wp_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, wp_gpios, {0}),                          \
		.size = DT_INST_PROP(inst, size),                                                  \
		.pagesize = DT_INST_PROP(inst, pagesize),                                          \
		.write_cycle_time = DT_INST_PROP(inst, write_cycle_time),                          \
		.readonly = DT_INST_PROP(inst, read_only)};                                        \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, eeprom_24xx01_init, NULL, &eeprom_24xx01_data_##inst,          \
			      &eeprom_24xx01_config_##inst, POST_KERNEL,                           \
			      CONFIG_EEPROM_INIT_PRIORITY, &eeprom_24xx01_driver_api);

DT_INST_FOREACH_STATUS_OKAY(EEPROM_24XX01_DEFINE)
