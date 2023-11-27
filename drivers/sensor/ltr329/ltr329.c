/*
 * Copyright (c) 2025 Konrad Sikora
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT liteon_ltr329

#include <zephyr/device.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/sensor.h>

LOG_MODULE_REGISTER(LTR329, CONFIG_SENSOR_LOG_LEVEL);

/* Register addresses */
#define LTR329_ALS_CONTR      0x80
#define LTR329_MEAS_RATE      0x85
#define LTR329_PART_ID        0x86
#define LTR329_MANUFAC_ID     0x87
#define LTR329_ALS_DATA_CH1_0 0x88
#define LTR329_ALS_DATA_CH1_1 0x89
#define LTR329_ALS_DATA_CH0_0 0x8A
#define LTR329_ALS_DATA_CH0_1 0x8B
#define LTR329_ALS_STATUS     0x8C

/* Bit masks and shifts for ALS_CONTR register */
#define LTR329_ALS_CONTR_MODE_MASK      BIT(0)
#define LTR329_ALS_CONTR_MODE_SHIFT     0
#define LTR329_ALS_CONTR_SW_RESET_MASK  BIT(1)
#define LTR329_ALS_CONTR_SW_RESET_SHIFT 1
#define LTR329_ALS_CONTR_GAIN_MASK      GENMASK(4, 2)
#define LTR329_ALS_CONTR_GAIN_SHIFT     2

/* Bit masks and shifts for MEAS_RATE register */
#define LTR329_MEAS_RATE_REPEAT_MASK    GENMASK(2, 0)
#define LTR329_MEAS_RATE_REPEAT_SHIFT   0
#define LTR329_MEAS_RATE_INT_TIME_MASK  GENMASK(5, 3)
#define LTR329_MEAS_RATE_INT_TIME_SHIFT 3

/* Bit masks and shifts for PART_ID register */
#define LTR329_PART_ID_REVISION_MASK  GENMASK(3, 0)
#define LTR329_PART_ID_REVISION_SHIFT 0
#define LTR329_PART_ID_NUMBER_MASK    GENMASK(7, 4)
#define LTR329_PART_ID_NUMBER_SHIFT   4

/* Bit masks and shifts for MANUFAC_ID register */
#define LTR329_MANUFAC_ID_IDENTIFICATION_MASK  GENMASK(7, 0)
#define LTR329_MANUFAC_ID_IDENTIFICATION_SHIFT 0

/* Bit masks and shifts for ALS_STATUS register */
#define LTR329_ALS_STATUS_DATA_MASK        GENMASK(7, 0)
#define LTR329_ALS_STATUS_DATA_SHIFT       0
#define LTR329_ALS_STATUS_DATA_READY_MASK  BIT(2)
#define LTR329_ALS_STATUS_DATA_READY_SHIFT 2
#define LTR329_ALS_STATUS_DATA_GAIN_MASK   GENMASK(6, 4)
#define LTR329_ALS_STATUS_DATA_GAIN_SHIFT  4
#define LTR329_ALS_STATUS_DATA_VALID_MASK  BIT(7)
#define LTR329_ALS_STATUS_DATA_VALID_SHIFT 7

/* Expected sensor IDs */
#define LTR329_PART_ID_VALUE         0xA0
#define LTR329_MANUFACTURER_ID_VALUE 0x05

/* Timing definitions - refer to LTR-329ALS-01 datasheet */
#define LTR329_INIT_STARTUP_MS        100
#define LTR329_WAKEUP_FROM_STANDBY_MS 10

/* Macros to set and get register fields */
#define LTR329_REG_SET(reg, field, value)				\
	(((value) << reg##_##field##_SHIFT) & reg##_##field##_MASK)
#define LTR329_REG_GET(reg, field, value)				\
	(((value) & reg##_##field##_MASK) >> reg##_##field##_SHIFT)

struct ltr329_config {
	const struct i2c_dt_spec bus;
	uint8_t gain;
	uint8_t integration_time;
	uint8_t measurement_rate;
};

struct ltr329_data {
	uint16_t ch0;
	uint16_t ch1;
};

static int ltr329_check_device_id(const struct i2c_dt_spec *bus)
{
	uint8_t id;
	int rc;

	rc = i2c_reg_read_byte_dt(bus, LTR329_PART_ID, &id);
	if (rc < 0) {
		LOG_ERR("Failed to read PART_ID");
		return rc;
	}
	if (id != LTR329_PART_ID_VALUE) {
		LOG_ERR("PART_ID mismatch: expected 0x%02X, got 0x%02X", LTR329_PART_ID_VALUE, id);
		return -ENODEV;
	}

	rc = i2c_reg_read_byte_dt(bus, LTR329_MANUFAC_ID, &id);
	if (rc < 0) {
		LOG_ERR("Failed to read MANUFAC_ID");
		return rc;
	}
	if (id != LTR329_MANUFACTURER_ID_VALUE) {
		LOG_ERR("MANUFAC_ID mismatch: expected 0x%02X, got 0x%02X",
			LTR329_MANUFACTURER_ID_VALUE, id);
		return -ENODEV;
	}

	return 0;
}

static int ltr329_init_registers(const struct i2c_dt_spec *bus, const struct ltr329_config *cfg)
{
	const uint8_t control_reg = LTR329_REG_SET(LTR329_ALS_CONTR, MODE, 1) |
				    LTR329_REG_SET(LTR329_ALS_CONTR, GAIN, cfg->gain);
	const uint8_t meas_reg = LTR329_REG_SET(LTR329_MEAS_RATE, REPEAT, cfg->measurement_rate) |
				 LTR329_REG_SET(LTR329_MEAS_RATE, INT_TIME, cfg->integration_time);
	uint8_t buffer;
	int rc;

	rc = i2c_reg_write_byte_dt(bus, LTR329_ALS_CONTR, control_reg);
	if (rc < 0) {
		LOG_ERR("Failed to set ALS_CONTR register");
		return rc;
	}

	rc = i2c_reg_write_byte_dt(bus, LTR329_MEAS_RATE, meas_reg);
	if (rc < 0) {
		LOG_ERR("Failed to set MEAS_RATE register");
		return rc;
	}

	/* Read back the MEAS_RATE register to verify the settings */
	rc = i2c_reg_read_byte_dt(bus, LTR329_MEAS_RATE, &buffer);
	if (rc < 0) {
		LOG_ERR("Failed to read back MEAS_RATE register");
		return rc;
	}
	if (LTR329_REG_GET(LTR329_MEAS_RATE, REPEAT, buffer) != cfg->measurement_rate) {
		LOG_ERR("Measurement rate mismatch: expected %u, got %u", cfg->measurement_rate,
			(uint8_t)LTR329_REG_GET(LTR329_MEAS_RATE, REPEAT, buffer));
		return -ENODEV;
	}
	if (LTR329_REG_GET(LTR329_MEAS_RATE, INT_TIME, buffer) != cfg->integration_time) {
		LOG_ERR("Integration time mismatch: expected %u, got %u", cfg->integration_time,
			(uint8_t)LTR329_REG_GET(LTR329_MEAS_RATE, INT_TIME, buffer));
		return -ENODEV;
	}

	return 0;
}

static int ltr329_init(const struct device *dev)
{
	const struct ltr329_config *cfg = dev->config;
	int rc;

	if (!i2c_is_ready_dt(&cfg->bus)) {
		LOG_ERR("I2C bus not ready");
		return -ENODEV;
	}

	/* Wait for sensor startup */
	k_sleep(K_MSEC(LTR329_INIT_STARTUP_MS));

	rc = ltr329_check_device_id(&cfg->bus);
	if (rc < 0) {
		return rc;
	}

	/* Init register to enable sensor to active mode */
	rc = ltr329_init_registers(&cfg->bus, cfg);
	if (rc < 0) {
		return rc;
	}

	return 0;
}

static int ltr329_check_data_ready(const struct i2c_dt_spec *bus)
{
	uint8_t status;
	int rc;

	rc = i2c_reg_read_byte_dt(bus, LTR329_ALS_STATUS, &status);
	if (rc < 0) {
		LOG_ERR("Failed to read ALS_STATUS register");
		return rc;
	}

	if (!LTR329_REG_GET(LTR329_ALS_STATUS, DATA_READY, status)) {
		LOG_WRN("Data not ready");
		return -EBUSY;
	}

	return 0;
}

static int ltr329_read_als_data(const struct i2c_dt_spec *bus, struct ltr329_data *data)
{
	uint8_t reg = LTR329_ALS_DATA_CH1_0;
	uint8_t buff[4];
	int rc;

	rc = i2c_write_read_dt(bus, &reg, sizeof(reg), buff, sizeof(buff));
	if (rc < 0) {
		LOG_ERR("Failed to read ALS data registers");
		return rc;
	}

	data->ch1 = sys_get_le16(buff);
	data->ch0 = sys_get_le16(buff + 2);

	return 0;
}

static int ltr329_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct ltr329_config *cfg = dev->config;
	struct ltr329_data *data = dev->data;
	int rc;

	if ((chan != SENSOR_CHAN_ALL) && (chan != SENSOR_CHAN_LIGHT)) {
		return -ENOTSUP;
	}

	rc = ltr329_check_data_ready(&cfg->bus);
	if (rc < 0) {
		return rc;
	}

	rc = ltr329_read_als_data(&cfg->bus, data);
	if (rc < 0) {
		return rc;
	}

	return 0;
}

static int ltr329_get_mapped_gain(const uint8_t reg_val, uint8_t *const output)
{
	/* Map the register value to the gain used in the lux calculation
	 * Indices 4 and 5 are invalid.
	 */
	static const uint8_t gain_lux_calc[] = {1, 2, 4, 8, 0, 0, 48, 96};

	if (reg_val < ARRAY_SIZE(gain_lux_calc)) {
		*output = gain_lux_calc[reg_val];
		/* 0 is not a valid value */
		return (*output == 0) ? -EINVAL : 0;
	}

	return -EINVAL;
}

static int ltr329_get_mapped_int_time(const uint8_t reg_val, uint8_t *const output)
{
	/* Map the register value to the integration time used in the lux calculation */
	static const uint8_t int_time_lux_calc[] = {10, 5, 20, 40, 15, 25, 30, 35};

	if (reg_val < ARRAY_SIZE(int_time_lux_calc)) {
		*output = int_time_lux_calc[reg_val];
		return 0;
	}

	*output = 0;
	return -EINVAL;
}

static int ltr329_channel_get(const struct device *dev, enum sensor_channel chan,
			      struct sensor_value *val)
{
	const struct ltr329_data *data = dev->data;
	const struct ltr329_config *cfg = dev->config;
	uint8_t gain_value;
	uint8_t integration_time_value;

	if (chan != SENSOR_CHAN_LIGHT) {
		return -ENOTSUP;
	}

	if (ltr329_get_mapped_gain(cfg->gain, &gain_value) != 0) {
		LOG_ERR("Invalid gain configuration");
		return -EINVAL;
	}

	if (ltr329_get_mapped_int_time(cfg->integration_time, &integration_time_value) != 0) {
		LOG_ERR("Invalid integration time configuration");
		return -EINVAL;
	}

	if ((data->ch0 == 0) && (data->ch1 == 0)) {
		LOG_WRN("Both channels are zero; cannot compute ratio");
		return -EINVAL;
	}

	/* Calculate lux value according to the appendix A of the datasheet. */
	uint64_t lux;
	/* The calculation is scaled by 1000000 to avoid floating point. */
	uint64_t scaled_ratio = (data->ch1 * UINT64_C(1000000)) / (uint64_t)(data->ch0 + data->ch1);

	if (scaled_ratio < UINT64_C(450000)) {
		lux = (UINT64_C(1774300) * data->ch0 + UINT64_C(1105900) * data->ch1);
	} else if (scaled_ratio < UINT64_C(640000)) {
		lux = (UINT64_C(4278500) * data->ch0 - UINT64_C(1954800) * data->ch1);
	} else if (scaled_ratio < UINT64_C(850000)) {
		lux = (UINT64_C(592600) * data->ch0 + UINT64_C(118500) * data->ch1);
	} else {
		LOG_WRN("Invalid ratio: %llu", scaled_ratio);
		return -EINVAL;
	}

	/* Adjust lux value for gain and integration time.
	 * Multiply by 10 before to compensate for the integration time scaling.
	 */
	lux = (lux * 10) / (gain_value * integration_time_value);

	/* Get the integer and fractional parts from fixed point value */
	val->val1 = (int32_t)(lux / UINT64_C(1000000));
	val->val2 = (int32_t)(lux % UINT64_C(1000000));

	return 0;
}

static DEVICE_API(sensor, ltr329_driver_api) = {
	.sample_fetch = ltr329_sample_fetch,
	.channel_get = ltr329_channel_get,
};

#define DEFINE_LTR329(_num)									\
	static struct ltr329_data ltr329_data_##_num;						\
	static const struct ltr329_config ltr329_config_##_num = {				\
		.bus = I2C_DT_SPEC_INST_GET(_num),						\
		.gain = DT_INST_PROP(_num, gain),						\
		.integration_time = DT_INST_PROP(_num, integration_time),			\
		.measurement_rate = DT_INST_PROP(_num, measurement_rate),			\
	};											\
	SENSOR_DEVICE_DT_INST_DEFINE(_num, ltr329_init, NULL, &ltr329_data_##_num,		\
				     &ltr329_config_##_num, POST_KERNEL,			\
				     CONFIG_SENSOR_INIT_PRIORITY, &ltr329_driver_api);

DT_INST_FOREACH_STATUS_OKAY(DEFINE_LTR329)
