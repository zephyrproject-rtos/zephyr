/*
 * Copyright (c) 2022, Hiroki Tada
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://datasheetspdf.com/pdf/1323325/Hamamatsu/S11059-02DT/1
 */

#define DT_DRV_COMPAT hamamatsu_s11059

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(S11059, CONFIG_SENSOR_LOG_LEVEL);

/* register address */
#define S11059_REG_ADDR_CONTROL	      0x00
#define S11059_REG_ADDR_MANUAL_TIMING 0x01
#define S11059_REG_ADDR_DATA	      0x03

/* control bit */
#define S11059_CONTROL_GAIN	       3
#define S11059_CONTROL_STANDBY_MONITOR 5
#define S11059_CONTROL_STADBY	       6
#define S11059_CONTROL_ADC_RESET       7

/* bit mask for control */
#define S11059_BIT_MASK_INTEGRATION_TIME	0x03
#define S11059_BIT_MASK_CONTROL_STANDBY_MONITOR 0x20

/* factors for converting sensor samples to Lux */
#define S11059_CONVERT_FACTOR_LOW_RED	 (112)
#define S11059_CONVERT_FACTOR_LOW_GREEN	 (83)
#define S11059_CONVERT_FACTOR_LOW_BLUE	 (44)
#define S11059_CONVERT_FACTOR_LOW_IR	 (3 * 10)
#define S11059_CONVERT_FACTOR_HIGH_RED	 (117 * 10)
#define S11059_CONVERT_FACTOR_HIGH_GREEN (85 * 10)
#define S11059_CONVERT_FACTOR_HIGH_BLUE	 (448)
#define S11059_CONVERT_FACTOR_HIGH_IR	 (30 * 10)

#define S11059_INTEGRATION_TIME_MODE_00 175
#define S11059_INTEGRATION_TIME_MODE_01 2800
#define S11059_INTEGRATION_TIME_MODE_10 44800
#define S11059_INTEGRATION_TIME_MODE_11 358400

#define S11059_WAIT_PER_LOOP	 400
#define S11059_INITIAL_CONTROL	 0x04
#define S11059_MAX_MANUAL_TIMING UINT16_MAX
#define S11059_CARRY_UP		 10000

#define S11059_NUM_GAIN_MODE 2

enum s11059_channel {
	RED,
	GREEN,
	BLUE,
	IR,
	NUM_OF_COLOR_CHANNELS
};

struct s11059_dev_config {
	struct i2c_dt_spec bus;
	uint8_t gain;
	int64_t integration_time; /* integration period (unit: us) */
};

struct s11059_data {
	uint16_t samples[NUM_OF_COLOR_CHANNELS];
};

static const uint16_t convert_factors[S11059_NUM_GAIN_MODE][NUM_OF_COLOR_CHANNELS] = {
	{S11059_CONVERT_FACTOR_LOW_RED, S11059_CONVERT_FACTOR_LOW_GREEN,
	 S11059_CONVERT_FACTOR_LOW_BLUE, S11059_CONVERT_FACTOR_LOW_IR},
	{S11059_CONVERT_FACTOR_HIGH_RED, S11059_CONVERT_FACTOR_HIGH_GREEN,
	 S11059_CONVERT_FACTOR_HIGH_BLUE, S11059_CONVERT_FACTOR_HIGH_IR}};

/* Integration timing in Manual integration mode */
static const uint32_t integ_time_factor[] = {
	S11059_INTEGRATION_TIME_MODE_00, S11059_INTEGRATION_TIME_MODE_01,
	S11059_INTEGRATION_TIME_MODE_10, S11059_INTEGRATION_TIME_MODE_11};

static int s11059_convert_channel_to_index(enum sensor_channel chan)
{
	switch (chan) {
	case SENSOR_CHAN_RED:
		return RED;
	case SENSOR_CHAN_GREEN:
		return GREEN;
	case SENSOR_CHAN_BLUE:
		return BLUE;
	default:
		return IR;
	}
}

static int s11059_samples_read(const struct device *dev, uint8_t addr, uint16_t *val, uint32_t size)
{
	const struct s11059_dev_config *cfg = dev->config;
	int rc;

	if (size < NUM_OF_COLOR_CHANNELS * 2) {
		return -EINVAL;
	}

	rc = i2c_burst_read_dt(&cfg->bus, addr, (uint8_t *)val, size);
	if (rc < 0) {
		return rc;
	}

	for (size_t i = 0; i < NUM_OF_COLOR_CHANNELS; i++) {
		val[i] = sys_be16_to_cpu(val[i]);
	}

	return 0;
}

static int s11059_control_write(const struct device *dev, uint8_t control)
{
	const struct s11059_dev_config *cfg = dev->config;
	const uint8_t opcode[] = {S11059_REG_ADDR_CONTROL, control};

	return i2c_write_dt(&cfg->bus, opcode, sizeof(opcode));
}

static int s11059_manual_timing_write(const struct device *dev, uint16_t manual_time)
{
	const struct s11059_dev_config *cfg = dev->config;
	const uint8_t opcode[] = {S11059_REG_ADDR_MANUAL_TIMING, manual_time >> 8,
				  manual_time & 0xFF};

	return i2c_write_dt(&cfg->bus, opcode, sizeof(opcode));
}

static int s11059_start_measurement(const struct device *dev)
{
	const struct s11059_dev_config *cfg = dev->config;
	uint8_t control;
	int rc;

	/* read current control */
	rc = i2c_reg_read_byte_dt(&cfg->bus, S11059_REG_ADDR_CONTROL, &control);
	if (rc < 0) {
		LOG_ERR("%s, Failed to read current control.", dev->name);
		return rc;
	}

	/* reset adc block */
	WRITE_BIT(control, S11059_CONTROL_ADC_RESET, 1);
	WRITE_BIT(control, S11059_CONTROL_STADBY, 0);
	rc = s11059_control_write(dev, control);
	if (rc < 0) {
		LOG_ERR("%s, Failed to reset adc.", dev->name);
		return rc;
	}

	/* start device */
	WRITE_BIT(control, S11059_CONTROL_ADC_RESET, 0);
	rc = s11059_control_write(dev, control);
	if (rc < 0) {
		LOG_ERR("%s, Failed to start device.", dev->name);
		return rc;
	}

	return 0;
}

static int s11059_integ_time_calculate(const struct device *dev, uint16_t *manual_time,
				       uint8_t *mode)
{
	const struct s11059_dev_config *cfg = dev->config;
	int64_t tmp;

	if (cfg->integration_time < integ_time_factor[0]) {
		*mode = 0;
		*manual_time = 1;
	} else {
		*manual_time = S11059_MAX_MANUAL_TIMING;
		for (uint8_t i = 0; i < ARRAY_SIZE(integ_time_factor); i++) {
			*mode = i;
			tmp = cfg->integration_time / integ_time_factor[i];
			if (tmp < S11059_MAX_MANUAL_TIMING) {
				*manual_time = (uint16_t)tmp;
				break;
			}
		}
	}

	return 0;
}

static int s11059_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct s11059_dev_config *cfg = dev->config;
	struct s11059_data *drv_data = dev->data;
	uint16_t values[NUM_OF_COLOR_CHANNELS];
	uint8_t control;
	int rc;

	if (chan != SENSOR_CHAN_ALL) {
		LOG_ERR("%s, Unsupported sensor channel", dev->name);
		return -ENOTSUP;
	}

	rc = s11059_start_measurement(dev);
	if (rc < 0) {
		LOG_ERR("%s, Failed to start measurement.", dev->name);
		return rc;
	}

	do {
		rc = i2c_reg_read_byte_dt(&cfg->bus, S11059_REG_ADDR_CONTROL, &control);
		if (rc < 0) {
			LOG_ERR("%s, Failed to read control.", dev->name);
			return rc;
		}
		k_usleep(S11059_WAIT_PER_LOOP);

	} while (!(control & S11059_BIT_MASK_CONTROL_STANDBY_MONITOR));

	rc = s11059_samples_read(dev, S11059_REG_ADDR_DATA, values, sizeof(values));
	if (rc < 0) {
		LOG_ERR("%s, Failed to get sample.", dev->name);
		return rc;
	}

	for (size_t i = 0; i < NUM_OF_COLOR_CHANNELS; i++) {
		drv_data->samples[i] = values[i];
	}

	return 0;
}

static int s11059_channel_get(const struct device *dev, enum sensor_channel chan,
			      struct sensor_value *val)
{
	const struct s11059_dev_config *cfg = dev->config;
	struct s11059_data *drv_data = dev->data;
	const uint8_t index = s11059_convert_channel_to_index(chan);
	const uint16_t factor = convert_factors[cfg->gain][index];
	uint32_t meas_value;

	meas_value = drv_data->samples[index] * S11059_CARRY_UP / factor;
	val->val1 = meas_value / (S11059_CARRY_UP / 10);
	val->val2 = meas_value % (S11059_CARRY_UP / 10);

	return 0;
}

static int s11059_init(const struct device *dev)
{
	const struct s11059_dev_config *cfg = dev->config;
	uint8_t control = S11059_INITIAL_CONTROL;
	uint16_t manual_time;
	uint8_t timing_mode;
	int rc;

	/* device set */
	if (!i2c_is_ready_dt(&cfg->bus)) {
		LOG_ERR("%s, device is not ready.", dev->name);
		return -ENODEV;
	}

	rc = s11059_integ_time_calculate(dev, &manual_time, &timing_mode);
	if (rc < 0) {
		LOG_ERR("%s, Failed to calculate manual timing.", dev->name);
		return rc;
	}

	rc = s11059_manual_timing_write(dev, manual_time);
	if (rc < 0) {
		LOG_ERR("%s, Failed to set manual timing.", dev->name);
		return rc;
	}

	/* set integration time mode and gain*/
	control |= timing_mode & S11059_BIT_MASK_INTEGRATION_TIME;
	WRITE_BIT(control, S11059_CONTROL_GAIN, cfg->gain);
	rc = s11059_control_write(dev, control);
	if (rc < 0) {
		LOG_ERR("%s, Failed to set gain and integration time.", dev->name);
		return rc;
	}

	return 0;
}

static DEVICE_API(sensor, s11059_driver_api) = {
	.sample_fetch = s11059_sample_fetch,
	.channel_get = s11059_channel_get,
};

#define S11059_INST(inst)                                                                          \
	static struct s11059_data s11059_data_##inst;                                              \
	static const struct s11059_dev_config s11059_config_##inst = {                             \
		.bus = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.gain = DT_INST_PROP(inst, high_gain),                                             \
		.integration_time = DT_INST_PROP(inst, integration_time)};                         \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, s11059_init, NULL, &s11059_data_##inst,                 \
				     &s11059_config_##inst, POST_KERNEL,                           \
				     CONFIG_SENSOR_INIT_PRIORITY, &s11059_driver_api);

DT_INST_FOREACH_STATUS_OKAY(S11059_INST)
