/*
 * Copyright (c) 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT asahi_kasei_akm09918c

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor_data_types.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>

#include "akm09918c.h"
#include "akm09918c_reg.h"

LOG_MODULE_REGISTER(AKM09918C, CONFIG_SENSOR_LOG_LEVEL);

/**
 * @brief Perform the bus transaction to fetch samples
 *
 * @param dev Sensor device to operate on
 * @param chan Channel ID to fetch
 * @param x Location to write X channel sample.
 * @param y Location to write Y channel sample.
 * @param z Location to write Z channel sample.
 * @return int 0 if successful or error code
 */
int akm09918c_sample_fetch_helper(const struct device *dev, enum sensor_channel chan, int16_t *x,
				  int16_t *y, int16_t *z)
{
	struct akm09918c_data *data = dev->data;
	const struct akm09918c_config *cfg = dev->config;
	uint8_t buf[9] = {0};

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_MAGN_X && chan != SENSOR_CHAN_MAGN_Y &&
	    chan != SENSOR_CHAN_MAGN_Z && chan != SENSOR_CHAN_MAGN_XYZ) {
		LOG_DBG("Invalid channel %d", chan);
		return -EINVAL;
	}

	if (data->mode == AKM09918C_CNTL2_PWR_DOWN) {
		if (i2c_reg_write_byte_dt(&cfg->i2c, AKM09918C_REG_CNTL2,
					  AKM09918C_CNTL2_SINGLE_MEASURE) != 0) {
			LOG_ERR("Failed to start measurement.");
			return -EIO;
		}

		/* Wait for sample */
		LOG_DBG("Waiting for sample...");
		k_usleep(AKM09918C_MEASURE_TIME_US);
	}

	/* We have to read through the TMPS register or the data_ready bit won't clear */
	if (i2c_burst_read_dt(&cfg->i2c, AKM09918C_REG_ST1, buf, ARRAY_SIZE(buf)) != 0) {
		LOG_ERR("Failed to read sample data.");
		return -EIO;
	}

	if (FIELD_GET(AKM09918C_ST1_DRDY, buf[0]) == 0) {
		LOG_ERR("Data not ready, st1=0x%02x", buf[0]);
		return -EBUSY;
	}

	*x = sys_le16_to_cpu(buf[1] | (buf[2] << 8));
	*y = sys_le16_to_cpu(buf[3] | (buf[4] << 8));
	*z = sys_le16_to_cpu(buf[5] | (buf[6] << 8));

	return 0;
}

static int akm09918c_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct akm09918c_data *data = dev->data;

	return akm09918c_sample_fetch_helper(dev, chan, &data->x_sample, &data->y_sample,
					     &data->z_sample);
}

static void akm09918c_convert(struct sensor_value *val, int16_t sample)
{
	int64_t conv_val = sample * AKM09918C_MICRO_GAUSS_PER_BIT;

	val->val1 = conv_val / 1000000;
	val->val2 = conv_val - (val->val1 * 1000000);
}

static int akm09918c_channel_get(const struct device *dev, enum sensor_channel chan,
				 struct sensor_value *val)
{
	struct akm09918c_data *data = dev->data;

	if (chan == SENSOR_CHAN_MAGN_XYZ) {
		akm09918c_convert(val, data->x_sample);
		akm09918c_convert(val + 1, data->y_sample);
		akm09918c_convert(val + 2, data->z_sample);
	} else if (chan == SENSOR_CHAN_MAGN_X) {
		akm09918c_convert(val, data->x_sample);
	} else if (chan == SENSOR_CHAN_MAGN_Y) {
		akm09918c_convert(val, data->y_sample);
	} else if (chan == SENSOR_CHAN_MAGN_Z) {
		akm09918c_convert(val, data->z_sample);
	} else {
		LOG_DBG("Invalid channel %d", chan);
		return -ENOTSUP;
	}

	return 0;
}

static int akm09918c_attr_get(const struct device *dev, enum sensor_channel chan,
			      enum sensor_attribute attr, struct sensor_value *val)
{
	struct akm09918c_data *data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_MAGN_X:
	case SENSOR_CHAN_MAGN_Y:
	case SENSOR_CHAN_MAGN_Z:
	case SENSOR_CHAN_MAGN_XYZ:
		if (attr != SENSOR_ATTR_SAMPLING_FREQUENCY) {
			LOG_WRN("Invalid attribute %d", attr);
			return -EINVAL;
		}
		akm09918c_reg_to_hz(data->mode, val);
		break;
	default:
		LOG_WRN("Invalid channel %d", chan);
		return -EINVAL;
	}
	return 0;
}

static int akm09918c_attr_set(const struct device *dev, enum sensor_channel chan,
			      enum sensor_attribute attr, const struct sensor_value *val)
{
	const struct akm09918c_config *cfg = dev->config;
	struct akm09918c_data *data = dev->data;
	int res;

	switch (chan) {
	case SENSOR_CHAN_MAGN_X:
	case SENSOR_CHAN_MAGN_Y:
	case SENSOR_CHAN_MAGN_Z:
	case SENSOR_CHAN_MAGN_XYZ:
		if (attr != SENSOR_ATTR_SAMPLING_FREQUENCY) {
			LOG_WRN("Invalid attribute %d", attr);
			return -EINVAL;
		}

		uint8_t mode = akm09918c_hz_to_reg(val);

		res = i2c_reg_write_byte_dt(&cfg->i2c, AKM09918C_REG_CNTL2, mode);
		if (res != 0) {
			LOG_ERR("Failed to set sample frequency");
			return -EIO;
		}
		data->mode = mode;
		break;
	default:
		LOG_WRN("Invalid channel %d", chan);
		return -EINVAL;
	}

	return 0;
}

static inline int akm09918c_check_who_am_i(const struct i2c_dt_spec *i2c)
{
	uint8_t buffer[2];
	int rc;

	rc = i2c_burst_read_dt(i2c, AKM09918C_REG_WIA1, buffer, ARRAY_SIZE(buffer));
	if (rc != 0) {
		LOG_ERR("Failed to read who-am-i register (rc=%d)", rc);
		return -EIO;
	}

	if (buffer[0] != AKM09918C_WIA1 || buffer[1] != AKM09918C_WIA2) {
		LOG_ERR("Wrong who-am-i value");
		return -EINVAL;
	}

	return 0;
}

static int akm09918c_init(const struct device *dev)
{
	const struct akm09918c_config *cfg = dev->config;
	struct akm09918c_data *data = dev->data;
	int rc;

	if (!i2c_is_ready_dt(&cfg->i2c)) {
		LOG_ERR("I2C bus device not ready");
		return -ENODEV;
	}

	/* Soft reset the chip */
	rc = i2c_reg_write_byte_dt(&cfg->i2c, AKM09918C_REG_CNTL3,
				   FIELD_PREP(AKM09918C_CNTL3_SRST, 1));
	if (rc != 0) {
		LOG_ERR("Failed to soft reset");
		return -EIO;
	}

	/* check chip ID */
	rc = akm09918c_check_who_am_i(&cfg->i2c);
	if (rc != 0) {
		return rc;
	}
	data->mode = AKM09918C_CNTL2_PWR_DOWN;

	return 0;
}

static const struct sensor_driver_api akm09918c_driver_api = {
	.sample_fetch = akm09918c_sample_fetch,
	.channel_get = akm09918c_channel_get,
	.attr_get = akm09918c_attr_get,
	.attr_set = akm09918c_attr_set,
#ifdef CONFIG_SENSOR_ASYNC_API
	.submit = akm09918c_submit,
	.get_decoder = akm09918c_get_decoder,
#endif
};

#define AKM09918C_DEFINE(inst)                                                                     \
	static struct akm09918c_data akm09918c_data_##inst;                                        \
                                                                                                   \
	static const struct akm09918c_config akm09918c_config_##inst = {                           \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
	};                                                                                         \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, akm09918c_init, NULL, &akm09918c_data_##inst,           \
				     &akm09918c_config_##inst, POST_KERNEL,                        \
				     CONFIG_SENSOR_INIT_PRIORITY, &akm09918c_driver_api);

DT_INST_FOREACH_STATUS_OKAY(AKM09918C_DEFINE)
