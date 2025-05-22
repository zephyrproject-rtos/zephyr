/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_ina745

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/byteorder.h>

#include "ina745.h"

LOG_MODULE_REGISTER(INA745, CONFIG_SENSOR_LOG_LEVEL);

static int ina745_read(const struct device *dev, uint8_t reg_addr, uint64_t *reg_data,
		       uint8_t bytes)
{
	const struct ina745_config *cfg = dev->config;
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

static int ina745_write(const struct device *dev, uint8_t addr, uint16_t reg_data)
{
	const struct ina745_config *cfg = dev->config;
	uint8_t tx_buf[3];

	tx_buf[0] = addr;
	sys_put_be16(reg_data, &tx_buf[1]);

	return i2c_write_dt(&cfg->bus, tx_buf, sizeof(tx_buf));
}

static int ina745_reg_field_update(const struct device *dev, uint8_t addr, uint16_t mask,
				   uint16_t field)
{
	uint64_t reg_data;
	int rc;

	rc = ina745_read(dev, addr, &reg_data, 2);
	if (rc) {
		return rc;
	}

	reg_data = (reg_data & ~mask) | field;

	return ina745_write(dev, addr, reg_data);
}

static int ina745_set_config(const struct device *dev)
{
	const struct ina745_config *cfg = dev->config;
	uint16_t reg_data;
	int rc;

	reg_data = (cfg->convdly & INA745_CONVDLY_MASK) << INA745_CONVDLY_SHIFT;

	rc = ina745_write(dev, INA745_REG_CONFIG, reg_data);
	if (rc) {
		LOG_ERR("Could not write config register.");
		return rc;
	}

	reg_data = (cfg->mode & INA745_MODE_MASK) << INA745_MODE_SHIFT |
		   (cfg->vbusct & INA745_VBUSCT_MASK) << INA745_VBUSCT_SHIFT |
		   (cfg->vsenct & INA745_VSENCT_MASK) << INA745_VSENCT_SHIFT |
		   (cfg->tct & INA745_TCT_MASK) << INA745_TCT_SHIFT |
		   (cfg->avg & INA745_AVG_MASK) << INA745_AVG_SHIFT;

	rc = ina745_write(dev, INA745_REG_ADC_CONFIG, reg_data);

	if (rc) {
		LOG_ERR("Could not write ADC config register.");
		return rc;
	}

	return 0;
}

static int ina745_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct ina745_config *cfg = dev->config;
	struct ina745_data *data = dev->data;
	uint64_t tmp;
	int rc;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_VOLTAGE && chan != SENSOR_CHAN_POWER &&
	    chan != SENSOR_CHAN_DIE_TEMP && chan != SENSOR_CHAN_CURRENT) {
		return -ENOTSUP;
	}

	if (cfg->mode != INA745_MODE_CONTI) {
		/* Trigger measurement and wait for completion */
		rc = ina745_reg_field_update(dev, INA745_REG_ADC_CONFIG,
					     (INA745_MODE_MASK << INA745_MODE_SHIFT),
					     INA745_MODE_TRIGGER << INA745_MODE_SHIFT);
		if (rc) {
			LOG_ERR("Failed to start measurement.");
			return rc;
		}

		k_sleep(K_USEC(data->convdly));
	} else {
		/* Device is in continuous mode for temperature, current, and bus voltage */
	}

	if (chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_VOLTAGE) {

		rc = ina745_read(dev, INA745_REG_BUS_VOLTAGE, &tmp, 2);
		if (rc) {
			LOG_ERR("Error reading bus voltage.");
			return rc;
		}
		data->v_bus = tmp;
	}

	if (chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_POWER) {

		rc = ina745_read(dev, INA745_REG_POWER, &tmp, 3);
		if (rc) {
			LOG_ERR("Error reading power register.");
			return rc;
		}
		data->power = tmp;
	}

	if (chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_CURRENT) {

		rc = ina745_read(dev, INA745_REG_CURRENT, &tmp, 2);
		if (rc) {
			LOG_ERR("Error reading current register.");
			return rc;
		}
		data->current = tmp;
	}

	if (chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_DIE_TEMP) {

		rc = ina745_read(dev, INA745_REG_DIE_TEMP, &tmp, 2);

		if (rc) {
			LOG_ERR("Error reading temperatur register.");
			return rc;
		}

		data->die_temp = (tmp >> INA745_REG_DIE_TEMP_SHIFT);
	}

	return rc;
}

static int ina745_channel_get(const struct device *dev, enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct ina745_data *data = dev->data;
	float tmp = 0;
	int8_t sign = 1;

	switch (chan) {
	case SENSOR_CHAN_VOLTAGE:
		tmp = data->v_bus * INA745_BUS_VOLTAGE_MUL;
		break;
	case SENSOR_CHAN_DIE_TEMP:
		tmp = data->die_temp * (float)INA745_TEMP_SCALE;
		break;
	case SENSOR_CHAN_POWER:
		tmp = data->power * (float)INA745_POWER_MUL;
		break;
	case SENSOR_CHAN_CURRENT:
		if (INA745_SIGN_BIT(data->current)) {
			data->current = ~data->current + 1;
			sign = -1;
		}
		tmp = sign * data->current * INA745_CURRENT_MUL;
		break;
	default:
		LOG_DBG("Channel not supported by device!");
		return -ENOTSUP;
	}

	return sensor_value_from_double(val, tmp);
}

#ifdef CONFIG_PM_DEVICE
static int ina745_pm_action(const struct device *dev, enum pm_device_action action)
{
	uint16_t reg_val;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		return ina745_init(dev);
	case PM_DEVICE_ACTION_SUSPEND:
		reg_val = INA745_MODE_SLEEP;
		break;
	case PM_DEVICE_ACTION_TURN_OFF:
		reg_val = INA745_MODE_OFF;
		break;
	default:
		return -ENOTSUP;
	}

	return ina745_reg_field_update(dev, INA745_REG_CONFIG, INA745_MODE_MASK, reg_val);
}
#endif /* CONFIG_PM_DEVICE */

static int ina745_init(const struct device *dev)
{
	const struct ina745_config *cfg = dev->config;
	int rc;
	uint64_t tmp;

	if (!device_is_ready(cfg->bus.bus)) {
		LOG_ERR("Device not ready.");
		return -ENODEV;
	}

	rc = ina745_write(dev, INA745_REG_CONFIG, INA745_RST_MASK << INA745_RST_SHIFT);
	if (rc) {
		LOG_ERR("Could not reset device.");
		return rc;
	}

	rc = ina745_read(dev, INA745_REG_ID, &tmp, 2);
	if (rc) {
		LOG_ERR("Failed to read chip id: %x", rc);
		return rc;
	}

	if (!(tmp == INA745_ID)) {
		LOG_ERR("Invalid chip id: %02x", (uint16_t)tmp);
		return -EIO;
	}
	LOG_INF("INA7xx chip id: 0x%x", (uint16_t)tmp);

	rc = ina745_set_config(dev);
	if (rc) {
		LOG_ERR("Could not set configuration data.");
		return rc;
	}

	k_sleep(K_USEC(INA745_WAIT_STARTUP));

	return 0;
}

static DEVICE_API(sensor, ina745_api) = {
	.sample_fetch = ina745_sample_fetch,
	.channel_get = ina745_channel_get,
};

#define INA745_INIT(n)                                                                             \
	static struct ina745_data ina745_data_##n;                                                 \
                                                                                                   \
	static const struct ina745_config ina745_config_##n = {.bus = I2C_DT_SPEC_INST_GET(n),     \
							       .convdly =                          \
								       DT_INST_PROP(n, convdly),   \
							       .mode = DT_INST_PROP(n, mode),      \
							       .vbusct = DT_INST_PROP(n, vbusct),  \
							       .vsenct = DT_INST_PROP(n, vsenct),  \
							       .tct = DT_INST_PROP(n, tct),        \
							       .avg = DT_INST_PROP(n, avg)};       \
                                                                                                   \
	PM_DEVICE_DT_INST_DEFINE(n, ina745_pm_action);                                             \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(n, ina745_init, PM_DEVICE_DT_INST_GET(n), &ina745_data_##n,   \
				     &ina745_config_##n, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, \
				     &ina745_api);

DT_INST_FOREACH_STATUS_OKAY(INA745_INIT)
