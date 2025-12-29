/*
 * Copyright (c) 2025 Konrad Sikora
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT liteon_ltr329

#include "ltr55x.h"

#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

LOG_MODULE_REGISTER(LTR55X, CONFIG_SENSOR_LOG_LEVEL);

static int ltr55x_check_device_id(const struct i2c_dt_spec *bus)
{
	uint8_t id;
	int rc;

	rc = i2c_reg_read_byte_dt(bus, LTR55X_PART_ID, &id);
	if (rc < 0) {
		LOG_ERR("Failed to read PART_ID");
		return rc;
	}
	if (id != LTR329_PART_ID_VALUE) {
		LOG_ERR("PART_ID mismatch: expected 0x%02X, got 0x%02X", LTR329_PART_ID_VALUE, id);
		return -ENODEV;
	}

	rc = i2c_reg_read_byte_dt(bus, LTR55X_MANUFAC_ID, &id);
	if (rc < 0) {
		LOG_ERR("Failed to read MANUFAC_ID");
		return rc;
	}
	if (id != LTR55X_MANUFACTURER_ID_VALUE) {
		LOG_ERR("MANUFAC_ID mismatch: expected 0x%02X, got 0x%02X",
			LTR55X_MANUFACTURER_ID_VALUE, id);
		return -ENODEV;
	}

	return 0;
}

static int ltr55x_init_als_registers(const struct ltr55x_config *cfg)
{
	const struct i2c_dt_spec *bus = &cfg->bus;
	const uint8_t control_reg = LTR55X_REG_SET(ALS_CONTR, MODE, LTR553_ALS_CONTR_MODE_ACTIVE) |
				    LTR55X_REG_SET(ALS_CONTR, GAIN, cfg->als_gain);
	const uint8_t meas_reg = LTR55X_REG_SET(MEAS_RATE, REPEAT, cfg->als_measurement_rate) |
				 LTR55X_REG_SET(MEAS_RATE, INT_TIME, cfg->als_integration_time);
	uint8_t buffer;
	int rc;

	rc = i2c_reg_write_byte_dt(bus, LTR55X_ALS_CONTR, control_reg);
	if (rc < 0) {
		LOG_ERR("Failed to set ALS_CONTR register");
		return rc;
	}

	rc = i2c_reg_write_byte_dt(bus, LTR55X_MEAS_RATE, meas_reg);
	if (rc < 0) {
		LOG_ERR("Failed to set MEAS_RATE register");
		return rc;
	}

	/* Read back the MEAS_RATE register to verify the settings */
	rc = i2c_reg_read_byte_dt(bus, LTR55X_MEAS_RATE, &buffer);
	if (rc < 0) {
		LOG_ERR("Failed to read back MEAS_RATE register");
		return rc;
	}
	if (LTR55X_REG_GET(MEAS_RATE, REPEAT, buffer) != cfg->als_measurement_rate) {
		LOG_ERR("Measurement rate mismatch: expected %u, got %u", cfg->als_measurement_rate,
			(uint8_t)LTR55X_REG_GET(MEAS_RATE, REPEAT, buffer));
		return -ENODEV;
	}
	if (LTR55X_REG_GET(MEAS_RATE, INT_TIME, buffer) != cfg->als_integration_time) {
		LOG_ERR("Integration time mismatch: expected %u, got %u", cfg->als_integration_time,
			(uint8_t)LTR55X_REG_GET(MEAS_RATE, INT_TIME, buffer));
		return -ENODEV;
	}

	return 0;
}

static int ltr55x_init(const struct device *dev)
{
	const struct ltr55x_config *cfg = dev->config;
	int rc;

	if (!i2c_is_ready_dt(&cfg->bus)) {
		LOG_ERR("I2C bus not ready");
		return -ENODEV;
	}

	/* Wait for sensor startup */
	k_sleep(K_MSEC(LTR55X_INIT_STARTUP_MS));

	rc = ltr55x_check_device_id(&cfg->bus);
	if (rc < 0) {
		return rc;
	}

	/* Init register to enable sensor to active mode */
	rc = ltr55x_init_als_registers(cfg);
	if (rc < 0) {
		return rc;
	}

	return 0;
}

static int ltr55x_check_data_ready(const struct i2c_dt_spec *bus)
{
	uint8_t status;
	int rc;

	rc = i2c_reg_read_byte_dt(bus, LTR55X_ALS_PS_STATUS, &status);
	if (rc < 0) {
		LOG_ERR("Failed to read ALS_STATUS register");
		return rc;
	}

	if (!LTR55X_REG_GET(ALS_PS_STATUS, ALS_DATA_STATUS, status)) {
		LOG_WRN("Data not ready");
		return -EBUSY;
	}

	return 0;
}

static int ltr55x_read_als_data(const struct i2c_dt_spec *bus, struct ltr55x_data *data)
{
	uint8_t reg = LTR55X_ALS_DATA_CH1_0;
	uint8_t buff[4];
	int rc;

	rc = i2c_write_read_dt(bus, &reg, sizeof(reg), buff, sizeof(buff));
	if (rc < 0) {
		LOG_ERR("Failed to read ALS data registers");
		return rc;
	}

	data->als_ch1 = sys_get_le16(buff);
	data->als_ch0 = sys_get_le16(buff + 2);

	return 0;
}

static int ltr55x_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct ltr55x_config *cfg = dev->config;
	struct ltr55x_data *data = dev->data;
	int rc;

	if ((chan != SENSOR_CHAN_ALL) && (chan != SENSOR_CHAN_LIGHT)) {
		return -ENOTSUP;
	}

	rc = ltr55x_check_data_ready(&cfg->bus);
	if (rc < 0) {
		return rc;
	}

	rc = ltr55x_read_als_data(&cfg->bus, data);
	if (rc < 0) {
		return rc;
	}

	return 0;
}

static int ltr55x_get_mapped_gain(const uint8_t reg_val, uint8_t *const output)
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

static int ltr55x_get_mapped_int_time(const uint8_t reg_val, uint8_t *const output)
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

static int ltr55x_channel_get(const struct device *dev, enum sensor_channel chan,
			      struct sensor_value *val)
{
	const struct ltr55x_data *data = dev->data;
	const struct ltr55x_config *cfg = dev->config;
	uint8_t gain_value;
	uint8_t integration_time_value;

	if (chan != SENSOR_CHAN_LIGHT) {
		return -ENOTSUP;
	}

	if (ltr55x_get_mapped_gain(cfg->als_gain, &gain_value) != 0) {
		LOG_ERR("Invalid gain configuration");
		return -EINVAL;
	}

	if (ltr55x_get_mapped_int_time(cfg->als_integration_time, &integration_time_value) != 0) {
		LOG_ERR("Invalid integration time configuration");
		return -EINVAL;
	}

	if ((data->als_ch0 == 0) && (data->als_ch1 == 0)) {
		LOG_WRN("Both channels are zero; cannot compute ratio");
		return -EINVAL;
	}

	/* Calculate lux value according to the appendix A of the datasheet. */
	uint64_t lux;
	/* The calculation is scaled by 1000000 to avoid floating point. */
	uint64_t scaled_ratio =
		(data->als_ch1 * UINT64_C(1000000)) / (uint64_t)(data->als_ch0 + data->als_ch1);

	if (scaled_ratio < UINT64_C(450000)) {
		lux = (UINT64_C(1774300) * data->als_ch0 + UINT64_C(1105900) * data->als_ch1);
	} else if (scaled_ratio < UINT64_C(640000)) {
		lux = (UINT64_C(4278500) * data->als_ch0 - UINT64_C(1954800) * data->als_ch1);
	} else if (scaled_ratio < UINT64_C(850000)) {
		lux = (UINT64_C(592600) * data->als_ch0 + UINT64_C(118500) * data->als_ch1);
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

static DEVICE_API(sensor, ltr55x_driver_api) = {
	.sample_fetch = ltr55x_sample_fetch,
	.channel_get = ltr55x_channel_get,
};

#define LTR55X_ALS_GAIN_REG(n)                                                                     \
	COND_CODE_1(DT_NODE_HAS_PROP(n, gain), (DT_PROP(n, gain)),                                 \
		    (UTIL_CAT(LTR55X_ALS_GAIN_VALUE_, DT_PROP_OR(n, als_gain, 1))))
#define LTR55X_ALS_INT_TIME_REG(n)                                                                 \
	COND_CODE_1(DT_NODE_HAS_PROP(n, integration_time), (DT_PROP(n, integration_time)),         \
		    (DT_ENUM_IDX_OR(n, als_integration_time, 0)))
#define LTR55X_ALS_MEAS_RATE_REG(n)                                                                \
	COND_CODE_1(DT_NODE_HAS_PROP(n, measurement_rate), (DT_PROP(n, measurement_rate)),         \
		    (DT_ENUM_IDX_OR(n, als_measurement_rate, 3)))

#define DEFINE_LTR55X(_num)                                                                        \
	static struct ltr55x_data ltr55x_data_##_num;                                              \
	static const struct ltr55x_config ltr55x_config_##_num = {                                 \
		.bus = I2C_DT_SPEC_INST_GET(_num),                                                 \
		.als_gain = LTR55X_ALS_GAIN_REG(_num),                                             \
		.als_integration_time = LTR55X_ALS_INT_TIME_REG(_num),                             \
		.als_measurement_rate = LTR55X_ALS_MEAS_RATE_REG(_num),                            \
	};                                                                                         \
	SENSOR_DEVICE_DT_INST_DEFINE(_num, ltr55x_init, NULL, &ltr55x_data_##_num,                 \
				     &ltr55x_config_##_num, POST_KERNEL,                           \
				     CONFIG_SENSOR_INIT_PRIORITY, &ltr55x_driver_api);

DT_INST_FOREACH_STATUS_OKAY(DEFINE_LTR55X)
