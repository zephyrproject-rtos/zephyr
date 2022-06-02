/*
 * Copyright (c) 2022 Stephen Oliver
 *
 * SPDX-License-Identifier: Apache-2.0
 */   

#define DT_DRV_COMPAT sensirion_scd4x

#include <stdio.h>

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/crc.h>

#include <zephyr/drivers/sensor/scd4x.h>
#include "scd4x.h"

LOG_MODULE_REGISTER(SCD4X, CONFIG_SENSOR_LOG_LEVEL);


static uint8_t scd4x_compute_crc(uint16_t value)
{

	uint8_t buf[2];

	sys_put_be16(value, buf);

	return crc8(buf, 2, SCD4X_CRC_POLY, SCD4X_CRC_INIT, false);
}


static int scd4x_write_command(const struct device *dev, uint16_t cmd)
{
	const struct scd4x_config *cfg = dev->config;
	uint8_t tx_buf[2];

	sys_put_be16(cmd, tx_buf);

	return i2c_write_dt(&cfg->bus, tx_buf, sizeof(tx_buf));
}


static int scd4x_read_reg(const struct device *dev, uint16_t reg_addr, uint8_t *rx_buf, uint8_t rx_buf_size)
{
	const struct scd4x_config *cfg = dev->config;
	int rc;

	rc = scd4x_write_command(dev, reg_addr);

	k_sleep(K_USEC(1));

	rc = i2c_read_dt(&cfg->bus, rx_buf, rx_buf_size);

	return rc;
}


static int scd4x_write_reg(const struct device *dev, uint16_t cmd, uint16_t val)
{
	const struct scd4x_config *cfg = dev->config;
	uint8_t tx_buf[5];

	sys_put_be16(cmd, tx_buf);
	sys_put_be16(val, &tx_buf[2]);
	tx_buf[4] = scd4x_compute_crc(val);

	return i2c_write_dt(&cfg->bus, tx_buf, sizeof(tx_buf));
}

#if defined(SCD4X_POWER_DOWN_SINGLE_SHOT_MEASUREMENT) || defined(CONFIG_PM_DEVICE) 
static int scd4x_power_down(const struct device *dev)
{
	int rc;

	rc = scd4x_write_command(dev, SCD4X_CMD_POWER_DOWN);
	k_sleep(K_MSEC(SCD4X_POWER_DOWN_WAIT_MS));

	return rc;
}
#endif


static void scd4x_wake_up(const struct device *dev)
{
	/*
	 * The sensor does not respond to this command, regardless of whether it was successfully
	 * received and executed or not. As a result, any error that occurs here is not detectable.
	 */
	scd4x_write_command(dev, SCD4X_CMD_WAKE_UP);
	k_sleep(K_MSEC(SCD4X_WAKE_UP_WAIT_MS));
}


static int scd4x_stop_periodic_measurement(const struct device *dev)
{
	int rc;

	rc = scd4x_write_command(dev, SCD4X_CMD_STOP_PERIODIC_MEASUREMENT);
	k_sleep(K_MSEC(SCD4X_STOP_PERIODIC_MEASUREMENT_WAIT_MS));
	
	return rc;
}


static int scd4x_reinit(const struct device *dev)
{
	int rc;

	rc = scd4x_write_command(dev, SCD4X_CMD_REINIT);
	k_sleep(K_MSEC(SCD4X_REINIT_WAIT_MS));

	return rc;
}


static int scd4x_set_temperature_offset(const struct device *dev, uint16_t offset)
{
	int rc;

	/* Datasheet 1.2, section 3.6.1: set_temperature_offset expects converted value */
	uint16_t offset_raw = (uint16_t)(((offset * 65535U) + 87U) / 175U);

	rc = scd4x_write_reg(dev, SCD4X_CMD_SET_TEMPERATURE_OFFSET, offset_raw);
	k_sleep(K_MSEC(SCD4X_SET_TEMPERATURE_OFFSET_WAIT_MS));

	return rc;
}


static int scd4x_get_temperature_offset(const struct device *dev, uint16_t *offset)
{
	int rc;

	uint8_t rx_buf[3];

	rc = scd4x_read_reg(dev, SCD4X_CMD_GET_TEMPERATURE_OFFSET, rx_buf, sizeof(rx_buf));
	k_sleep(K_MSEC(SCD4X_GET_TEMPERATURE_OFFSET_WAIT_MS));

	/* Datasheet 1.2, section 3.6.2: get_temperature_offset provides conversion formula */
	uint16_t offset_raw = sys_get_be16(&rx_buf[0]);

	if (scd4x_compute_crc(offset_raw) != rx_buf[2]) {
		LOG_ERR("Invalid CRC for temperature offset.");
		return -EIO;
	}

	*offset = (uint16_t)((offset_raw * 175U) / 65535U);

	return rc;
}


static int scd4x_set_sensor_altitude(const struct device *dev, uint16_t altitude)
{
	int rc;

	rc = scd4x_write_reg(dev, SCD4X_CMD_SET_SENSOR_ALTITUDE, altitude);
	k_sleep(K_MSEC(SCD4X_SET_SENSOR_ALTITUDE_WAIT_MS));

	return rc;
}


static int scd4x_get_sensor_altitude(const struct device *dev, uint16_t *altitude)
{
	int rc;
	uint8_t rx_buf[3];

	rc = scd4x_read_reg(dev, SCD4X_CMD_GET_SENSOR_ALTITUDE, rx_buf, sizeof(rx_buf));
	k_sleep(K_MSEC(SCD4X_GET_SENSOR_ALTITUDE_WAIT_MS));

	*altitude = sys_get_be16(&rx_buf[0]);

	if (scd4x_compute_crc(*altitude) != rx_buf[2]) {
		LOG_ERR("Invalid CRC for sensor altitude.");
		return -EIO;
	}

	return rc;
}


int scd4x_set_ambient_pressure(const struct device *dev, uint16_t pressure)
{	
	int rc;

	/* Datasheet 1.2, section 3.4: set_sensor_altitude expects altitude value divided by 100, add
	 * 50 first to correct for rounding errors
	 */
	uint16_t raw_value = (pressure + 50U) / 100U;
	rc = scd4x_write_reg(dev, SCD4X_CMD_SET_AMBIENT_PRESSURE, raw_value);
	
	k_sleep(K_MSEC(SCD4X_SET_AMBIENT_PRESSURE_WAIT_MS));

	return rc;
}


static int scd4x_start_periodic_measurement(const struct device *dev, enum scd4x_measure_mode measure_mode) {
	int cmd;

	if (measure_mode == MEASURE_MODE_LOW_POWER) {
		cmd = SCD4X_CMD_START_LOW_POWER_PERIODIC_MEASUREMENT;
	} else if (measure_mode == MEASURE_MODE_NORMAL) {
		cmd = SCD4X_CMD_START_PERIODIC_MEASUREMENT;
	}

	return scd4x_write_command(dev, cmd);
}


/*
 * Retrieve the sensor serial number and stores it in the scd4x_data struct for debugging or future use
 */
static int scd4x_get_serial_number(const struct device *dev)
{
	const struct scd4x_config *cfg = dev->config;
	struct scd4x_data *data = dev->data;
	int rc;

	uint8_t rx_buf[15];

	rc = scd4x_read_reg(dev, SCD4X_CMD_GET_SERIAL_NUMBER, rx_buf, sizeof(rx_buf));
	if (rc < 0) {
		LOG_ERR("Failed to read data from device. (%d)", rc);
		return rc;
	}

	k_sleep(K_MSEC(1));

	uint16_t serial_number0 = sys_get_be16(&rx_buf[0]);
	if (scd4x_compute_crc(serial_number0) != rx_buf[2]) {
		LOG_ERR("Invalid CRC0 for serial number.");
		return -EIO;
	}

	uint16_t serial_number1 = sys_get_be16(&rx_buf[3]);
	if (scd4x_compute_crc(serial_number1) != rx_buf[5]) {
		LOG_ERR("Invalid CRC1 for serial number.");
		return -EIO;
	}

	uint16_t serial_number2 = sys_get_be16(&rx_buf[6]);
	if (scd4x_compute_crc(serial_number2) != rx_buf[8]) {
		LOG_ERR("Invalid CRC2 for serial number.");
		return -EIO;
	}

	snprintf(data->serial_number, sizeof(data->serial_number), "0x%04x%04x%04x", serial_number0, serial_number1, serial_number2);

	return rc;
}


/*
 * Process the measurement returned from the sensor. Response is a 9 byte buffer containing 3 sensor values.
 * 
 * Sensor measurement values are 2 bytes long, and each one is followed by a 1 byte CRC calculated by the
 * sensor.
 * 
 * On SCD41 in single shot measurement mode, if only the temperature and humidity channels have been requested
 * by the user, the sensor will still produce a CO2 value but it will always be 0 ppm.
 */ 
static int scd4x_read_sample(const struct device *dev,
		uint16_t *t_sample,
		uint16_t *rh_sample,
		uint16_t *co2_sample)
{

	const struct scd4x_config *cfg = dev->config;
	uint8_t rx_buf[9];
	int rc;

	rc = i2c_read_dt(&cfg->bus, rx_buf, sizeof(rx_buf));
	if (rc < 0) {
		LOG_ERR("Failed to read data from device.");
		return rc;
	}

	*co2_sample = sys_get_be16(rx_buf);
	if (scd4x_compute_crc(*co2_sample) != rx_buf[2]) {
		LOG_ERR("Invalid CRC for CO2.");
		return -EIO;
	}

	*t_sample = sys_get_be16(&rx_buf[3]);
	if (scd4x_compute_crc(*t_sample) != rx_buf[5]) {
		LOG_ERR("Invalid CRC for T.");
		return -EIO;
	}

	*rh_sample = sys_get_be16(&rx_buf[6]);
	if (scd4x_compute_crc(*rh_sample) != rx_buf[8]) {
		LOG_ERR("Invalid CRC for RH.");
		return -EIO;
	}

	return 0;
}


static int scd4x_sample_fetch(const struct device *dev,
							  enum sensor_channel chan)
{
	const struct scd4x_config *cfg = dev->config;
	struct scd4x_data *data = dev->data;
	int rc;

	if (chan != SENSOR_CHAN_ALL &&
		chan != SENSOR_CHAN_AMBIENT_TEMP &&
		chan != SENSOR_CHAN_HUMIDITY &&
		chan != SENSOR_CHAN_CO2) {
		return -ENOTSUP;
	}
	/*
	 * SCD41 in single shot measure mode. The requested sensor channels determine which command is sent
	 * because the wait time is different by a factor of 100. The full measurement takes 5000ms while
	 * the temperature/humidity only command takes 50ms.
	 */
	if (cfg->model == SCD41 && cfg->measure_mode == MEASURE_MODE_SINGLE_SHOT) {
		#if defined(SCD4X_POWER_DOWN_SINGLE_SHOT_MEASUREMENT)
		/*
		 * Wake up the sensor if necessary before issuing a single shot command, will be powered
		 * down again after reading the measurement.
		 */
		scd4x_wake_up(dev);
		#endif

		if ((chan & SENSOR_CHAN_AMBIENT_TEMP) ||
			(chan & SENSOR_CHAN_HUMIDITY)) {
			rc = scd4x_write_command(dev, SCD4X_CMD_MEASURE_SINGLE_SHOT_RHT_ONLY);
			if (rc < 0 && cfg->model == SCD41) {
				LOG_ERR("Failed to send single shot measure command");
				return rc;
			}
			k_sleep(K_MSEC(SCD4X_MEASURE_SINGLE_SHOT_RHT_ONLY_WAIT_MS));
		} else {
			rc = scd4x_write_command(dev, SCD4X_CMD_MEASURE_SINGLE_SHOT);
			if (rc < 0 && cfg->model == SCD41) {
				LOG_ERR("Failed to send single shot measure command");
				return rc;
			}
			k_sleep(K_MSEC(SCD4X_MEASURE_SINGLE_SHOT_WAIT_MS));
		}
	} else {
		/*
		 * Poll the data ready flag before attempting to read the measurement, otherwise the sensor
		 * will respond with a NACK. 
		 * 
		 * It is assumed that if the sensor has lost power or is otherwise not responding, then scd4x_read_reg
		 * will return an error, which should prevent the kernel from getting stuck in an infinite loop here.
		 */
		uint16_t status_register;
		while (!(SCD4X_MEASURE_READY(status_register))) {
			uint8_t rx_buf[3];
			rc = scd4x_read_reg(dev, SCD4X_CMD_GET_DATA_READY_STATUS, rx_buf, sizeof(rx_buf));
			if (rc) {
				LOG_ERR("Failed to read device status.");
				return rc;
			}

			status_register = sys_get_be16(&rx_buf[0]);

			if (scd4x_compute_crc(status_register) != rx_buf[2]) {
				LOG_ERR("Invalid CRC for data ready flag.");
				return -EIO;
			}

			/*
			 * It could be up to 5000ms before the sensor measurement is ready, checking more often
			 * than this could interfere with other I2C devices on the bus.
			 */
			k_sleep(K_USEC(500));
		}
	}

	/*
	 * Measurement is read from the sensor the same way regardless of which mode is in use.
	 */
	rc = scd4x_write_command(dev, SCD4X_CMD_READ_MEASUREMENT);
	if (rc < 0) {
		LOG_ERR("Failed to start measurement.");
		return rc;
	}
	k_sleep(K_USEC(SCD4X_READ_MEASUREMENT_WAIT_MS));


	rc = scd4x_read_sample(dev, &data->t_sample, &data->rh_sample, &data->co2_sample);
	if (rc < 0) {
		LOG_ERR("Failed to read measurement from device.");
		return rc;
	}

	#if defined(SCD4X_POWER_DOWN_SINGLE_SHOT_MEASUREMENT)
	if (cfg->model == SCD41 && cfg->measure_mode == MEASURE_MODE_SINGLE_SHOT) {
		/* 
		 * Put the sensor to sleep again until the next measurement
		 */
		scd4x_power_down(dev);
	}
	#endif

	return 0;
}

static int scd4x_channel_get(const struct device *dev,
							 enum sensor_channel chan,
							 struct sensor_value *val)
{

	const struct scd4x_data *data = dev->data;

	if (chan == SENSOR_CHAN_AMBIENT_TEMP) {
		int64_t tmp;

		tmp = data->t_sample * 175U;
		val->val1 = (int32_t)(tmp / 0xFFFFU) - 45U;
		val->val2 = ((tmp % 0xFFFFU) * 1000000U) / 0xFFFFU;
	} else if (chan == SENSOR_CHAN_HUMIDITY) {
		uint64_t tmp;

		tmp = data->rh_sample * 100U;
		val->val1 = tmp / 0x10000U;
		val->val2 = (tmp % 0x10000U) * 15625U / 1024U;
	} else if (chan == SENSOR_CHAN_CO2) {
		val->val1 = data->co2_sample;
		val->val2 = 0;
	} else {
		return -ENOTSUP;
	}

	return 0;
}


#if defined(CONFIG_PM_DEVICE)
static int scd4x_pm_action(const struct device *dev, 
						   enum pm_device_action action)
{

	const struct scd4x_config *cfg = dev->config;
	int rc;
	
	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		scd4x_wake_up(dev);
		rc = scd4x_start_periodic_measurement(dev, cfg->measure_mode);
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		rc = scd4x_stop_periodic_measurement(dev);
		rc = scd4x_power_down(dev);
		break;
	default:
		return -ENOTSUP;
	}

	return rc;
}
#endif /* CONFIG_PM_DEVICE */


static int scd4x_init(const struct device *dev)
{
	const struct scd4x_config *cfg = dev->config;
	struct scd4x_data *data = dev->data;
	int rc = 0;

	if (!device_is_ready(cfg->bus.bus)) {
		LOG_ERR("Device not ready.");
		return -ENODEV;
	}

	scd4x_wake_up(dev);

	rc = scd4x_stop_periodic_measurement(dev);
	if (rc < 0) {
		LOG_ERR("Failed to stop periodic measurement on the device.");
		return rc;
	}

	rc = scd4x_reinit(dev);
	if (rc < 0) {
		LOG_ERR("Failed to reinitialize the device.");
		return rc;
	}

	rc = scd4x_set_sensor_altitude(dev, cfg->altitude);
	if (rc < 0) {
		LOG_ERR("Failed to set sensor altitude on the device.");
		return rc;
	}


	uint16_t sensor_altitude;
	rc = scd4x_get_sensor_altitude(dev, &sensor_altitude);
	if (rc < 0) {
		LOG_ERR("Failed to get sensor altitude from the device.");
		return rc;
	}


	rc = scd4x_set_temperature_offset(dev, cfg->temperature_offset);
	if (rc < 0) {
		LOG_ERR("Failed to set temperature offset on the device.");
		return rc;
	}


	int16_t temperature_offset;
	rc = scd4x_get_temperature_offset(dev, &temperature_offset);
	if (rc < 0) {
		LOG_ERR("Failed to get temperature offset from the device.");
		return rc;
	}


	rc = scd4x_get_serial_number(dev);
	if (rc < 0) {
		LOG_ERR("Failed to read serial number from the device.");
		return rc;
	}


	rc = scd4x_write_reg(dev, SCD4X_CMD_SET_AUTOMATIC_SELF_CALIBRATION_ENABLED, cfg->auto_calibration);
	if (rc < 0) {
		LOG_ERR("Failed to set auto calibration on the device.");
		return rc;
	}
	k_sleep(K_MSEC(SCD4X_SET_AUTOMATIC_CALIBRATION_WAIT_MS));

	if (cfg->measure_mode == MEASURE_MODE_SINGLE_SHOT) {
		#if defined(SCD4X_POWER_DOWN_SINGLE_SHOT_MEASUREMENT)
		/*
		 * Power down the sensor until the first measurement is requested
		 */
		scd4x_power_down(dev);
		#endif
	} else {
		rc = scd4x_start_periodic_measurement(dev, cfg->measure_mode);
		if (rc < 0) {
			LOG_ERR("Failed to start periodic measurement on the device.");
			return rc;
		}
	}

	return 0;
}


static const struct sensor_driver_api scd4x_api = {
	.sample_fetch = scd4x_sample_fetch,
	.channel_get = scd4x_channel_get,
};


#define SCD4X_INIT(n)						\
	static struct scd4x_data scd4x_data_##n;		\
								\
	static const struct scd4x_config scd4x_config_##n = {	\
		.bus = I2C_DT_SPEC_INST_GET(n),			\
		.model = DT_INST_ENUM_IDX(n, model),	\
		.measure_mode = DT_INST_ENUM_IDX(n, measure_mode),	\
		.auto_calibration = DT_INST_PROP(n, auto_calibration),	\
		.temperature_offset = DT_INST_PROP(n, temperature_offset),	\
		.altitude = DT_INST_PROP(n, altitude)	\
	};							\
								\
	PM_DEVICE_DT_INST_DEFINE(n, scd4x_pm_action);		\
								\
	DEVICE_DT_INST_DEFINE(n,				\
			      scd4x_init,			\
			      NULL,				\
			      &scd4x_data_##n,			\
			      &scd4x_config_##n,		\
			      POST_KERNEL,			\
			      CONFIG_SENSOR_INIT_PRIORITY,	\
			      &scd4x_api);

DT_INST_FOREACH_STATUS_OKAY(SCD4X_INIT)

