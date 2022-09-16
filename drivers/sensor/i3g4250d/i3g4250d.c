/* ST Microelectronics I3G4250D gyro driver
 *
 * Copyright (c) 2021 Jonathan Hahn
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/i3g4250d.pdf
 */

#define DT_DRV_COMPAT st_i3g4250d

#include <zephyr/init.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

#include "i3g4250d.h"

#define RAW_TO_MICRODEGREEPERSEC 8750

LOG_MODULE_REGISTER(i3g4250d, CONFIG_SENSOR_LOG_LEVEL);

static int i3g4250d_sample_fetch(const struct device *dev,
			enum sensor_channel chan)
{
	struct i3g4250d_data *i3g4250d = dev->data;
	int ret;
	uint8_t reg;
	int16_t buf[3] = { 0 };

	if ((chan != SENSOR_CHAN_ALL) && (chan != SENSOR_CHAN_GYRO_XYZ)) {
		return -ENOTSUP;
	}

	ret = i3g4250d_flag_data_ready_get(i3g4250d->ctx, &reg);
	if (ret < 0 || reg != 1) {
		return ret;
	}

	ret = i3g4250d_angular_rate_raw_get(i3g4250d->ctx, buf);
	if (ret < 0) {
		LOG_ERR("Failed to fetch raw data sample!");
		return ret;
	}

	memcpy(i3g4250d->angular_rate, buf, sizeof(i3g4250d->angular_rate));

	return 0;
}

static inline void i3g4250d_convert(struct sensor_value *val, int16_t raw_value)
{
	val->val1 = (int16_t)(raw_value * RAW_TO_MICRODEGREEPERSEC / 1000000LL);
	val->val2 = (int16_t)(raw_value * RAW_TO_MICRODEGREEPERSEC) % 1000000LL;
}

static void i3g4250d_channel_convert(enum sensor_channel chan,
					 uint16_t *raw_xyz,
					 struct sensor_value *val)
{
	uint8_t ofs_start, ofs_stop;

	switch (chan) {
	case SENSOR_CHAN_GYRO_X:
		ofs_start = ofs_stop = 0U;
		break;
	case SENSOR_CHAN_GYRO_Y:
		ofs_start = ofs_stop = 1U;
		break;
	case SENSOR_CHAN_GYRO_Z:
		ofs_start = ofs_stop = 2U;
		break;
	default:
		ofs_start = 0U;
		ofs_stop = 2U;
		break;
	}

	for (int i = ofs_start; i <= ofs_stop; i++) {
		i3g4250d_convert(val++, raw_xyz[i]);
	}
}

static int i3g4250d_channel_get(const struct device *dev,
				enum sensor_channel chan,
				struct sensor_value *val)
{
	struct i3g4250d_data *i3g4250d = dev->data;

	switch (chan) {
	case SENSOR_CHAN_GYRO_X:
		__fallthrough;
	case SENSOR_CHAN_GYRO_Y:
		__fallthrough;
	case SENSOR_CHAN_GYRO_Z:
		__fallthrough;
	case SENSOR_CHAN_GYRO_XYZ:
		i3g4250d_channel_convert(chan, i3g4250d->angular_rate, val);
		return 0;
	default:
		return -ENOTSUP;
	}
}

static i3g4250d_dr_t gyr_odr_to_reg(const struct sensor_value *val)
{
	double odr = sensor_value_to_double(val);
	i3g4250d_dr_t reg = I3G4250D_ODR_OFF;

	if ((odr > 0.0) && (odr < 100.0)) {
		reg = I3G4250D_ODR_SLEEP;
	} else if ((odr >= 100.0) && (odr < 200.0)) {
		reg = I3G4250D_ODR_100Hz;
	} else if ((odr >= 200.0) && (odr < 400.0)) {
		reg = I3G4250D_ODR_200Hz;
	} else if ((odr >= 400.0) && (odr < 800.0)) {
		reg = I3G4250D_ODR_400Hz;
	} else if (odr >= 800.0) {
		reg = I3G4250D_ODR_800Hz;
	}

	return reg;
}

static int i3g4250d_config_gyro(const struct device *dev,
				enum sensor_attribute attr,
				const struct sensor_value *val)
{
	struct i3g4250d_data *i3g4250d = dev->data;
	i3g4250d_dr_t dr_reg;

	switch (attr) {
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		dr_reg = gyr_odr_to_reg(val);
		return i3g4250d_data_rate_set(i3g4250d->ctx, dr_reg);
	default:
		LOG_ERR("Gyro attribute not supported");
		break;
	}

	return -ENOTSUP;
}

static int i3g4250d_attr_set(const struct device *dev, enum sensor_channel chan,
				 enum sensor_attribute attr,
				 const struct sensor_value *val)
{
	switch (chan) {
	case SENSOR_CHAN_GYRO_XYZ:
		return i3g4250d_config_gyro(dev, attr, val);
	default:
		LOG_ERR("attr_set() not supported on this channel %d.", chan);
		break;
	}

	return -ENOTSUP;
}

static const struct sensor_driver_api i3g4250d_driver_api = {
	.attr_set = i3g4250d_attr_set,
	.sample_fetch = i3g4250d_sample_fetch,
	.channel_get = i3g4250d_channel_get,
};

static int i3g4250d_init(const struct device *dev)
{
	struct i3g4250d_data *i3g4250d = dev->data;
	uint8_t wai;
	int ret = 0;

	ret = i3g4250d_spi_init(dev);
	if (ret != 0) {
		return ret;
	}

	ret = i3g4250d_device_id_get(i3g4250d->ctx, &wai);
	if (ret != 0) {
		return ret;
	}

	if (wai != I3G4250D_ID) {
		LOG_ERR("Invalid chip ID: %02x", wai);
		return -EIO;
	}

	/* Configure filtering chain -  Gyroscope - High Pass */
	ret = i3g4250d_filter_path_set(i3g4250d->ctx, I3G4250D_LPF1_HP_ON_OUT);
	if (ret != 0) {
		LOG_ERR("Failed setting filter path");
		return ret;
	}

	ret = i3g4250d_hp_bandwidth_set(i3g4250d->ctx, I3G4250D_HP_LEVEL_3);
	if (ret != 0) {
		LOG_ERR("Failed setting high pass");
		return ret;
	}

	/* Set Output data rate */
	ret = i3g4250d_data_rate_set(i3g4250d->ctx, I3G4250D_ODR_100Hz);
	if (ret != 0) {
		LOG_ERR("Failed setting data rate");
		return ret;
	}

	return 0;
}

#define I3G4250D_DEVICE_INIT(inst)                                           \
	static struct i3g4250d_data i3g4250d_data_##inst;                        \
	static const struct i3g4250d_device_config i3g4250d_config_##inst = {    \
		.spi = SPI_DT_SPEC_INST_GET(inst,                                    \
					SPI_OP_MODE_MASTER | SPI_MODE_CPOL |                     \
					SPI_MODE_CPHA | SPI_WORD_SET(8) | SPI_LINES_SINGLE,      \
					0),                                                      \
	};                                                                       \
	DEVICE_DT_INST_DEFINE(inst,                                              \
				i3g4250d_init,                                               \
				NULL,                                                        \
				&i3g4250d_data_##inst,                                       \
				&i3g4250d_config_##inst,                                     \
				POST_KERNEL,                                                 \
				CONFIG_SENSOR_INIT_PRIORITY,                                 \
				&i3g4250d_driver_api);

DT_INST_FOREACH_STATUS_OKAY(I3G4250D_DEVICE_INIT)
