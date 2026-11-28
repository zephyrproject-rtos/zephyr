/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT isentek_ist8308

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(IST8308, CONFIG_SENSOR_LOG_LEVEL);

#define IST8308_REG_WAI     0x00U
#define IST8308_REG_STAT    0x10U
#define IST8308_REG_ACTR    0x20U
#define IST8308_REG_CNTL1   0x30U
#define IST8308_REG_CNTL2   0x31U
#define IST8308_REG_CNTL3   0x32U
#define IST8308_REG_CNTL4   0x34U
#define IST8308_REG_OSRCNTL 0x41U

#define IST8308_WAI_VALUE 0x08U

#define IST8308_STAT_DRDY        BIT(0)
#define IST8308_ACTR_SUSPEND_EN  BIT(1)
#define IST8308_CNTL1_NSF_MASK   GENMASK(6, 5)
#define IST8308_CNTL2_MODE_MASK  GENMASK(4, 0)
#define IST8308_CNTL2_MODE_50HZ  0x06U
#define IST8308_CNTL3_SRST       BIT(0)
#define IST8308_CNTL4_DR_MASK    GENMASK(1, 0)
#define IST8308_CNTL4_DR_200UT   0x01U
#define IST8308_OSRCNTL_MASK     GENMASK(5, 0)
#define IST8308_OSRCNTL_32       0x2DU

/* the power-on reset takes at most 50 ms */
#define IST8308_RESET_TIME_MS 50

/* +/-200 uT range: 13.2 LSB/uT */
#define IST8308_LSB_PER_GAUSS 1320

struct ist8308_config {
	struct i2c_dt_spec i2c;
};

struct ist8308_data {
	int16_t sample[3];
	bool has_sample;
};

static int ist8308_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct ist8308_config *config = dev->config;
	struct ist8308_data *data = dev->data;
	uint8_t buf[7];
	int ret;

	if ((chan != SENSOR_CHAN_ALL) && (chan != SENSOR_CHAN_MAGN_XYZ)) {
		return -ENOTSUP;
	}

	ret = i2c_burst_read_dt(&config->i2c, IST8308_REG_STAT, buf, sizeof(buf));
	if (ret != 0) {
		return ret;
	}

	/* polled faster than its output data rate, the chip has the previous sample */
	if ((buf[0] & IST8308_STAT_DRDY) == 0U) {
		return data->has_sample ? 0 : -EBUSY;
	}

	for (size_t i = 0; i < ARRAY_SIZE(data->sample); i++) {
		data->sample[i] = (int16_t)sys_get_le16(&buf[1 + (2 * i)]);
	}

	data->has_sample = true;

	return 0;
}

static void ist8308_convert(struct sensor_value *val, int16_t raw)
{
	val->val1 = raw / IST8308_LSB_PER_GAUSS;
	val->val2 = ((int32_t)raw % IST8308_LSB_PER_GAUSS) * 1000000 / IST8308_LSB_PER_GAUSS;
}

static int ist8308_channel_get(const struct device *dev, enum sensor_channel chan,
			       struct sensor_value *val)
{
	const struct ist8308_data *data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_MAGN_X:
		ist8308_convert(val, data->sample[0]);
		break;
	case SENSOR_CHAN_MAGN_Y:
		ist8308_convert(val, data->sample[1]);
		break;
	case SENSOR_CHAN_MAGN_Z:
		ist8308_convert(val, data->sample[2]);
		break;
	case SENSOR_CHAN_MAGN_XYZ:
		for (size_t i = 0; i < ARRAY_SIZE(data->sample); i++) {
			ist8308_convert(&val[i], data->sample[i]);
		}
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static DEVICE_API(sensor, ist8308_api) = {
	.sample_fetch = ist8308_sample_fetch,
	.channel_get = ist8308_channel_get,
};

static int ist8308_init(const struct device *dev)
{
	/* the measurement mode comes last, with the other settings in place */
	static const struct {
		uint8_t reg;
		uint8_t mask;
		uint8_t value;
	} settings[] = {
		{IST8308_REG_ACTR, IST8308_ACTR_SUSPEND_EN, 0U},
		{IST8308_REG_CNTL1, IST8308_CNTL1_NSF_MASK, 0U},
		{IST8308_REG_CNTL4, IST8308_CNTL4_DR_MASK, IST8308_CNTL4_DR_200UT},
		{IST8308_REG_OSRCNTL, IST8308_OSRCNTL_MASK, IST8308_OSRCNTL_32},
		{IST8308_REG_CNTL2, IST8308_CNTL2_MODE_MASK, IST8308_CNTL2_MODE_50HZ},
	};
	const struct ist8308_config *config = dev->config;
	uint8_t value;
	int ret;

	if (!i2c_is_ready_dt(&config->i2c)) {
		return -ENODEV;
	}

	ret = i2c_reg_read_byte_dt(&config->i2c, IST8308_REG_WAI, &value);
	if (ret != 0) {
		LOG_ERR("Failed to read the chip ID: %d", ret);
		return ret;
	}

	if (value != IST8308_WAI_VALUE) {
		LOG_ERR("Invalid chip ID 0x%02x", value);
		return -ENODEV;
	}

	ret = i2c_reg_write_byte_dt(&config->i2c, IST8308_REG_CNTL3, IST8308_CNTL3_SRST);
	if (ret != 0) {
		return ret;
	}

	k_msleep(IST8308_RESET_TIME_MS);

	ret = i2c_reg_read_byte_dt(&config->i2c, IST8308_REG_CNTL3, &value);
	if (ret != 0) {
		return ret;
	}

	if ((value & IST8308_CNTL3_SRST) != 0U) {
		LOG_ERR("Soft reset not complete");
		return -EIO;
	}

	for (size_t i = 0; i < ARRAY_SIZE(settings); i++) {
		ret = i2c_reg_update_byte_dt(&config->i2c, settings[i].reg, settings[i].mask,
					     settings[i].value);
		if (ret != 0) {
			LOG_ERR("Failed to write register 0x%02x: %d", settings[i].reg, ret);
			return ret;
		}
	}

	return 0;
}

#define IST8308_DEFINE(inst)                                                                       \
	static struct ist8308_data ist8308_data_##inst;                                            \
	static const struct ist8308_config ist8308_config_##inst = {                               \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
	};                                                                                         \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, ist8308_init, NULL, &ist8308_data_##inst,               \
				     &ist8308_config_##inst, POST_KERNEL,                          \
				     CONFIG_SENSOR_INIT_PRIORITY, &ist8308_api);

DT_INST_FOREACH_STATUS_OKAY(IST8308_DEFINE)
