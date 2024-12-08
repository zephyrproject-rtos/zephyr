/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* TODO: Add functionality for Trigger. */

#define DT_DRV_COMPAT ti_ina226

#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/dt-bindings/sensor/ina226.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/sys/util_macro.h>
#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>

/* Device register addresses. */
#define INA226_REG_CONFIG		0x00
#define INA226_REG_SHUNT_VOLT		0x01
#define INA226_REG_BUS_VOLT		0x02
#define INA226_REG_POWER		0x03
#define INA226_REG_CURRENT		0x04
#define INA226_REG_CALIB		0x05
#define INA226_REG_MASK			0x06
#define INA226_REG_ALERT		0x07
#define INA226_REG_MANUFACTURER_ID	0xFE
#define INA226_REG_DEVICE_ID		0xFF

/* Device register values. */
#define INA226_MANUFACTURER_ID		0x5449
#define INA226_DEVICE_ID		0x2260

struct ina226_data {
	const struct device *dev;
	int16_t current;
	uint16_t bus_voltage;
	uint16_t power;
#ifdef CONFIG_INA226_VSHUNT
	int16_t shunt_voltage;
#endif
	enum sensor_channel chan;
};

struct ina226_config {
	const struct i2c_dt_spec bus;
	uint16_t config;
	uint32_t current_lsb;
	uint16_t cal;
};

LOG_MODULE_REGISTER(INA226, CONFIG_SENSOR_LOG_LEVEL);

/** @brief Calibration scaling value (scaled by 10^-5) */
#define INA226_CAL_SCALING		512ULL

/** @brief The LSB value for the bus voltage register, in microvolts/LSB. */
#define INA226_BUS_VOLTAGE_TO_uV(x)	((x) * 1250U)

/** @brief The LSB value for the shunt voltage register, in microvolts/LSB. */
#define INA226_SHUNT_VOLTAGE_TO_uV(x)	((x) * 2500U / 1000U)

/** @brief Power scaling (need factor of 0.2) */
#define INA226_POWER_TO_uW(x)		((x) * 25ULL)


int ina226_reg_read_16(const struct i2c_dt_spec *bus, uint8_t reg, uint16_t *val)
{
	uint8_t data[2];
	int ret;

	ret = i2c_burst_read_dt(bus, reg, data, sizeof(data));
	if (ret < 0) {
		return ret;
	}

	*val = sys_get_be16(data);

	return ret;
}

int ina226_reg_write(const struct i2c_dt_spec *bus, uint8_t reg, uint16_t val)
{
	uint8_t tx_buf[3];

	tx_buf[0] = reg;
	sys_put_be16(val, &tx_buf[1]);

	return i2c_write_dt(bus, tx_buf, sizeof(tx_buf));
}

static void micro_s32_to_sensor_value(struct sensor_value *val, int32_t value_microX)
{
	val->val1 = value_microX / 1000000L;
	val->val2 = value_microX % 1000000L;
}

static int ina226_channel_get(const struct device *dev, enum sensor_channel chan,
			      struct sensor_value *val)
{
	const struct ina226_data *data = dev->data;
	const struct ina226_config *config = dev->config;

	switch (chan) {
	case SENSOR_CHAN_VOLTAGE:
		micro_s32_to_sensor_value(val, INA226_BUS_VOLTAGE_TO_uV(data->bus_voltage));
		break;
	case SENSOR_CHAN_CURRENT:
		/* see datasheet "Current and Power calculations" section */
		micro_s32_to_sensor_value(val, data->current * config->current_lsb);
		break;
	case SENSOR_CHAN_POWER:
		/* power in uW is power_reg * current_lsb * 0.2 */
		micro_s32_to_sensor_value(val,
			INA226_POWER_TO_uW((uint32_t)data->power * config->current_lsb));
		break;
#ifdef CONFIG_INA226_VSHUNT
	case SENSOR_CHAN_VSHUNT:
		micro_s32_to_sensor_value(val, INA226_SHUNT_VOLTAGE_TO_uV(data->shunt_voltage));
		break;
#endif /* CONFIG_INA226_VSHUNT */
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int ina226_read_data(const struct device *dev)
{
	struct ina226_data *data = dev->data;
	const struct ina226_config *config = dev->config;
	int ret;

	if ((data->chan == SENSOR_CHAN_ALL) || (data->chan == SENSOR_CHAN_VOLTAGE)) {
		ret = ina226_reg_read_16(&config->bus, INA226_REG_BUS_VOLT, &data->bus_voltage);
		if (ret < 0) {
			LOG_ERR("Failed to read bus voltage");
			return ret;
		}
	}

	if ((data->chan == SENSOR_CHAN_ALL) || (data->chan == SENSOR_CHAN_CURRENT)) {
		ret = ina226_reg_read_16(&config->bus, INA226_REG_CURRENT, &data->current);
		if (ret < 0) {
			LOG_ERR("Failed to read current");
			return ret;
		}
	}

	if ((data->chan == SENSOR_CHAN_ALL) || (data->chan == SENSOR_CHAN_POWER)) {
		ret = ina226_reg_read_16(&config->bus, INA226_REG_POWER, &data->power);
		if (ret < 0) {
			LOG_ERR("Failed to read power");
			return ret;
		}
	}

#ifdef CONFIG_INA226_VSHUNT
	if ((data->chan == SENSOR_CHAN_ALL) || (data->chan == SENSOR_CHAN_VSHUNT)) {
		ret = ina226_reg_read_16(&config->bus, INA226_REG_SHUNT_VOLT, &data->shunt_voltage);
		if (ret < 0) {
			LOG_ERR("Failed to read shunt voltage");
			return ret;
		}
	}
#endif /* CONFIG_INA226_VSHUNT */

	return 0;
}

static int ina226_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct ina226_data *data = dev->data;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_VOLTAGE
	    && chan != SENSOR_CHAN_CURRENT && chan != SENSOR_CHAN_POWER
#ifdef CONFIG_INA226_VSHUNT
	    && chan != SENSOR_CHAN_VSHUNT
#endif /* CONFIG_INA226_VSHUNT */
	    ) {
		return -ENOTSUP;
	}

	data->chan = chan;

	return ina226_read_data(dev);
}

static int ina226_attr_set(const struct device *dev, enum sensor_channel chan,
			   enum sensor_attribute attr, const struct sensor_value *val)
{
	const struct ina226_config *config = dev->config;
	uint16_t data = val->val1;

	switch (attr) {
	case SENSOR_ATTR_CONFIGURATION:
		return ina226_reg_write(&config->bus, INA226_REG_CONFIG, data);
	case SENSOR_ATTR_CALIBRATION:
		return ina226_reg_write(&config->bus, INA226_REG_CALIB, data);
	default:
		LOG_ERR("INA226 attribute not supported.");
		return -ENOTSUP;
	}
}

static int ina226_attr_get(const struct device *dev, enum sensor_channel chan,
			   enum sensor_attribute attr, struct sensor_value *val)
{
	const struct ina226_config *config = dev->config;
	uint16_t data;
	int ret;

	switch (attr) {
	case SENSOR_ATTR_CONFIGURATION:
		ret = ina226_reg_read_16(&config->bus, INA226_REG_CONFIG, &data);
		if (ret < 0) {
			return ret;
		}
		break;
	case SENSOR_ATTR_CALIBRATION:
		ret = ina226_reg_read_16(&config->bus, INA226_REG_CALIB, &data);
		if (ret < 0) {
			return ret;
		}
		break;
	default:
		LOG_ERR("INA226 attribute not supported.");
		return -ENOTSUP;
	}

	val->val1 = data;
	val->val2 = 0;

	return 0;
}

static int ina226_calibrate(const struct device *dev)
{
	const struct ina226_config *config = dev->config;
	int ret;

	ret = ina226_reg_write(&config->bus, INA226_REG_CALIB, config->cal);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int ina226_init(const struct device *dev)
{
	struct ina226_data *data = dev->data;
	const struct ina226_config *config = dev->config;
	uint16_t id;
	int ret;

	if (!i2c_is_ready_dt(&config->bus)) {
		LOG_ERR("I2C bus %s is not ready", config->bus.bus->name);
		return -ENODEV;
	}

	data->dev = dev;

	ret = ina226_reg_read_16(&config->bus, INA226_REG_MANUFACTURER_ID, &id);
	if (ret < 0) {
		LOG_ERR("Failed to read manufacturer register.");
		return ret;
	}

	if (id != INA226_MANUFACTURER_ID) {
		LOG_ERR("Manufacturer ID doesn't match.");
		return -ENODEV;
	}

	ret = ina226_reg_read_16(&config->bus, INA226_REG_DEVICE_ID, &id);
	if (ret < 0) {
		LOG_ERR("Failed to read device register.");
		return ret;
	}

	if (id != INA226_DEVICE_ID) {
		LOG_ERR("Device ID doesn't match.");
		return -ENODEV;
	}

	ret = ina226_reg_write(&config->bus, INA226_REG_CONFIG, config->config);
	if (ret < 0) {
		LOG_ERR("Failed to write configuration register.");
		return ret;
	}

	ret = ina226_calibrate(dev);
	if (ret < 0) {
		LOG_ERR("Failed to write calibration register.");
		return ret;
	}

	return 0;
}

static DEVICE_API(sensor, ina226_driver_api) = {
	.attr_set = ina226_attr_set,
	.attr_get = ina226_attr_get,
	.sample_fetch = ina226_sample_fetch,
	.channel_get = ina226_channel_get,
};

#define INA226_DRIVER_INIT(inst)							\
	static struct ina226_data ina226_data_##inst;					\
	static const struct ina226_config ina226_config_##inst = {			\
		.bus = I2C_DT_SPEC_INST_GET(inst),					\
		.current_lsb = DT_INST_PROP(inst, current_lsb_microamps),		\
		.cal = INA226_CAL_SCALING * 10000000ULL /				\
			(DT_INST_PROP(inst, current_lsb_microamps) *			\
			DT_INST_PROP(inst, rshunt_micro_ohms)),				\
		.config = (DT_INST_ENUM_IDX(inst, avg_count) << 9) |			\
			(DT_INST_ENUM_IDX(inst, vbus_conversion_time_us) << 6) |	\
			(DT_INST_ENUM_IDX(inst, vshunt_conversion_time_us) << 3) |	\
			DT_INST_ENUM_IDX(inst, operating_mode),				\
	};										\
	SENSOR_DEVICE_DT_INST_DEFINE(inst,						\
				     &ina226_init,					\
				     NULL,						\
				     &ina226_data_##inst,				\
				     &ina226_config_##inst,				\
				     POST_KERNEL,					\
				     CONFIG_SENSOR_INIT_PRIORITY,			\
				     &ina226_driver_api);

DT_INST_FOREACH_STATUS_OKAY(INA226_DRIVER_INIT)
