/* ST Microelectronics LIS2DS12 3-axis accelerometer driver
 *
 * Copyright (c) 2019 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/lis2ds12.pdf
 */

#define DT_DRV_COMPAT st_lis2ds12

#include <drivers/sensor.h>
#include <kernel.h>
#include <device.h>
#include <init.h>
#include <string.h>
#include <sys/byteorder.h>
#include <sys/__assert.h>
#include <logging/log.h>

#include "lis2ds12.h"

LOG_MODULE_REGISTER(LIS2DS12, CONFIG_SENSOR_LOG_LEVEL);

static struct lis2ds12_data lis2ds12_data;

static struct lis2ds12_config lis2ds12_config = {
	.comm_master_dev_name = DT_INST_BUS_LABEL(0),
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
	.bus_init = lis2ds12_spi_init,
#elif DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
	.bus_init = lis2ds12_i2c_init,
#else
#error "BUS MACRO NOT DEFINED IN DTS"
#endif
#ifdef CONFIG_LIS2DS12_TRIGGER
	.irq_port	= DT_INST_GPIO_LABEL(0, irq_gpios),
	.irq_pin	= DT_INST_GPIO_PIN(0, irq_gpios),
	.irq_flags	= DT_INST_GPIO_FLAGS(0, irq_gpios),
#endif
};

#if defined(LIS2DS12_ODR_RUNTIME)
static const uint16_t lis2ds12_hr_odr_map[] = {0, 12, 25, 50, 100, 200, 400, 800};

static int lis2ds12_freq_to_odr_val(uint16_t freq)
{
	size_t i;

	for (i = 0; i < ARRAY_SIZE(lis2ds12_hr_odr_map); i++) {
		if (freq == lis2ds12_hr_odr_map[i]) {
			return i;
		}
	}

	return -EINVAL;
}

static int lis2ds12_accel_odr_set(const struct device *dev, uint16_t freq)
{
	struct lis2ds12_data *data = dev->data;
	int odr;

	odr = lis2ds12_freq_to_odr_val(freq);
	if (odr < 0) {
		return odr;
	}

	if (data->hw_tf->update_reg(data,
				    LIS2DS12_REG_CTRL1,
				    LIS2DS12_MASK_CTRL1_ODR,
				    odr << LIS2DS12_SHIFT_CTRL1_ODR) < 0) {
		LOG_DBG("failed to set accelerometer sampling rate");
		return -EIO;
	}

	return 0;
}
#endif

#ifdef LIS2DS12_FS_RUNTIME
static const uint16_t lis2ds12_accel_fs_map[] = {2, 16, 4, 8};
static const uint16_t lis2ds12_accel_fs_sens[] = {1, 8, 2, 4};

static int lis2ds12_accel_range_to_fs_val(int32_t range)
{
	size_t i;

	for (i = 0; i < ARRAY_SIZE(lis2ds12_accel_fs_map); i++) {
		if (range == lis2ds12_accel_fs_map[i]) {
			return i;
		}
	}

	return -EINVAL;
}

static int lis2ds12_accel_range_set(const struct device *dev, int32_t range)
{
	int fs;
	struct lis2ds12_data *data = dev->data;

	fs = lis2ds12_accel_range_to_fs_val(range);
	if (fs < 0) {
		return fs;
	}

	if (data->hw_tf->update_reg(data,
				    LIS2DS12_REG_CTRL1,
				    LIS2DS12_MASK_CTRL1_FS,
				    fs << LIS2DS12_SHIFT_CTRL1_FS) < 0) {
		LOG_DBG("failed to set accelerometer full-scale");
		return -EIO;
	}

	data->gain = (float)(lis2ds12_accel_fs_sens[fs] * GAIN_XL);
	return 0;
}
#endif

static int lis2ds12_accel_config(const struct device *dev,
				 enum sensor_channel chan,
				 enum sensor_attribute attr,
				 const struct sensor_value *val)
{
	switch (attr) {
#ifdef LIS2DS12_FS_RUNTIME
	case SENSOR_ATTR_FULL_SCALE:
		return lis2ds12_accel_range_set(dev, sensor_ms2_to_g(val));
#endif
#ifdef LIS2DS12_ODR_RUNTIME
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		return lis2ds12_accel_odr_set(dev, val->val1);
#endif
	default:
		LOG_DBG("Accel attribute not supported.");
		return -ENOTSUP;
	}

	return 0;
}

static int lis2ds12_attr_set(const struct device *dev,
			     enum sensor_channel chan,
			     enum sensor_attribute attr,
			     const struct sensor_value *val)
{
	switch (chan) {
	case SENSOR_CHAN_ACCEL_XYZ:
		return lis2ds12_accel_config(dev, chan, attr, val);
	default:
		LOG_WRN("attr_set() not supported on this channel.");
		return -ENOTSUP;
	}

	return 0;
}

static int lis2ds12_sample_fetch_accel(const struct device *dev)
{
	struct lis2ds12_data *data = dev->data;
	uint8_t buf[6];

	if (data->hw_tf->read_data(data, LIS2DS12_REG_OUTX_L,
				   buf, sizeof(buf)) < 0) {
		LOG_DBG("failed to read sample");
		return -EIO;
	}

	data->sample_x = (int16_t)((uint16_t)(buf[0]) | ((uint16_t)(buf[1]) << 8));
	data->sample_y = (int16_t)((uint16_t)(buf[2]) | ((uint16_t)(buf[3]) << 8));
	data->sample_z = (int16_t)((uint16_t)(buf[4]) | ((uint16_t)(buf[5]) << 8));

	return 0;
}

static int lis2ds12_sample_fetch(const struct device *dev,
				 enum sensor_channel chan)
{
	switch (chan) {
	case SENSOR_CHAN_ACCEL_XYZ:
		lis2ds12_sample_fetch_accel(dev);
		break;
#if defined(CONFIG_LIS2DS12_ENABLE_TEMP)
	case SENSOR_CHAN_DIE_TEMP:
		/* ToDo:
		lis2ds12_sample_fetch_temp(dev)
		*/
		break;
#endif
	case SENSOR_CHAN_ALL:
		lis2ds12_sample_fetch_accel(dev);
#if defined(CONFIG_LIS2DS12_ENABLE_TEMP)
		/* ToDo:
		lis2ds12_sample_fetch_temp(dev)
		*/
#endif
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static inline void lis2ds12_convert(struct sensor_value *val, int raw_val,
				    float gain)
{
	int64_t dval;

	/* Gain is in mg/LSB */
	/* Convert to m/s^2 */
	dval = ((int64_t)raw_val * gain * SENSOR_G) / 1000;
	val->val1 = dval / 1000000LL;
	val->val2 = dval % 1000000LL;
}

static inline int lis2ds12_get_channel(enum sensor_channel chan,
					     struct sensor_value *val,
					     struct lis2ds12_data *data,
					     float gain)
{
	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
		lis2ds12_convert(val, data->sample_x, gain);
		break;
	case SENSOR_CHAN_ACCEL_Y:
		lis2ds12_convert(val, data->sample_y, gain);
		break;
	case SENSOR_CHAN_ACCEL_Z:
		lis2ds12_convert(val, data->sample_z, gain);
		break;
	case SENSOR_CHAN_ACCEL_XYZ:
		lis2ds12_convert(val, data->sample_x, gain);
		lis2ds12_convert(val + 1, data->sample_y, gain);
		lis2ds12_convert(val + 2, data->sample_z, gain);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int lis2ds12_channel_get(const struct device *dev,
				enum sensor_channel chan,
				struct sensor_value *val)
{
	struct lis2ds12_data *data = dev->data;

	return lis2ds12_get_channel(chan, val, data, data->gain);
}

static const struct sensor_driver_api lis2ds12_api_funcs = {
	.attr_set = lis2ds12_attr_set,
#if defined(CONFIG_LIS2DS12_TRIGGER)
	.trigger_set = lis2ds12_trigger_set,
#endif
	.sample_fetch = lis2ds12_sample_fetch,
	.channel_get = lis2ds12_channel_get,
};

static int lis2ds12_init(const struct device *dev)
{
	const struct lis2ds12_config * const config = dev->config;
	struct lis2ds12_data *data = dev->data;
	uint8_t chip_id;

	data->comm_master = device_get_binding(config->comm_master_dev_name);
	if (!data->comm_master) {
		LOG_DBG("master not found: %s",
			    config->comm_master_dev_name);
		return -EINVAL;
	}

	config->bus_init(dev);

	/* s/w reset the sensor */
	if (data->hw_tf->write_reg(data,
				    LIS2DS12_REG_CTRL2,
				    LIS2DS12_SOFT_RESET) < 0) {
		LOG_DBG("s/w reset fail");
		return -EIO;
	}

	if (data->hw_tf->read_reg(data, LIS2DS12_REG_WHO_AM_I, &chip_id) < 0) {
		LOG_DBG("failed reading chip id");
		return -EIO;
	}

	if (chip_id != LIS2DS12_VAL_WHO_AM_I) {
		LOG_DBG("invalid chip id 0x%x", chip_id);
		return -EIO;
	}

	LOG_DBG("chip id 0x%x", chip_id);

#ifdef CONFIG_LIS2DS12_TRIGGER
	if (lis2ds12_trigger_init(dev) < 0) {
		LOG_ERR("Failed to initialize triggers.");
		return -EIO;
	}
#endif

	/* set sensor default odr */
	if (data->hw_tf->update_reg(data,
				    LIS2DS12_REG_CTRL1,
				    LIS2DS12_MASK_CTRL1_ODR,
				    LIS2DS12_DEFAULT_ODR) < 0) {
		LOG_DBG("failed setting odr");
		return -EIO;
	}

	/* set sensor default scale */
	if (data->hw_tf->update_reg(data,
				    LIS2DS12_REG_CTRL1,
				    LIS2DS12_MASK_CTRL1_FS,
				    LIS2DS12_DEFAULT_FS) < 0) {
		LOG_DBG("failed setting scale");
		return -EIO;
	}
	data->gain = LIS2DS12_DEFAULT_GAIN;

	return 0;
}

DEVICE_DT_INST_DEFINE(0, lis2ds12_init, device_pm_control_nop,
		    &lis2ds12_data, &lis2ds12_config, POST_KERNEL,
		    CONFIG_SENSOR_INIT_PRIORITY, &lis2ds12_api_funcs);
