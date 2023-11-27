/*
 * Copyright (c) 2023 Konrad Sikora
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ltr_329

#include <zephyr/device.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor.h>

#define LTR329_ALS_CONTR      0x80
#define LTR329_MEAS_RATE      0x85
#define LTR329_PART_ID        0x86
#define LTR329_MANUFAC_ID     0x87
#define LTR329_ALS_DATA_CH1_0 0x88
#define LTR329_ALS_DATA_CH1_1 0x89
#define LTR329_ALS_DATA_CH0_0 0x8A
#define LTR329_ALS_DATA_CH0_1 0x8B
#define LTR329_ALS_STATUS     0x8C

#define LTR329_ALS_CONTR_MODE_MASK      BIT(0)
#define LTR329_ALS_CONTR_MODE_SHIFT     0
#define LTR329_ALS_CONTR_SW_RESET_MASK  BIT(1)
#define LTR329_ALS_CONTR_SW_RESET_SHIFT 1
#define LTR329_ALS_CONTR_GAIN_MASK      GENMASK(4, 2)
#define LTR329_ALS_CONTR_GAIN_SHIFT     2

#define LTR329_MEAS_RATE_REPEAT_MASK    GENMASK(2, 0)
#define LTR329_MEAS_RATE_REPEAT_SHIFT   0
#define LTR329_MEAS_RATE_INT_TIME_MASK  GENMASK(5, 3)
#define LTR329_MEAS_RATE_INT_TIME_SHIFT 3

#define LTR329_PART_ID_REVISION_MASK  GENMASK(3, 0)
#define LTR329_PART_ID_REVISION_SHIFT 0
#define LTR329_PART_ID_NUMBER_MASK    GENMASK(7, 4)
#define LTR329_PART_ID_NUMBER_SHIFT   4

#define LTR329_MANUFAC_ID_IDENTIFICATION_MASK  GENMASK(7, 0)
#define LTR329_MANUFAC_ID_IDENTIFICATION_SHIFT 0

#define LTR329_ALS_STATUS_DATA_MASK  GENMASK(7, 0)
#define LTR329_ALS_STATUS_DATA_SHIFT 0

#define LTR329_ALS_STATUS_DATA_READY_MASK  BIT(2)
#define LTR329_ALS_STATUS_DATA_READY_SHIFT 2
#define LTR329_ALS_STATUS_DATA_GAIN_MASK   GENMASK(6, 4)
#define LTR329_ALS_STATUS_DATA_GAIN_SHIFT  4
#define LTR329_ALS_STATUS_DATA_VALID_MASK  BIT(7)
#define LTR329_ALS_STATUS_DATA_VALID_SHIFT 7

#define LTR329_PART_ID_VALUE         0xA0
#define LTR329_MANUFACTURER_ID_VALUE 0x05

#define LTR329_INIT_STARTUP_MS        100
#define LTR329_WAKEUP_FROM_STANDBY_MS 10

#define LTR329_REG_SET(reg, field, value)                                                          \
	(((value) << reg##_##field##_SHIFT) & reg##_##field##_MASK)
#define LTR329_REG_GET(reg, field, value)                                                          \
	(((value) & reg##_##field##_MASK) >> reg##_##field##_SHIFT)

struct ltr329_config {
	const struct i2c_dt_spec bus;
	uint8_t gain;
	uint8_t integration_time;
	uint8_t measurement_rate;
};

struct ltr329_data {
	int32_t ch0;
	int32_t ch1;
};

LOG_MODULE_REGISTER(LTR329, CONFIG_SENSOR_LOG_LEVEL);

static int ltr329_send_command(const struct i2c_dt_spec *bus, uint8_t reg, uint8_t value)
{
	uint8_t frame[2] = {reg, value};

	return i2c_write_dt(bus, frame, sizeof(frame));
}

static int ltr329_read_register(const struct i2c_dt_spec *bus, uint8_t reg, uint8_t *frame)
{
	return i2c_write_read_dt(bus, &reg, sizeof(reg), frame, sizeof(*frame));
}

static int ltr329_check_device_id(const struct i2c_dt_spec *bus)
{
	uint8_t frame;
	int rc;

	rc = ltr329_read_register(bus, LTR329_PART_ID, &frame);
	if (rc < 0) {
		LOG_ERR("Failed to get LTR329 part id");
		return rc;
	}
	if (frame != LTR329_PART_ID_VALUE) {
		LOG_ERR("LTR329 part id mismatch");
		return -ENODEV;
	}

	rc = ltr329_read_register(bus, LTR329_MANUFAC_ID, &frame);
	if (rc < 0) {
		LOG_ERR("Failed to get LTR329 manufacturer id");
		return rc;
	}
	if (frame != LTR329_MANUFACTURER_ID_VALUE) {
		LOG_ERR("LTR329 manufacturer id mismatch");
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

	rc = ltr329_send_command(&cfg->bus, LTR329_ALS_CONTR, control_reg);
	if (rc < 0) {
		LOG_ERR("Failed to set LTR329 ALS_CONTR");
		return rc;
	}

	rc = ltr329_send_command(&cfg->bus, LTR329_MEAS_RATE, meas_reg);
	if (rc < 0) {
		LOG_ERR("Failed to set LTR329 MEAS_RATE");
		return rc;
	}

	rc = ltr329_read_register(bus, LTR329_MEAS_RATE, &buffer);
	if (rc < 0) {
		LOG_ERR("Failed to get LTR329 MEAS_RATE");
		return rc;
	}
	if (LTR329_REG_GET(LTR329_MEAS_RATE, REPEAT, buffer) != cfg->measurement_rate) {
		LOG_ERR("Measurement rate mismatch, check if its higher than integration_time");
		return -ENODEV;
	}
	if (LTR329_REG_GET(LTR329_MEAS_RATE, INT_TIME, buffer) != cfg->integration_time) {
		LOG_ERR("Integration time mismatch, check if its lower than measurement_rate");
		return -ENODEV;
	}

	return 0;
}

static int ltr329_init(const struct device *dev)
{
	const struct ltr329_config *cfg = dev->config;
	int rc;

	if (!i2c_is_ready_dt(&cfg->bus)) {
		LOG_ERR("I2C bus device not ready");
		return -ENODEV;
	}

	k_sleep(K_MSEC(LTR329_INIT_STARTUP_MS));

	rc = ltr329_check_device_id(&cfg->bus);
	if (rc < 0) {
		return rc;
	}

	rc = ltr329_init_registers(&cfg->bus, cfg);
	if (rc < 0) {
		return rc;
	}

	k_sleep(K_MSEC(LTR329_WAKEUP_FROM_STANDBY_MS));

	return 0;
}

static int ltr329_wait_for_data_ready(const struct i2c_dt_spec *bus)
{
	uint8_t status;
	int rc;

	do {
		rc = ltr329_read_register(bus, LTR329_ALS_STATUS, &status);
		if (rc < 0) {
			LOG_ERR("Failed to get LTR329 ALS_STATUS");
			return rc;
		}
		k_sleep(K_MSEC(3));
	} while (!(LTR329_REG_GET(LTR329_ALS_STATUS, DATA_READY, status)));

	return 0;
}

static int ltr329_read_als_data(const struct i2c_dt_spec *bus, struct ltr329_data *data)
{
	uint8_t buff[4];
	int rc;

	rc = ltr329_read_register(bus, LTR329_ALS_DATA_CH1_0, &buff[0]);
	if (rc < 0) {
		LOG_ERR("Failed to read LTR329 ALS_DATA_CH1_0");
		return rc;
	}

	rc = ltr329_read_register(bus, LTR329_ALS_DATA_CH1_1, &buff[1]);
	if (rc < 0) {
		LOG_ERR("Failed to read LTR329 ALS_DATA_CH1_1");
		return rc;
	}

	rc = ltr329_read_register(bus, LTR329_ALS_DATA_CH0_0, &buff[2]);
	if (rc < 0) {
		LOG_ERR("Failed to read LTR329 ALS_DATA_CH0_0");
		return rc;
	}

	rc = ltr329_read_register(bus, LTR329_ALS_DATA_CH0_1, &buff[3]);
	if (rc < 0) {
		LOG_ERR("Failed to read LTR329 ALS_DATA_CH0_1");
		return rc;
	}

	data->ch1 = buff[0] | (buff[1] << 8);
	data->ch0 = buff[2] | (buff[3] << 8);

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

	rc = ltr329_wait_for_data_ready(&cfg->bus);
	if (rc < 0) {
		return rc;
	}

	rc = ltr329_read_als_data(&cfg->bus, data);
	if (rc < 0) {
		return rc;
	}

	return 0;
}

static int ltr329_get_mapped_gain(const uint8_t register_val, uint8_t *const output)
{
	/* Map the register value to the gain used in the lux calculation */
	const uint8_t gain_lux_calc[] = {1, 2, 4, 8, 0, 0, 48, 96};

	if (register_val < ARRAY_SIZE(gain_lux_calc)) {
		*output = gain_lux_calc[register_val];
		/* 0 is not a valid gain value */
		return (*output == 0) ? -EINVAL : 0;
	}

	return -EINVAL;
}

static int ltr329_get_mapped_int_time(const uint8_t register_val, uint8_t *const output)
{
	/* Map the register value to the integration time used in the lux calculation */
	const uint8_t int_time_lux_calc[] = {10, 5, 20, 40, 15, 25, 30, 35};

	if (register_val < ARRAY_SIZE(int_time_lux_calc)) {
		*output = int_time_lux_calc[register_val];
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
		LOG_ERR("Invalid configuration, failed to get gain");
		return -EINVAL;
	}

	if (ltr329_get_mapped_int_time(cfg->integration_time, &integration_time_value) != 0) {
		LOG_ERR("Invalid configuration, failed to get integration time");
		return -EINVAL;
	}

	if ((data->ch0 == 0) && (data->ch1 == 0)) {
		LOG_WRN("The ratio cannot be calculated when both channels are 0");
		return -EINVAL;
	}

	/* Calculate lux value according to the appendix A of the datasheet.*/
	uint64_t lux;
	/* The calculation is scaled by 1000000 to avoid floating point. */
	uint64_t scaled_ratio = (data->ch1 * 1000000ULL) / (uint64_t)(data->ch0 + data->ch1);

	if (scaled_ratio < 450000ULL) {
		lux = (1774300ULL * data->ch0 + 1105900ULL * data->ch1);
	} else if (scaled_ratio < 640000ULL) {
		lux = (4278500ULL * data->ch0 - 1954800ULL * data->ch1);
	} else if (scaled_ratio < 850000ULL) {
		lux = (592600ULL * data->ch0 + 118500ULL * data->ch1);
	} else {
		LOG_WRN("Invalid ratio");
		return -EINVAL;
	}

	/* Divide by 10, because integration time is scaled to avoid floating point */
	lux /= gain_value * integration_time_value / 10;

	/* Get the integer and fractional parts from fixed point value */
	val->val1 = (int32_t)(lux / 1000000ULL);
	val->val2 = (int32_t)(lux % 1000000ULL);

	return 0;
}

static const struct sensor_driver_api ltr329_driver_api = {
	.sample_fetch = ltr329_sample_fetch,
	.channel_get = ltr329_channel_get,
};

#define DEFINE_LTR329(_num)                                                                        \
	static struct ltr329_data ltr329_data_##_num;                                              \
	static const struct ltr329_config ltr329_config_##_num = {                                 \
		.bus = I2C_DT_SPEC_INST_GET(_num),                                                 \
		.gain = DT_INST_PROP(_num, gain),                                                  \
		.integration_time = DT_INST_PROP(_num, integration_time),                          \
		.measurement_rate = DT_INST_PROP(_num, measurement_rate),                          \
	};                                                                                         \
	SENSOR_DEVICE_DT_INST_DEFINE(_num, ltr329_init, NULL, &ltr329_data_##_num,                 \
				     &ltr329_config_##_num, POST_KERNEL,                           \
				     CONFIG_SENSOR_INIT_PRIORITY, &ltr329_driver_api);

DT_INST_FOREACH_STATUS_OKAY(DEFINE_LTR329)
