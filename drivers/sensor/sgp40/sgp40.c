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
#include <sys/__assert.h>
#include <logging/log.h>
#include <sys/crc.h>

#include "sgp40.h"

LOG_MODULE_REGISTER(SGP40, CONFIG_SENSOR_LOG_LEVEL);

/*
 * CRC parameters were taken from the
 * "Checksum Calculation" section of the datasheet.
 */
static uint8_t sgp40_compute_crc(uint16_t value)
{
	uint8_t buf[2] = { value >> 8, value & 0xFF };
	return crc8(buf, 2, 0x31, 0xFF, false);;
}

int sgp40_write_command(const struct device *dev, uint16_t cmd)
{

	const struct sgp40_config *cfg = dev->config;
	uint8_t tx_buf[2] = { cmd >> 8, cmd & 0xFF };

	return i2c_write(cfg->bus, tx_buf, sizeof(tx_buf),
			 cfg->i2c_addr);
}

int sgp40_measure_raw(const struct device *dev)
{
	const struct sgp40_config *cfg = dev->config;
	struct sgp40_data *data = dev->data;

	uint8_t tx_buf[8] = { SGP40_CMD_MEASURE_RAW >> 8 ,
		SGP40_CMD_MEASURE_RAW & 0xFF,
		data->rh_param[0], data->rh_param[1], data->rh_param[2],
		data->t_param[0], data->t_param[1], data->t_param[2],
	};

	return i2c_write(cfg->bus, tx_buf, sizeof(tx_buf),
			 cfg->i2c_addr);
}

/*
 * Set the params for RH/T compensation
 */
int sgp40_update_compensation_params(const struct device *dev,
			   const struct sensor_value *val)
{
	struct sgp40_data *data = dev->data;
	int16_t rh_ticks, t_ticks;

	uint16_t rh = val->val1;
	uint16_t t = val->val2;

	/* crop values to allowed ranges */
	if ( rh < 0 ){
		rh = 0;
	} else if ( rh > 100 ) {
		rh = 100;
	}

	if ( t < -45 ){
		t = -45;
	} else if ( t > 130 ){
		t = 130;
	}

	/*
	 * Temperature and RH conversion to ticks as explained in datasheet
	 * in section "I2C commands"
	 */
	rh_ticks = (rh * 0xFFFF / 100U) + 0.5;
	t_ticks = ((t + 45U) * 0xFFFF / 175U) + 0.5;

	data->rh_param[0] = rh_ticks >> 8;
	data->rh_param[1] = rh_ticks & 0xFF;
	data->rh_param[2] = sgp40_compute_crc(rh_ticks);

	data->t_param[0] = t_ticks >> 8;
	data->t_param[1] = t_ticks & 0xFF;
	data->t_param[2] = sgp40_compute_crc(t_ticks);

	return 0;
}

int sgp40_selftest(const struct device *dev)
{
	const struct sgp40_config *cfg = dev->config;
	uint8_t rx_buf[3];
	uint16_t raw_sample;

	if ( sgp40_write_command(dev, SGP40_CMD_MEASURE_TEST) ) {
		LOG_DBG("%s: Failed to start selftest!", dev->name);
		return -EIO;
	}

	k_sleep(K_MSEC(SGP40_TEST_WAIT_MS));

	if ( i2c_read(cfg->bus, rx_buf, sizeof(rx_buf), cfg->i2c_addr )) {
		LOG_DBG("%s: Failed to read data sample.", dev->name);
		return -EIO;
	}

	raw_sample = (rx_buf[0] << 8) | rx_buf[1];
	if ( sgp40_compute_crc(raw_sample) != rx_buf[2] ) {
		LOG_DBG("%s: Received invalid CRC from selftest.", dev->name);
		return -EIO;
	}

	if ( raw_sample != SGP40_TEST_OK ) {
		LOG_DBG("%s: Selftest failed.", dev->name);
		return -EIO;
	}

	return 0;
}

static int sgp40_attr_set(const struct device *dev, enum sensor_channel chan,
			   enum sensor_attribute attr,
			   const struct sensor_value *val)
{
	if ( chan != SENSOR_CHAN_GAS_RES || attr != SENSOR_ATTR_OFFSET ) {
		return -ENOTSUP;
	}

	sgp40_update_compensation_params(dev, val);

	return 0;
}

static int sgp40_sample_fetch(const struct device *dev,
			       enum sensor_channel chan)
{
	struct sgp40_data *data = dev->data;
	const struct sgp40_config *cfg = dev->config;

	uint8_t rx_buf[3];
	uint16_t raw_sample;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

	if ( chan != SENSOR_CHAN_GAS_RES && chan != SENSOR_CHAN_ALL ) {
		return -ENOTSUP;
	}

	if ( sgp40_measure_raw(dev) ) {
		LOG_DBG("%s: Failed to start measurement.", dev->name);
		return -EIO;
	}

	k_sleep(K_MSEC(SGP40_MEASURE_WAIT_MS));

	if ( i2c_read(cfg->bus, rx_buf, sizeof(rx_buf), cfg->i2c_addr) ) {
		LOG_DBG("%s: Failed to read data sample.", dev->name);
		return -EIO;
	}

	raw_sample = (rx_buf[0] << 8) | rx_buf[1];
 	if ( sgp40_compute_crc(raw_sample) != rx_buf[2] ) {
		LOG_DBG("%s: Invalid CRC for data sample.", dev->name);
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

	if ( chan != SENSOR_CHAN_GAS_RES ) {
		return -ENOTSUP;
	}

	val->val1 = data->raw_sample;
	val->val2 = 0;

	return 0;
}


#ifdef CONFIG_PM_DEVICE
static int sgp40_set_power_state(const struct device *dev,
				  uint32_t power_state)
{
	struct sgp40_data *data = dev->data;
	uint16_t cmd;

	if ( data->pm_state == power_state ) {
		LOG_DBG("%s: Device already in requested PM_STATE.", dev->name);
		return 0;
	}

	if ( power_state == PM_DEVICE_STATE_ACTIVE ) {
		/* activate the hotplate by sending a measure command */
		cmd = SGP40_CMD_MEASURE_RAW;
	} else if ( power_state == PM_DEVICE_STATE_LOW_POWER ||
			power_state == PM_DEVICE_STATE_SUSPEND) {
		cmd = SGP40_CMD_HEATER_OFF;
	} else {
		LOG_DBG("%s: Power state not implemented.", dev->name);
		return -ENOSYS;
	}

	if ( sgp40_write_command(dev, cmd) ){
		LOG_DBG("%s: Failed to set power state.", dev->name);
		return -EIO;
	}

	data->pm_state = power_state;

	return 0;
}

static uint32_t sgp40_get_power_state(const struct device *dev,
		uint32_t *state)
{
	struct sgp40_data *data = dev->data;

	*state = data->pm_state;

	return 0;
}

static int sgp40_pm_ctrl(const struct device *dev,
	uint32_t ctrl_command,
	uint32_t *state,
	pm_device_cb cb,
	void *arg)

{
	int rc = 0;

	if ( ctrl_command == PM_DEVICE_STATE_SET ) {
		rc = sgp40_set_power_state(dev, *state);
	} else if (ctrl_command == PM_DEVICE_STATE_GET) {
		rc = sgp40_get_power_state(dev, state);
	}

	if ( cb ) {
		cb(dev, rc, state, arg);
	}
	return rc;
}
#endif /* CONFIG_PM_DEVICE */

static int sgp40_init(const struct device *dev)
{
	const struct sgp40_config *cfg = dev->config;
	struct sensor_value comp_data;
	comp_data.val1 = 50;
	comp_data.val2 = 25;

	if ( !device_is_ready(cfg->bus) ) {
		LOG_DBG("%s: Device not ready.", dev->name);
		return -ENODEV;
	}

#ifdef CONFIG_SGP40_SELFTEST
	if ( sgp40_selftest(dev) ) {
		LOG_ERR("%s: Selftest failed!", dev->name);
		return -EIO;
	}
	LOG_DBG("%s: Selftest succeded!", dev->name);
#endif
	sgp40_update_compensation_params(dev, &comp_data);
	return 0;
}

static const struct sensor_driver_api sgp40_api = {
	.sample_fetch = sgp40_sample_fetch,
	.channel_get = sgp40_channel_get,
	.attr_set = sgp40_attr_set,
};

struct sgp40_data sgp40_data_0;

static const struct sgp40_config sgp40_cfg_0 = {
	.bus = DEVICE_DT_GET(DT_INST_BUS(0)),
	.i2c_addr = DT_INST_REG_ADDR(0),
};

DEVICE_DT_INST_DEFINE(0, sgp40_init, sgp40_pm_ctrl,
		    &sgp40_data_0, &sgp40_cfg_0,
		    POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,
		    &sgp40_api);
