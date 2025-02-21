/*
 * Copyright (c) 2024 Jan Fäh
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/crc.h>
#include <zephyr/devicetree.h>

#include <zephyr/drivers/sensor/scd4x.h>
#include "scd4x.h"

LOG_MODULE_REGISTER(SCD4X, CONFIG_SENSOR_LOG_LEVEL);

static uint8_t scd4x_calc_crc(uint16_t value)
{
	uint8_t buf[2];

	sys_put_be16(value, buf);

	return crc8(buf, 2, SCD4X_CRC_POLY, SCD4X_CRC_INIT, false);
}

static int scd4x_write_command(const struct device *dev, uint8_t cmd)
{
	const struct scd4x_config *cfg = dev->config;
	uint8_t tx_buf[2];
	int ret;

	sys_put_be16(scd4x_cmds[cmd].cmd, tx_buf);

	ret = i2c_write_dt(&cfg->bus, tx_buf, sizeof(tx_buf));

	if (scd4x_cmds[cmd].cmd_duration_ms) {
		k_msleep(scd4x_cmds[cmd].cmd_duration_ms);
	}

	return ret;
}

static int scd4x_read_reg(const struct device *dev, uint8_t *rx_buf, uint8_t rx_buf_size)
{
	const struct scd4x_config *cfg = dev->config;
	int ret;

	ret = i2c_read_dt(&cfg->bus, rx_buf, rx_buf_size);
	if (ret < 0) {
		LOG_ERR("Failed to read i2c data.");
		return ret;
	}

	for (uint8_t i = 0; i < (rx_buf_size / 3); i++) {
		ret = scd4x_calc_crc(sys_get_be16(&rx_buf[i * 3]));
		if (ret != rx_buf[(i * 3) + 2]) {
			LOG_ERR("Invalid CRC.");
			return -EIO;
		}
	}

	return 0;
}

static int scd4x_write_reg(const struct device *dev, uint8_t cmd, uint16_t *data, uint8_t data_size)
{
	const struct scd4x_config *cfg = dev->config;
	int ret;
	uint8_t tx_buf[((data_size * 3) + 2)];

	sys_put_be16(scd4x_cmds[cmd].cmd, tx_buf);

	uint8_t tx_buf_pos = 2;

	for (uint8_t i = 0; i < data_size; i++) {
		sys_put_be16(data[i], &tx_buf[tx_buf_pos]);
		tx_buf_pos += 2;
		tx_buf[tx_buf_pos++] = scd4x_calc_crc(data[i]);
	}

	ret = i2c_write_dt(&cfg->bus, tx_buf, sizeof(tx_buf));
	if (ret < 0) {
		LOG_ERR("Failed to write i2c data.");
		return ret;
	}

	if (scd4x_cmds[cmd].cmd_duration_ms) {
		k_msleep(scd4x_cmds[cmd].cmd_duration_ms);
	}
	return 0;
}

static int scd4x_data_ready(const struct device *dev, bool *is_data_ready)
{
	uint8_t rx_data[3];
	int ret;
	*is_data_ready = false;

	ret = scd4x_write_command(dev, SCD4X_CMD_GET_DATA_READY_STATUS);
	if (ret < 0) {
		LOG_ERR("Failed to write get_data_ready_status command.");
		return ret;
	}

	ret = scd4x_read_reg(dev, rx_data, sizeof(rx_data));
	if (ret < 0) {
		LOG_ERR("Failed to read get_data_ready_status register.");
		return ret;
	}

	uint16_t word = sys_get_be16(rx_data);
	/* Least significant 11 bits = 0 --> data not ready */
	if ((word & 0x07FF) > 0) {
		*is_data_ready = true;
	}

	return 0;
}

static int scd4x_read_sample(const struct device *dev)
{
	struct scd4x_data *data = dev->data;
	uint8_t rx_data[9];
	int ret;

	ret = scd4x_write_command(dev, SCD4X_CMD_READ_MEASUREMENT);
	if (ret < 0) {
		LOG_ERR("Failed to write read_measurement command.");
		return ret;
	}

	ret = scd4x_read_reg(dev, rx_data, sizeof(rx_data));
	if (ret < 0) {
		LOG_ERR("Failed to read read_measurement register.");
		return ret;
	}

	data->co2_sample = sys_get_be16(rx_data);
	data->temp_sample = sys_get_be16(&rx_data[3]);
	data->humi_sample = sys_get_be16(&rx_data[6]);

	return 0;
}

static int scd4x_setup_measurement(const struct device *dev)
{
	const struct scd4x_config *cfg = dev->config;
	int ret;

	switch ((enum scd4x_mode_t)cfg->mode) {
	case SCD4X_MODE_NORMAL:
		ret = scd4x_write_command(dev, SCD4X_CMD_START_PERIODIC_MEASUREMENT);
		if (ret < 0) {
			LOG_ERR("Failed to write start_periodic_measurement command.");
			return ret;
		}
		break;
	case SCD4X_MODE_LOW_POWER:
		ret = scd4x_write_command(dev, SCD4X_CMD_LOW_POWER_PERIODIC_MEASUREMENT);
		if (ret < 0) {
			LOG_ERR("Failed to write start_low_power_periodic_measurement command.");
			return ret;
		}
		break;
	case SCD4X_MODE_SINGLE_SHOT:
		ret = scd4x_write_command(dev, SCD4X_CMD_POWER_DOWN);
		if (ret < 0) {
			LOG_ERR("Failed to write power_down command.");
			return ret;
		}
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int scd4x_set_idle_mode(const struct device *dev)
{
	const struct scd4x_config *cfg = dev->config;
	int ret;

	if (cfg->mode == SCD4X_MODE_SINGLE_SHOT) {
		/*send wake up command twice because of an expected nack return in power down mode*/
		scd4x_write_command(dev, SCD4X_CMD_WAKE_UP);
		ret = scd4x_write_command(dev, SCD4X_CMD_WAKE_UP);
		if (ret < 0) {
			LOG_ERR("Failed write wake_up command.");
			return ret;
		}
	} else {
		ret = scd4x_write_command(dev, SCD4X_CMD_STOP_PERIODIC_MEASUREMENT);
		if (ret < 0) {
			LOG_ERR("Failed to write stop_periodic_measurement command.");
			return ret;
		}
	}

	return 0;
}

static int scd4x_set_temperature_offset(const struct device *dev, const struct sensor_value *val)
{
	int ret;
	/*Calculation from Datasheet*/
	uint16_t offset_temp =
		(float)(val->val1 + (val->val2 / 1000000.0)) * 0xFFFF / SCD4X_MAX_TEMP;

	ret = scd4x_write_reg(dev, SCD4X_CMD_SET_TEMPERATURE_OFFSET, &offset_temp, 1);
	if (ret < 0) {
		LOG_ERR("Failed to write set_temperature_offset register.");
		return ret;
	}

	return 0;
}

static int scd4x_set_sensor_altitude(const struct device *dev, const struct sensor_value *val)
{
	int ret;
	uint16_t altitude = val->val1;

	ret = scd4x_write_reg(dev, SCD4X_CMD_SET_SENSOR_ALTITUDE, &altitude, 1);
	if (ret < 0) {
		LOG_ERR("Failed to write set_sensor_altitude register.");
		return ret;
	}
	return 0;
}

static int scd4x_set_ambient_pressure(const struct device *dev, const struct sensor_value *val)
{
	int ret;

	uint16_t ambient_pressure = val->val1;

	ret = scd4x_write_reg(dev, SCD4X_CMD_SET_AMBIENT_PRESSURE, &ambient_pressure, 1);
	if (ret < 0) {
		LOG_ERR("Failed to write set_ambient_pressure register.");
		return ret;
	}

	return 0;
}

static int scd4x_set_automatic_calib_enable(const struct device *dev,
					    const struct sensor_value *val)
{
	int ret;
	uint16_t automatic_calib_enable = val->val1;

	ret = scd4x_write_reg(dev, SCD4X_CMD_SET_AUTOMATIC_CALIB_ENABLE, &automatic_calib_enable,
			      1);
	if (ret < 0) {
		LOG_ERR("Failed to write set_automatic_calibration_enable register.");
		return ret;
	}

	return 0;
}

static int scd4x_set_self_calib_initial_period(const struct device *dev,
					       const struct sensor_value *val)
{
	int ret;
	uint16_t initial_period = val->val1;

	ret = scd4x_write_reg(dev, SCD4X_CMD_SET_SELF_CALIB_INITIAL_PERIOD, &initial_period, 1);
	if (ret < 0) {
		LOG_ERR("Failed to write set_automatic_self_calibration_initial_period register.");
		return ret;
	}

	return 0;
}

static int scd4x_set_self_calib_standard_period(const struct device *dev,
						const struct sensor_value *val)
{
	int ret;
	uint16_t standard_period = val->val1;

	ret = scd4x_write_reg(dev, SCD4X_CMD_SET_SELF_CALIB_STANDARD_PERIOD, &standard_period, 1);
	if (ret < 0) {
		LOG_ERR("Failed to write set_automatic_self_calibration_standard_period register.");
		return ret;
	}

	return 0;
}

static int scd4x_get_temperature_offset(const struct device *dev, struct sensor_value *val)
{
	int ret;
	uint8_t rx_buf[3];

	ret = scd4x_write_command(dev, SCD4X_CMD_GET_TEMPERATURE_OFFSET);
	if (ret < 0) {
		LOG_ERR("Failed to write get_temperature_offset command.");
		return ret;
	}

	ret = scd4x_read_reg(dev, rx_buf, sizeof(rx_buf));
	if (ret < 0) {
		LOG_ERR("Failed to read get_temperature_offset register.");
		return ret;
	}

	int32_t temp;

	/*Calculation from Datasheet*/
	temp = sys_get_be16(rx_buf) * SCD4X_MAX_TEMP;
	val->val1 = (int32_t)(temp / 0xFFFF);
	val->val2 = ((temp % 0xFFFF) * 1000000) / 0xFFFF;

	return 0;
}

static int scd4x_get_sensor_altitude(const struct device *dev, struct sensor_value *val)
{
	int ret;
	uint8_t rx_buf[3];

	ret = scd4x_write_command(dev, SCD4X_CMD_GET_SENSOR_ALTITUDE);
	if (ret < 0) {
		LOG_ERR("Failed to write get_sensor_altitude command.");
		return ret;
	}

	ret = scd4x_read_reg(dev, rx_buf, sizeof(rx_buf));
	if (ret < 0) {
		LOG_ERR("Failed to read get_sensor_altitude register.");
		return ret;
	}

	val->val1 = sys_get_be16(rx_buf);
	val->val2 = 0;

	return 0;
}

static int scd4x_get_ambient_pressure(const struct device *dev, struct sensor_value *val)
{
	int ret;
	uint8_t rx_buf[3];

	ret = scd4x_write_command(dev, SCD4X_CMD_GET_AMBIENT_PRESSURE);
	if (ret < 0) {
		LOG_ERR("Failed to write get_ambient_pressure command.");
		return ret;
	}

	ret = scd4x_read_reg(dev, rx_buf, sizeof(rx_buf));
	if (ret < 0) {
		LOG_ERR("Failed to read get_ambient_pressure register.");
		return ret;
	}

	val->val1 = sys_get_be16(rx_buf);
	val->val2 = 0;

	return 0;
}

static int scd4x_get_automatic_calib_enable(const struct device *dev, struct sensor_value *val)
{
	int ret;
	uint8_t rx_buf[3];

	ret = scd4x_write_command(dev, SCD4X_CMD_GET_AUTOMATIC_CALIB_ENABLE);
	if (ret < 0) {
		LOG_ERR("Failed to write get_automatic_calibration_enable command.");
		return ret;
	}

	ret = scd4x_read_reg(dev, rx_buf, sizeof(rx_buf));
	if (ret < 0) {
		LOG_ERR("Failed to read get_automatic_calibration_enabled register.");
		return ret;
	}

	val->val1 = sys_get_be16(rx_buf);
	val->val2 = 0;

	return 0;
}

static int scd4x_get_self_calib_initial_period(const struct device *dev, struct sensor_value *val)
{
	int ret;
	uint8_t rx_buf[3];

	ret = scd4x_write_command(dev, SCD4X_CMD_GET_SELF_CALIB_INITIAL_PERIOD);
	if (ret < 0) {
		LOG_ERR("Failed to write get_automati_calibration_initial_period command.");
		return ret;
	}

	ret = scd4x_read_reg(dev, rx_buf, sizeof(rx_buf));
	if (ret < 0) {
		LOG_ERR("Failed to read get_automatic_calibration_initial_period register.");
		return ret;
	}

	val->val1 = sys_get_be16(rx_buf);
	val->val2 = 0;

	return 0;
}

static int scd4x_get_self_calib_standard_period(const struct device *dev, struct sensor_value *val)
{
	int ret;
	uint8_t rx_buf[3];

	ret = scd4x_write_command(dev, SCD4X_CMD_GET_SELF_CALIB_STANDARD_PERIOD);
	if (ret < 0) {
		LOG_ERR("Failed to write get_automatic_self_calibration_standard_period command.");
		return ret;
	}

	ret = scd4x_read_reg(dev, rx_buf, sizeof(rx_buf));
	if (ret < 0) {
		LOG_ERR("Failed to read get_automatic_self_calibration_standard_period register.");
		return ret;
	}

	val->val1 = sys_get_be16(rx_buf);
	val->val2 = 0;

	return 0;
}

int scd4x_forced_recalibration(const struct device *dev, uint16_t target_concentration,
			       uint16_t *frc_correction)
{
	uint8_t rx_buf[3];
	int ret;

	ret = scd4x_set_idle_mode(dev);
	if (ret < 0) {
		LOG_ERR("Failed to set idle mode.");
		return ret;
	}

	ret = scd4x_write_reg(dev, SCD4X_CMD_FORCED_RECALIB, &target_concentration, 1);
	if (ret < 0) {
		LOG_ERR("Failed to write perform_forced_recalibration register.");
		return ret;
	}

	ret = scd4x_read_reg(dev, rx_buf, sizeof(rx_buf));
	if (ret < 0) {
		LOG_ERR("Failed to read perform_forced_recalibration register.");
		return ret;
	}

	*frc_correction = sys_get_be16(rx_buf);

	/*from datasheet*/
	if (*frc_correction == 0xFFFF) {
		LOG_ERR("FRC failed. Returned 0xFFFF.");
		return -EIO;
	}

	*frc_correction -= 0x8000;

	ret = scd4x_setup_measurement(dev);
	if (ret < 0) {
		LOG_ERR("Failed to setup measurement.");
		return ret;
	}

	return 0;
}

int scd4x_self_test(const struct device *dev)
{
	int ret;
	uint8_t rx_buf[3];

	ret = scd4x_set_idle_mode(dev);
	if (ret < 0) {
		LOG_ERR("Failed to set idle mode.");
		return ret;
	}

	ret = scd4x_write_command(dev, SCD4X_CMD_SELF_TEST);
	if (ret < 0) {
		LOG_ERR("Failed to write perform_self_test command.");
		return ret;
	}

	ret = scd4x_read_reg(dev, rx_buf, sizeof(rx_buf));
	if (ret < 0) {
		LOG_ERR("Failed to read perform_self_test register.");
		return ret;
	}

	uint16_t is_malfunction = sys_get_be16(rx_buf);

	if (is_malfunction) {
		LOG_ERR("malfunction detected.");
		return -EIO;
	}

	ret = scd4x_setup_measurement(dev);
	if (ret < 0) {
		LOG_ERR("Failed to setup measurement.");
		return ret;
	}

	return 0;
}

int scd4x_persist_settings(const struct device *dev)
{
	int ret;

	ret = scd4x_set_idle_mode(dev);
	if (ret < 0) {
		LOG_ERR("Failed to set idle mode.");
		return ret;
	}

	ret = scd4x_write_command(dev, SCD4X_CMD_PERSIST_SETTINGS);
	if (ret < 0) {
		LOG_ERR("Failed to write persist_settings command.");
		return ret;
	}

	ret = scd4x_setup_measurement(dev);
	if (ret < 0) {
		LOG_ERR("Failed to setup measurement.");
		return ret;
	}

	return 0;
}

int scd4x_factory_reset(const struct device *dev)
{
	int ret;

	ret = scd4x_set_idle_mode(dev);
	if (ret < 0) {
		LOG_ERR("Failed to set idle mode.");
		return ret;
	}

	ret = scd4x_write_command(dev, SCD4X_CMD_FACTORY_RESET);
	if (ret < 0) {
		LOG_ERR("Failed to write perfom_factory_reset command.");
		return ret;
	}

	ret = scd4x_setup_measurement(dev);
	if (ret < 0) {
		LOG_ERR("Failed to setup measurement.");
		return ret;
	}

	return 0;
}

static int scd4x_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct scd4x_config *cfg = dev->config;
	bool is_data_ready;
	int ret;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_AMBIENT_TEMP &&
	    chan != SENSOR_CHAN_HUMIDITY && chan != SENSOR_CHAN_CO2) {
		return -ENOTSUP;
	}

	if (cfg->mode == SCD4X_MODE_SINGLE_SHOT) {
		ret = scd4x_set_idle_mode(dev);
		if (ret < 0) {
			LOG_ERR("Failed to set idle mode.");
			return ret;
		}

		if (chan == SENSOR_CHAN_HUMIDITY || chan == SENSOR_CHAN_AMBIENT_TEMP) {
			ret = scd4x_write_command(dev, SCD4X_CMD_MEASURE_SINGLE_SHOT_RHT);
			if (ret < 0) {
				LOG_ERR("Failed to write measure_single_shot_rht_only command.");
				return ret;
			}
		} else {
			ret = scd4x_write_command(dev, SCD4X_CMD_MEASURE_SINGLE_SHOT);
			if (ret < 0) {
				LOG_ERR("Failed to write measure_single_shot command.");
				return ret;
			}
		}
	} else {
		ret = scd4x_data_ready(dev, &is_data_ready);
		if (ret < 0) {
			LOG_ERR("Failed to check data ready.");
			return ret;
		}
		if (!is_data_ready) {
			return 0;
		}
	}

	ret = scd4x_read_sample(dev);
	if (ret < 0) {
		LOG_ERR("Failed to get sample data.");
		return ret;
	}

	if (cfg->mode == SCD4X_MODE_SINGLE_SHOT) {
		ret = scd4x_setup_measurement(dev);
		if (ret < 0) {
			LOG_ERR("Failed to setup measurement.");
			return ret;
		}
	}
	return 0;
}

static int scd4x_channel_get(const struct device *dev, enum sensor_channel chan,
			     struct sensor_value *val)
{
	const struct scd4x_data *data = dev->data;
	int64_t tmp_val;

	switch ((enum sensor_channel)chan) {
	case SENSOR_CHAN_AMBIENT_TEMP:
		/*Calculation from Datasheet*/
		tmp_val = data->temp_sample * SCD4X_MAX_TEMP;
		val->val1 = (int32_t)(tmp_val / 0xFFFF) + SCD4X_MIN_TEMP;
		val->val2 = ((tmp_val % 0xFFFF) * 1000000) / 0xFFFF;
		break;
	case SENSOR_CHAN_HUMIDITY:
		/*Calculation from Datasheet*/
		tmp_val = data->humi_sample * 100;
		val->val1 = (int32_t)(tmp_val / 0xFFFF);
		val->val2 = ((tmp_val % 0xFFFF) * 1000000) / 0xFFFF;
		break;
	case SENSOR_CHAN_CO2:
		val->val1 = data->co2_sample;
		val->val2 = 0;
		break;
	default:
		return -ENOTSUP;
	}
	return 0;
}

int scd4x_attr_set(const struct device *dev, enum sensor_channel chan, enum sensor_attribute attr,
		   const struct sensor_value *val)
{
	const struct scd4x_config *cfg = dev->config;
	int ret;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_AMBIENT_TEMP &&
	    chan != SENSOR_CHAN_HUMIDITY && chan != SENSOR_CHAN_CO2) {
		return -ENOTSUP;
	}

	if ((enum sensor_attribute_scd4x)attr != SENSOR_ATTR_SCD4X_AMBIENT_PRESSURE) {
		ret = scd4x_set_idle_mode(dev);
		if (ret < 0) {
			LOG_ERR("Failed to set idle mode.");
			return ret;
		}
	}

	if (val->val1 < 0 || val->val2 < 0) {
		return -EINVAL;
	}

	switch ((enum sensor_attribute_scd4x)attr) {
	case SENSOR_ATTR_SCD4X_TEMPERATURE_OFFSET:
		if (val->val1 > SCD4X_TEMPERATURE_OFFSET_IDX_MAX) {
			return -EINVAL;
		}
		ret = scd4x_set_temperature_offset(dev, val);
		if (ret < 0) {
			LOG_ERR("Failed to set temperature offset.");
			return ret;
		}
		break;
	case SENSOR_ATTR_SCD4X_SENSOR_ALTITUDE:
		if (val->val1 > SCD4X_SENSOR_ALTITUDE_IDX_MAX) {
			return -EINVAL;
		}
		ret = scd4x_set_sensor_altitude(dev, val);
		if (ret < 0) {
			LOG_ERR("Failed to set sensor altitude.");
			return ret;
		}
		break;
	case SENSOR_ATTR_SCD4X_AMBIENT_PRESSURE:
		if (val->val1 > SCD4X_AMBIENT_PRESSURE_IDX_MAX || val->val1 < 700) {
			return -EINVAL;
		}
		ret = scd4x_set_ambient_pressure(dev, val);
		if (ret < 0) {
			LOG_ERR("Failed to set ambient pressure.");
			return ret;
		}
		/* return 0 to not call scd4x_start_measurement */
		return 0;
	case SENSOR_ATTR_SCD4X_AUTOMATIC_CALIB_ENABLE:
		if (val->val1 > SCD4X_BOOL_IDX_MAX) {
			return -EINVAL;
		}
		ret = scd4x_set_automatic_calib_enable(dev, val);
		if (ret < 0) {
			LOG_ERR("Failed to set automatic calib enable.");
			return ret;
		}
		break;
	case SENSOR_ATTR_SCD4X_SELF_CALIB_INITIAL_PERIOD:
		if (val->val1 % 4) {
			return -EINVAL;
		}
		if (cfg->model == SCD4X_MODEL_SCD40) {
			LOG_ERR("SELF_CALIB_INITIAL_PERIOD not available for SCD40.");
			return -ENOTSUP;
		}
		ret = scd4x_set_self_calib_initial_period(dev, val);
		if (ret < 0) {
			LOG_ERR("Failed to set self calib initial period.");
			return ret;
		}
		break;
	case SENSOR_ATTR_SCD4X_SELF_CALIB_STANDARD_PERIOD:
		if (val->val1 % 4) {
			return -EINVAL;
		}
		if (cfg->model == SCD4X_MODEL_SCD40) {
			LOG_ERR("SELF_CALIB_STANDARD_PERIOD not available for SCD40.");
			return -ENOTSUP;
		}
		ret = scd4x_set_self_calib_standard_period(dev, val);
		if (ret < 0) {
			LOG_ERR("Failed to set self calib standard period.");
			return ret;
		}
		break;
	default:
		return -ENOTSUP;
	}

	ret = scd4x_setup_measurement(dev);
	if (ret < 0) {
		LOG_ERR("Failed to setup measurement.");
		return ret;
	}

	return 0;
}

static int scd4x_attr_get(const struct device *dev, enum sensor_channel chan,
			  enum sensor_attribute attr, struct sensor_value *val)
{
	const struct scd4x_config *cfg = dev->config;
	int ret;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_AMBIENT_TEMP &&
	    chan != SENSOR_CHAN_HUMIDITY && chan != SENSOR_CHAN_CO2) {
		return -ENOTSUP;
	}

	if ((enum sensor_attribute_scd4x)attr != SENSOR_ATTR_SCD4X_AMBIENT_PRESSURE ||
	    cfg->mode == SCD4X_MODE_SINGLE_SHOT) {
		ret = scd4x_set_idle_mode(dev);
		if (ret < 0) {
			LOG_ERR("Failed to set idle mode.");
			return ret;
		}
	}

	switch ((enum sensor_attribute_scd4x)attr) {
	case SENSOR_ATTR_SCD4X_TEMPERATURE_OFFSET:
		ret = scd4x_get_temperature_offset(dev, val);
		if (ret < 0) {
			LOG_ERR("Failed to get temperature offset.");
			return ret;
		}
		break;
	case SENSOR_ATTR_SCD4X_SENSOR_ALTITUDE:
		ret = scd4x_get_sensor_altitude(dev, val);
		if (ret < 0) {
			LOG_ERR("Failed to get sensor altitude.");
			return ret;
		}
		break;
	case SENSOR_ATTR_SCD4X_AMBIENT_PRESSURE:
		ret = scd4x_get_ambient_pressure(dev, val);
		if (ret < 0) {
			LOG_ERR("Failed to get ambient pressure.");
			return ret;
		}
		/* return 0 to not call scd4x_setup_measurement */
		return 0;
	case SENSOR_ATTR_SCD4X_AUTOMATIC_CALIB_ENABLE:
		ret = scd4x_get_automatic_calib_enable(dev, val);
		if (ret < 0) {
			LOG_ERR("Failed to get automatic calib.");
			return ret;
		}
		break;
	case SENSOR_ATTR_SCD4X_SELF_CALIB_INITIAL_PERIOD:
		if (cfg->model == SCD4X_MODEL_SCD40) {
			LOG_ERR("SELF_CALIB_INITIAL_PERIOD not available for SCD40.");
			return -ENOTSUP;
		}
		ret = scd4x_get_self_calib_initial_period(dev, val);
		if (ret < 0) {
			LOG_ERR("Failed to set get self calib initial period.");
			return ret;
		}
		break;
	case SENSOR_ATTR_SCD4X_SELF_CALIB_STANDARD_PERIOD:
		if (cfg->model == SCD4X_MODEL_SCD40) {
			LOG_ERR("SELF_CALIB_STANDARD_PERIOD not available for SCD40.");
			return -ENOTSUP;
		}
		ret = scd4x_get_self_calib_standard_period(dev, val);
		if (ret < 0) {
			LOG_ERR("Failed to set get self calib standard period.");
			return ret;
		}
		break;
	default:
		return -ENOTSUP;
	}

	ret = scd4x_setup_measurement(dev);
	if (ret < 0) {
		LOG_ERR("Failed to setup measurement.");
		return ret;
	}

	return 0;
}

static int scd4x_init(const struct device *dev)
{
	const struct scd4x_config *cfg = dev->config;
	int ret;

	if (!i2c_is_ready_dt(&cfg->bus)) {
		LOG_ERR("Device not ready.");
		return -ENODEV;
	}

	ret = scd4x_write_command(dev, SCD4X_CMD_STOP_PERIODIC_MEASUREMENT);
	if (ret < 0) {
		/*send wake up command twice because of an expected nack return in power down mode*/
		scd4x_write_command(dev, SCD4X_CMD_WAKE_UP);
		ret = scd4x_write_command(dev, SCD4X_CMD_WAKE_UP);
		if (ret < 0) {
			LOG_ERR("Failed to put the device in idle mode.");
			return ret;
		}
	}

	ret = scd4x_write_command(dev, SCD4X_CMD_REINIT);
	if (ret < 0) {
		LOG_ERR("Failed to reset the device.");
		return ret;
	}

	ret = scd4x_setup_measurement(dev);
	if (ret < 0) {
		LOG_ERR("Failed to setup measurement.");
		return ret;
	}
	return 0;
}

static DEVICE_API(sensor, scd4x_api_funcs) = {
	.sample_fetch = scd4x_sample_fetch,
	.channel_get = scd4x_channel_get,
	.attr_set = scd4x_attr_set,
	.attr_get = scd4x_attr_get,
};

#define SCD4X_INIT(inst, scd4x_model)                                                              \
	static struct scd4x_data scd4x_data_##scd4x_model##_##inst;                                \
	static const struct scd4x_config scd4x_config_##scd4x_model##_##inst = {                   \
		.bus = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.model = scd4x_model,                                                              \
		.mode = DT_INST_ENUM_IDX_OR(inst, mode, SCD4X_MODE_NORMAL),                        \
	};                                                                                         \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, scd4x_init, NULL, &scd4x_data_##scd4x_model##_##inst,   \
				     &scd4x_config_##scd4x_model##_##inst, POST_KERNEL,            \
				     CONFIG_SENSOR_INIT_PRIORITY, &scd4x_api_funcs);

#define DT_DRV_COMPAT sensirion_scd40
DT_INST_FOREACH_STATUS_OKAY_VARGS(SCD4X_INIT, SCD4X_MODEL_SCD40)
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT sensirion_scd41
DT_INST_FOREACH_STATUS_OKAY_VARGS(SCD4X_INIT, SCD4X_MODEL_SCD41)
#undef DT_DRV_COMPAT
