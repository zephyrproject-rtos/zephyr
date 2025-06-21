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
	uint64_t v_bus;
	float die_temp;
	uint64_t current;
	uint64_t power;
	uint64_t energy;
	uint64_t charge;
	uint16_t col;
	uint8_t valid_readings_mask;
};

static int ina7xx_read(const struct device *dev, uint8_t reg_addr, uint64_t *reg_data,
		       uint8_t bytes)
{
	const struct ina7xx_config *cfg = dev->config;
	uint8_t rx_buf[bytes];
	int rc;

	rc = i2c_write_read_dt(&cfg->bus, &reg_addr, sizeof(reg_addr), rx_buf, sizeof(rx_buf));

	if (bytes == 2) {
		*reg_data = sys_get_be16(rx_buf);
	} else if (bytes == 3) {
		*reg_data = sys_get_be24(rx_buf);
	} else if (bytes == 4) {
		*reg_data = sys_get_be32(rx_buf);
	} else {
		*reg_data = sys_get_be40(rx_buf);
	}

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
	uint64_t reg_data;
	int rc;

	rc = ina7xx_read(dev, addr, &reg_data, 2);
	if (rc) {
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

	reg_data = (cfg->convdly & INA7xx_CONVDLY_MASK) << INA7xx_CONVDLY_SHIFT;

	rc = ina7xx_write(dev, INA7xx_REG_CONFIG, reg_data);
	if (rc) {
		LOG_ERR("Could not write config register.");
		return rc;
	}

	reg_data = (FIELD_PREP(INA7xx_MODE, cfg->mode)) | (FIELD_PREP(INA7xx_VBUSCT, cfg->vbusct)) |
		   (FIELD_PREP(INA7xx_VSENCT, cfg->vsenct)) | (FIELD_PREP(INA7xx_TCT, cfg->tct)) |
		   (FIELD_PREP(INA7xx_AVG, cfg->avg));
	rc = ina7xx_write(dev, INA7xx_REG_ADC_CONFIG, reg_data);

	if (rc) {
		LOG_ERR("Could not write ADC config register.");
		return rc;
	}

	return 0;
}

static int ina7xx_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct ina7xx_config *cfg = dev->config;
	struct ina7xx_data *data = dev->data;
	uint64_t tmp;
	int rc;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_VOLTAGE && chan != SENSOR_CHAN_POWER &&
	    chan != SENSOR_CHAN_DIE_TEMP && chan != SENSOR_CHAN_CURRENT) {
		return -ENOTSUP;
	}

	if (cfg->mode != INA7xx_MODE_CONTI) {
		/* Trigger measurement and wait for completion */
		rc = ina7xx_reg_field_update(dev, INA7xx_REG_ADC_CONFIG,
					     (INA7xx_MODE_MASK << INA7xx_MODE_SHIFT),
					     INA7xx_MODE_TRIGGER << INA7xx_MODE_SHIFT);
		if (rc) {
			LOG_ERR("Failed to start measurement.");
			return rc;
		}

		k_sleep(K_USEC(data->convdly));
	} else {
		/* Device is in continuous mode for temperature, current, and bus voltage */
	}

	if (chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_VOLTAGE) {

		rc = ina7xx_read(dev, INA7xx_REG_BUS_VOLTAGE, &data->v_bus, 2);
		if (rc) {
			LOG_ERR("Error reading bus voltage.");
			WRITE_BIT(data->valid_readings_mask, 0, 0);
			return rc;
		}
		WRITE_BIT(data->valid_readings_mask, 0, 1);
	}

	if (chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_POWER) {

		rc = ina7xx_read(dev, INA7xx_REG_POWER, &data->power, 3);
		if (rc) {
			LOG_ERR("Error reading power register.");
			WRITE_BIT(data->valid_readings_mask, 1, 0);
			return rc;
		}
		WRITE_BIT(data->valid_readings_mask, 1, 1);
	}

	if (chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_CURRENT) {

		rc = ina7xx_read(dev, INA7xx_REG_CURRENT, &data->current, 2);
		if (rc) {
			LOG_ERR("Error reading current register.");
			WRITE_BIT(data->valid_readings_mask, 2, 0);
			return rc;
		}
		WRITE_BIT(data->valid_readings_mask, 2, 1);
	}

	if (chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_DIE_TEMP) {

		rc = ina7xx_read(dev, INA7xx_REG_DIE_TEMP, &tmp, 2);

		if (rc) {
			LOG_ERR("Error reading temperatur register.");
			WRITE_BIT(data->valid_readings_mask, 3, 0);
			return rc;
		}
		WRITE_BIT(data->valid_readings_mask, 3, 1);
		data->die_temp = (tmp >> INA7xx_REG_DIE_TEMP_SHIFT);
	}

	return rc;
}

static int ina7xx_channel_get(const struct device *dev, enum sensor_channel chan,
			      struct sensor_value *val)
{
	const struct ina7xx_config *cfg = dev->config;
	struct ina7xx_data *data = dev->data;
	float tmp = 0;
	int8_t sign = 1;

	switch (chan) {
	case SENSOR_CHAN_VOLTAGE:
		if ((data->valid_readings_mask & INA7xx_HAS_VOLTAGE_MASK) == 0) {
			LOG_WRN("Last fetch did not include a voltage reading");
			return -ENODATA;
		}
		tmp = data->v_bus * INA7xx_BUS_VOLTAGE_MUL;
		break;
	case SENSOR_CHAN_DIE_TEMP:
		if ((data->valid_readings_mask & INA7xx_HAS_DIE_TEMP_MASK) == 0) {
			LOG_WRN("Last fetch did not include a die temp reading");
			return -ENODATA;
		}
		tmp = data->die_temp * (float)INA7xx_TEMP_SCALE;
		break;
	case SENSOR_CHAN_POWER:
		if ((data->valid_readings_mask & INA7xx_HAS_POWER_MASK) == 0) {
			LOG_WRN("Last fetch did not include a power reading");
			return -ENODATA;
		}

		if (cfg->inatype == DEVICE_TYPE_INA700) {
			tmp = data->power * (float)INA700_POWER_MUL;
		} else if (cfg->inatype == DEVICE_TYPE_INA745) {
			tmp = data->power * (float)INA745_POWER_MUL;
		} else {
			tmp = data->power * (float)INA780_POWER_MUL;
		}
		break;
	case SENSOR_CHAN_CURRENT:
		if ((data->valid_readings_mask & INA7xx_HAS_CURRENT_MASK) == 0) {
			LOG_WRN("Last fetch did not include a current reading");
			return -ENODATA;
		}
		if (INA7xx_SIGN_BIT(data->current)) {
			data->current = (uint16_t)(~data->current) + 1;
			sign = -1;
		}

		if (cfg->inatype == DEVICE_TYPE_INA700) {
			tmp = sign * data->current * INA700_CURRENT_MUL;
		} else if (cfg->inatype == DEVICE_TYPE_INA745) {
			tmp = sign * (data->current * INA745_CURRENT_MUL);
		} else {
			tmp = sign * data->current * INA780_CURRENT_MUL;
		}
		break;
	default:
		LOG_DBG("Channel not supported by device!");
		return -ENOTSUP;
	}

	return sensor_value_from_double(val, tmp);
}

#ifdef CONFIG_PM_DEVICE
static int ina7xx_pm_action(const struct device *dev, enum pm_device_action action)
{
	uint16_t reg_val;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		return ina7xx_init(dev);
	case PM_DEVICE_ACTION_SUSPEND:
		reg_val = INA7xx_MODE_SLEEP;
		break;
	case PM_DEVICE_ACTION_TURN_OFF:
		reg_val = INA7xx_MODE_OFF;
		break;
	default:
		return -ENOTSUP;
	}

	return ina7xx_reg_field_update(dev, INA7xx_REG_CONFIG, INA7xx_MODE_MASK, reg_val);
}
#endif /* CONFIG_PM_DEVICE */

static int ina7xx_init(const struct device *dev)
{
	const struct ina7xx_config *cfg = dev->config;
	int rc;
	uint64_t tmp;

	if (!device_is_ready(cfg->bus.bus)) {
		LOG_ERR("Device not ready.");
		return -ENODEV;
	}

	rc = ina7xx_write(dev, INA7xx_REG_CONFIG, INA7xx_RST_MASK << INA7xx_RST_SHIFT);
	if (rc) {
		LOG_ERR("Could not reset device.");
		return rc;
	}

	rc = ina7xx_read(dev, INA7xx_REG_ID, &tmp, 2);
	if (rc) {
		LOG_ERR("Failed to read chip id: %x", rc);
		return rc;
	}

	if (tmp != VENDOR_ID) {
		LOG_ERR("Invalid vendor id: %02x", (uint16_t)tmp);
		return -EIO;
	}
	LOG_DBG("INA7xx chip id: 0x%x", (uint16_t)tmp);

	rc = ina7xx_set_config(dev);
	if (rc) {
		LOG_ERR("Could not set configuration data.");
		return rc;
	}

	return 0;
}

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
