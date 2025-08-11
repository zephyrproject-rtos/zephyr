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

	return ret < 0 ? ret : 0;
}

int max30210_reg_read(const struct device *dev, uint8_t reg_addr, uint8_t *val, uint8_t length)
{
	return max30210_reg_access(dev, reg_addr, val, true, length);
}

int max30210_reg_write(const struct device *dev, uint8_t reg_addr, uint8_t val, uint8_t length)
{
	return max30210_reg_access(dev, reg_addr, &val, false, 1);
}

int max30210_reg_write_multiple(const struct device *dev, uint8_t reg_addr, const uint8_t *val,
				uint8_t length)
{
	const struct max30210_config *config = dev->config;
	int ret;
	if (length < 2) {
		return -EINVAL; // Invalid length
	}
	return max30210_reg_access(dev, reg_addr, val, false, length);
}

int max30210_reg_update(const struct device *dev, uint8_t reg_addr, uint8_t mask, uint8_t val)
{
	uint8_t reg_val = 0;
	int ret;

	ret = max30210_reg_read(dev, reg_addr, &reg_val, 1);
	if (ret < 0) {
		return ret;
	}

	reg_val &= ~mask;                 // Clear the bits specified by the mask
	reg_val |= FIELD_PREP(mask, val); // Set the bits specified by val

	return max30210_reg_write(dev, reg_addr, reg_val, 1);
}

static int max30210_attr_set(const struct device *dev, enum sensor_channel chan,
			     enum sensor_attribute attr, const struct sensor_value *val)
{
	const struct max30210_config *config = dev->config;
	int ret;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_AMBIENT_TEMP) {
		return -ENOTSUP;
	}

	struct max30210_data *temp_data = dev->data;
	const struct max30210_config *cfg = dev->config;
	uint16_t whole_numbers_data;
	uint16_t fractional_data;

	switch ((int)attr) {
	case SENSOR_ATTR_MAX30210_CONTINUOUS_CONVERSION_MODE:
		ret = max30210_reg_write(dev, TEMP_CONVERT, 0X03, 1);
		break;

	case SENSOR_ATTR_LOWER_THRESH:

		if (val->val1 < 0 || val->val1 > 164) {
			return -EINVAL; // Temperature out of range
		}
		if (val->val2 < 0 || val->val2 > 999999) {
			return -EINVAL; // Fractional part out of range
		}
		whole_numbers_data = (val->val1) * 200;
		fractional_data =
			(val->val2 * 200) / 1000000; // Convert microdegrees to 200ths of a degree
		uint16_t lower_thresh = whole_numbers_data + fractional_data;
		temp_data->temp_alarm_low_setup =
			lower_thresh; // Store the lower threshold value in the config structure
		// Prepare the data to write to the register
		uint8_t lower_thresh_data[2] = {
			(lower_thresh >> 8) & 0xFF, // MSB
			lower_thresh & 0xFF         // LSB
		};
		// Write the lower threshold value to the TEMP_ALARM_LOW_SETUP register
		ret = max30210_reg_write_multiple(dev, TEMP_ALARM_LOW_MSB, lower_thresh_data, 2);
		if (ret < 0) {
			return ret;
		}
		break;

	case SENSOR_ATTR_UPPER_THRESH:

		if (val->val1 < 0 || val->val1 > 164) {
			return -EINVAL; // Temperature out of range
		}
		if (val->val2 < 0 || val->val2 > 999999) {
			return -EINVAL; // Fractional part out of range
		}

		whole_numbers_data = (val->val1) * 200;
		fractional_data =
			(val->val2 * 200) / 1000000; // Convert microdegrees to 200ths of a degree
		uint16_t upper_thresh = whole_numbers_data + fractional_data;
		temp_data->temp_alarm_high_setup =
			upper_thresh; // Store the upper threshold value in the config structure
		// Prepare the data to write to the register
		uint8_t upper_thresh_data[2] = {
			(upper_thresh >> 8) & 0xFF, // MSB
			upper_thresh & 0xFF         // LSB
		};
		// Write the upper threshold value to the TEMP_ALARM_HIGH_SETUP register
		ret = max30210_reg_write_multiple(dev, TEMP_ALARM_HIGH_MSB, upper_thresh_data, 2);
		if (ret < 0) {
			return ret;
		}
		break;

	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		if (val->val1 < 0) {
			return -EINVAL;
		}
		if (val->val1 > 10) {
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
				return -EINVAL; // Invalid sampling rate
			}

		}

		else {
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
				return -EINVAL; // Invalid sampling rate
			}
		}

		ret = max30210_reg_update(dev, TEMP_CONFIG_2, TEMP_PERIOD_MASK, sampling_rate);
		break;

	case SENSOR_ATTR_MAX30210_TEMP_INC_FAST_THRESH:
		if (val->val1 < 0 || val->val1 > 1) {
			return -EINVAL; // Temperature out of range
		}

		if (val->val2 < 0 || (val->val2 % 5000 == 0) || val->val2 > 999999) {
			return -EINVAL; // Fractional part out of range
		}

		whole_numbers_data = (val->val1) * 200;
		fractional_data = ((val->val2) / 1000000) * 200;
		uint8_t inc_fast_thresh =
			whole_numbers_data + fractional_data; // Convert to 200ths of a degree
		temp_data->temp_inc_fast_thresh =
			inc_fast_thresh; // Store the increment fast threshold value in the config
					 // structure
		if (inc_fast_thresh > 255) {
			return -EINVAL; // Threshold out of range
		}
		// Write the increment fast threshold value to the TEMP_INC_FAST_THRESH register
		ret = max30210_reg_write(dev, TEMP_INC_FAST_THRESH, inc_fast_thresh, 1);
		if (ret < 0) {
			return ret;
		}
		break;

	case SENSOR_ATTR_MAX30210_TEMP_DEC_FAST_THRESH:
		if (val->val1 < 0 || val->val1 > 1) {
			return -EINVAL; // Temperature out of range
		}

		if (val->val2 < 0 || (val->val2 % 5000 == 0) || val->val2 > 999999) {
			return -EINVAL; // Fractional part out of range
		}

		whole_numbers_data = (val->val1) * 200;
		fractional_data = ((val->val2) / 1000000) * 200;
		uint8_t dec_fast_thresh =
			whole_numbers_data + fractional_data; // Convert to 200ths of a degree
		temp_data->temp_dec_fast_thresh =
			dec_fast_thresh; // Store the decrement fast threshold value in the config
					 // structure
		if (dec_fast_thresh > 255) {
			return -EINVAL; // Threshold out of range
		}
		// Write the decrement fast threshold value to the TEMP_DEC_FAST_THRESH register
		ret = max30210_reg_write(dev, TEMP_DEC_FAST_THRESH, dec_fast_thresh, 1);
		break;

	case SENSOR_ATTR_MAX30210_SOFTWARE_RESET:

		ret = max30210_reg_write(dev, SYS_CONFIG, RESET_MASK, 1); // Software reset
		if (ret < 0) {
			return ret;
		}
		k_sleep(K_MSEC(10)); // Wait for the device to reset
		break;

	case SENSOR_ATTR_MAX30210_RATE_CHG_FILTER:
		if (val->val1 < 0 || val->val1 > 7) {
			return -EINVAL; // Invalid rate change filter value
		}
		ret = max30210_reg_update(dev, TEMP_CONFIG_1, RATE_CHRG_FILTER_MASK, val->val1);
		if (ret < 0) {
			return ret;
		}
		break;

	case SENSOR_ATTR_MAX30210_HI_NON_CONSECUTIVE_MODE:
		if (val->val1 < 0 || val->val1 > 1) {
			return -EINVAL; // Invalid consecutive mode value
		}

		ret = max30210_reg_update(dev, TEMP_ALARM_HIGH_SETUP, TEMP_HI_ALARM_TRIP_MASK,
					  val->val1);
		if (ret < 0) {
			printf("Failed to set high consecutive mode: %d\n", ret);
			return ret;
		}
		break;

	case SENSOR_ATTR_MAX30210_LO_NON_CONSECUTIVE_MODE:
		if (val->val1 < 0 || val->val1 > 1) {
			return -EINVAL; // Invalid consecutive mode value
		}

		ret = max30210_reg_update(dev, TEMP_ALARM_LOW_SETUP, TEMP_LO_ALARM_TRIP_MASK,
					  val->val1);
		if (ret < 0) {
			printf("Failed to set low consecutive mode: %d\n", ret);
			return ret;
		}
		break;

	case SENSOR_ATTR_MAX30210_HI_TRIP_COUNT:
		if (val->val1 < 1 || val->val1 > 4) {
			return -EINVAL; // Invalid high trip count value
		}
		ret = max30210_reg_update(dev, TEMP_ALARM_HIGH_SETUP, TEMP_HI_TRIP_COUNTER_MASK,
					  (val->val1) - 1);
		if (ret < 0) {
			printf("Failed to set high trip count: %d\n", ret);
			return ret;
		}

		break;

	case SENSOR_ATTR_MAX30210_LO_TRIP_COUNT:
		if (val->val1 < 1 || val->val1 > 4) {
			return -EINVAL; // Invalid low trip count value
		}
		ret = max30210_reg_update(dev, TEMP_ALARM_LOW_SETUP, TEMP_LO_TRIP_COUNTER_MASK,
					  (val->val1) - 1);
		if (ret < 0) {
			printf("Failed to set low trip count: %d\n", ret);
			return ret;
		}
		break;

	case SENSOR_ATTR_MAX30210_HI_TRIP_COUNT_RESET:
		if (val->val1 < 0 || val->val1 > 1) {
			return -EINVAL; // Invalid high trip count reset value
		}
		ret = max30210_reg_update(dev, TEMP_ALARM_HIGH_SETUP, TEMP_RST_HI_COUNTER,
					  val->val1);
		if (ret < 0) {
			printf("Failed to set high trip count reset: %d\n", ret);
			return ret;
		}
		break;

	case SENSOR_ATTR_MAX30210_LO_TRIP_COUNT_RESET:
		if (val->val1 < 0 || val->val1 > 1) {
			return -EINVAL; // Invalid low trip count reset value
		}
		ret = max30210_reg_update(dev, TEMP_ALARM_LOW_SETUP, TEMP_RST_LO_COUNTER,
					  val->val1);
		if (ret < 0) {
			printf("Failed to set low trip count reset: %d\n", ret);
			return ret;
		}
		break;

	case SENSOR_ATTR_MAX30210_ALERT_MODE:
		if (val->val1 < 0 || val->val1 > 1) {
			return -EINVAL; // Invalid alert mode value
		}
		ret = max30210_reg_update(dev, TEMP_CONFIG_2, ALERT_MODE_MASK, val->val1);
		if (ret < 0) {
			return ret;
		}
		break;

	default:
		return -ENOTSUP;
	}

	return ret < 0 ? ret : 0;
}

static int max30210_init(const struct device *dev)
{
	const struct max30210_config *config = dev->config;

	if (!i2c_is_ready_dt(&config->i2c)) {
		return -ENODEV;
	}
#ifdef CONFIG_MAX30210_TRIGGER
	if (config->interrupt_gpio.port) {
		if (max30210_init_interrupt(dev)) {
			return -EIO;
		}
	}
#endif
	// /* Check PART_ID */
	uint8_t part_id;
	int ret = max30210_reg_read(dev, PART_ID, &part_id, 1);

	if (part_id != MAX30210_PART_ID) {
		return -ENODEV;
	}
	ret = max30210_reg_write(dev, SYS_CONFIG, RESET_MASK, 1);
	if (ret < 0) {
		return ret; // Failed to write SYS_CONFIG register
	}

	uint8_t status;
	ret = max30210_reg_read(dev, STATUS, &status, 1);
	if (ret < 0) {
		return ret; // Failed to read status register
	}

	printf("MAX30210 device initialized successfully with Part ID: 0x%02X\n", part_id);
	return 0;
}

static int max30210_attr_get(const struct device *dev, enum sensor_channel chan,
			     enum sensor_attribute attr, struct sensor_value *val)
{
	uint8_t reg_val;
	int ret;

	// switch (attr) {
	// case SENSOR_ATTR_TEMP_DATA_MSB:
	//     ret = max30210_reg_read(dev, TEMP_DATA_MSB, &reg_val);
	//     if (ret < 0) {
	//         return ret;
	//     }
	//     val->val1 = reg_val;
	//     break;
	// case SENSOR_ATTR_TEMP_DATA_LSB:
	//     ret = max30210_reg_read(dev, TEMP_DATA_LSB, &reg_val);
	//     if (ret < 0) {
	//         return ret;
	//     }
	//     val->val2 = reg_val;
	//     break;
	// default:
	//     LOG_ERR("Unsupported attribute: %d", attr);
	//     return -ENOTSUP;
	// }

	return 0;
}

static int max30210_sample_fetch(const struct device *dev, enum sensor_channel chan)
{

	const struct max30210_config *config = dev->config;
	struct max30210_data *data = dev->data;
	uint8_t temp_data[2];
	uint8_t cvt_setup;
	int ret;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_AMBIENT_TEMP) {
		// LOG_ERR("Unsupported channel: %d", chan);
		return -ENOTSUP;
	}

#ifndef CONFIG_MAX30210_FIFO_MODE
	ret = max30210_reg_read(dev, TEMP_DATA_MSB, temp_data, 2);

	if (ret < 0) {
		// LOG_ERR("Failed to read TEMP_DATA_MSB: %d", ret);
		return ret; // Failed to read register
	}

	data->temp_data = (temp_data[0] << 8) | temp_data[1];
#else

	/** FIFO MODE */
	uint8_t fifo_data[MAX30210_FIFO_DEPTH * MAX30210_BYTES_PER_SAMPLE];
	uint8_t fifo_count;
	uint8_t fifo_ovf;
	uint8_t num_bytes;
	ret = max30210_reg_read(dev, FIFO_COUNTER_1, &fifo_ovf, 1);
	if (ret < 0) {
		// LOG_ERR("Failed to read FIFO_COUNTER_1: %d", ret);
		return ret; // Failed to read register
	}
	if (fifo_ovf & FIFO_OVF_MASK) {
		LOG_ERR("FIFO overflow detected");
		// return -EIO; // FIFO overflow error
	}
	ret = max30210_reg_read(dev, FIFO_COUNTER_2, &fifo_count, 1);
	if (ret < 0) {
		// LOG_ERR("Failed to read FIFO_COUNTER_2: %d", ret);
		return ret; // Failed to read register
	}
	data->fifo_data_count = fifo_count;
	if (fifo_count == 0) {
		// LOG_ERR("FIFO is empty");
		return -ENODATA; // No data in FIFO
	}
	num_bytes = fifo_count * MAX30210_BYTES_PER_SAMPLE;
	if (num_bytes > sizeof(fifo_data)) {
		// LOG_ERR("FIFO data size exceeds buffer size");
		return -ENOMEM; // Buffer overflow error
	}
	ret = max30210_reg_read(dev, FIFO_DATA, fifo_data, num_bytes);
	if (ret < 0) {
		// LOG_ERR("Failed to read FIFO_DATA: %d", ret);
		return ret; // Failed to read register
	}
	// Process FIFO data

	for (int i = 0; i < fifo_count; i++) {
		data->fifo_status_data[i] = fifo_data[i * MAX30210_BYTES_PER_SAMPLE];
		data->fifo_temp_data[i] = ((fifo_data[i * MAX30210_BYTES_PER_SAMPLE + 1] << 8) |
					   fifo_data[i * MAX30210_BYTES_PER_SAMPLE + 2]);
	}

#endif
	return 0;
}

static int max30210_channel_get(const struct device *dev, enum sensor_channel chan,
				struct sensor_value *val)
{
	struct max30210_data *data = dev->data;

#ifndef CONFIG_MAX30210_FIFO_MODE
	int32_t value;
	if (chan != SENSOR_CHAN_AMBIENT_TEMP) {
		// LOG_ERR("Unsupported channel: %d", chan);
		return -ENOTSUP;
	}
	value = (int32_t)(data->temp_data * 5000); // Convert to microdegrees Celsius
	val->val1 = value / 1000000U;              // Integer part
	val->val2 = value % 1000000U;              // Fractional part in microdegrees
#else
	if (chan != SENSOR_CHAN_AMBIENT_TEMP) {
		// LOG_ERR("Unsupported channel: %d", chan);
		return -ENOTSUP;
	}

#endif
	return 0;
}

static DEVICE_API(sensor, max30210_driver_api) = {
	.attr_set = max30210_attr_set,
	.attr_get = max30210_attr_get,
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
		IF_ENABLED(CONFIG_MAX30210_TRIGGER,						   \
			    (.interrupt_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, interrupt_gpios, {0}),)) };        \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, &max30210_init, NULL, &max30210_prv_data_##inst,        \
				     &max30210_config_##inst, POST_KERNEL,                         \
				     CONFIG_SENSOR_INIT_PRIORITY, &max30210_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MAX30210_DEFINE)
