/*
 * Copyright (c) 2023, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_mcp7940n

#include <string.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/bbram.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(bbram_microchip_mcp7940n, CONFIG_BBRAM_LOG_LEVEL);

#define MICROCHIP_MCP7940N_SRAM_OFFSET 0x20
#define MICROCHIP_MCP7940N_SRAM_SIZE 64
#define MICROCHIP_MCP7940N_RTCWKDAY_REGISTER_ADDRESS 0x03
#define MICROCHIP_MCP7940N_RTCWKDAY_VBATEN_BIT BIT(3)
#define MICROCHIP_MCP7940N_RTCWKDAY_PWRFAIL_BIT BIT(4)

struct microchip_mcp7940n_bbram_data {
	struct k_mutex lock;
};

struct microchip_mcp7940n_bbram_config {
	struct i2c_dt_spec i2c;
};

static int microchip_mcp7940n_bbram_init(const struct device *dev)
{
	const struct microchip_mcp7940n_bbram_config *config = dev->config;
	struct microchip_mcp7940n_bbram_data *data = dev->data;
	int32_t rc = 0;
	uint8_t buffer;

	if (!device_is_ready(config->i2c.bus)) {
		LOG_ERR("I2C device %s is not ready", config->i2c.bus->name);
		return -ENODEV;
	}

	k_mutex_init(&data->lock);

	rc = i2c_reg_read_byte_dt(&config->i2c,
				  MICROCHIP_MCP7940N_RTCWKDAY_REGISTER_ADDRESS,
				  &buffer);

	if (rc != 0) {
		LOG_ERR("Failed to read RTCWKDAY register: %d", rc);
	}

	return rc;
}

static int microchip_mcp7940n_bbram_size(const struct device *dev, size_t *size)
{
	*size = MICROCHIP_MCP7940N_SRAM_SIZE;

	return 0;
}

static int microchip_mcp7940n_bbram_is_invalid(const struct device *dev)
{
	const struct microchip_mcp7940n_bbram_config *config = dev->config;
	struct microchip_mcp7940n_bbram_data *data = dev->data;
	int32_t rc = 0;
	uint8_t buffer;
	bool data_valid = true;

	k_mutex_lock(&data->lock, K_FOREVER);

	rc = i2c_reg_read_byte_dt(&config->i2c,
				  MICROCHIP_MCP7940N_RTCWKDAY_REGISTER_ADDRESS,
				  &buffer);


	if ((buffer & MICROCHIP_MCP7940N_RTCWKDAY_PWRFAIL_BIT)) {
		data_valid = false;

		buffer &= (buffer ^ MICROCHIP_MCP7940N_RTCWKDAY_PWRFAIL_BIT);

		rc = i2c_reg_write_byte_dt(&config->i2c,
					   MICROCHIP_MCP7940N_RTCWKDAY_REGISTER_ADDRESS,
					   buffer);

		if (rc != 0) {
			LOG_ERR("Failed to write RTCWKDAY register: %d", rc);
			goto finish;
		}

	}

finish:
	k_mutex_unlock(&data->lock);

	if (rc == 0 && data_valid == true) {
		rc = 1;
	}

	return rc;
}

static int microchip_mcp7940n_bbram_check_standby_power(const struct device *dev)
{
	const struct microchip_mcp7940n_bbram_config *config = dev->config;
	struct microchip_mcp7940n_bbram_data *data = dev->data;
	int32_t rc = 0;
	uint8_t buffer;
	bool power_enabled = true;

	k_mutex_lock(&data->lock, K_FOREVER);

	rc = i2c_reg_read_byte_dt(&config->i2c,
				  MICROCHIP_MCP7940N_RTCWKDAY_REGISTER_ADDRESS,
				  &buffer);


	if (!(buffer & MICROCHIP_MCP7940N_RTCWKDAY_VBATEN_BIT)) {
		power_enabled = false;

		buffer |= MICROCHIP_MCP7940N_RTCWKDAY_VBATEN_BIT;

		rc = i2c_reg_write_byte_dt(&config->i2c,
					   MICROCHIP_MCP7940N_RTCWKDAY_REGISTER_ADDRESS,
					   buffer);

		if (rc != 0) {
			LOG_ERR("Failed to write RTCWKDAY register: %d", rc);
			goto finish;
		}

	}

finish:
	k_mutex_unlock(&data->lock);

	if (rc == 0 && power_enabled == true) {
		rc = 1;
	}

	return rc;
}

static int microchip_mcp7940n_bbram_read(const struct device *dev, size_t offset, size_t size,
					 uint8_t *buffer)
{
	const struct microchip_mcp7940n_bbram_config *config = dev->config;
	struct microchip_mcp7940n_bbram_data *data = dev->data;
	size_t i = 0;
	int32_t rc = 0;

	if ((offset + size) > MICROCHIP_MCP7940N_SRAM_SIZE) {
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	while (i < size) {
		LOG_DBG("Read from 0x%x", (MICROCHIP_MCP7940N_SRAM_OFFSET + offset + i));
		rc = i2c_reg_read_byte_dt(&config->i2c,
					  (MICROCHIP_MCP7940N_SRAM_OFFSET + offset + i),
					  &buffer[i]);

		if (rc != 0) {
			goto finish;
		}

		++i;
	}

finish:
	k_mutex_unlock(&data->lock);

	return rc;
}

static int microchip_mcp7940n_bbram_write(const struct device *dev, size_t offset, size_t size,
					  const uint8_t *buffer)
{
	const struct microchip_mcp7940n_bbram_config *config = dev->config;
	struct microchip_mcp7940n_bbram_data *data = dev->data;
	size_t i = 0;
	int32_t rc = 0;

	if ((offset + size) > MICROCHIP_MCP7940N_SRAM_SIZE) {
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	while (i < size) {
		LOG_DBG("Write 0x%x to 0x%x", buffer[i],
			(MICROCHIP_MCP7940N_SRAM_OFFSET + offset + i));
		rc = i2c_reg_write_byte_dt(&config->i2c,
					   (MICROCHIP_MCP7940N_SRAM_OFFSET + offset + i),
					   buffer[i]);

		if (rc != 0) {
			goto finish;
		}

		++i;
	}

finish:
	k_mutex_unlock(&data->lock);

	return rc;
}

static const struct bbram_driver_api microchip_mcp7940n_bbram_api = {
	.get_size = microchip_mcp7940n_bbram_size,
	.check_invalid = microchip_mcp7940n_bbram_is_invalid,
	.check_standby_power = microchip_mcp7940n_bbram_check_standby_power,
	.read = microchip_mcp7940n_bbram_read,
	.write = microchip_mcp7940n_bbram_write,
};

#define MICROCHIP_MCP7940N_BBRAM_DEVICE(inst)							\
	static struct microchip_mcp7940n_bbram_data microchip_mcp7940n_bbram_data_##inst;	\
	static const struct microchip_mcp7940n_bbram_config					\
		microchip_mcp7940n_bbram_config_##inst = {					\
		.i2c = I2C_DT_SPEC_INST_GET(inst),						\
	};											\
	DEVICE_DT_INST_DEFINE(inst,								\
			      &microchip_mcp7940n_bbram_init,					\
			      NULL,								\
			      &microchip_mcp7940n_bbram_data_##inst,				\
			      &microchip_mcp7940n_bbram_config_##inst,				\
			      POST_KERNEL,							\
			      CONFIG_BBRAM_INIT_PRIORITY,					\
			      &microchip_mcp7940n_bbram_api);

DT_INST_FOREACH_STATUS_OKAY(MICROCHIP_MCP7940N_BBRAM_DEVICE)
