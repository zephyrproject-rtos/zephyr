/*
 * Copyright (c) 2022, Felipe Neves
 * Copyright (c) 2025, Cameron Ewing
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ams_as5600

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor/ams_as5600.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ams_as5600, CONFIG_SENSOR_LOG_LEVEL);

#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 0
#warning "AMS AS5600 driver enabled without any devices"
#endif

#define AS5600_FULL_ANGLE     360
#define AS5600_PULSES_PER_REV 4096
#define AS5600_MILLION_UNIT   1000000

/**
 * @brief Set a bitfield within a byte buffer.
 *
 * This function modifies a specific bitfield within a byte buffer by first clearing
 * the bits specified by the mask and then setting them to the provided value.
 *
 * @param buf Pointer to the byte buffer to be modified.
 * @param value The value to set the bitfield to.
 * @param shift The number of bits to shift the value to the correct position.
 * @param mask The bitmask specifying the bitfield to be modified.
 */
static inline void as5600_set_bitfield(uint8_t *buf, int32_t value, uint8_t shift, uint8_t mask)
{
	*buf &= ~mask;
	*buf |= (value << shift) & mask;
}

/**
 * @brief Convert a value to a register format.
 *
 * This function converts a given 12-bit value to bytes and stores it in the provided buffer.
 *
 * @param buf Pointer to the buffer where the register value will be stored.
 * @param val Pointer to the value to be converted.
 * @param mask Mask to be applied to the value during conversion.
 */
static inline void as5600_12bit_to_reg(uint8_t *buf, int32_t value)
{
	buf[0] = (value >> 8) & 0x0F;
	buf[1] = value & 0xFF;
}

/**
 * @brief Fetch a sample from the AS5600 sensor for a specific channel.
 *
 * This function retrieves a sample from the AS5600 sensor and stores it in the
 * appropriate data structure for the specified sensor channel.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param chan The sensor channel to fetch the sample for.
 *
 * @return 0 if successful, a negative error code otherwise.
 */
static int as5600_sample_fetch_chan(const struct device *dev, enum sensor_channel chan)
{
	struct as5600_data *data = dev->data;
	const struct as5600_config *config = dev->config;

	uint8_t buf[4];
	int err;

	switch (chan) {
	case SENSOR_CHAN_ALL: {
		/* Reads are split due to the special behavior of the Angle registers */

		err = i2c_burst_read_dt(&config->i2c, AS5600_RAW_STEPS_H, buf, AS5600_TWO_REG);
		if (err < 0) {
			return err;
		}

		err = i2c_burst_read_dt(&config->i2c, AS5600_FILTERED_STEPS_H, &buf[2],
					AS5600_TWO_REG);
		if (err < 0) {
			return err;
		}

		data->raw_steps = AS5600_REG_TO_12BIT(buf[0], AS5600_12BIT_H_MASK, buf[1]);
		data->filtered_steps = AS5600_REG_TO_12BIT(buf[2], AS5600_12BIT_H_MASK, buf[3]);

		err = i2c_burst_read_dt(&config->i2c, AS5600_AGC, buf, 3);
		if (err < 0) {
			return err;
		}

		data->agc = buf[0];
		data->magnitude = AS5600_REG_TO_12BIT(buf[1], AS5600_12BIT_H_MASK, buf[2]);

		return 0;
	}

	/* Match original implementation: use the filtered step count for calculating rotation */
	case SENSOR_CHAN_ROTATION: {
		err = i2c_burst_read_dt(&config->i2c, AS5600_FILTERED_STEPS_H, buf, AS5600_TWO_REG);
		if (err < 0) {
			return err;
		}

		data->filtered_steps = AS5600_REG_TO_12BIT(buf[0], AS5600_12BIT_H_MASK, buf[1]);
		return 0;
	}

	default:
		break;
	}

	switch ((enum as5600_sensor_channel)chan) {
	case AS5600_SENSOR_CHAN_RAW_STEPS: {
		err = i2c_burst_read_dt(&config->i2c, AS5600_RAW_STEPS_H, buf, AS5600_TWO_REG);
		if (err < 0) {
			return err;
		}

		data->raw_steps = AS5600_REG_TO_12BIT(buf[0], AS5600_12BIT_H_MASK, buf[1]);
		break;
	}

	case AS5600_SENSOR_CHAN_FILTERED_STEPS: {
		err = i2c_burst_read_dt(&config->i2c, AS5600_FILTERED_STEPS_H, buf, AS5600_TWO_REG);
		if (err < 0) {
			return err;
		}

		data->filtered_steps = AS5600_REG_TO_12BIT(buf[0], AS5600_12BIT_H_MASK, buf[1]);
		break;
	}

	case AS5600_SENSOR_CHAN_AGC: {
		err = i2c_reg_read_byte_dt(&config->i2c, AS5600_AGC, &buf[0]);
		if (err < 0) {
			return err;
		}

		data->agc = buf[0];
		break;
	}

	case AS5600_SENSOR_CHAN_MAGNITUDE: {
		err = i2c_burst_read_dt(&config->i2c, AS5600_MAGNITUDE_H, buf, AS5600_TWO_REG);
		if (err < 0) {
			return err;
		}

		data->magnitude = AS5600_REG_TO_12BIT(buf[0], AS5600_12BIT_H_MASK, buf[1]);
		break;
	}

	default:
		return -ENOTSUP;
	}

	return 0;
}

/**
 * @brief Get the sensor value for a specific channel from the internal data.
 *
 * This function retrieves the sensor value for a specified channel and stores it
 * in the provided sensor_value structure.
 *
 * Device specific sensor channels are defined in the sensor header file.
 *
 * @param dev Pointer to the device structure.
 * @param chan The sensor channel to retrieve the value from.
 * @param val Pointer to the sensor_value structure where the value will be stored.
 *
 * @return 0 on success, -ENOTSUP if the channel is not supported.
 */
static int as5600_channel_get(const struct device *dev, enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct as5600_data *data = dev->data;

	/* Matches original implementation: angular range is not accounted for */
	if (chan == SENSOR_CHAN_ROTATION) {
		val->val1 =
			((int32_t)data->filtered_steps * AS5600_FULL_ANGLE) / AS5600_PULSES_PER_REV;

		val->val2 = (((int32_t)data->filtered_steps * AS5600_FULL_ANGLE) %
			     AS5600_PULSES_PER_REV) *
			    (AS5600_MILLION_UNIT / AS5600_PULSES_PER_REV);
		return 0;
	}

	switch ((enum as5600_sensor_channel)chan) {
	case AS5600_SENSOR_CHAN_RAW_STEPS:
		val->val1 = data->raw_steps;
		break;

	case AS5600_SENSOR_CHAN_FILTERED_STEPS:
		val->val1 = data->filtered_steps;
		break;

	case AS5600_SENSOR_CHAN_AGC:
		val->val1 = data->agc;
		break;

	case AS5600_SENSOR_CHAN_MAGNITUDE:
		val->val1 = data->magnitude;
		break;

	default:
		return -ENOTSUP;
	}

	val->val2 = 0;
	return 0;
}

/**
 * @brief Get device attributes/configuration
 *
 * This function retrieves the specified attribute of the AS5600 sensor.
 * The passed channel is ignored as the attributes are global to the device.
 *
 * Device specific attributes are defined in the sensor header file.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param chan Ignored.
 * @param attr The attribute to retrieve.
 * @param val Pointer to the sensor_value structure to store the retrieved value.
 *
 * @return 0 if successful, negative error code otherwise.
 *
 */
static int as5600_attr_get(const struct device *dev, enum sensor_channel chan,
			   enum sensor_attribute attr, struct sensor_value *val)
{
	ARG_UNUSED(chan);
	const struct as5600_config *config = dev->config;

	uint8_t buf[AS5600_TWO_REG];
	int err;

	if (attr == SENSOR_ATTR_HYSTERESIS) {
		err = i2c_reg_read_byte_dt(&config->i2c, AS5600_CONF_L, &buf[0]);
		if (err < 0) {
			return err;
		}

		val->val1 = AS5600_BITFIELD_TO_VAL(buf[0], AS5600_CONF_L_HYST, 2);
		val->val2 = 0;
		return 0;
	}

	switch ((enum as5600_sensor_attribute)attr) {

	case AS5600_ATTR_BURNS:
		err = i2c_reg_read_byte_dt(&config->i2c, AS5600_ZMCO, &buf[0]);
		if (err < 0) {
			return err;
		}

		val->val1 = AS5600_BITFIELD_TO_VAL(buf[0], AS5600_ZMCO_MASK, 0);
		break;
	case AS5600_ATTR_START_POSITION:
		err = i2c_burst_read_dt(&config->i2c, AS5600_ZPOS_H, &buf[0], AS5600_TWO_REG);
		if (err < 0) {
			return err;
		}

		val->val1 = AS5600_REG_TO_12BIT(buf[0], AS5600_12BIT_H_MASK, buf[1]);
		break;

	case AS5600_ATTR_END_POSITION:
		err = i2c_burst_read_dt(&config->i2c, AS5600_MPOS_H, &buf[0], AS5600_TWO_REG);
		if (err < 0) {
			return err;
		}

		val->val1 = AS5600_REG_TO_12BIT(buf[0], AS5600_12BIT_H_MASK, buf[1]);
		break;

	case AS5600_ATTR_ANGULAR_RANGE:
		err = i2c_burst_read_dt(&config->i2c, AS5600_MANG_H, &buf[0], AS5600_TWO_REG);
		if (err < 0) {
			return err;
		}

		val->val1 = AS5600_REG_TO_12BIT(buf[0], AS5600_12BIT_H_MASK, buf[1]);
		break;

	case AS5600_ATTR_POWER_MODE:
		err = i2c_reg_read_byte_dt(&config->i2c, AS5600_CONF_L, &buf[0]);
		if (err < 0) {
			return err;
		}

		val->val1 = AS5600_BITFIELD_TO_VAL(buf[0], AS5600_CONF_L_PM, 0);
		break;

	case AS5600_ATTR_OUTPUT_STAGE:
		err = i2c_reg_read_byte_dt(&config->i2c, AS5600_CONF_L, &buf[0]);
		if (err < 0) {
			return err;
		}

		val->val1 = AS5600_BITFIELD_TO_VAL(buf[0], AS5600_CONF_L_OUTS, 4);
		break;

	case AS5600_ATTR_PWM_FREQ:
		err = i2c_reg_read_byte_dt(&config->i2c, AS5600_CONF_L, &buf[0]);
		if (err < 0) {
			return err;
		}

		val->val1 = AS5600_BITFIELD_TO_VAL(buf[0], AS5600_CONF_L_PWMF, 6);
		break;

	case AS5600_ATTR_SLOW_FILTER:
		err = i2c_reg_read_byte_dt(&config->i2c, AS5600_CONF_H, &buf[0]);
		if (err < 0) {
			return err;
		}

		val->val1 = AS5600_BITFIELD_TO_VAL(buf[0], AS5600_CONF_H_SF, 0);
		break;

	case AS5600_ATTR_FAST_FILTER_THRESHOLD:
		err = i2c_reg_read_byte_dt(&config->i2c, AS5600_CONF_H, &buf[0]);
		if (err < 0) {
			return err;
		}

		val->val1 = AS5600_BITFIELD_TO_VAL(buf[0], AS5600_CONF_H_FTH, 2);
		break;

	case AS5600_ATTR_WATCH_DOG:
		err = i2c_reg_read_byte_dt(&config->i2c, AS5600_CONF_H, &buf[0]);
		if (err < 0) {
			return err;
		}

		val->val1 = AS5600_BITFIELD_TO_VAL(buf[0], AS5600_CONF_H_WD, 5);
		break;

	case AS5600_ATTR_I2CADDR:
		err = i2c_reg_read_byte_dt(&config->i2c, AS5600_I2CADDR, &buf[0]);
		if (err < 0) {
			return err;
		}

		val->val1 = AS5600_BITFIELD_TO_VAL(buf[0], AS5600_I2CADDR_MASK, 1);
		break;

	case AS5600_ATTR_STATUS:
		err = i2c_reg_read_byte_dt(&config->i2c, AS5600_STATUS, &buf[0]);
		if (err < 0) {
			return err;
		}

		val->val1 = AS5600_BITFIELD_TO_VAL(buf[0], AS5600_STATUS_MASK, 3);
		break;

	default:
		return -ENOTSUP;
	}

	val->val2 = 0;
	return 0;
};

/**
 * @brief Set sensor attribute for the AS5600 sensor.
 *
 * This function sets a specific attribute for the AS5600 sensor ignoring the channel as all
 * attributes are global for this sensor.
 *
 * Device specific attributes are defined in the sensor header file.
 *
 * NOTE: END_POSITION and ANGULAR_RANGE are mutually exclusive.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param chan The sensor channel to set the attribute for.
 * @param attr The sensor attribute to set.
 * @param val Pointer to the value to set the attribute to.
 *
 * @return 0 if successful, -ENOTSUP for read only registers, negative errno code if failure.
 */
static int as5600_attr_set(const struct device *dev, enum sensor_channel chan,
			   enum sensor_attribute attr, const struct sensor_value *val)
{
	ARG_UNUSED(chan);
	const struct as5600_config *config = dev->config;
	uint8_t buf[AS5600_TWO_REG];
	int err;

	if (attr == SENSOR_ATTR_HYSTERESIS) {
		err = i2c_reg_read_byte_dt(&config->i2c, AS5600_CONF_L, &buf[0]);
		if (err < 0) {
			return err;
		}

		as5600_set_bitfield(&buf[0], val->val1, 2, AS5600_CONF_L_HYST);

		err = i2c_reg_write_byte_dt(&config->i2c, AS5600_CONF_L, buf[0]);
		return err;
	}

	switch ((enum as5600_sensor_attribute)attr) {

	case AS5600_ATTR_START_POSITION:
		as5600_12bit_to_reg(&buf[0], val->val1);

		err = i2c_burst_write_dt(&config->i2c, AS5600_ZPOS_H, &buf[0], AS5600_TWO_REG);
		return err;

	case AS5600_ATTR_END_POSITION:
		as5600_12bit_to_reg(&buf[0], val->val1);

		err = i2c_burst_write_dt(&config->i2c, AS5600_MPOS_H, &buf[0], AS5600_TWO_REG);
		return err;

	case AS5600_ATTR_ANGULAR_RANGE:
		as5600_12bit_to_reg(&buf[0], val->val1);

		err = i2c_burst_write_dt(&config->i2c, AS5600_MANG_H, &buf[0], AS5600_TWO_REG);
		return err;

	case AS5600_ATTR_POWER_MODE:
		err = i2c_reg_read_byte_dt(&config->i2c, AS5600_CONF_L, &buf[0]);
		if (err < 0) {
			return err;
		}

		as5600_set_bitfield(&buf[0], val->val1, 0, AS5600_CONF_L_PM);

		err = i2c_reg_write_byte_dt(&config->i2c, AS5600_CONF_L, buf[0]);
		return err;

	case AS5600_ATTR_OUTPUT_STAGE:
		err = i2c_reg_read_byte_dt(&config->i2c, AS5600_CONF_L, &buf[0]);
		if (err < 0) {
			return err;
		}

		as5600_set_bitfield(&buf[0], val->val1, 4, AS5600_CONF_L_OUTS);

		err = i2c_reg_write_byte_dt(&config->i2c, AS5600_CONF_L, buf[0]);
		return err;

	case AS5600_ATTR_PWM_FREQ:
		err = i2c_reg_read_byte_dt(&config->i2c, AS5600_CONF_L, &buf[0]);
		if (err < 0) {
			return err;
		}

		as5600_set_bitfield(&buf[0], val->val1, 6, AS5600_CONF_L_PWMF);

		err = i2c_reg_write_byte_dt(&config->i2c, AS5600_CONF_L, buf[0]);
		return err;

	case AS5600_ATTR_SLOW_FILTER:
		err = i2c_reg_read_byte_dt(&config->i2c, AS5600_CONF_H, &buf[0]);
		if (err < 0) {
			return err;
		}

		as5600_set_bitfield(&buf[0], val->val1, 0, AS5600_CONF_H_SF);

		err = i2c_reg_write_byte_dt(&config->i2c, AS5600_CONF_H, buf[0]);
		return err;

	case AS5600_ATTR_FAST_FILTER_THRESHOLD:
		err = i2c_reg_read_byte_dt(&config->i2c, AS5600_CONF_H, &buf[0]);
		if (err < 0) {
			return err;
		}

		as5600_set_bitfield(&buf[0], val->val1, 2, AS5600_CONF_H_FTH);

		err = i2c_reg_write_byte_dt(&config->i2c, AS5600_CONF_H, buf[0]);
		return err;

	case AS5600_ATTR_WATCH_DOG:
		err = i2c_reg_read_byte_dt(&config->i2c, AS5600_CONF_H, &buf[0]);
		if (err < 0) {
			return err;
		}

		as5600_set_bitfield(&buf[0], val->val1, 5, AS5600_CONF_H_WD);

		err = i2c_reg_write_byte_dt(&config->i2c, AS5600_CONF_H, buf[0]);
		return err;

	case AS5600_ATTR_BURNS:
	case AS5600_ATTR_STATUS:
	case AS5600_ATTR_I2CADDR:
		LOG_WRN("Attribute %d is read only", attr);
	default:
		return -ENOTSUP;
	}
};

DEVICE_API(sensor, as5600_api) = {
	.sample_fetch = &as5600_sample_fetch_chan,
	.channel_get = &as5600_channel_get,
	.attr_get = &as5600_attr_get,
	.attr_set = &as5600_attr_set,
};

/**
 * @brief Initialize the AS5600 sensor device.
 *
 * This function initializes the AS5600 sensor device. If the device is not configured
 * in the device tree, it will be initialized with its power-on reset settings.
 * This function does not verify the device settings. If the device was previously burned
 * settings may not be applied. The user is expected to confirm that the settings were
 * applied correctly.
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @return 0 if successful, or a negative error code on failure.
 */
static int as5600_init(const struct device *dev)
{
	const struct as5600_config *config = dev->config;
	const struct i2c_dt_spec *i2c = &config->i2c;

	int err;
	struct sensor_value val;
	uint8_t conf[AS5600_TWO_REG];

	/* Check the I2C address from the device */
	err = as5600_attr_get(dev, SENSOR_CHAN_ALL, AS5600_ATTR_I2CADDR, &val);
	if (err < 0) {
		LOG_ERR("Could not read I2C address");
		return err;
	}

	if (val.val1 != i2c->addr) {
		LOG_ERR("I2C address is incorrect. Found 0x%x, Expected 0x%x", val.val1,
			config->i2c.addr);
		return -EINVAL;
	}

	/* End Position and Angular Range are mutually exclusive, Error if both are set */
	if ((config->end_position != AS5600_UNCONFIGURED_2_REG) &&
	    (config->angular_range != AS5600_UNCONFIGURED_2_REG)) {
		LOG_ERR("End Position and Angular Range are mutually exclusive");
		return -EINVAL;
	}

	/* Apply the device tree settings to the sensor if configured, otherwise use power on reset
	 * values
	 */

	err = i2c_burst_read_dt(i2c, AS5600_CONF_H, conf, AS5600_TWO_REG);
	if (err < 0) {
		LOG_ERR("Could not read CONF registers");
		return err;
	}

	if (config->power_mode != AS5600_UNCONFIGURED_1_REG) {
		as5600_set_bitfield(&conf[1], config->power_mode, 0, AS5600_CONF_L_PM);
	}

	if (config->hysteresis != AS5600_UNCONFIGURED_1_REG) {
		as5600_set_bitfield(&conf[1], config->hysteresis, 2, AS5600_CONF_L_HYST);
	}

	if (config->output_stage != AS5600_UNCONFIGURED_1_REG) {
		as5600_set_bitfield(&conf[1], config->output_stage, 4, AS5600_CONF_L_OUTS);
	}

	if (config->pwm_frequency != AS5600_UNCONFIGURED_1_REG) {
		as5600_set_bitfield(&conf[1], config->pwm_frequency, 6, AS5600_CONF_L_PWMF);
	}

	if (config->slow_filter != AS5600_UNCONFIGURED_1_REG) {
		as5600_set_bitfield(&conf[0], config->slow_filter, 0, AS5600_CONF_H_SF);
	}

	if (config->fast_filter_threshold != AS5600_UNCONFIGURED_1_REG) {
		as5600_set_bitfield(&conf[0], config->fast_filter_threshold, 2, AS5600_CONF_H_FTH);
	}

	if (config->watch_dog != AS5600_UNCONFIGURED_1_REG) {
		as5600_set_bitfield(&conf[0], config->watch_dog, 5, AS5600_CONF_H_WD);
	}

	err = i2c_burst_write_dt(i2c, AS5600_CONF_H, conf, AS5600_TWO_REG);
	if (err < 0) {
		LOG_ERR("Could not write CONF registers");
		return err;
	}

	if (config->start_position != AS5600_UNCONFIGURED_2_REG) {
		as5600_12bit_to_reg(conf, config->start_position);

		err = i2c_burst_write_dt(i2c, AS5600_ZPOS_H, conf, AS5600_TWO_REG);
		if (err < 0) {
			LOG_ERR("Could not write ZPOS registers");
			return err;
		}
	}

	if (config->end_position != AS5600_UNCONFIGURED_2_REG) {
		as5600_12bit_to_reg(conf, config->end_position);

		err = i2c_burst_write_dt(i2c, AS5600_MPOS_H, conf, AS5600_TWO_REG);
		if (err < 0) {
			LOG_ERR("Could not write MPOS registers");
			return err;
		}
	}

	if (config->angular_range != AS5600_UNCONFIGURED_2_REG) {
		as5600_12bit_to_reg(conf, config->angular_range);

		err = i2c_burst_write_dt(i2c, AS5600_MANG_H, conf, AS5600_TWO_REG);
		if (err < 0) {
			LOG_ERR("Could not write MANG registers");
			return err;
		}
	}

	return 0;
}

#define AS5600_INIT(n)                                                                             \
	static struct as5600_data as5600_data##n;                                                  \
	static const struct as5600_config as5600_config##n = {                                     \
		.i2c = I2C_DT_SPEC_INST_GET(n),                                                    \
		.start_position = DT_INST_PROP_OR(n, start_position, AS5600_UNCONFIGURED_2_REG),   \
		.end_position = DT_INST_PROP_OR(n, end_position, AS5600_UNCONFIGURED_2_REG),       \
		.angular_range = DT_INST_PROP_OR(n, angular_range, AS5600_UNCONFIGURED_2_REG),     \
		.power_mode = DT_INST_ENUM_IDX_OR(n, power_mode, AS5600_UNCONFIGURED_1_REG),       \
		.hysteresis = DT_INST_ENUM_IDX_OR(n, hysteresis, AS5600_UNCONFIGURED_1_REG),       \
		.output_stage = DT_INST_ENUM_IDX_OR(n, output_stage, AS5600_UNCONFIGURED_1_REG),   \
		.pwm_frequency = DT_INST_ENUM_IDX_OR(n, pwm_frequency, AS5600_UNCONFIGURED_1_REG), \
		.slow_filter = DT_INST_ENUM_IDX_OR(n, slow_filter, AS5600_UNCONFIGURED_1_REG),     \
		.fast_filter_threshold =                                                           \
			DT_INST_ENUM_IDX_OR(n, fast_filter_threshold, AS5600_UNCONFIGURED_1_REG),  \
		.watch_dog = DT_INST_ENUM_IDX_OR(n, watch_dog, AS5600_UNCONFIGURED_1_REG),         \
	};                                                                                         \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(n, as5600_init, NULL, &as5600_data##n, &as5600_config##n,     \
				     POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, &as5600_api);

DT_INST_FOREACH_STATUS_OKAY(AS5600_INIT)
