/*
 * Copyright (c) 2025 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_max30210

#include <zephyr/device.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include "max30210.h"
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(MAX30210, CONFIG_SENSOR_LOG_LEVEL);

/**
 * @brief Generic register access function for MAX30210
 *
 * @param dev Device Pointer
 * @param addr_reg Register Address
 * @param data Data Pointer
 * @param read Read/Write Flag
 * @param length Number of Bytes to Read/Write
 * @return int 0 on success, negative error code on failure
 */
static int max30210_reg_access(const struct device *dev, uint8_t addr_reg, uint8_t *data, bool read,
			       uint8_t length)
{

	const struct max30210_config *config = dev->config;
	int ret;

	if (read) {
		ret = i2c_burst_read_dt(&config->i2c, addr_reg, data, length);
	} else {
		ret = i2c_burst_write_dt(&config->i2c, addr_reg, data, length);
	}

	return ret;
}

/**
 * @brief Read from MAX30210 register
 *
 * @param dev Device Pointer
 * @param reg_addr Register Address
 * @param val Value Pointer
 * @param length Number of Bytes to Read
 * @return int 0 on success, negative error code on failure
 */
int max30210_reg_read(const struct device *dev, uint8_t reg_addr, uint8_t *val, uint8_t length)
{
	return max30210_reg_access(dev, reg_addr, val, true, length);
}

/**
 * @brief Single byte write to MAX30210 register
 *
 * @param dev Device Pointer
 * @param reg_addr Register Address
 * @param val Value to Write
 * @param length Ignored
 * @return int
 */
int max30210_reg_write(const struct device *dev, uint8_t reg_addr, uint8_t val)
{
	return max30210_reg_access(dev, reg_addr, &val, false, 1);
}

/**
 * @brief Multiple byte write to MAX30210 register
 *
 * @param dev Device Pointer
 * @param reg_addr Register Address
 * @param val Value Pointer
 * @param length Number of Bytes to Write
 * @return int 0 on success, negative error code on failure
 */
int max30210_reg_write_multiple(const struct device *dev, uint8_t reg_addr, uint8_t *val,
				uint8_t length)
{
	if (length < 2) {
		return -EINVAL;
	}
	return max30210_reg_access(dev, reg_addr, val, false, length);
}

/**
 * @brief Update specific bits in a MAX30210 register
 *
 * @param dev Device Pointer
 * @param reg_addr Register Address
 * @param mask Bit Mask
 * @param val Value to Set
 * @return int 0 on success, negative error code on failure
 */
int max30210_reg_update(const struct device *dev, uint8_t reg_addr, uint8_t mask, uint8_t val)
{
	uint8_t reg_val = 0;
	int ret;

	ret = max30210_reg_read(dev, reg_addr, &reg_val, 1);
	if (ret < 0) {
		return ret;
	}

	reg_val &= ~mask;
	reg_val |= FIELD_PREP(mask, val);

	return max30210_reg_write(dev, reg_addr, reg_val);
}

/**
 * @brief Convert temperature in millidegrees Celsius to MAX30210 register bytes
 *
 * @param temp_mc Temperature in millidegrees Celsius
 * @param msb Pointer to store Most Significant Byte
 * @param lsb Pointer to store Least Significant Byte
 */
static inline void max30210_temp_to_bytes(int32_t temp_mc, uint8_t *msb, uint8_t *lsb)
{
	/* Convert millidegrees Celsius to register value */
	int16_t reg_val = (int16_t)(temp_mc / 5);

	/* Split into two bytes (big-endian: MSB first, LSB second) */
	*msb = (uint8_t)((reg_val >> 8) & 0xFF);
	*lsb = (uint8_t)(reg_val & 0xFF);
}

/**
 * @brief Set sensor attributes for MAX30210
 *
 * @param dev Device Pointer
 * @param chan Sensor Channel
 * @param attr Sensor Attribute
 * @param val Value Pointer
 * @return int 0 on success, negative error code on failure
 */
static int max30210_attr_set(const struct device *dev, enum sensor_channel chan,
			     enum sensor_attribute attr, const struct sensor_value *val)
{
	int ret;
	struct max30210_data *temp_data = dev->data;
	uint16_t whole_numbers_data;
	uint16_t fractional_data;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_AMBIENT_TEMP) {
		return -ENOTSUP;
	}

	switch ((int)attr) {
	case SENSOR_ATTR_MAX30210_CONTINUOUS_CONVERSION_MODE:
		ret = max30210_reg_write(dev, TEMP_CONVERT, 0X03);
		if (ret < 0) {
			LOG_ERR("Failed to set CONTINUOUS CONVERSION MODE: %d\n", ret);
			return ret;
		}
		break;

	case SENSOR_ATTR_LOWER_THRESH:
		if (val->val1 < 0 || val->val1 > 164) {
			LOG_ERR("Invalid whole part for LOWER THRESH\n");
			return -EINVAL;
		}
		if (val->val2 < 0 || val->val2 > 1000000 || val->val2 % 5000 != 0) {
			LOG_ERR("Invalid fractional part for LOWER THRESH\n");
			return -EINVAL;
		}
		whole_numbers_data = (val->val1) * 200;
		fractional_data = (val->val2 * 200) / 1000000;
		uint16_t lower_thresh = whole_numbers_data + fractional_data;

		temp_data->temp_alarm_low_setup = lower_thresh;

		uint8_t lower_thresh_data[2] = {(lower_thresh >> 8) & 0xFF, lower_thresh & 0xFF};

		ret = max30210_reg_write_multiple(dev, TEMP_ALARM_LOW_MSB, lower_thresh_data, 2);
		if (ret < 0) {
			LOG_ERR("Failed to set LOWER THRESH: %d\n", ret);
			return ret;
		}
		break;

	case SENSOR_ATTR_UPPER_THRESH:
		if (val->val1 < 0 || val->val1 > 164) {
			LOG_ERR("Invalid whole part for UPPER THRESH\n");
			return -EINVAL;
		}
		if (val->val2 < 0 || val->val2 > 1000000 || val->val2 % 5000 != 0) {
			LOG_ERR("Invalid fractional part for UPPER THRESH\n");
			return -EINVAL;
		}

		whole_numbers_data = (val->val1) * 200;
		fractional_data = (val->val2 * 200) / 1000000;
		uint16_t upper_thresh = whole_numbers_data + fractional_data;

		temp_data->temp_alarm_high_setup = upper_thresh;

		uint8_t upper_thresh_data[2] = {(upper_thresh >> 8) & 0xFF, upper_thresh & 0xFF};

		ret = max30210_reg_write_multiple(dev, TEMP_ALARM_HIGH_MSB, upper_thresh_data, 2);
		if (ret < 0) {
			LOG_ERR("Failed to set UPPER THRESH: %d\n", ret);
			return ret;
		}
		break;

	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		if (val->val1 < 0 || val->val1 > 8) {
			LOG_ERR("Invalid whole part for SAMPLING FREQUENCY\n");
			return -EINVAL;
		}
		uint8_t sampling_rate = 0;

		if (val->val1 == 0) {
			switch ((val->val2)) {
			case (15625):
				sampling_rate = SENSOR_SAMPLING_RATE_MAX30210_0_015625;
				break;

			case (31250):
				sampling_rate = SENSOR_SAMPLING_RATE_MAX30210_0_03125;
				break;

			case (62500):
				sampling_rate = SENSOR_SAMPLING_RATE_MAX30210_0_0625;
				break;

			case (125000):
				sampling_rate = SENSOR_SAMPLING_RATE_MAX30210_0_125;
				break;

			case (250000):
				sampling_rate = SENSOR_SAMPLING_RATE_MAX30210_0_25;
				break;

			case (500000):
				sampling_rate = SENSOR_SAMPLING_RATE_MAX30210_0_5;
				break;

			default:
				return -EINVAL;
			}

		} else {
			switch ((val->val1)) {
			case (1):
				sampling_rate = SENSOR_SAMPLING_RATE_MAX30210_1;
				break;

			case (2):
				sampling_rate = SENSOR_SAMPLING_RATE_MAX30210_2;
				break;

			case (4):
				sampling_rate = SENSOR_SAMPLING_RATE_MAX30210_4;
				break;

			case (8):
				sampling_rate = SENSOR_SAMPLING_RATE_MAX30210_8;
				break;

			default:
				return -EINVAL;
			}
		}

		ret = max30210_reg_update(dev, TEMP_CONFIG_2, TEMP_PERIOD_MASK, sampling_rate);
		if (ret < 0) {
			LOG_ERR("Failed to set SAMPLING FREQUENCY: %d\n", ret);
			return ret;
		}
		break;

	case SENSOR_ATTR_MAX30210_TEMP_INC_FAST_THRESH:
		/* Highest possible whole number is 1 degrees celsius */
		if (val->val1 < 0 || val->val1 > 1) {
			LOG_ERR("Invalid whole part for INC FAST THRESH\n");
			return -EINVAL;
		}

		/* Validate value is non-negative, <= 1 degree, and a multiple of 0.005 degrees */
		if (val->val2 < 0 || val->val2 > 1000000 || val->val2 % 5000 != 0) {
			LOG_ERR("Invalid fractional part for INC FAST THRESH\n");
			return -EINVAL;
		}

		whole_numbers_data = (val->val1) * 200;
		fractional_data = ((val->val2) / 1000000) * 200;
		uint8_t inc_fast_thresh = whole_numbers_data + fractional_data;

		temp_data->temp_inc_fast_thresh = inc_fast_thresh;

		if (inc_fast_thresh > 255) {
			LOG_ERR("INC FAST THRESH exceeds maximum value\n");
			return -EINVAL;
		}

		ret = max30210_reg_write(dev, TEMP_INC_FAST_THRESH, inc_fast_thresh);
		if (ret < 0) {
			LOG_ERR("Failed to set INC FAST THRESH: %d\n", ret);
			return ret;
		}
		break;

	case SENSOR_ATTR_MAX30210_TEMP_DEC_FAST_THRESH:
		if (val->val1 < 0 || val->val1 > 1) {
			LOG_ERR("Invalid whole part for DEC FAST THRESH\n");
			return -EINVAL;
		}
		/* Validate value is non-negative, <= 1 degree, and a multiple of 0.005 degrees */
		if (val->val2 < 0 || val->val2 > 1000000 || val->val2 % 5000 != 0) {
			LOG_ERR("Invalid fractional part for DEC FAST THRESH\n");
			return -EINVAL;
		}

		whole_numbers_data = (val->val1) * 200;
		fractional_data = ((val->val2) / 1000000) * 200;
		uint8_t dec_fast_thresh = whole_numbers_data + fractional_data;

		temp_data->temp_dec_fast_thresh = dec_fast_thresh;

		if (dec_fast_thresh > 255) {
			LOG_ERR("DEC FAST THRESH exceeds maximum value\n");
			return -EINVAL;
		}

		ret = max30210_reg_write(dev, TEMP_DEC_FAST_THRESH, dec_fast_thresh);
		if (ret < 0) {
			LOG_ERR("Failed to set DEC FAST THRESH: %d\n", ret);
			return ret;
		}
		break;

	case SENSOR_ATTR_MAX30210_SOFTWARE_RESET:
		ret = max30210_reg_write(dev, SYS_CONFIG, RESET_MASK);
		if (ret < 0) {
			LOG_ERR("Failed to perform software reset: %d\n", ret);
			return ret;
		}
		k_sleep(K_MSEC(10));
		break;

	case SENSOR_ATTR_MAX30210_RATE_CHG_FILTER:
		if (val->val1 < 0 || val->val1 > 7) {
			LOG_ERR("Invalid value for rate change filter\n");
			return -EINVAL;
		}
		ret = max30210_reg_update(dev, TEMP_CONFIG_1, RATE_CHRG_FILTER_MASK, val->val1);
		if (ret < 0) {
			LOG_ERR("Failed to set rate change filter: %d\n", ret);
			return ret;
		}
		break;

	case SENSOR_ATTR_MAX30210_HI_NON_CONSECUTIVE_MODE:
		if (val->val1 < 0 || val->val1 > 1) {
			LOG_ERR("Invalid value for high consecutive mode\n");
			return -EINVAL;
		}

		ret = max30210_reg_update(dev, TEMP_ALARM_HIGH_SETUP, TEMP_HI_ALARM_TRIP_MASK,
					  val->val1);
		if (ret < 0) {
			LOG_ERR("Failed to set high consecutive mode: %d\n", ret);
			return ret;
		}
		break;

	case SENSOR_ATTR_MAX30210_LO_NON_CONSECUTIVE_MODE:
		if (val->val1 < 0 || val->val1 > 1) {
			return -EINVAL;
		}

		ret = max30210_reg_update(dev, TEMP_ALARM_LOW_SETUP, TEMP_LO_ALARM_TRIP_MASK,
					  val->val1);
		if (ret < 0) {
			LOG_ERR("Failed to set low consecutive mode: %d\n", ret);
			return ret;
		}
		break;

	case SENSOR_ATTR_MAX30210_HI_TRIP_COUNT:
		if (val->val1 < 1 || val->val1 > 4) {
			LOG_ERR("Invalid value for high trip count\n");
			return -EINVAL;
		}
		ret = max30210_reg_update(dev, TEMP_ALARM_HIGH_SETUP, TEMP_HI_TRIP_COUNTER_MASK,
					  (val->val1) - 1);
		if (ret < 0) {
			LOG_ERR("Failed to set high trip count: %d\n", ret);
			return ret;
		}

		break;

	case SENSOR_ATTR_MAX30210_LO_TRIP_COUNT:
		if (val->val1 < 1 || val->val1 > 4) {
			LOG_ERR("Invalid value for low trip count\n");
			return -EINVAL;
		}
		ret = max30210_reg_update(dev, TEMP_ALARM_LOW_SETUP, TEMP_LO_TRIP_COUNTER_MASK,
					  (val->val1) - 1);
		if (ret < 0) {
			LOG_ERR("Failed to set low trip count: %d\n", ret);
			return ret;
		}
		break;

	case SENSOR_ATTR_MAX30210_HI_TRIP_COUNT_RESET:
		if (val->val1 < 0 || val->val1 > 1) {
			LOG_ERR("Invalid value for high trip count reset\n");
			return -EINVAL;
		}
		ret = max30210_reg_update(dev, TEMP_ALARM_HIGH_SETUP, TEMP_RST_HI_COUNTER,
					  val->val1);
		if (ret < 0) {
			LOG_ERR("Failed to set high trip count reset: %d\n", ret);
			return ret;
		}
		break;

	case SENSOR_ATTR_MAX30210_LO_TRIP_COUNT_RESET:
		if (val->val1 < 0 || val->val1 > 1) {
			LOG_ERR("Invalid value for low trip count reset\n");
			return -EINVAL;
		}
		ret = max30210_reg_update(dev, TEMP_ALARM_LOW_SETUP, TEMP_RST_LO_COUNTER,
					  val->val1);
		if (ret < 0) {
			LOG_ERR("Failed to set low trip count reset: %d\n", ret);
			return ret;
		}
		break;

	case SENSOR_ATTR_MAX30210_ALERT_MODE:
		if (val->val1 < 0 || val->val1 > 1) {
			LOG_ERR("Invalid value for alert mode\n");
			return -EINVAL;
		}
		ret = max30210_reg_update(dev, TEMP_CONFIG_2, ALERT_MODE_MASK, val->val1);
		if (ret < 0) {
			LOG_ERR("Failed to set alert mode: %d\n", ret);
			return ret;
		}
		break;

	default:
		return -ENOTSUP;
	}

	return 0;
}

/**
 * @brief Fetch sample data from MAX30210
 *
 * @param dev Device Pointer
 * @param chan Sensor Channel
 * @return int 0 on success, negative error code on failure
 */
static int max30210_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct max30210_data *data = dev->data;
	uint8_t temp_data[2];
	int ret;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_AMBIENT_TEMP) {
		LOG_ERR("Unsupported channel: %d", chan);
		return -ENOTSUP;
	}

	ret = max30210_reg_read(dev, TEMP_DATA_MSB, temp_data, 2);
	if (ret < 0) {
		LOG_ERR("Failed to read TEMP_DATA_MSB: %d", ret);
		return ret;
	}

	data->temp_data = (temp_data[0] << 8) | temp_data[1];

	return 0;
}

/**
 * @brief Get sensor channel data from MAX30210
 *
 * @param dev Device Pointer
 * @param chan Sensor Channel
 * @param val Value Pointer
 * @return int 0 on success, negative error code on failure
 */
static int max30210_channel_get(const struct device *dev, enum sensor_channel chan,
				struct sensor_value *val)
{
	struct max30210_data *data = dev->data;

	if (chan != SENSOR_CHAN_AMBIENT_TEMP) {
		LOG_ERR("Unsupported channel: %d", chan);
		return -ENOTSUP;
	}
	/** 0.005/LSB*/
	int32_t value = sign_extend(data->temp_data, 15) * 5000;

	val->val1 = value / 1000000U;
	val->val2 = value % 1000000U;
	return 0;
}

/**
 * @brief Probe and configure MAX30210 device
 *
 * @param dev Device Pointer
 * @return int 0 on success, negative error code on failure
 */
static int max30210_probe(const struct device *dev)
{
	const struct max30210_config *config = dev->config;
	int ret = 0;
	/* Set Alarm High Setup */

	uint8_t alarm_setup[2];

	max30210_temp_to_bytes(config->alarm_high_setup, &alarm_setup[0], &alarm_setup[1]);

	ret = max30210_reg_write_multiple(dev, TEMP_ALARM_HIGH_MSB, alarm_setup, 2);
	if (ret < 0) {
		return ret;
	}

	/* Set Alarm Low Setup */

	max30210_temp_to_bytes(config->alarm_low_setup, &alarm_setup[0], &alarm_setup[1]);
	ret = max30210_reg_write_multiple(dev, TEMP_ALARM_LOW_MSB, alarm_setup, 2);
	if (ret < 0) {
		return ret;
	}

	/* Set Increment Fast Threshold */
	ret = max30210_reg_write(dev, TEMP_INC_FAST_THRESH, config->inc_fast_thresh);
	if (ret < 0) {
		return ret;
	}

	/* Set Decrement Fast Threshold */
	ret = max30210_reg_write(dev, TEMP_DEC_FAST_THRESH, config->dec_fast_thresh);
	if (ret < 0) {
		return ret;
	}

	/* Set Sampling Rate */
	ret = max30210_reg_update(dev, TEMP_CONFIG_2, TEMP_PERIOD_MASK, config->sampling_rate);
	if (ret < 0) {
		return ret;
	}

	/* Set High Trip Count */
	ret = max30210_reg_update(dev, TEMP_ALARM_HIGH_SETUP, TEMP_HI_TRIP_COUNTER_MASK,
				  (config->hi_trip_count) - 1);
	if (ret < 0) {
		return ret;
	}

	/* Set High Non-Consecutive Mode */
	ret = max30210_reg_update(dev, TEMP_ALARM_HIGH_SETUP, TEMP_HI_ALARM_TRIP_MASK,
				  config->hi_trip_non_consecutive);

	if (ret < 0) {
		return ret;
	}

	/* Set Low Trip Count */
	ret = max30210_reg_update(dev, TEMP_ALARM_LOW_SETUP, TEMP_LO_TRIP_COUNTER_MASK,
				  (config->lo_trip_count) - 1);

	if (ret < 0) {
		return ret;
	}

	/* Set Low Non-Consecutive Mode */
	ret = max30210_reg_update(dev, TEMP_ALARM_LOW_SETUP, TEMP_LO_ALARM_TRIP_MASK,
				  config->lo_trip_non_consecutive);
	if (ret < 0) {
		return ret;
	}

	ret = max30210_reg_update(dev, TEMP_CONFIG_1, CHG_DET_EN_MASK, 1);
	if (ret < 0) {
		return ret;
	}

	ret = max30210_reg_write(dev, TEMP_CONVERT, 0X03);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

/**
 * @brief Initialize MAX30210 device
 *
 * @param dev Device Pointer
 * @return int 0 on success, negative error code on failure
 */
static int max30210_init(const struct device *dev)
{
	const struct max30210_config *config = dev->config;
	uint8_t part_id;
	uint8_t status;

	if (!i2c_is_ready_dt(&config->i2c)) {
		return -ENODEV;
	}
	/* Check PART_ID */
	int ret = max30210_reg_read(dev, PART_ID, &part_id, 1);

	if (ret < 0) {
		return ret;
	}

	if (part_id != MAX30210_PART_ID) {
		return -ENODEV;
	}
	/* Perform Software Reset */
	ret = max30210_reg_write(dev, SYS_CONFIG, RESET_MASK);
	if (ret < 0) {
		return ret;
	}

	ret = max30210_reg_read(dev, STATUS, &status, 1);
	if (ret < 0) {
		return ret;
	}

	ret = max30210_probe(dev);
	if (ret < 0) {
		return ret;
	}

#ifdef CONFIG_MAX30210_TRIGGER
	if (config->interrupt_gpio.port) {
		if (max30210_init_interrupt(dev)) {
			return -EIO;
		}
	}
#endif

	LOG_INF("MAX30210 device initialized successfully with Part ID: 0x%02X\n", part_id);
	return 0;
}

#define MAX30210_CONFIG(inst)                                                                      \
	.sampling_rate = DT_INST_PROP_OR(inst, sampling_rate, 0),                                  \
	.hi_trip_count = DT_INST_PROP(inst, hi_trip_count),                                        \
	.lo_trip_count = DT_INST_PROP(inst, lo_trip_count),                                        \
	.hi_trip_non_consecutive = DT_INST_PROP(inst, hi_trip_non_consecutive),                    \
	.lo_trip_non_consecutive = DT_INST_PROP(inst, lo_trip_non_consecutive),                    \
	.alarm_high_setup = DT_INST_PROP_OR(inst, alarm_high_setup, 0x7FFF),                       \
	.alarm_low_setup = DT_INST_PROP_OR(inst, alarm_low_setup, 0x8000),                         \
	.inc_fast_thresh = DT_INST_PROP_OR(inst, inc_fast_thresh, 0),                              \
	.dec_fast_thresh = DT_INST_PROP_OR(inst, dec_fast_thresh, 0)

static DEVICE_API(sensor, max30210_driver_api) = {
	.attr_set = max30210_attr_set,
	.sample_fetch = max30210_sample_fetch,
	.channel_get = max30210_channel_get,
#ifdef CONFIG_MAX30210_TRIGGER
	.trigger_set = max30210_trigger_set,
#endif
};

#define MAX30210_DEFINE(inst)                                                                      \
	static struct max30210_data max30210_prv_data_##inst;                                      \
	static const struct max30210_config max30210_config_##inst = {                             \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		MAX30210_CONFIG(inst),                                                             \
		IF_ENABLED(CONFIG_MAX30210_TRIGGER,\
			    (.interrupt_gpio =			\
					GPIO_DT_SPEC_INST_GET_OR( \
					inst, interrupt_gpios, {0}),)) };        \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, &max30210_init, NULL, &max30210_prv_data_##inst,        \
				     &max30210_config_##inst, POST_KERNEL,                         \
				     CONFIG_SENSOR_INIT_PRIORITY, &max30210_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MAX30210_DEFINE)
