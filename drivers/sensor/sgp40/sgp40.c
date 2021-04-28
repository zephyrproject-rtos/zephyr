/*
 * Copyright (c) 2021 Leonard Pollak
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sensirion_sgp40

#include <device.h>
#include <drivers/i2c.h>
#include <kernel.h>
#include <drivers/sensor.h>
#include <logging/log.h>
#include <sys/byteorder.h>
#include <sys/crc.h>

#include <drivers/sensor/sgp40.h>
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

	return i2c_write(cfg->bus, tx_buf, sizeof(tx_buf),
			 cfg->i2c_addr);
}

static int sgp40_start_measurement(const struct device *dev)
{
	const struct sgp40_config *cfg = dev->config;
	struct sgp40_data *data = dev->data;
	uint8_t tx_buf[8];

	sys_put_be16(SGP40_CMD_MEASURE_RAW, tx_buf);
	sys_put_be24(sys_get_be24(data->rh_param), &tx_buf[2]);
	sys_put_be24(sys_get_be24(data->t_param), &tx_buf[5]);

	return i2c_write(cfg->bus, tx_buf, sizeof(tx_buf),
			 cfg->i2c_addr);
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
		int16_t t_ticks;
		int32_t tmp;

		tmp = CLAMP(val->val1, SGP40_COMP_MIN_T, SGP40_COMP_MAX_T);
		t_ticks = ((tmp + 45U) * 0xFFFF / 175U) + 0.5;
		sys_put_be16(t_ticks, data->t_param);
		data->t_param[2] = sgp40_compute_crc(t_ticks);
	}
		break;
	case SENSOR_ATTR_SGP40_HUMIDITY:
	{
		int16_t rh_ticks;
		int32_t tmp;

		tmp = CLAMP(val->val1, SGP40_COMP_MIN_RH, SGP40_COMP_MAX_RH);
		rh_ticks = (tmp * 0xFFFF / 100U) + 0.5;
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

	rc = i2c_read(cfg->bus, rx_buf, sizeof(rx_buf), cfg->i2c_addr);
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

	rc = i2c_read(cfg->bus, rx_buf, sizeof(rx_buf), cfg->i2c_addr);
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
static int sgp40_set_power_state(const struct device *dev,
				  enum pm_device_state power_state)
{
	struct sgp40_data *data = dev->data;
	uint16_t cmd;
	int rc;

	if (data->pm_state == power_state) {
		LOG_DBG("Device already in requested PM_STATE.");
		return 0;
	}

	if (power_state == PM_DEVICE_STATE_ACTIVE) {
		/* activate the hotplate by sending a measure command */
		cmd = SGP40_CMD_MEASURE_RAW;
	} else if (power_state == PM_DEVICE_STATE_SUSPEND) {
		cmd = SGP40_CMD_HEATER_OFF;
	} else {
		LOG_DBG("Power state not implemented.");
		return -ENOTSUP;
	}

	rc = sgp40_write_command(dev, cmd);
	if (rc < 0) {
		LOG_ERR("Failed to set power state.");
		return rc;
	}

	data->pm_state = power_state;

	return 0;
}

static uint32_t sgp40_get_power_state(const struct device *dev,
		enum pm_device_state *state)
{
	struct sgp40_data *data = dev->data;

	*state = data->pm_state;

	return 0;
}

static int sgp40_pm_ctrl(const struct device *dev,
	uint32_t ctrl_command,
	enum pm_device_state *state)
{
	int rc = 0;

	if (ctrl_command == PM_DEVICE_STATE_SET) {
		rc = sgp40_set_power_state(dev, *state);
	} else if (ctrl_command == PM_DEVICE_STATE_GET) {
		rc = sgp40_get_power_state(dev, state);
	}

	return rc;
}
#endif /* CONFIG_PM_DEVICE */

static int sgp40_init(const struct device *dev)
{
	const struct sgp40_config *cfg = dev->config;
	struct sensor_value comp_data;

	if (!device_is_ready(cfg->bus)) {
		LOG_ERR("Device not ready.");
		return -ENODEV;
	}

	if (cfg->selftest) {
		int rc = sgp40_selftest(dev);

		if (rc < 0) {
			LOG_ERR("Selftest failed!");
			return rc;
		}
		LOG_DBG("Selftest succeded!");
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
		.bus = DEVICE_DT_GET(DT_INST_BUS(n)),		\
		.i2c_addr = DT_INST_REG_ADDR(n),		\
		.selftest = DT_INST_PROP(n, enable_selftest),	\
	};							\
								\
	DEVICE_DT_INST_DEFINE(n,				\
			      sgp40_init,			\
			      sgp40_pm_ctrl,\
			      &sgp40_data_##n,			\
			      &sgp40_config_##n,		\
			      POST_KERNEL,			\
			      CONFIG_SENSOR_INIT_PRIORITY,	\
			      &sgp40_api);

DT_INST_FOREACH_STATUS_OKAY(SGP40_INIT)
