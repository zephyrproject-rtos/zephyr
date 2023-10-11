/*
 * Copyright (c) 2021 Leonard Pollak
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sensirion_sgp40

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/crc.h>

#include <zephyr/drivers/sensor/sgp40.h>
#include "sgp40.h"

LOG_MODULE_REGISTER(SGP40, CONFIG_SENSOR_LOG_LEVEL);

static uint8_t sgp40_compute_crc(uint16_t value)
{
	uint8_t buf[2];

	sys_put_be16(value, buf);

	return crc8(buf, 2, SGP40_CRC_POLY, SGP40_CRC_INIT, false);
}

static int sgp40_write_command(const struct device *dev, uint16_t cmd)
{

	const struct sgp40_config *cfg = dev->config;
	uint8_t tx_buf[2];

	sys_put_be16(cmd, tx_buf);

	return i2c_write_dt(&cfg->bus, tx_buf, sizeof(tx_buf));
}

static int sgp40_start_measurement(const struct device *dev)
{
	const struct sgp40_config *cfg = dev->config;
	struct sgp40_data *data = dev->data;
	uint8_t tx_buf[8];

	sys_put_be16(SGP40_CMD_MEASURE_RAW, tx_buf);
	sys_put_be24(sys_get_be24(data->rh_param), &tx_buf[2]);
	sys_put_be24(sys_get_be24(data->t_param), &tx_buf[5]);

	return i2c_write_dt(&cfg->bus, tx_buf, sizeof(tx_buf));
}

static int sgp40_attr_set(const struct device *dev,
				enum sensor_channel chan,
				enum sensor_attribute attr,
				const struct sensor_value *val)
{
	struct sgp40_data *data = dev->data;

	/*
	 * Temperature and RH conversion to ticks as explained in datasheet
	 * in section "I2C commands"
	 */

	switch ((enum sensor_attribute_sgp40)attr) {
	case SENSOR_ATTR_SGP40_TEMPERATURE:
	{
		uint16_t t_ticks;
		int16_t tmp;

		tmp = (int16_t)CLAMP(val->val1, SGP40_COMP_MIN_T, SGP40_COMP_MAX_T);
		/* adding +87 to avoid most rounding errors through truncation */
		t_ticks = (uint16_t)((((tmp + 45) * 65535) + 87) / 175);
		sys_put_be16(t_ticks, data->t_param);
		data->t_param[2] = sgp40_compute_crc(t_ticks);
	}
		break;
	case SENSOR_ATTR_SGP40_HUMIDITY:
	{
		uint16_t rh_ticks;
		uint8_t tmp;

		tmp = (uint8_t)CLAMP(val->val1, SGP40_COMP_MIN_RH, SGP40_COMP_MAX_RH);
		/* adding +50 to eliminate rounding errors through truncation */
		rh_ticks = (uint16_t)(((tmp * 65535U) + 50U) / 100U);
		sys_put_be16(rh_ticks, data->rh_param);
		data->rh_param[2] = sgp40_compute_crc(rh_ticks);
	}
		break;
	default:
		return -ENOTSUP;
	}
	return 0;
}

static int sgp40_selftest(const struct device *dev)
{
	const struct sgp40_config *cfg = dev->config;
	uint8_t rx_buf[3];
	uint16_t raw_sample;
	int rc;

	rc = sgp40_write_command(dev, SGP40_CMD_MEASURE_TEST);
	if (rc < 0) {
		LOG_ERR("Failed to start selftest!");
		return rc;
	}

	k_sleep(K_MSEC(SGP40_TEST_WAIT_MS));

	rc = i2c_read_dt(&cfg->bus, rx_buf, sizeof(rx_buf));
	if (rc < 0) {
		LOG_ERR("Failed to read data sample.");
		return rc;
	}

	raw_sample = sys_get_be16(rx_buf);
	if (sgp40_compute_crc(raw_sample) != rx_buf[2]) {
		LOG_ERR("Received invalid CRC from selftest.");
		return -EIO;
	}

	if (raw_sample != SGP40_TEST_OK) {
		LOG_ERR("Selftest failed.");
		return -EIO;
	}

	return 0;
}

static int sgp40_sample_fetch(const struct device *dev,
			       enum sensor_channel chan)
{
	struct sgp40_data *data = dev->data;
	const struct sgp40_config *cfg = dev->config;
	uint8_t rx_buf[3];
	uint16_t raw_sample;
	int rc;

	if (chan != SENSOR_CHAN_GAS_RES && chan != SENSOR_CHAN_ALL) {
		return -ENOTSUP;
	}

	rc = sgp40_start_measurement(dev);
	if (rc < 0) {
		LOG_ERR("Failed to start measurement.");
		return rc;
	}

	k_sleep(K_MSEC(SGP40_MEASURE_WAIT_MS));

	rc = i2c_read_dt(&cfg->bus, rx_buf, sizeof(rx_buf));
	if (rc < 0) {
		LOG_ERR("Failed to read data sample.");
		return rc;
	}

	raw_sample = sys_get_be16(rx_buf);
	if (sgp40_compute_crc(raw_sample) != rx_buf[2]) {
		LOG_ERR("Invalid CRC8 for data sample.");
		return -EIO;
	}

	data->raw_sample = raw_sample;

	return 0;
}

static int sgp40_channel_get(const struct device *dev,
			      enum sensor_channel chan,
			      struct sensor_value *val)
{
	const struct sgp40_data *data = dev->data;

	if (chan != SENSOR_CHAN_GAS_RES) {
		return -ENOTSUP;
	}

	val->val1 = data->raw_sample;
	val->val2 = 0;

	return 0;
}


#ifdef CONFIG_PM_DEVICE
static int sgp40_pm_action(const struct device *dev,
			   enum pm_device_action action)
{
	uint16_t cmd;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		/* activate the hotplate by sending a measure command */
		cmd = SGP40_CMD_MEASURE_RAW;
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		cmd = SGP40_CMD_HEATER_OFF;
		break;
	default:
		return -ENOTSUP;
	}

	return sgp40_write_command(dev, cmd);
}
#endif /* CONFIG_PM_DEVICE */

static int sgp40_init(const struct device *dev)
{
	const struct sgp40_config *cfg = dev->config;
	struct sensor_value comp_data;

	if (!device_is_ready(cfg->bus.bus)) {
		LOG_ERR("Device not ready.");
		return -ENODEV;
	}

	if (cfg->selftest) {
		int rc = sgp40_selftest(dev);

		if (rc < 0) {
			LOG_ERR("Selftest failed!");
			return rc;
		}
		LOG_DBG("Selftest succeeded!");
	}

	comp_data.val1 = SGP40_COMP_DEFAULT_T;
	sensor_attr_set(dev,
			SENSOR_CHAN_GAS_RES,
			(enum sensor_attribute) SENSOR_ATTR_SGP40_TEMPERATURE,
			&comp_data);
	comp_data.val1 = SGP40_COMP_DEFAULT_RH;
	sensor_attr_set(dev,
			SENSOR_CHAN_GAS_RES,
			(enum sensor_attribute) SENSOR_ATTR_SGP40_HUMIDITY,
			&comp_data);

	return 0;
}

static const struct sensor_driver_api sgp40_api = {
	.sample_fetch = sgp40_sample_fetch,
	.channel_get = sgp40_channel_get,
	.attr_set = sgp40_attr_set,
};

#define SGP40_INIT(n)						\
	static struct sgp40_data sgp40_data_##n;		\
								\
	static const struct sgp40_config sgp40_config_##n = {	\
		.bus = I2C_DT_SPEC_INST_GET(n),			\
		.selftest = DT_INST_PROP(n, enable_selftest),	\
	};							\
								\
	PM_DEVICE_DT_INST_DEFINE(n, sgp40_pm_action);		\
								\
	SENSOR_DEVICE_DT_INST_DEFINE(n,				\
			      sgp40_init,			\
			      PM_DEVICE_DT_INST_GET(n),	\
			      &sgp40_data_##n,			\
			      &sgp40_config_##n,		\
			      POST_KERNEL,			\
			      CONFIG_SENSOR_INIT_PRIORITY,	\
			      &sgp40_api);

DT_INST_FOREACH_STATUS_OKAY(SGP40_INIT)
