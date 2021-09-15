/*
 * Copyright (c) 2021 Benjamin S. Scarlet
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Driver for Microchip 24AA02XEXX I2C EEPROMs with EUI node identity.
 */

#include <drivers/eeprom.h>
#include <drivers/i2c.h>
#include <zephyr.h>

#define EEPROM_24AA02XEXX_SIZE 2048

#define DT_DRV_COMPAT microchip_24aa02xexx

struct eeprom_24aa02xexx_config {
	struct i2c_dt_spec i2c;
	uint8_t pagesize;
};

struct eeprom_24aa02xexx_data {
};

static int eeprom_24aa02xexx_read(const struct device *dev, off_t offset,
				  void *buf, size_t len)
{
	const struct eeprom_24aa02xexx_config *const config = dev->config;

	return i2c_burst_read_dt(&config->i2c, offset, buf, len);
}

/* Per the "Acknowledge Polling" section of the data sheet, wait for the chip
 * to ack an I2C write to know when it's done writing an EEPROM page. Send just writes
 * to the address register, not actual data. */
static int eeprom_24aa02xexx_wait_for_ack(const struct device *dev, off_t offset)
{
	const struct eeprom_24aa02xexx_config *const config = dev->config;
	uint8_t offset_byte = offset;

	for (int tries = 0; tries < CONFIG_EEPROM_24AA02XEXX_MAX_WRITE_RETRIES; tries++) {
		k_sleep(K_MSEC(3)); /* data sheet says "Page Write Time 3ms, typical" */
		const int result = i2c_write_dt(&config->i2c, &offset_byte, sizeof(offset_byte));
		if (result != -EIO) {
			return result;
		}
	}
	return -EIO;
}

static int eeprom_24aa02xexx_write(const struct device *dev,
				   off_t offset, const void *data, size_t len)
{
	const struct eeprom_24aa02xexx_config *const config = dev->config;
	const size_t pagesize = config->pagesize;
	const uint8_t *data_bytes = data;

	while (len > 0) {
		const size_t page_space = pagesize - offset % pagesize;
		const size_t write_len = MIN(len, page_space);
		const int error = i2c_burst_write_dt(&config->i2c, offset, data_bytes, write_len);
		if (error) {
			return error;
		}
		len -= write_len;
		data_bytes += write_len;
		offset += write_len;
		/* wait for the write to complete */
		if (eeprom_24aa02xexx_wait_for_ack(dev, offset)) {
			return -EIO;
		}
	}
	return 0;
}

static size_t eeprom_24aa02xexx_size(const struct device *dev)
{
	ARG_UNUSED(dev);
	return EEPROM_24AA02XEXX_SIZE;
}

static int eeprom_24aa02xexx_init(const struct device *dev)
{
	ARG_UNUSED(dev);
	return 0;
}

static const struct eeprom_driver_api eeprom_24aa02xexx_api = {
	.read = eeprom_24aa02xexx_read,
	.write = eeprom_24aa02xexx_write,
	.size = eeprom_24aa02xexx_size,
};

#define EEPROM_24AA02XEXX_INSTATIATE(n)							\
	static const struct eeprom_24aa02xexx_config eeprom_24aa02xexx_##n##_config = {	\
		.i2c = I2C_DT_SPEC_INST_GET(n),						\
		.pagesize = DT_INST_PROP(n, pagesize),					\
	};										\
	static struct eeprom_24aa02xexx_data eeprom_24aa02xexx_##n##_data;		\
	DEVICE_DT_INST_DEFINE(n, &eeprom_24aa02xexx_init,				\
			      NULL, &eeprom_24aa02xexx_##n##_data,			\
			      &eeprom_24aa02xexx_##n##_config, POST_KERNEL,		\
			      CONFIG_EEPROM_24AA02XEXX_INIT_PRIORITY, &eeprom_24aa02xexx_api);

DT_INST_FOREACH_STATUS_OKAY(EEPROM_24AA02XEXX_INSTATIATE)
