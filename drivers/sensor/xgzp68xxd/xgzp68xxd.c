/* xgzp68xxd.c - Driver for CFSensor XGZP68XXD Pressure sensor family */

/*
 * Copyright (c) 2026 Nexrem
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT cfsensor_xgzp68xxd

#include <zephyr/device.h>
#include <zephyr/pm/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(XGZP68XXD, CONFIG_SENSOR_LOG_LEVEL);

#define XGZP68XXD_REG_PRES_MSB   0x06
#define XGZP68XXD_REG_PRES_CSB   0x07
#define XGZP68XXD_REG_PRES_LSB   0x08
#define XGZP68XXD_REG_TEMP_MSB   0x09
#define XGZP68XXD_REG_TEMP_LSB   0x0A
#define XGZP68XXD_REG_CMD        0x30
#define XGZP68XXD_REG_SYS_CONFIG 0xA5
#define XGZP68XXD_REG_P_CONFIG   0xA6

#define XGZP68XXD_CMD_SCO_MSK        BIT(3)
#define XGZP68XXD_CMD_SLEEP_TIME_MSK 0xF0
#define XGZP68XXD_CMD_CTRL_MSK       0x03

#define XGZP68XXD_CMD_CTRL_NORM 0x02
#define XGZP68XXD_CMD_CTRL_AUTO 0x03

#define XGZP68XXD_WAIT_PER_LOOP      400
#define XGZP68XXD_SAMPLE_PRES_CNT    3
#define XGZP68XXD_SAMPLE_TEMP_CNT    2
#define XGZP68XXD_TEMP_SCALE         256
#define XGZP68XXD_CONVERSION_TIMEOUT K_MSEC(20)

struct xgzp68xxd_config {
	struct i2c_dt_spec i2c;
	uint16_t k_value;
};

struct xgzp68xxd_data {
	int32_t raw_pressure;
	int32_t raw_temperature;
};

static int xgzp68xxd_start_measurement(const struct device *dev)
{
	const struct xgzp68xxd_config *config = dev->config;
	int rc;

	const uint8_t command[] = {XGZP68XXD_REG_CMD,
				   XGZP68XXD_CMD_SCO_MSK | XGZP68XXD_CMD_CTRL_NORM};

	rc = i2c_write_dt(&config->i2c, command, sizeof(command));
	if (rc < 0) {
		return rc;
	}

	return 0;
}

static int xgzp68xxd_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct xgzp68xxd_config *config = dev->config;
	struct xgzp68xxd_data *data = dev->data;
	uint8_t control;
	uint8_t samples[XGZP68XXD_SAMPLE_PRES_CNT + XGZP68XXD_SAMPLE_TEMP_CNT];
	int rc;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_PRESS &&
	    chan != SENSOR_CHAN_AMBIENT_TEMP) {
		LOG_ERR("%s, Unsupported sensor channel", dev->name);
		return -ENOTSUP;
	}

	rc = xgzp68xxd_start_measurement(dev);
	if (rc < 0) {
		LOG_ERR("%s, Failed to start measurement.", dev->name);
		return rc;
	}

	k_timepoint_t timeout = sys_timepoint_calc(XGZP68XXD_CONVERSION_TIMEOUT);

	while (true) {
		rc = i2c_reg_read_byte_dt(&config->i2c, XGZP68XXD_REG_CMD, &control);
		if (rc < 0) {
			LOG_ERR("%s, Failed to read control.", dev->name);
			return rc;
		}

		if (!(control & XGZP68XXD_CMD_SCO_MSK)) {
			break;
		}

		if (sys_timepoint_expired(timeout)) {
			LOG_ERR("%s, Control read timeout.", dev->name);
			return -ETIMEDOUT;
		}

		k_usleep(XGZP68XXD_WAIT_PER_LOOP);
	}

	rc = i2c_burst_read_dt(&config->i2c, XGZP68XXD_REG_PRES_MSB, samples, sizeof(samples));
	if (rc < 0) {
		LOG_ERR("%s, Failed to get sample.", dev->name);
		return rc;
	}

	data->raw_pressure = sign_extend(sys_get_be24(&samples[0]), 23);
	data->raw_temperature = (int16_t)sys_get_be16(&samples[3]);

	return 0;
}

static int xgzp68xxd_channel_get(const struct device *dev, enum sensor_channel chan,
				 struct sensor_value *val)
{
	const struct xgzp68xxd_config *config = dev->config;
	struct xgzp68xxd_data *data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_PRESS: {
		int64_t p_ukpa =
			(int64_t)data->raw_pressure * 1000000 / ((int64_t)config->k_value * 1000);
		val->val1 = p_ukpa / 1000000;
		val->val2 = p_ukpa % 1000000;
	}
		return 0;
	case SENSOR_CHAN_AMBIENT_TEMP:
		val->val1 = data->raw_temperature / XGZP68XXD_TEMP_SCALE;
		val->val2 = (data->raw_temperature % XGZP68XXD_TEMP_SCALE) * 1000000 /
			    XGZP68XXD_TEMP_SCALE;
		return 0;
	default:
		return -ENOTSUP;
	}
}

/* Initialize device */
static int xgzp68xxd_init(const struct device *dev)
{
	const struct xgzp68xxd_config *config = dev->config;

	if (!device_is_ready(config->i2c.bus)) {
		LOG_ERR_DEVICE_NOT_READY(config->i2c.bus);
		return -ENODEV;
	}

	return 0;
}

static DEVICE_API(sensor, xgzp68xxd_driver_api) = {
	.sample_fetch = xgzp68xxd_sample_fetch,
	.channel_get = xgzp68xxd_channel_get,
};

#define XGZP68XXD_DEFINE(inst)                                                                     \
	static struct xgzp68xxd_data xgzp68xxd_data##inst = {0};                                   \
                                                                                                   \
	static const struct xgzp68xxd_config xgzp68xxd_config##inst = {                            \
		.i2c = I2C_DT_SPEC_INST_GET(inst), .k_value = DT_INST_PROP(inst, k_value)};        \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, xgzp68xxd_init, NULL, &xgzp68xxd_data##inst,            \
				     &xgzp68xxd_config##inst, POST_KERNEL,                         \
				     CONFIG_SENSOR_INIT_PRIORITY, &xgzp68xxd_driver_api);

DT_INST_FOREACH_STATUS_OKAY(XGZP68XXD_DEFINE)
