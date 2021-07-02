/*
 * Copyright (c) 2021 Vossloh
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_m24c16_eeprom

#include <drivers/eeprom.h>
#include <drivers/gpio.h>
#include <drivers/i2c.h>
#include <sys/byteorder.h>
#include <zephyr.h>

#define LOG_LEVEL CONFIG_EEPROM_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(eeprom_m24c16);

#define GET_M24C16_I2C_DEV_ADDR(dev_type_id, eeprom_addr) (((dev_type_id) >> 1) \
		+ (((eeprom_addr) >> 8) & BIT_MASK(3)))

struct eeprom_m24c16_config {
	const char *bus_dev_name;
	uint16_t bus_addr;
	size_t size;
	size_t pagesize;
	uint8_t addr_width;
	uint16_t timeout;
	gpio_pin_t wc_gpio_pin;
	gpio_dt_flags_t wc_gpio_flags;
	const char *wc_gpio_name;
};

struct eeprom_m24c16_data {
	const struct device *bus_dev;
	const struct device *wc_gpio_dev;
	struct k_mutex lock;
};

static size_t eeprom_m24c16_adjust_read_count(const struct device *dev,
		off_t offset, size_t len)
{
	const struct eeprom_m24c16_config *config = dev->config;
	const size_t remainder = BIT(config->addr_width) - offset;

	if (len > remainder) {
		len = remainder;
	}

	return len;
}

static int eeprom_m24c16_read_max_bytes(const struct device *dev, off_t offset, void *buf,
		size_t len)
{
	const struct eeprom_m24c16_config *config = dev->config;
	struct eeprom_m24c16_data *data = dev->data;
	int64_t timeout;
	uint8_t addr[2];
	uint16_t bus_addr;
	int err;

	bus_addr = GET_M24C16_I2C_DEV_ADDR(config->bus_addr, offset);

	addr[0] = offset & BIT_MASK(8);

	len = eeprom_m24c16_adjust_read_count(dev, offset, len);

	/*
	 * A write cycle may be in progress so we try to read until success or
	 * timeout specified by datasheet for write cycle time length.
	 */
	timeout = k_uptime_get() + config->timeout;
	while (1) {
		int64_t now = k_uptime_get();

		err = i2c_write_read(data->bus_dev, bus_addr,
				addr, config->addr_width / 8,
				buf, len);
		if (!err || now > timeout) {
			break;
		}
		k_sleep(K_MSEC(1));
	}

	if (err < 0) {
		return err;
	}

	return len;
}

static int eeprom_m24c16_read(const struct device *dev, off_t offset, void *buf,
		size_t len)
{
	const struct eeprom_m24c16_config *config = dev->config;
	struct eeprom_m24c16_data *data = dev->data;
	uint8_t *pbuf = buf;
	int ret;

	if (!len) {
		return 0;
	}

	if ((offset + len) > config->size) {
		LOG_WRN("attempt to read past device boundary");
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	while (len) {
		ret = eeprom_m24c16_read_max_bytes(dev, offset, pbuf, len);
		if (ret < 0) {
			LOG_ERR("failed to read EEPROM (err %d)", ret);
			k_mutex_unlock(&data->lock);
			return ret;
		}

		pbuf += ret;
		offset += ret;
		len -= ret;
	}

	k_mutex_unlock(&data->lock);

	return 0;
}

static int eeprom_m24c16_write_protect(const struct device *dev)
{
	const struct eeprom_m24c16_config *config = dev->config;
	struct eeprom_m24c16_data *data = dev->data;

	if (!data->wc_gpio_dev) {
		return 0;
	}

	return gpio_pin_set(data->wc_gpio_dev, config->wc_gpio_pin, 1);
}

static int eeprom_m24c16_write_enable(const struct device *dev)
{
	const struct eeprom_m24c16_config *config = dev->config;
	struct eeprom_m24c16_data *data = dev->data;

	if (!data->wc_gpio_dev) {
		return 0;
	}

	return gpio_pin_set(data->wc_gpio_dev, config->wc_gpio_pin, 0);
}

static size_t eeprom_m24c16_limit_write_count(const struct device *dev,
		off_t offset,
		size_t len)
{
	const struct eeprom_m24c16_config *config = dev->config;
	size_t count = len;
	off_t page_boundary;

	/* We can at most write one page at a time */
	if (count > config->pagesize) {
		count = config->pagesize;
	}

	/* Writes can not cross a page boundary */
	page_boundary = ROUND_UP(offset + 1, config->pagesize);
	if (offset + count > page_boundary) {
		count = page_boundary - offset;
	}

	return count;
}

static int eeprom_m24c16_write_page(const struct device *dev, off_t offset,
		const void *buf, size_t len)
{
	const struct eeprom_m24c16_config *config = dev->config;
	struct eeprom_m24c16_data *data = dev->data;
	int count = eeprom_m24c16_limit_write_count(dev, offset, len);
	uint8_t block[config->addr_width / 8 + count];
	int64_t timeout;
	uint16_t bus_addr;
	int i = 0;
	int err;

	bus_addr = GET_M24C16_I2C_DEV_ADDR(config->bus_addr, offset);
	block[i++] = offset & BIT_MASK(8);
	memcpy(&block[i], buf, count);

	/*
	 * A write cycle may be in progress so we try to read until success or
	 * timeout specified by datasheet for write cycle time length.
	 */
	timeout = k_uptime_get() + config->timeout;
	while (1) {
		int64_t now = k_uptime_get();

		err = i2c_write(data->bus_dev, block, sizeof(block),
				bus_addr);
		if (!err || now > timeout) {
			break;
		}
		k_sleep(K_MSEC(1));
	}

	if (err < 0) {
		return err;
	}

	return count;
}

static int eeprom_m24c16_write(const struct device *dev, off_t offset, const void *buf,
		size_t len)
{
	const struct eeprom_m24c16_config *config = dev->config;
	struct eeprom_m24c16_data *data = dev->data;
	const uint8_t *pbuf = buf;
	int ret;

	if (!len) {
		return 0;
	}

	if ((offset + len) > config->size) {
		LOG_WRN("attempt to write past device boundary");
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	ret = eeprom_m24c16_write_enable(dev);
	if (ret) {
		LOG_ERR("failed to write-enable EEPROM (err %d)", ret);
		k_mutex_unlock(&data->lock);
		return ret;
	}

	while (len) {
		ret = eeprom_m24c16_write_page(dev, offset, pbuf, len);
		if (ret < 0) {
			LOG_ERR("failed to write to EEPROM (err %d)", ret);
			eeprom_m24c16_write_protect(dev);
			k_mutex_unlock(&data->lock);
			return ret;
		}

		pbuf += ret;
		offset += ret;
		len -= ret;
	}

	ret = eeprom_m24c16_write_protect(dev);
	if (ret) {
		LOG_ERR("failed to write-protect EEPROM (err %d)", ret);
	}

	k_mutex_unlock(&data->lock);

	return 0;
}

static size_t eeprom_m24c16_size(const struct device *dev)
{
	const struct eeprom_m24c16_config *config = dev->config;

	return config->size;
}

static int eeprom_m24c16_init(const struct device *dev)
{
	const struct eeprom_m24c16_config *config = dev->config;
	struct eeprom_m24c16_data *data = dev->data;
	int err;

	k_mutex_init(&data->lock);

	data->bus_dev = device_get_binding(config->bus_dev_name);
	if (!data->bus_dev) {
		LOG_ERR("could not get parent bus device");
		return -EINVAL;
	}

	if (config->wc_gpio_name) {
		data->wc_gpio_dev = device_get_binding(config->wc_gpio_name);
		if (!data->wc_gpio_dev) {
			LOG_ERR("could not get WP GPIO device");
			return -EINVAL;
		}

		err = gpio_pin_configure(data->wc_gpio_dev, config->wc_gpio_pin,
				GPIO_OUTPUT_ACTIVE | config->wc_gpio_flags);
		if (err) {
			LOG_ERR("failed to configure WP GPIO pin (err %d)",
					err);
			return err;
		}
	}

	return 0;
}

static const struct eeprom_driver_api eeprom_m24c16_api = {
	.read = eeprom_m24c16_read,
	.write = eeprom_m24c16_write,
	.size = eeprom_m24c16_size,
};

static const struct eeprom_m24c16_config eeprom_config = {
	.bus_dev_name = DT_BUS_LABEL(DT_DRV_INST(0)),
	.bus_addr = DT_REG_ADDR(DT_DRV_INST(0)),
	.size = DT_INST_PROP(0, size),
	.pagesize = DT_INST_PROP(0, pagesize),
	.addr_width = DT_INST_PROP(0, address_width),
	.timeout = DT_INST_PROP(0, timeout),
	.wc_gpio_pin = UTIL_AND(DT_NODE_HAS_PROP(DT_DRV_INST(0), wc_gpios),
			DT_GPIO_PIN(DT_DRV_INST(0), wc_gpios)),
	.wc_gpio_name = UTIL_AND(DT_NODE_HAS_PROP(DT_DRV_INST(0), wc_gpios),
			DT_GPIO_LABEL(DT_DRV_INST(0), wc_gpios)),
	.wc_gpio_flags = UTIL_AND(DT_NODE_HAS_PROP(DT_DRV_INST(0), wc_gpios),
			DT_GPIO_FLAGS(DT_DRV_INST(0), wc_gpios)),
};

static struct eeprom_m24c16_data eeprom_data;

DEVICE_DT_INST_DEFINE(0, &eeprom_m24c16_init, device_pm_control_nop, &eeprom_data,
		&eeprom_config, POST_KERNEL,
		CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &eeprom_m24c16_api);
