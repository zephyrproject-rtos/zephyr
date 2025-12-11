/*
 * Copyright (c) 2025, Thomas Schmid <tom@lfence.de>
 * Copyright (c) 2022, Maxmillion McLaughlin
 * Copyright (c) 2020, SER Consulting LLC / Steven Riedl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_mcp9600

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/mcp9600.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(MCP9600, CONFIG_SENSOR_LOG_LEVEL);

#define MCP9600_REG_TEMP_HOT 0x00

#define MCP9600_REG_TEMP_DELTA 0x01
#define MCP9600_REG_TEMP_COLD  0x02
#define MCP9600_REG_RAW_ADC    0x03

#define MCP9600_REG_STATUS 0x04

#define MCP9600_REG_TC_CONFIG  0x05
#define MCP9600_REG_DEV_CONFIG 0x06

#define MCP9600_REG_A1_CONFIG 0x08
#define MCP9600_REG_A2_CONFIG 0x09
#define MCP9600_REG_A3_CONFIG 0x0A
#define MCP9600_REG_A4_CONFIG 0x0B

#define MCP9600_A1_HYST	 0x0C
#define MCP9600_A2_HYST	 0x0D
#define MCP9600_A3_HYST	 0x0E
#define MCP9600_A4_HYST	 0x0F

#define MCP9600_A1_LIMIT 0x10
#define MCP9600_A2_LIMIT 0x11
#define MCP9600_A3_LIMIT 0x12
#define MCP9600_A4_LIMIT 0x13

#define MCP9600_REG_ID_REVISION 0x20

#define MCP9600_REG_TC_CONFIG_OFFSET_TC_TYPE            0x05
#define MCP9600_REG_TC_CONFIG_OFFSET_FILTER_COEF        0x00
#define MCP9600_REG_DEV_CONFIG_OFFSET_ADC_RES           0x05
#define MCP9600_REG_DEV_CONFIG_COLD_JUNCTION_RES_OFFSET 0x07

struct mcp9600_data {
	int32_t temp_hot_junction;
	int32_t temp_cold_junction;
	int32_t temp_delta;
	int32_t adc_raw;
	bool temp_hot_valid;
	bool temp_cold_valid;
	bool temp_delta_valid;
	bool adc_raw_valid;
};

struct mcp9600_config {
	const struct i2c_dt_spec bus;
	const uint8_t thermocouple_type;
	const uint8_t filter_coefficient;
	const uint8_t adc_resolution;
	const uint8_t cold_junction_temp_resolution;
};

static int mcp9600_reg_read(const struct device *dev, uint8_t start, uint8_t *buf, int size)
{
	const struct mcp9600_config *cfg = dev->config;

	return i2c_burst_read_dt(&cfg->bus, start, buf, size);
}

static int mcp9600_reg_write(const struct device *dev, uint8_t start, uint8_t *buf, int size)
{
	const struct mcp9600_config *cfg = dev->config;

	return i2c_burst_write_dt(&cfg->bus, start, buf, size);
}

static int mcp9600_attr_set(const struct device *dev, enum sensor_channel chan,
			    enum sensor_attribute attr, const struct sensor_value *value)
{
	__ASSERT(chan == SENSOR_CHAN_ALL, "Attribute set is only supported for SENSOR_CHAN_ALL");
	uint8_t register_value = 0;
	int rc = 0;
	uint8_t register_address;
	uint32_t attribute_id = attr;

	if (attribute_id == SENSOR_ATTR_MCP9600_ADC_RES) {
		register_address = MCP9600_REG_DEV_CONFIG;
		rc = mcp9600_reg_read(dev, register_address, &register_value, 1);
		if (rc != 0) {
			return rc;
		}

		register_value &= ~(BIT_MASK(2) << MCP9600_REG_DEV_CONFIG_OFFSET_ADC_RES);
		register_value |= (value->val1 & BIT_MASK(2))
				  << MCP9600_REG_DEV_CONFIG_OFFSET_ADC_RES;
	} else if (attribute_id == SENSOR_ATTR_MCP9600_COLD_JUNCTION_RESOLUTION) {
		register_address = MCP9600_REG_DEV_CONFIG;
		rc = mcp9600_reg_read(dev, register_address, &register_value, 1);
		if (rc != 0) {
			return rc;
		}

		register_value &= ~(1 << MCP9600_REG_DEV_CONFIG_COLD_JUNCTION_RES_OFFSET);
		register_value |= (value->val1 & 1)
				  << MCP9600_REG_DEV_CONFIG_COLD_JUNCTION_RES_OFFSET;
	} else if (attribute_id == SENSOR_ATTR_MCP9600_FILTER_COEFFICIENT) {
		register_address = MCP9600_REG_TC_CONFIG;
		rc = mcp9600_reg_read(dev, register_address, &register_value, 1);
		if (rc != 0) {
			return rc;
		}

		register_value &= ~(BIT_MASK(3) << MCP9600_REG_TC_CONFIG_OFFSET_FILTER_COEF);
		register_value |= (value->val1 & BIT_MASK(3))
				  << MCP9600_REG_TC_CONFIG_OFFSET_FILTER_COEF;
	} else if (attribute_id == SENSOR_ATTR_MCP9600_THERMOCOUPLE_TYPE) {
		register_address = MCP9600_REG_TC_CONFIG;
		rc = mcp9600_reg_read(dev, register_address, &register_value, 1);
		if (rc != 0) {
			return rc;
		}

		register_value &= ~(BIT_MASK(3) << MCP9600_REG_TC_CONFIG_OFFSET_TC_TYPE);
		register_value |= (value->val1 & BIT_MASK(3))
				  << MCP9600_REG_TC_CONFIG_OFFSET_TC_TYPE;
	} else {
		return -ENOTSUP;
	}

	rc = mcp9600_reg_write(dev, register_address, &register_value, 1);
	return rc;
}

static int mcp9600_attr_get(const struct device *dev, enum sensor_channel chan,
			    enum sensor_attribute attr, struct sensor_value *value)
{
	uint8_t register_value[2];
	int rc = -1;
	uint32_t attribute_id = attr;

	if (attribute_id == SENSOR_ATTR_MCP9600_ADC_RES) {
		rc = mcp9600_reg_read(dev, MCP9600_REG_DEV_CONFIG, register_value, 1);
		if (rc != 0) {
			return rc;
		}

		value->val1 =
			(register_value[0] >> MCP9600_REG_DEV_CONFIG_OFFSET_ADC_RES) & BIT_MASK(2);
	} else if (attribute_id == SENSOR_ATTR_MCP9600_COLD_JUNCTION_RESOLUTION) {
		rc = mcp9600_reg_read(dev, MCP9600_REG_DEV_CONFIG, register_value, 1);
		if (rc != 0) {
			return rc;
		}

		value->val1 =
			(register_value[0] >> MCP9600_REG_DEV_CONFIG_COLD_JUNCTION_RES_OFFSET) & 1;
	} else if (attribute_id == SENSOR_ATTR_MCP9600_THERMOCOUPLE_TYPE) {
		rc = mcp9600_reg_read(dev, MCP9600_REG_TC_CONFIG, register_value, 1);
		if (rc != 0) {
			return rc;
		}

		value->val1 =
			(register_value[0] >> MCP9600_REG_TC_CONFIG_OFFSET_TC_TYPE) & BIT_MASK(3);
	} else if (attribute_id == SENSOR_ATTR_MCP9600_FILTER_COEFFICIENT) {
		rc = mcp9600_reg_read(dev, MCP9600_REG_TC_CONFIG, register_value, 1);
		if (rc != 0) {
			return rc;
		}

		value->val1 = (register_value[0] >> MCP9600_REG_TC_CONFIG_OFFSET_FILTER_COEF) &
			      BIT_MASK(3);
	} else if (attribute_id == SENSOR_ATTR_MCP9600_DEV_ID) {
		rc = mcp9600_reg_read(dev, MCP9600_REG_ID_REVISION, register_value,
				      sizeof(register_value));
		if (rc != 0) {
			return rc;
		}
		value->val1 = (register_value[0] << 8) | register_value[1];
	} else {
		return -ENOTSUP;
	}

	return rc;
}

static int mcp9600_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct mcp9600_data *data = dev->data;
	uint8_t buf[9];
	uint8_t start_register_address;
	uint8_t register_bytes_count = 2;
	uint8_t current_byte_index = 0;
	int ret;

	/* Configure I2C read and initialize runtime data. All registers containing measurement data
	 * are located contiguously without gaps, starting with MCP9600_REG_TEMP_HOT. Configure
	 * start register and number of bytes to read.
	 */
	switch ((uint32_t)chan) {
	case SENSOR_CHAN_ALL:
		start_register_address = MCP9600_REG_TEMP_HOT;
		data->adc_raw_valid = false;
		data->temp_hot_valid = false;
		data->temp_cold_valid = false;
		data->temp_delta_valid = false;
		register_bytes_count = 9;
		break;
	case SENSOR_CHAN_AMBIENT_TEMP:
		/* fall through */
	case SENSOR_CHAN_MCP9600_HOT_JUNCTION_TEMP:
		start_register_address = MCP9600_REG_TEMP_HOT;
		data->temp_hot_valid = false;
		break;
	case SENSOR_CHAN_MCP9600_COLD_JUNCTION_TEMP:
		start_register_address = MCP9600_REG_TEMP_COLD;
		data->temp_cold_valid = false;
		break;
	case SENSOR_CHAN_MCP9600_DELTA_TEMP:
		start_register_address = MCP9600_REG_TEMP_DELTA;
		data->temp_delta_valid = false;
		break;
	case SENSOR_CHAN_MCP9600_RAW_ADC:
		start_register_address = MCP9600_REG_RAW_ADC;
		data->adc_raw_valid = false;
		register_bytes_count = 3;
		break;
	default:
		LOG_ERR("Unsupported sensor channel");
		return -ENOTSUP;
	}

	/* read selected register values*/
	ret = mcp9600_reg_read(dev, start_register_address, buf, register_bytes_count);
	if (ret < 0) {
		return ret;
	}

	/* interpret register values based on register address and bytes in the buffer */
	while (current_byte_index < register_bytes_count) {
		if (start_register_address == MCP9600_REG_TEMP_HOT) {
			/* device's hot junction register is a 16 bit signed int */
			data->temp_hot_junction = (int32_t)(int16_t)(buf[current_byte_index] << 8) |
						  buf[current_byte_index + 1];
			data->temp_hot_junction *= 62500;
			data->temp_hot_valid = true;
			current_byte_index += 2;
		} else if (start_register_address == MCP9600_REG_TEMP_DELTA) {
			/* device's delta temperature register is a 16 bit signed int */
			data->temp_delta = (int32_t)(int16_t)(buf[current_byte_index] << 8) |
					   buf[current_byte_index + 1];
			data->temp_delta *= 62500;
			data->temp_delta_valid = true;
			current_byte_index += 2;
		} else if (start_register_address == MCP9600_REG_TEMP_COLD) {
			/* device's cold junction register is a 16 bit signed int */
			data->temp_cold_junction =
				(int32_t)(int16_t)(buf[current_byte_index] << 8) |
				buf[current_byte_index + 1];
			data->temp_cold_junction *= 62500;
			data->temp_cold_valid = true;
			current_byte_index += 2;
		} else if (start_register_address == MCP9600_REG_RAW_ADC) {
			/* device's raw adc value register is a 24 bit signed int */
			data->adc_raw = ((int32_t)(buf[current_byte_index] << 24) |
					 (buf[current_byte_index + 1] << 16) |
					 (buf[current_byte_index + 2] << 8)) >>
					8;
			data->adc_raw_valid = true;
			current_byte_index += 3;
		} else {
			return -EINVAL;
		}

		start_register_address += 1;
	}

	return 0;
}

static int mcp9600_channel_get(const struct device *dev, enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct mcp9600_data *data = dev->data;

	switch ((uint32_t)chan) {
	case SENSOR_CHAN_AMBIENT_TEMP:
		/* fall through */
	case SENSOR_CHAN_MCP9600_HOT_JUNCTION_TEMP:
		if (!data->temp_hot_valid) {
			return -EINVAL;
		}
		val->val1 = data->temp_hot_junction / 1000000;
		val->val2 = data->temp_hot_junction % 1000000;
		break;
	case SENSOR_CHAN_MCP9600_COLD_JUNCTION_TEMP:
		if (!data->temp_cold_valid) {
			return -EINVAL;
		}
		val->val1 = data->temp_cold_junction / 1000000;
		val->val2 = data->temp_cold_junction % 1000000;
		break;
	case SENSOR_CHAN_MCP9600_DELTA_TEMP:
		if (!data->temp_delta_valid) {
			return -EINVAL;
		}
		val->val1 = data->temp_delta / 1000000;
		val->val2 = data->temp_delta % 1000000;
		break;
	case SENSOR_CHAN_MCP9600_RAW_ADC:
		if (!data->adc_raw_valid) {
			return -EINVAL;
		}
		val->val1 = data->adc_raw;
		val->val2 = 0;
		break;
	default:
		LOG_ERR("Unsupported sensor channel");
		return -ENOTSUP;
	}

	return 0;
}

static DEVICE_API(sensor, mcp9600_api) = {
	.sample_fetch = mcp9600_sample_fetch,
	.channel_get = mcp9600_channel_get,
	.attr_set = mcp9600_attr_set,
	.attr_get = mcp9600_attr_get,
};

static int mcp9600_init(const struct device *dev)
{
	const struct mcp9600_config *cfg = dev->config;
	uint8_t buf[2];
	uint8_t thermocouple_config_reg_value;
	uint8_t device_config_reg_value;
	int ret;

	if (!i2c_is_ready_dt(&cfg->bus)) {
		LOG_ERR("mcp9600 i2c bus %s not ready", cfg->bus.bus->name);
		return -ENODEV;
	}

	ret = mcp9600_reg_read(dev, MCP9600_REG_ID_REVISION, buf, sizeof(buf));
	LOG_DBG("id: 0x%02x version: 0x%02x", buf[0], buf[1]);
	if (ret != 0) {
		return ret;
	}

	/* Start from reset state, which is 0 according to the datasheet */
	thermocouple_config_reg_value = 0;
	device_config_reg_value = 0;

	/* Configure device for thermocouple type and filter coefficient */
	thermocouple_config_reg_value |=
		((cfg->thermocouple_type & BIT_MASK(3)) << MCP9600_REG_TC_CONFIG_OFFSET_TC_TYPE) |
		((cfg->filter_coefficient & BIT_MASK(3))
		 << MCP9600_REG_TC_CONFIG_OFFSET_FILTER_COEF);
	ret = mcp9600_reg_write(dev, MCP9600_REG_TC_CONFIG, &thermocouple_config_reg_value, 1);

	if (ret != 0) {
		LOG_ERR("Unable to write tc config register. Error %d", ret);
		return ret;
	}
	LOG_DBG("set tc config register: 0x%02x", thermocouple_config_reg_value);

	/* Configure adc resolution and cold junction temperature resolution*/
	device_config_reg_value |=
		((cfg->adc_resolution & BIT_MASK(2)) << MCP9600_REG_DEV_CONFIG_OFFSET_ADC_RES) |
		((cfg->cold_junction_temp_resolution & 1)
		 << MCP9600_REG_DEV_CONFIG_COLD_JUNCTION_RES_OFFSET);
	ret = mcp9600_reg_write(dev, MCP9600_REG_DEV_CONFIG, &device_config_reg_value, 1);

	if (ret != 0) {
		LOG_ERR("Unable to write dev config register. Error %d", ret);
		return ret;
	}

	LOG_DBG("set dev config register: 0x%02x", device_config_reg_value);

	return ret;
}

#define MCP9600_DEFINE(id)                                                                         \
	static struct mcp9600_data mcp9600_data_##id;                                              \
                                                                                                   \
	static const struct mcp9600_config mcp9600_config_##id = {                                 \
		.bus = I2C_DT_SPEC_INST_GET(id),                                                   \
		.thermocouple_type = DT_INST_PROP(id, thermocouple_type),                          \
		.filter_coefficient = DT_INST_PROP(id, filter_coefficient),                        \
		.adc_resolution = DT_INST_PROP(id, adc_resolution),                                \
		.cold_junction_temp_resolution = DT_INST_PROP(id, cold_junction_temp_resolution)}; \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(id, mcp9600_init, NULL, &mcp9600_data_##id,                   \
				     &mcp9600_config_##id, POST_KERNEL,                            \
				     CONFIG_SENSOR_INIT_PRIORITY, &mcp9600_api);

DT_INST_FOREACH_STATUS_OKAY(MCP9600_DEFINE)
