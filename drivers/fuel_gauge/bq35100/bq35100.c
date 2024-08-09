/*
 * Copyright (c) 2024 Orgatex GmbH
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT ti_bq35100

#include "bq35100.h"

#include <zephyr/kernel.h>
#include <zephyr/drivers/fuel_gauge.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(BQ35100);

#define BQ35100_MAC_DATA_LEN     32
#define BQ35100_MAC_OVERHEAD_LEN 4 /* 2 cmd bytes, 1 length byte, 1 checksum byte */
#define BQ35100_MAC_COMPLETE_LEN (BQ35100_MAC_DATA_LEN + BQ35100_MAC_OVERHEAD_LEN)

/**
 * @brief Reads a 16-bit value from the BQ35100 device over I2C.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param reg The register address to read from.
 * @param value Pointer to the variable where the read value will be stored.
 *
 * @return 0 on success, negative error code otherwise.
 */
static int bq35100_read16(const struct device *dev, uint8_t reg, uint16_t *value)
{
	/* Buffer to store the read data */
	uint8_t i2c_data[2];

	/* Pointer to the device's configuration */
	const struct bq35100_config *cfg = dev->config;

	/* Read the 16-bit value from the device */
	const int status = i2c_burst_read_dt(&cfg->i2c, reg, i2c_data, sizeof(i2c_data));

	/* Check if the read operation was successful */
	if (status < 0) {
		/* Log the error */
		LOG_ERR("Unable to read register");
		/* Return the error code */
		return status;
	}
	/* Store the read value in the provided variable */
	*value = sys_get_le16(i2c_data);

	/* Return success */
	return 0;
}

/**
 * @brief Reads a 32-bit value from the BQ35100 device over I2C.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param reg The register address to read from.
 * @param value Pointer to the variable where the read value will be stored.
 *
 * @return 0 on success, negative error code otherwise.
 */
static int bq35100_read32(const struct device *dev, uint8_t reg, uint32_t *value)
{
	/* Buffer to store the read data */
	uint8_t i2c_data[4];

	/* Pointer to the device's configuration */
	const struct bq35100_config *cfg = dev->config;

	/* Read the 32-bit value from the device */
	const int status = i2c_burst_read_dt(&cfg->i2c, reg, i2c_data, sizeof(i2c_data));

	/* Check if the read operation was successful */
	if (status < 0) {
		/* Log the error */
		LOG_ERR("Unable to read register");
		/* Return the error code */
		return status;
	}

	/* Store the read value in the provided variable */
	*value = sys_get_le32(i2c_data);

	/* Return success */
	return 0;
}

/**
 * @brief Writes a 16-bit value to the BQ35100 device over I2C.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param reg The register address to write to.
 * @param value The 16-bit value to write.
 *
 * @return 0 on success, negative error code otherwise.
 */
static int bq35100_write16(const struct device *dev, uint8_t reg, uint16_t value)
{
	/* Buffer to store the data to be written */
	uint8_t buf[3];

	/* Pointer to the device's configuration */
	const struct bq35100_config *cfg = dev->config;

	/* Write the register address to the buffer */
	buf[0] = reg;

	/* Write the 16-bit value to the buffer in little-endian format */
	sys_put_le16(value, &buf[1]);

	/* Write the buffer to the device over I2C */
	return i2c_write_dt(&cfg->i2c, buf, sizeof(buf));
}

static int bq35100_read_mac(const struct device *dev, uint16_t cmd, uint8_t *data, int len)
{
	if (len > BQ35100_MAC_DATA_LEN) {
		return -EINVAL;
	}

	uint8_t buf[BQ35100_MAC_COMPLETE_LEN];
	const struct bq35100_config *cfg = dev->config;

	int ret = bq35100_write16(dev, BQ35100_REG_MANUFACTURER_ACCESS_CONTROL, cmd);

	if (ret != 0) {
		return ret;
	}

	/*
	 * The data read from BQ35100_REG_CONTROL_STATUS is:
	 * 0..1: The command (for verification)
	 * 2..33: The data
	 * 34: Checksum calculated as (uint8_t)(0xFF - (sum of all command and data bytes))
	 * 35: Length including command, checksum and length (e.g. data length + 4)
	 */

	ret = i2c_burst_read_dt(&cfg->i2c, BQ35100_REG_MANUFACTURER_ACCESS_CONTROL, buf,
				BQ35100_MAC_COMPLETE_LEN);
	if (ret != 0) {
		return ret;
	}

	/* The first two bytes read is the command and is used for verification */
	const uint16_t read_cmd = sys_get_le16(buf);

	if (read_cmd != cmd) {
		LOG_ERR("Read command 0x%x != written command 0x%x", read_cmd, cmd);
		return -EIO;
	}

	const uint8_t checksum_actual = buf[34];
	uint8_t sum = 0; /* Intentionally 8 bit wide and overflowing */

	for (int i = 0; i < BQ35100_MAC_COMPLETE_LEN - 2; i++) {
		sum += buf[i];
	}

	const uint8_t checksum_expected = 0xFF - sum;

	if (checksum_expected != checksum_actual) {
		LOG_ERR("Checksum mismatch");
		return -EIO;
	}

	/* First byte of the user buffer is the length */
	data[0] = buf[35] - BQ35100_MAC_OVERHEAD_LEN;

	/* Copy only the data to the user buffer (= skipping the first two command bytes) */
	memcpy(&data[1], &buf[2], len);

	return ret;
}

static int bq35100_get_prop(const struct device *dev, fuel_gauge_prop_t prop,
			    union fuel_gauge_prop_val *val)
{
	int rc = 0;
	uint16_t tmp_val = 0;
	uint32_t tmp_val32 = 0;

	switch (prop) {
	case FUEL_GAUGE_CURRENT:
		rc = bq35100_read16(dev, BQ35100_REG_CURRENT, &tmp_val);

		// #TODO adjust conversion factor
		val->current = (int16_t)tmp_val * 1000;
		break;
	case FUEL_GAUGE_REMAINING_CAPACITY:
		rc = bq35100_read32(dev, BQ35100_REG_ACCUMULATED_CAPACITY, &tmp_val32);

		// #TODO adjust conversion factor
		val->remaining_capacity = tmp_val32 * 1000;
		break;
	case FUEL_GAUGE_SBS_MFR_ACCESS:
		rc = bq35100_read16(dev, BQ35100_REG_CONTROL_STATUS, &tmp_val);

		val->sbs_mfr_access_word = tmp_val;
		break;
	case FUEL_GAUGE_RELATIVE_STATE_OF_CHARGE:
		rc = bq35100_read16(dev, BQ35100_REG_ACCUMULATED_CAPACITY, &tmp_val);

		// #TODO adjust conversion factor and convert to %
		val->relative_state_of_charge = tmp_val;
		break;
	case FUEL_GAUGE_VOLTAGE:
		rc = bq35100_read16(dev, BQ35100_REG_VOLTAGE, &tmp_val);

		// #TODO adjust conversion factor
		val->voltage = tmp_val;
		break;
	case FUEL_GAUGE_TEMPERATURE:
		rc = bq35100_read16(dev, BQ35100_REG_TEMPERATURE, &tmp_val);

		// #TODO adjust conversion factor
		val->temperature = tmp_val * 10;
		break;
	case FUEL_GAUGE_DESIGN_CAPACITY:
		rc = bq35100_read16(dev, BQ35100_REG_DESIGN_CAPACITY, &tmp_val);

		// #TODO adjust conversion factor
		val->design_cap = tmp_val;
		break;
	default:
		rc = -ENOTSUP;
	}

	return rc;
}

static int bq35100_set_prop(const struct device *dev, fuel_gauge_prop_t prop,
			    union fuel_gauge_prop_val val)
{
	int rc = 0;
	uint16_t tmp_val = 0;

	switch (prop) {
		// #TODO add stuff that can be set like design capacity and
		// accumulated charge and new batteries and stuff
	default:
		rc = -ENOTSUP;
	}

	return rc;
}

static int bq35100_init(const struct device *dev)
{
	const struct bq35100_config *cfg;

	cfg = dev->config;

	if (!device_is_ready(cfg->i2c.bus)) {
		LOG_ERR("Bus device is not ready");
		return -ENODEV;
	}

	return 0;
}

static const struct fuel_gauge_driver_api bq35100_driver_api = {
	.get_property = &bq35100_get_prop,
	.set_property = &bq35100_set_prop,
};

#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 0
#error "BQ35100 device is not defined in DTS"
#endif

#define BQ35100_INIT(index)                                                                        \
                                                                                                   \
	static const struct bq35100_config bq35100_config_##index = {                              \
		.i2c = I2C_DT_SPEC_INST_GET(index),                                                \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(index, &bq35100_init, NULL, NULL, &bq35100_config_##index,           \
			      POST_KERNEL, CONFIG_FUEL_GAUGE_INIT_PRIORITY, &bq35100_driver_api);

DT_INST_FOREACH_STATUS_OKAY(BQ35100_INIT)
