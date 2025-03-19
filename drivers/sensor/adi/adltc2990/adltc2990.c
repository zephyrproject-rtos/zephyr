/*
 * SPDX-FileCopyrightText: Copyright (c) 2023 Carl Zeiss Meditec AG
 * SPDX-FileCopyrightText: Copyright (c) 2024 Jilay Sandeep Pandya
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_adltc2990

#include <stdbool.h>

#include <zephyr/sys/util.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor/adltc2990.h>

#include "adltc2990_reg.h"
#include "adltc2990_internal.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(adltc2990, CONFIG_SENSOR_LOG_LEVEL);

static enum adltc2990_monitoring_type adltc2990_get_v1_v2_measurement_modes(uint8_t mode_4_3,
									    uint8_t mode_2_0)
{
	if (mode_2_0 > ADLTC2990_MODE_2_0_MAX_VALUE || mode_4_3 > ADLTC2990_MODE_4_3_MAX_VALUE) {
		LOG_ERR("Invalid Measurement Mode");
		return -EINVAL;
	}
	if (mode_4_3 == ADLTC2990_MEASURE_INTERNAL_TEMPERATURE_ONLY ||
	    mode_4_3 == ADLTC2990_MEASURE_PINS_V3_V4_ONLY) {
		return NOTHING;
	}

	enum adltc2990_monitoring_type type = NOTHING;

	switch (mode_2_0) {
	case ADLTC2990_MODE_V1_V2_TR2:
	case ADLTC2990_MODE_V1_V2_V3_V4:
		type = VOLTAGE_SINGLEENDED;
		break;

	case ADLTC2990_MODE_V1_MINUS_V2_TR2:
	case ADLTC2990_MODE_V1_MINUS_V2_V3_V4:
	case ADLTC2990_MODE_V1_MINUS_V2_V3_MINUS_V4:
		type = VOLTAGE_DIFFERENTIAL;
		break;

	case ADLTC2990_MODE_TR1_V3_V4:
	case ADLTC2990_MODE_TR1_V3_MINUS_V4:
	case ADLTC2990_MODE_TR1_TR2:
		type = TEMPERATURE;
		break;

	default:
		break;
	}
	return type;
}

static enum adltc2990_monitoring_type adltc2990_get_v3_v4_measurement_modes(uint8_t mode_4_3,
									    uint8_t mode_2_0)
{
	if (mode_2_0 > ADLTC2990_MODE_2_0_MAX_VALUE || mode_4_3 > ADLTC2990_MODE_4_3_MAX_VALUE) {
		LOG_ERR("Invalid Measurement Mode");
		return -EINVAL;
	}
	if (mode_4_3 == ADLTC2990_MEASURE_INTERNAL_TEMPERATURE_ONLY ||
	    mode_4_3 == ADLTC2990_MEASURE_PINS_V1_V2_ONLY) {
		return NOTHING;
	}

	enum adltc2990_monitoring_type type = NOTHING;

	switch (mode_2_0) {
	case ADLTC2990_MODE_V1_V2_TR2:
	case ADLTC2990_MODE_V1_MINUS_V2_TR2:
	case ADLTC2990_MODE_TR1_TR2:
		type = TEMPERATURE;
		break;

	case ADLTC2990_MODE_V1_MINUS_V2_V3_V4:
	case ADLTC2990_MODE_TR1_V3_V4:
	case ADLTC2990_MODE_V1_V2_V3_V4:
		type = VOLTAGE_SINGLEENDED;
		break;

	case ADLTC2990_MODE_TR1_V3_MINUS_V4:
	case ADLTC2990_MODE_V1_MINUS_V2_V3_MINUS_V4:
		type = VOLTAGE_DIFFERENTIAL;
		break;

	default:
		break;
	}
	return type;
}

int adltc2990_is_busy(const struct device *dev, bool *is_busy)
{
	const struct adltc2990_config *cfg = dev->config;
	uint8_t status_reg = 0;

	if (i2c_reg_read_byte_dt(&cfg->bus, ADLTC2990_REG_STATUS, &status_reg)) {
		return -EIO;
	}

	*is_busy = status_reg & BIT(0);

	return 0;
}

static void adltc2990_get_v1_v2_val(const struct device *dev, struct sensor_value *val,
				    uint8_t num_values, uint8_t *const offset_index)
{
	struct adltc2990_data *data = dev->data;

	for (uint8_t index = 0; index < num_values; index++) {
		val[index].val1 = data->pins_v1_v2_values[index] / 1000000;
		val[index].val2 = data->pins_v1_v2_values[index] % 1000000;
		*offset_index = index + 1;
	}
}

static void adltc2990_get_v3_v4_val(const struct device *dev, struct sensor_value *val,
				    uint8_t num_values, uint8_t const *const offset)
{
	struct adltc2990_data *data = dev->data;

	uint8_t offset_index = *offset;

	for (uint8_t index = 0; index < num_values; index++) {
		val[index + offset_index].val1 = data->pins_v3_v4_values[index] / 1000000;
		val[index + offset_index].val2 = data->pins_v3_v4_values[index] % 1000000;
	}
}

int adltc2990_trigger_measurement(const struct device *dev,
				  enum adltc2990_acquisition_format format)
{
	const struct adltc2990_config *cfg = dev->config;
	struct adltc2990_data *data = dev->data;

	if (data->acq_format == format) {
		goto trigger_conversion;
	}

	data->acq_format = format;
	uint8_t ctrl_reg_setting;

	if (i2c_reg_read_byte_dt(&cfg->bus, ADLTC2990_REG_CONTROL, &ctrl_reg_setting)) {
		LOG_ERR("reading control register failed.");
		return -EIO;
	}

	ctrl_reg_setting |= format << 6;
	if (i2c_reg_write_byte_dt(&cfg->bus, ADLTC2990_REG_CONTROL, ctrl_reg_setting)) {
		LOG_ERR("configuring for single bus failed.");
		return -EIO;
	}

trigger_conversion:
	return i2c_reg_write_byte_dt(&cfg->bus, ADLTC2990_REG_TRIGGER, 0x1);
}

static int adltc2990_fetch_property_value(const struct device *dev,
					  enum adltc2990_monitoring_type type,
					  enum adltc2990_monitor_pins pin, int32_t *output)
{
	const struct adltc2990_config *cfg = dev->config;

	uint8_t msb_value = 0, lsb_value = 0;
	uint8_t msb_address, lsb_address;

	switch (pin) {
	case V1:
		msb_address = ADLTC2990_REG_V1_MSB;
		lsb_address = ADLTC2990_REG_V1_LSB;
		break;

	case V2:
		msb_address = ADLTC2990_REG_V2_MSB;
		lsb_address = ADLTC2990_REG_V2_LSB;
		break;

	case V3:
		msb_address = ADLTC2990_REG_V3_MSB;
		lsb_address = ADLTC2990_REG_V3_LSB;
		break;

	case V4:
		msb_address = ADLTC2990_REG_V4_MSB;
		lsb_address = ADLTC2990_REG_V4_LSB;
		break;

	case INTERNAL_TEMPERATURE:
		msb_address = ADLTC2990_REG_INTERNAL_TEMP_MSB;
		lsb_address = ADLTC2990_REG_INTERNAL_TEMP_LSB;
		break;

	case SUPPLY_VOLTAGE:
		msb_address = ADLTC2990_REG_VCC_MSB;
		lsb_address = ADLTC2990_REG_VCC_LSB;
		break;

	default:
		LOG_ERR("Trying to access illegal register");
		return -EINVAL;
	}

	if (i2c_reg_read_byte_dt(&cfg->bus, msb_address, &msb_value)) {
		return -EIO;
	}

	if (i2c_reg_read_byte_dt(&cfg->bus, lsb_address, &lsb_value)) {
		return -EIO;
	}

	uint16_t conversion_factor;
	uint8_t negative_bit_index = 14U, sensor_val_divisor = 100U;

	if (type == VOLTAGE_SINGLEENDED) {
		conversion_factor = ADLTC2990_VOLTAGE_SINGLEENDED_CONVERSION_FACTOR;
	} else if (type == VOLTAGE_DIFFERENTIAL) {
		conversion_factor = ADLTC2990_VOLTAGE_DIFFERENTIAL_CONVERSION_FACTOR;
	} else if (type == TEMPERATURE) {
		conversion_factor = ADLTC2990_TEMPERATURE_CONVERSION_FACTOR;
		if (cfg->temp_format == ADLTC2990_TEMPERATURE_FORMAT_CELSIUS) {
			negative_bit_index = 12U;
		}
		sensor_val_divisor = 1U;
	} else {
		LOG_ERR("unknown type");
		return -EINVAL;
	}

	int16_t value = (msb_value << 8) + lsb_value;

	int32_t voltage_value = (value << (31 - negative_bit_index)) >> (31 - negative_bit_index);

	*output = (voltage_value * conversion_factor) / sensor_val_divisor;

	return 0;
}

static int adltc2990_init(const struct device *dev)
{
	const struct adltc2990_config *cfg = dev->config;
	struct adltc2990_data *data = dev->data;
	int err;

	if (!i2c_is_ready_dt(&cfg->bus)) {
		LOG_ERR("I2C bus %s not ready", cfg->bus.bus->name);
		return -ENODEV;
	}

	const uint8_t ctrl_reg_setting = cfg->temp_format << 7 | data->acq_format << 6 | 0 << 5 |
					 cfg->measurement_mode[1] << 3 | cfg->measurement_mode[0];

	LOG_DBG("Setting Control Register to: 0x%x", ctrl_reg_setting);
	if (i2c_reg_write_byte_dt(&cfg->bus, ADLTC2990_REG_CONTROL, ctrl_reg_setting)) {
		LOG_ERR("configuring for single bus failed.");
		return -EIO;
	}

	err = adltc2990_trigger_measurement(dev, data->acq_format);
	if (err < 0) {
		LOG_ERR("triggering measurement failed: %d", err);
	}

	LOG_INF("Initializing ADLTC2990 with name %s", dev->name);
	return 0;
}

static int fetch_pin_differential_voltage_value(const struct device *dev,
						const enum adltc2990_monitoring_type mode,
						const enum adltc2990_monitor_pins pin)
{
	if (mode != VOLTAGE_DIFFERENTIAL) {
		LOG_DBG("Pin is not configured to measure voltage differential");
		return 0;
	}

	struct adltc2990_data *data = dev->data;
	int32_t value;
	int ret;

	ret = adltc2990_fetch_property_value(dev, VOLTAGE_DIFFERENTIAL, pin, &value);
	if (ret) {
		return ret;
	}

	switch (pin) {
	case V1:
	case V2:
		data->pins_v1_v2_values[0] = value;
		break;

	case V3:
	case V4:
		data->pins_v3_v4_values[0] = value;
	default:
		break;
	}

	return 0;
}

static int fetch_pin_single_ended_voltage_value(const struct device *dev,
						const enum adltc2990_monitoring_type mode,
						const enum adltc2990_monitor_pins pin_1,
						const enum adltc2990_monitor_pins pin_2)
{
	if (mode != VOLTAGE_SINGLEENDED) {
		LOG_DBG("Pin is not configured to measure voltage single ended");
		return 0;
	}

	const struct adltc2990_config *cfg = dev->config;
	struct adltc2990_data *data = dev->data;
	int value_1, value_2, ret;

	ret = adltc2990_fetch_property_value(dev, VOLTAGE_SINGLEENDED, pin_1, &value_1);
	if (ret) {
		return ret;
	}

	ret = adltc2990_fetch_property_value(dev, VOLTAGE_SINGLEENDED, pin_2, &value_2);
	if (ret) {
		return ret;
	}

	if (pin_1 == V1 && pin_2 == V2) {

		uint32_t v1_r1 = cfg->pins_v1_v2.voltage_divider_resistors.v1_r1_r2[0];
		uint32_t v1_r2 = cfg->pins_v1_v2.voltage_divider_resistors.v1_r1_r2[1];

		float voltage_divider_ratio = (v1_r1 + v1_r2) / (float)v1_r2;

		data->pins_v1_v2_values[0] = value_1 * voltage_divider_ratio;

		uint32_t v2_r1 = cfg->pins_v1_v2.voltage_divider_resistors.v2_r1_r2[0];
		uint32_t v2_r2 = cfg->pins_v1_v2.voltage_divider_resistors.v2_r1_r2[1];

		voltage_divider_ratio = (v2_r1 + v2_r2) / (float)v2_r2;

		data->pins_v1_v2_values[1] = value_2 * voltage_divider_ratio;

	} else if (pin_1 == V3 && pin_2 == V4) {

		uint32_t v3_r1 = cfg->pins_v3_v4.voltage_divider_resistors.v3_r1_r2[0];
		uint32_t v3_r2 = cfg->pins_v3_v4.voltage_divider_resistors.v3_r1_r2[1];

		float voltage_divider_ratio = (v3_r1 + v3_r2) / (float)v3_r2;

		data->pins_v3_v4_values[0] = value_1 * voltage_divider_ratio;

		uint32_t v4_r1 = cfg->pins_v3_v4.voltage_divider_resistors.v4_r1_r2[0];
		uint32_t v4_r2 = cfg->pins_v3_v4.voltage_divider_resistors.v4_r1_r2[1];

		voltage_divider_ratio = (v4_r1 + v4_r2) / (float)v4_r2;

		data->pins_v3_v4_values[1] = value_2 * voltage_divider_ratio;

	} else {
		LOG_ERR("Invalid pin configuration");
		return -EINVAL;
	}
	return 0;
}

static int fetch_pin_temperature_value(const struct device *dev,
				       const enum adltc2990_monitoring_type mode,
				       const enum adltc2990_monitor_pins pin)
{
	if (mode != TEMPERATURE) {
		LOG_DBG("Pin is not configured to measure temperature");
		return 0;
	}

	struct adltc2990_data *data = dev->data;
	int32_t value;
	int ret;

	ret = adltc2990_fetch_property_value(dev, TEMPERATURE, pin, &value);
	if (ret) {
		return ret;
	}

	switch (pin) {
	case V1:
	case V2:
		data->pins_v1_v2_values[0] = value;
		break;

	case V3:
	case V4:
		data->pins_v3_v4_values[0] = value;
	default:
		break;
	}

	return 0;
}

static int fetch_pin_current_value(const struct device *dev,
				   const enum adltc2990_monitoring_type mode,
				   const enum adltc2990_monitor_pins pin)
{
	int ret;

	ret = fetch_pin_differential_voltage_value(dev, mode, pin);
	if (ret) {
		return ret;
	}

	const struct adltc2990_config *cfg = dev->config;
	struct adltc2990_data *data = dev->data;

	switch (pin) {
	case V1:
	case V2:
		data->pins_v1_v2_values[0] *= (ADLTC2990_MICROOHM_CONVERSION_FACTOR /
					       (float)cfg->pins_v1_v2.pins_current_resistor);
		break;

	case V3:
	case V4:
		data->pins_v3_v4_values[0] *= (ADLTC2990_MICROOHM_CONVERSION_FACTOR /
					       (float)cfg->pins_v3_v4.pins_current_resistor);
		break;

	default:
		break;
	}

	return 0;
}

static int adltc2990_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct adltc2990_data *data = dev->data;
	const struct adltc2990_config *cfg = dev->config;
	enum adltc2990_monitoring_type mode_v1_v2 = adltc2990_get_v1_v2_measurement_modes(
		cfg->measurement_mode[1], cfg->measurement_mode[0]);
	enum adltc2990_monitoring_type mode_v3_v4 = adltc2990_get_v3_v4_measurement_modes(
		cfg->measurement_mode[1], cfg->measurement_mode[0]);
	int ret;
	int32_t value;

	switch (chan) {
	case SENSOR_CHAN_DIE_TEMP:
		ret = adltc2990_fetch_property_value(dev, TEMPERATURE, INTERNAL_TEMPERATURE,
						     &value);
		if (ret) {
			return ret;
		}
		data->internal_temperature = value;
		break;

	case SENSOR_CHAN_CURRENT:
		if (!(mode_v1_v2 == VOLTAGE_DIFFERENTIAL || mode_v3_v4 == VOLTAGE_DIFFERENTIAL)) {
			LOG_ERR("Sensor is not configured to measure Current");
			return -EINVAL;
		}

		ret = fetch_pin_current_value(dev, mode_v1_v2, V1);
		if (ret) {
			return ret;
		}

		ret = fetch_pin_current_value(dev, mode_v3_v4, V3);
		if (ret) {
			return ret;
		}

		break;

	case SENSOR_CHAN_VOLTAGE:
		ret = adltc2990_fetch_property_value(dev, VOLTAGE_SINGLEENDED, SUPPLY_VOLTAGE,
						     &value);
		if (ret) {
			return ret;
		}
		data->supply_voltage = value + 2500000;

		ret = fetch_pin_differential_voltage_value(dev, mode_v1_v2, V1);
		if (ret) {
			return ret;
		}

		ret = fetch_pin_differential_voltage_value(dev, mode_v3_v4, V3);
		if (ret) {
			return ret;
		}

		ret = fetch_pin_single_ended_voltage_value(dev, mode_v1_v2, V1, V2);
		if (ret) {
			return ret;
		}

		ret = fetch_pin_single_ended_voltage_value(dev, mode_v3_v4, V3, V4);
		if (ret) {
			return ret;
		}

		break;

	case SENSOR_CHAN_AMBIENT_TEMP:
		if (!(mode_v1_v2 == TEMPERATURE || mode_v3_v4 == TEMPERATURE)) {
			LOG_ERR("Sensor is not configured to measure Ambient Temperature");
			return -EINVAL;
		}

		ret = fetch_pin_temperature_value(dev, mode_v1_v2, V1);
		if (ret) {
			return ret;
		}

		ret = fetch_pin_temperature_value(dev, mode_v3_v4, V3);
		if (ret) {
			return ret;
		}
		break;

	default:
		LOG_ERR("does not measure channel: %d", chan);
		return -ENOTSUP;
	}

	return 0;
}

static int adltc2990_channel_get(const struct device *dev, enum sensor_channel chan,
				 struct sensor_value *val)
{
	if (val == NULL) {
		LOG_ERR("Argument of type sensor_value* cannot be null ");
		return -EINVAL;
	}
	struct adltc2990_data *data = dev->data;
	const struct adltc2990_config *cfg = dev->config;
	enum adltc2990_monitoring_type mode_v1_v2 = adltc2990_get_v1_v2_measurement_modes(
		cfg->measurement_mode[1], cfg->measurement_mode[0]);
	enum adltc2990_monitoring_type mode_v3_v4 = adltc2990_get_v3_v4_measurement_modes(
		cfg->measurement_mode[1], cfg->measurement_mode[0]);

	uint8_t offset_index = 0, num_values_v1_v2 = 0, num_values_v3_v4 = 0;

	switch (chan) {
	case SENSOR_CHAN_DIE_TEMP:
		val->val1 = (data->internal_temperature) / 1000000;
		val->val2 = (data->internal_temperature) % 1000000;
		LOG_DBG("Internal Temperature Value is:%d.%d", val->val1, val->val2);
		break;

	case SENSOR_CHAN_VOLTAGE:
		if (mode_v1_v2 == VOLTAGE_SINGLEENDED) {
			LOG_DBG("Getting V1,V2");
			num_values_v1_v2 = ADLTC2990_VOLTAGE_SINGLE_ENDED_VALUES;
		} else if (mode_v1_v2 == VOLTAGE_DIFFERENTIAL) {
			LOG_DBG("Getting V3-V4");
			num_values_v1_v2 = ADLTC2990_VOLTAGE_DIFF_VALUES;
		}
		if (mode_v3_v4 == VOLTAGE_SINGLEENDED) {
			LOG_DBG("Getting V3,V4");
			num_values_v3_v4 = ADLTC2990_VOLTAGE_SINGLE_ENDED_VALUES;
		} else if (mode_v3_v4 == VOLTAGE_DIFFERENTIAL) {
			LOG_DBG("Getting V3-V4");
			num_values_v3_v4 = ADLTC2990_VOLTAGE_DIFF_VALUES;
		}
		/* Add VCC to the last index */
		val[num_values_v1_v2 + num_values_v3_v4].val1 = data->supply_voltage / 1000000;
		val[num_values_v1_v2 + num_values_v3_v4].val2 = data->supply_voltage % 1000000;
		break;

	case SENSOR_CHAN_CURRENT:
		if (!(mode_v1_v2 == VOLTAGE_DIFFERENTIAL || mode_v3_v4 == VOLTAGE_DIFFERENTIAL)) {
			LOG_ERR("Sensor is not configured to measure Current");
			return -EINVAL;
		}
		if (mode_v1_v2 == VOLTAGE_DIFFERENTIAL && mode_v3_v4 == VOLTAGE_DIFFERENTIAL) {
			LOG_DBG("Getting I12 and I34");
			num_values_v1_v2 = ADLTC2990_CURRENT_VALUES;
			num_values_v3_v4 = ADLTC2990_CURRENT_VALUES;
		} else if (mode_v1_v2 == VOLTAGE_DIFFERENTIAL) {
			LOG_DBG("Getting I12");
			num_values_v1_v2 = ADLTC2990_CURRENT_VALUES;
		} else if (mode_v3_v4 == VOLTAGE_DIFFERENTIAL) {
			LOG_DBG("Getting I34");
			num_values_v3_v4 = ADLTC2990_CURRENT_VALUES;
		}
		break;

	case SENSOR_CHAN_AMBIENT_TEMP:
		if (!(mode_v1_v2 == TEMPERATURE || mode_v3_v4 == TEMPERATURE)) {
			LOG_ERR("Sensor is not configured to measure Ambient Temperature");
			return -EINVAL;
		}
		if (mode_v1_v2 == TEMPERATURE && mode_v3_v4 == TEMPERATURE) {
			LOG_DBG("Getting T12 and T34");
			num_values_v1_v2 = ADLTC2990_TEMP_VALUES;
			num_values_v3_v4 = ADLTC2990_TEMP_VALUES;
		} else if (mode_v1_v2 == TEMPERATURE) {
			LOG_DBG("Getting T12");
			num_values_v1_v2 = ADLTC2990_TEMP_VALUES;
		} else if (mode_v3_v4 == TEMPERATURE) {
			LOG_DBG("Getting T34");
			num_values_v3_v4 = ADLTC2990_TEMP_VALUES;
		}
		break;

	default:
		return -ENOTSUP;
	}

	adltc2990_get_v1_v2_val(dev, val, num_values_v1_v2, &offset_index);
	adltc2990_get_v3_v4_val(dev, val, num_values_v3_v4, &offset_index);
	return 0;
}

static DEVICE_API(sensor, adltc2990_driver_api) = {
	.sample_fetch = adltc2990_sample_fetch,
	.channel_get = adltc2990_channel_get,
};

#define ADLTC2990_DEFINE(inst)                                                                     \
	static struct adltc2990_data adltc2990_data_##inst = {                                     \
		.acq_format = DT_INST_PROP(inst, acquistion_format),                               \
	};                                                                                         \
	static const struct adltc2990_config adltc2990_config_##inst = {                           \
		.bus = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.temp_format = DT_INST_PROP(inst, temperature_format),                             \
		.measurement_mode = DT_INST_PROP(inst, measurement_mode),                          \
		.pins_v1_v2.pins_current_resistor =                                                \
			DT_INST_PROP_OR(inst, pins_v1_v2_current_resistor, 1),                     \
		.pins_v1_v2.voltage_divider_resistors.v1_r1_r2 =                                   \
			DT_INST_PROP_OR(inst, pin_v1_voltage_divider_resistors, NULL),             \
		.pins_v1_v2.voltage_divider_resistors.v2_r1_r2 =                                   \
			DT_INST_PROP_OR(inst, pin_v2_voltage_divider_resistors, NULL),             \
		.pins_v3_v4.pins_current_resistor =                                                \
			DT_INST_PROP_OR(inst, pins_v3_v4_current_resistor, 1),                     \
		.pins_v3_v4.voltage_divider_resistors.v3_r1_r2 =                                   \
			DT_INST_PROP_OR(inst, pin_v3_voltage_divider_resistors, NULL),             \
		.pins_v3_v4.voltage_divider_resistors.v4_r1_r2 =                                   \
			DT_INST_PROP_OR(inst, pin_v4_voltage_divider_resistors, NULL)};            \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, adltc2990_init, NULL, &adltc2990_data_##inst,           \
				     &adltc2990_config_##inst, POST_KERNEL,                        \
				     CONFIG_SENSOR_INIT_PRIORITY, &adltc2990_driver_api);

DT_INST_FOREACH_STATUS_OKAY(ADLTC2990_DEFINE)
