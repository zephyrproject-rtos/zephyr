/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_ina7xx

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/byteorder.h>
#include "ina7xx.h"

LOG_MODULE_REGISTER(INA7xx, CONFIG_SENSOR_LOG_LEVEL);

#define MAX_RETRIES 4

struct ina7xx_config {
	struct i2c_dt_spec bus;
	uint8_t inatype;
	uint8_t mode;
	uint8_t convdly;
	uint8_t vbusct;
	uint8_t vsenct;
	uint8_t tct;
	uint8_t avg;
};

struct ina7xx_data {
	uint16_t config;
	uint8_t convdly;
	uint16_t adc_config;
	uint16_t v_bus;
	uint16_t die_temp;
	uint16_t current;
	uint32_t power;
	uint8_t valid_readings_mask;
	uint16_t current_mul_ua;
	uint16_t power_mul_uw;
};

static int ina7xx_read16(const struct device *dev, uint8_t reg_addr, uint16_t *reg_data)
{
	const struct ina7xx_config *cfg = dev->config;
	uint8_t rx_buf[2];
	int rc;

	rc = i2c_write_read_dt(&cfg->bus, &reg_addr, sizeof(reg_addr), rx_buf, sizeof(rx_buf));

	*reg_data = sys_get_be16(rx_buf);

	return rc;
}

static int ina7xx_read24(const struct device *dev, uint8_t reg_addr, uint32_t *reg_data)
{
	const struct ina7xx_config *cfg = dev->config;
	uint8_t rx_buf[3];
	int rc;

	rc = i2c_write_read_dt(&cfg->bus, &reg_addr, sizeof(reg_addr), rx_buf, sizeof(rx_buf));

	*reg_data = sys_get_be24(rx_buf);

	return rc;
}

static int ina7xx_write(const struct device *dev, uint8_t addr, uint16_t reg_data)
{
	const struct ina7xx_config *cfg = dev->config;
	uint8_t tx_buf[3];

	tx_buf[0] = addr;
	sys_put_be16(reg_data, &tx_buf[1]);

	return i2c_write_dt(&cfg->bus, tx_buf, sizeof(tx_buf));
}

static int ina7xx_reg_field_update(const struct device *dev, uint8_t addr, uint16_t mask,
				   uint16_t field)
{
	uint16_t reg_data;
	int rc;

	rc = ina7xx_read16(dev, addr, &reg_data);
	if (rc < 0) {
		return rc;
	}

	reg_data = (reg_data & ~mask) | field;

	return ina7xx_write(dev, addr, reg_data);
}

static int ina7xx_set_config(const struct device *dev)
{
	const struct ina7xx_config *cfg = dev->config;
	uint16_t reg_data;
	int rc;

	reg_data = (FIELD_PREP(INA7xx_RSTACC, INA7xx_RSTACC_RESET)) |
		   (FIELD_PREP(INA7xx_CONVDLY, cfg->convdly));

	rc = ina7xx_write(dev, INA7xx_REG_CONFIG, reg_data);
	if (rc < 0) {
		LOG_ERR("Could not write config register");
		return rc;
	}

	reg_data = (FIELD_PREP(INA7xx_MODE, cfg->mode)) | (FIELD_PREP(INA7xx_VBUSCT, cfg->vbusct)) |
		   (FIELD_PREP(INA7xx_VSENCT, cfg->vsenct)) | (FIELD_PREP(INA7xx_TCT, cfg->tct)) |
		   (FIELD_PREP(INA7xx_AVG, cfg->avg));
	rc = ina7xx_write(dev, INA7xx_REG_ADC_CONFIG, reg_data);

	if (rc < 0) {
		LOG_ERR("Could not write ADC config register");
		return rc;
	}

	return 0;
}

static int ina7xx_trigger_measurement(const struct device *dev)
{
	const struct ina7xx_config *cfg = dev->config;
	uint16_t tmp;
	int measurement_time;
	bool conversion_complete = false;
	int rc;

	static const int16_t samples_avg_count[8] = {1, 4, 16, 64, 128, 256, 512, 1024};
	static const int16_t tct_vsenct_time_us[8] = {50, 84, 150, 280, 540, 1052, 2074, 4120};

	/* Trigger measurement */
	rc = ina7xx_reg_field_update(dev, INA7xx_REG_ADC_CONFIG,
				     (INA7xx_MODE_MASK << INA7xx_MODE_SHIFT),
				     cfg->mode << INA7xx_MODE_SHIFT);
	if (rc < 0) {
		LOG_ERR("Failed to start measurement");
		return rc;
	}

	/* calculate measurement time */
	measurement_time = 2000 * cfg->convdly;

	if ((IS_BIT_SET(cfg->mode, INA7xx_MEAS_EN_DIE_TEMP_BIT)) &&
	    !(IS_BIT_SET(cfg->mode, INA7xx_MEAS_EN_CUR_BIT))) {
		measurement_time = measurement_time + tct_vsenct_time_us[cfg->tct];
	}

	if (IS_BIT_SET(cfg->mode, INA7xx_MEAS_EN_CUR_BIT)) {
		measurement_time = measurement_time + (2 * tct_vsenct_time_us[cfg->vsenct]);
	}

	if (IS_BIT_SET(cfg->mode, INA7xx_MEAS_EN_VOLTAGE_BIT)) {
		measurement_time = measurement_time + tct_vsenct_time_us[cfg->tct];
	}

	measurement_time =
		INA7xx_WAIT_STARTUP_USEC + (measurement_time * samples_avg_count[cfg->avg]);

	/* check for completion */
	for (size_t i = 0; i < MAX_RETRIES; ++i) {

		/* wait for completion */
		k_sleep(K_USEC(measurement_time));

		/* check for measurement completion */
		rc = ina7xx_read16(dev, INA7xx_REG_DIAG_ALRT, &tmp);
		if (rc < 0) {
			LOG_ERR("Error reading diagnostic flags");
			return rc;
		}

		if (IS_BIT_SET(tmp, INA7xx_FLAG_CNVRF)) {
			conversion_complete = true;
			break;
		}
	}

	if (!conversion_complete) {
		LOG_ERR("Measurement timed out");
		return -EIO;
	}

	return rc;
}

static int ina7xx_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct ina7xx_config *cfg = dev->config;
	struct ina7xx_data *data = dev->data;
	int rc = 0;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_VOLTAGE && chan != SENSOR_CHAN_POWER &&
	    chan != SENSOR_CHAN_DIE_TEMP && chan != SENSOR_CHAN_CURRENT) {
		return -ENOTSUP;
	}

	if (cfg->mode <= INA7xx_MODE_TRIGGER) {
		ina7xx_trigger_measurement(dev);
	}

	if (chan == SENSOR_CHAN_ALL) {
		data->valid_readings_mask = 0;
	}

	if (chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_VOLTAGE) {

		WRITE_BIT(data->valid_readings_mask, INA7xx_READ_VOLTAGE_BIT, false);
		rc = ina7xx_read16(dev, INA7xx_REG_BUS_VOLTAGE, &data->v_bus);
		if (rc < 0) {
			LOG_ERR("Error reading bus voltage");
			return rc;
		}
		WRITE_BIT(data->valid_readings_mask, INA7xx_READ_VOLTAGE_BIT, true);
	}

	if (chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_POWER) {

		WRITE_BIT(data->valid_readings_mask, INA7xx_READ_POWER_BIT, false);
		rc = ina7xx_read24(dev, INA7xx_REG_POWER, &data->power);
		if (rc < 0) {
			LOG_ERR("Error reading power register");
			return rc;
		}
		WRITE_BIT(data->valid_readings_mask, INA7xx_READ_POWER_BIT, true);
	}

	if (chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_CURRENT) {

		WRITE_BIT(data->valid_readings_mask, INA7xx_READ_CURRENT_BIT, false);
		rc = ina7xx_read16(dev, INA7xx_REG_CURRENT, &data->current);
		if (rc < 0) {
			LOG_ERR("Error reading current register");
			return rc;
		}
		WRITE_BIT(data->valid_readings_mask, INA7xx_READ_CURRENT_BIT, true);
	}

	if (chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_DIE_TEMP) {

		WRITE_BIT(data->valid_readings_mask, INA7xx_READ_DIE_TEMP_BIT, false);
		rc = ina7xx_read16(dev, INA7xx_REG_DIE_TEMP, &data->die_temp);
		if (rc < 0) {
			LOG_ERR("Error reading temperature register");
			return rc;
		}
		WRITE_BIT(data->valid_readings_mask, INA7xx_READ_DIE_TEMP_BIT, true);
	}

	return rc;
}

static int ina7xx_channel_get(const struct device *dev, enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct ina7xx_data *data = dev->data;
	int16_t signed_current;
	int16_t signed_die_temp;

	switch (chan) {
	case SENSOR_CHAN_VOLTAGE:
		if (IS_BIT_SET(data->valid_readings_mask, INA7xx_READ_VOLTAGE_BIT)) {
			sensor_value_from_micro(val,
						(int64_t)data->v_bus * INA7xx_BUS_VOLTAGE_MUL_UV);
		} else {
			LOG_WRN("Last fetch did not include a voltage reading");
			return -ENODATA;
		}
		break;
	case SENSOR_CHAN_DIE_TEMP:
		if (IS_BIT_SET(data->valid_readings_mask, INA7xx_READ_DIE_TEMP_BIT)) {
			signed_die_temp = (int16_t)data->die_temp;
			signed_die_temp /= 16; /* Remove RESERVED bits 3-0 */
			sensor_value_from_micro(val, signed_die_temp * INA7xx_TEMP_SCALE_MG);
		} else {
			LOG_WRN("Last fetch did not include a die temp reading");
			return -ENODATA;
		}
		break;
	case SENSOR_CHAN_POWER:
		if (IS_BIT_SET(data->valid_readings_mask, INA7xx_READ_POWER_BIT)) {
			sensor_value_from_micro(val, (int64_t)data->power * data->power_mul_uw);
		} else {
			LOG_WRN("Last fetch did not include a power reading");
			return -ENODATA;
		}
		break;
	case SENSOR_CHAN_CURRENT:
		if (IS_BIT_SET(data->valid_readings_mask, INA7xx_READ_CURRENT_BIT)) {
			signed_current = (int16_t)data->current;
			sensor_value_from_micro(val,
						signed_current * (int64_t)data->current_mul_ua);
		} else {
			LOG_WRN("Last fetch did not include a current reading");
			return -ENODATA;
		}
		break;
	default:
		LOG_DBG("Channel not supported by device");
		return -ENOTSUP;
	}

	return 0;
}

static int ina7xx_init(const struct device *dev)
{
	const struct ina7xx_config *cfg = dev->config;
	struct ina7xx_data *data = dev->data;
	int rc;
	uint16_t tmp;

	if (!i2c_is_ready_dt(&cfg->bus)) {
		LOG_ERR("Device not ready");
		return -ENODEV;
	}

	/* 300us power on time */
	k_sleep(K_TIMEOUT_ABS_US(INA7xx_POWERUP_USEC));

	rc = ina7xx_read16(dev, INA7xx_REG_ID, &tmp);
	if (rc < 0) {
		LOG_ERR("Failed to read chip id: %x", rc);
		return rc;
	}

	if (tmp != VENDOR_ID) {
		LOG_ERR("Invalid vendor id: 0x%x", tmp);
		return -EIO;
	}
	LOG_DBG("INA7xx chip id: 0x%x", tmp);

	rc = ina7xx_set_config(dev);
	if (rc < 0) {
		LOG_ERR("Could not set configuration data");
		return rc;
	}

	if (cfg->inatype == DEVICE_TYPE_INA700) {
		data->current_mul_ua = INA700_CURRENT_MUL_UA;
		data->power_mul_uw = INA700_POWER_MUL_UW;
	} else if (cfg->inatype == DEVICE_TYPE_INA745) {
		data->current_mul_ua = INA745_CURRENT_MUL_UA;
		data->power_mul_uw = INA745_POWER_MUL_UW;
	} else {
		data->current_mul_ua = INA780_CURRENT_MUL_UA;
		data->power_mul_uw = INA780_POWER_MUL_UW;
	}

	return 0;
}

#ifdef CONFIG_PM_DEVICE
static int ina7xx_pm_action(const struct device *dev, enum pm_device_action action)
{
	const struct ina7xx_config *cfg = dev->config;
	uint16_t reg_val;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		reg_val = cfg->mode;
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		reg_val = INA7xx_MODE_SHUTDOWN;
		break;
	default:
		return -ENOTSUP;
	}

	return ina7xx_reg_field_update(dev, INA7xx_REG_ADC_CONFIG, INA7xx_MODE, reg_val);
}
#endif /* CONFIG_PM_DEVICE */

static DEVICE_API(sensor, ina7xx_api) = {
	.sample_fetch = ina7xx_sample_fetch,
	.channel_get = ina7xx_channel_get,
};

#define INA7xx_INIT(n)                                                                             \
	static struct ina7xx_data ina7xx_data_##n;                                                 \
                                                                                                   \
	static const struct ina7xx_config ina7xx_config_##n = {                                    \
		.bus = I2C_DT_SPEC_INST_GET(n),                                                    \
		.inatype = DT_INST_PROP(n, inatype),                                               \
		.convdly = DT_INST_PROP(n, convdly),                                               \
		.mode = DT_INST_PROP(n, mode),                                                     \
		.vbusct = DT_INST_PROP(n, vbusct),                                                 \
		.vsenct = DT_INST_PROP(n, vsenct),                                                 \
		.tct = DT_INST_PROP(n, tct),                                                       \
		.avg = DT_INST_PROP(n, avg)};                                                      \
                                                                                                   \
	PM_DEVICE_DT_INST_DEFINE(n, ina7xx_pm_action);                                             \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(n, ina7xx_init, PM_DEVICE_DT_INST_GET(n), &ina7xx_data_##n,   \
				     &ina7xx_config_##n, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, \
				     &ina7xx_api);

DT_INST_FOREACH_STATUS_OKAY(INA7xx_INIT)
