/*
 * Copyright (c) 2024 SILA Embedded Solutions GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

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
	uint8_t address_length;
	bool readonly;
};

struct m24xxx_data {
	struct k_mutex lock;
};

static int m24xxx_write_protect_set(const struct device *dev, bool value)
{
	const struct m24xxx_config *config = dev->config;

	if (config->wp_gpio.port == NULL) {
		return 0;
	}

	return gpio_pin_set_dt(&config->wp_gpio, value ? 1 : 0);
}

static int m24xxx_init(const struct device *dev)
{
	const struct m24xxx_config *config = dev->config;
	struct m24xxx_data *data = dev->data;
	int result;

	k_mutex_init(&data->lock);

	if (!i2c_is_ready_dt(&config->i2c)) {
		LOG_ERR("%s: I2C bus device not ready", dev->name);
		return -EINVAL;
	}

	if (config->wp_gpio.port) {
		if (!gpio_is_ready_dt(&config->wp_gpio)) {
			LOG_ERR("%s: wp gpio device not ready", dev->name);
			return -EINVAL;
		}

		result = gpio_pin_configure_dt(&config->wp_gpio, GPIO_OUTPUT_ACTIVE);

		if (result != 0) {
			LOG_ERR("%s: failed to configure WP GPIO pin, error %i", dev->name, result);
			return result;
		}
	}

	return 0;
}

static void m24xxx_calculate_address(const struct device *dev, uint16_t *i2c_address,
				     uint8_t byte_address[2], off_t offset)
{
	const struct m24xxx_config *config = dev->config;

	*i2c_address = config->i2c.addr | (offset >> (config->address_length * 8));
	sys_put_be16(offset, byte_address);
}

static size_t m24xxx_calculate_read_block_length(const struct device *dev, off_t offset, size_t len)
{
	const struct m24xxx_config *config = dev->config;
	off_t offset_masked =
		GENMASK((config->address_length + 1) * 8, config->address_length * 8) & offset;
	off_t next_start = offset_masked + (1 << (config->address_length * 8));

	if (offset + len < next_start) {
		return len;
	} else {
		return next_start - offset;
	}
}

static size_t m24xxx_calculate_write_block_length(const struct device *dev, off_t offset,
						  size_t len)
{
	const struct m24xxx_config *config = dev->config;
	off_t offset_masked = offset - (offset % config->pagesize);
	off_t next_start = offset_masked + config->pagesize;

	if (offset + len < next_start) {
		return len;
	} else {
		return next_start - offset;
	}
}

static int m24xxx_read_internal(const struct device *dev, off_t offset, uint8_t *buf, size_t len)
{
	const struct m24xxx_config *config = dev->config;
	int result;
	uint16_t i2c_address;
	uint8_t byte_address[2];
	size_t block_length;
	size_t byte_address_buffer_offset = ARRAY_SIZE(byte_address) - config->address_length;

	while (len > 0) {
		m24xxx_calculate_address(dev, &i2c_address, byte_address, offset);
		block_length = m24xxx_calculate_read_block_length(dev, offset, len);
		LOG_DBG("%s: reading from offset 0x%04X %i bytes", dev->name, offset, block_length);

		result = i2c_write_read(dev, i2c_address, byte_address + byte_address_buffer_offset,
					config->address_length, buf, block_length);
		if (result < 0) {
			LOG_ERR("%s: read at offset 0x%04X with a length of %i bytes failed with "
				"error %i",
				dev->name, offset, block_length, result);
			return result;
		}

		len -= block_length;
		offset += block_length;
		buf += block_length;
	}

	return 0;
}

static int m24xxx_read(const struct device *dev, off_t offset, void *buf, size_t len)
{
	const struct m24xxx_config *config = dev->config;
	struct m24xxx_data *data = dev->data;
	int result;

	if (offset + len >= config->size) {
		LOG_ERR("%s: read of EEPROM would be out of bounds", dev->name);
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	result = m24xxx_read_internal(dev, offset, buf, len);
	k_mutex_unlock(&data->lock);

	return result;
}

static int m24xxx_write_internal(const struct device *dev, off_t offset, const uint8_t *buf,
				 size_t len)
{
	const struct m24xxx_config *config = dev->config;
	int result;
	uint16_t i2c_address;
	uint8_t byte_address[2];
	size_t block_length;
	size_t byte_address_buffer_offset = ARRAY_SIZE(byte_address) - config->address_length;
	struct i2c_msg messages[] = {
		{ .flags = I2C_MSG_WRITE },
		{ .flags = I2C_MSG_WRITE | I2C_MSG_STOP, }
	};

	while (len > 0) {
		m24xxx_calculate_address(dev, &i2c_address, byte_address, offset);
		block_length = m24xxx_calculate_write_block_length(dev, offset, len);

		messages[0].buf = byte_address + byte_address_buffer_offset;
		messages[0].len = config->address_length;
		messages[1].buf = (uint8_t *)buf;
		messages[1].len = block_length;

		LOG_DBG("%s: writing to offset 0x%04X %i bytes", dev->name, offset, block_length);

		result = i2c_transfer(dev, messages, ARRAY_SIZE(messages), i2c_address);
		if (result < 0) {
			LOG_ERR("%s: write of data for offset 0x%04X and length of %i bytes failed "
				"with error %i",
				dev->name, offset, block_length, result);
			return result;
		}

		len -= block_length;
		offset += block_length;
		buf += block_length;
	}

	return 0;
}

static int m24xxx_write(const struct device *dev, off_t offset, const void *buf, size_t len)
{
	const struct m24xxx_config *config = dev->config;
	struct m24xxx_data *data = dev->data;
	int result;

	if (config->readonly) {
		LOG_ERR("%s: attempt to write to read-only device", dev->name);
		return -EACCES;
	}

	if (offset + len >= config->size) {
		LOG_ERR("%s: write of EEPROM would be out of bounds", dev->name);
		return -EINVAL;
	}

	result = m24xxx_write_protect_set(dev, false);
	if (result != 0) {
		LOG_ERR("%s: failed to disable write protection for EEPROM, error %i", dev->name,
			result);
		return result;
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	result = m24xxx_write_internal(dev, offset, buf, len);
	k_mutex_unlock(&data->lock);
	if (result != 0) {
		return result;
	}

	result = m24xxx_write_protect_set(dev, true);
	if (result != 0) {
		LOG_ERR("%s: failed to enable write protection for EEPROM, error %i", dev->name,
			result);
		return result;
	}

	return 0;
}

static size_t m24xxx_get_size(const struct device *dev)
{
	const struct m24xxx_config *config = dev->config;

	return config->size;
}

static const struct eeprom_driver_api m24xxx_driver_api = {
	.read = &m24xxx_read,
	.write = &m24xxx_write,
	.size = &m24xxx_get_size,
};

#define EEPROM_M24XX_INST_DEFINE(index, name, pagesize_, size_, address_length_)                   \
	static struct m24xxx_data m24xxx_data_##name##_##index;                                    \
                                                                                                   \
	static const struct m24xxx_config m24xxx_config_##name##_##index = {                       \
		.i2c = I2C_DT_SPEC_INST_GET(index),                                                \
		IF_ENABLED(DT_INST_NODE_HAS_PROP(index, wp_gpios),                                 \
			   (.wp_gpio = GPIO_DT_SPEC_INST_GET(index, wp_gpios),))                   \
			.size = size_,                                                             \
		.pagesize = pagesize_,                                                             \
		.readonly = DT_INST_PROP(index, read_only),                                        \
		.address_length = address_length_,                                                 \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(index, m24xxx_init, NULL, &m24xxx_data_##name##_##index,             \
			      &m24xxx_config_##name##_##index, POST_KERNEL,                        \
			      CONFIG_EEPROM_INIT_PRIORITY, &m24xxx_driver_api);

#define DT_DRV_COMPAT st_m24m02_a125
#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)
#define EEPROM_M24M02_PAGESIZE       256
#define EEPROM_M24M02_SIZE           262144
#define EEPROM_M24M02_ADDRESS_LENGTH 2
DT_INST_FOREACH_STATUS_OKAY_VARGS(EEPROM_M24XX_INST_DEFINE, DT_DRV_COMPAT, EEPROM_M24M02_PAGESIZE,
				  EEPROM_M24M02_SIZE, EEPROM_M24M02_ADDRESS_LENGTH)
#endif
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT st_m24m01_a125
#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)
#define EEPROM_M24M01_PAGESIZE       256
#define EEPROM_M24M01_SIZE           131072
#define EEPROM_M24M01_ADDRESS_LENGTH 2
DT_INST_FOREACH_STATUS_OKAY_VARGS(EEPROM_M24XX_INST_DEFINE, DT_DRV_COMPAT, EEPROM_M24M01_PAGESIZE,
				  EEPROM_M24M01_SIZE, EEPROM_M24M01_ADDRESS_LENGTH)
#endif
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT st_m24c64_a125
#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)
#define EEPROM_M24C64_PAGESIZE       32
#define EEPROM_M24C64_SIZE           8192
#define EEPROM_M24C64_ADDRESS_LENGTH 2
DT_INST_FOREACH_STATUS_OKAY_VARGS(EEPROM_M24XX_INST_DEFINE, DT_DRV_COMPAT, EEPROM_M24C64_PAGESIZE,
				  EEPROM_M24C64_SIZE, EEPROM_M24C64_ADDRESS_LENGTH)
#endif
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT st_m24c32_a125
#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)
#define EEPROM_M24C32_PAGESIZE       32
#define EEPROM_M24C32_SIZE           4096
#define EEPROM_M24C32_ADDRESS_LENGTH 2
DT_INST_FOREACH_STATUS_OKAY_VARGS(EEPROM_M24XX_INST_DEFINE, DT_DRV_COMPAT, EEPROM_M24C32_PAGESIZE,
				  EEPROM_M24C32_SIZE, EEPROM_M24C32_ADDRESS_LENGTH)
#endif
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT st_m24c16_a125
#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)
#define EEPROM_M24C16_PAGESIZE       16
#define EEPROM_M24C16_SIZE           2048
#define EEPROM_M24C16_ADDRESS_LENGTH 1
DT_INST_FOREACH_STATUS_OKAY_VARGS(EEPROM_M24XX_INST_DEFINE, DT_DRV_COMPAT, EEPROM_M24C16_PAGESIZE,
				  EEPROM_M24C16_SIZE, EEPROM_M24C16_ADDRESS_LENGTH)
#endif
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT st_m24c08_a125
#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)
#define EEPROM_M24C08_PAGESIZE       16
#define EEPROM_M24C08_SIZE           1024
#define EEPROM_M24C08_ADDRESS_LENGTH 1
DT_INST_FOREACH_STATUS_OKAY_VARGS(EEPROM_M24XX_INST_DEFINE, DT_DRV_COMPAT, EEPROM_M24C08_PAGESIZE,
				  EEPROM_M24C08_SIZE, EEPROM_M24C08_ADDRESS_LENGTH)
#endif
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT st_m24c04_a125
#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)
#define EEPROM_M24C04_PAGESIZE       16
#define EEPROM_M24C04_SIZE           512
#define EEPROM_M24C04_ADDRESS_LENGTH 1
DT_INST_FOREACH_STATUS_OKAY_VARGS(EEPROM_M24XX_INST_DEFINE, DT_DRV_COMPAT, EEPROM_M24C04_PAGESIZE,
				  EEPROM_M24C04_SIZE, EEPROM_M24C04_ADDRESS_LENGTH)
#endif
#undef DT_DRV_COMPAT
