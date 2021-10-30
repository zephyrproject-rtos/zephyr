/* Kionix KX022 3-axis accelerometer driver
 *
 * Copyright (c) 2021 G-Technologies Sdn. Bhd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT kionix_kx022

#include <drivers/sensor.h>
#include <kernel.h>
#include <device.h>
#include <init.h>
#include <string.h>
#include <sys/byteorder.h>
#include <sys/__assert.h>
#include <logging/log.h>
#include <drivers/i2c.h>

#include "kx022.h"

LOG_MODULE_REGISTER(KX022, CONFIG_SENSOR_LOG_LEVEL);
static struct kx022_data dev_data;
static struct kx022_config dev_config = {
	.comm_master_dev_name = DT_INST_BUS_LABEL(0),
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
#error "KX022 SPI NOT IMPLEMENTED"
#elif DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
	.bus_init = kx022_i2c_init,
#else
#error "BUS MACRO NOT DEFINED IN DTS"
#endif

#ifdef CONFIG_KX022_TRIGGER
	.irq_port = DT_INST_GPIO_LABEL(0, int_gpios),
	.irq_pin = DT_INST_GPIO_PIN(0, int_gpios),
	.irq_flags = DT_INST_GPIO_FLAGS(0, int_gpios),
#endif

};

#define kx022_delay(msec) k_msleep(((uint16_t)msec) * 1000 / CONFIG_KX022_ODR + 1);

#if defined(KX022_ODR_RUNTIME)
static const uint16_t kx022_hr_odr_map[] = { 12, 25, 50, 100, 200, 400, 800, 1600 };

static int kx022_freq_to_odr_val(uint16_t freq)
{
	size_t i;

	for (i = 0; i < ARRAY_SIZE(kx022_hr_odr_map); i++) {
		if (freq == kx022_hr_odr_map[i]) {
			return i;
		}
	}

	return -EINVAL;
}

static int kx022_accel_odr_set(const struct device *dev, uint16_t freq)
{
	struct kx022_data *data = dev->data;
	int odr;

	odr = kx022_freq_to_odr_val(freq);
	if (odr < 0) {
		return odr;
	}

	kx022_mode(KX022_STANDY_MODE);

	if (data->hw_tf->update_reg(data, KX022_REG_ODCNTL, KX022_MASK_ODCNTL_OSA, (uint8_t)odr) <0) {
		LOG_DBG("Failed to set KX022 sampling rate");
		return -EIO;
	}

	kx022_mode(KX022_OPERATING_MODE);
	return 0;
}
#endif /* defined(KX022_ODR_RUNTIME) */

#ifdef KX022_FS_RUNTIME
static const uint16_t kx022_accel_fs_map[] = { 2, 4, 8 };
static const uint16_t kx022_accel_fs_sens[] = { 1, 2, 4 };

static int kx022_accel_range_to_fs_val(int32_t range)
{
	size_t i;

	for (i = 0; i < ARRAY_SIZE(kx022_accel_fs_map); i++) {
		if (range == kx022_accel_fs_map[i]) {
			return i;
		}
	}

	return -EINVAL;
}

static int kx022_accel_range_set(const struct device *dev, int32_t range)
{
	int fs;
	struct kx022_data *data = dev->data;

	fs = kx022_accel_range_to_fs_val(range);
	if (fs < 0) {
		return fs;
	}

	kx022_mode(KX022_STANDY_MODE);

	if (data->hw_tf->update_reg(data, KX022_REG_CNTL1, KX022_MASK_CNTL1_GSEL, fs) < 0) {
		LOG_DBG("failed to set accelerometer full-scale");
		return -EIO;
	}

	kx022_mode(KX022_OPERATING_MODE);

	data->gain = (float)(kx022_accel_fs_sens[fs] * GAIN_XL);

	return 0;
}
#endif /* KX022_FS_RUNTIME */

#ifdef KX022_RES_RUNTIME
static const uint16_t kx022_accel_res[] = { 8, 16 };

static int kx022_accel_res_set(const struct device *dev, uint16_t res)
{
	struct kx022_data *data = dev->data;
	int val;

	for (val = 0; val < ARRAY_SIZE(kx022_accel_res); val++) {
		if (res == kx022_accel_res[val]) {
			goto SET_RES;
		}
	}

	LOG_DBG("KX022 resolution invalid");
	return -EIO;

SET_RES:

	kx022_mode(KX022_STANDY_MODE);

	if (data->hw_tf->update_reg(data, KX022_REG_CNTL1, KX022_MASK_CNTL1_RES, (uint8_t)val) < 0) {
	 	LOG_DBG("Failed to set KX022 sampling rate");
		return -EIO;
	}

	kx022_mode(KX022_OPERATING_MODE);
	return 0;
}
#endif /* KX022_RES_RUNTIME */

static int kx022_accel_config(const struct device *dev, enum sensor_channel chan,
			      enum sensor_attribute attr, const struct sensor_value *val)
{
	switch (attr) {
#ifdef KX022_FS_RUNTIME
	case SENSOR_ATTR_FULL_SCALE:
		return kx022_accel_range_set(dev, sensor_g_to_ms2(val));
#endif /* KX022_FS_RUNTIME */
#ifdef KX022_ODR_RUNTIME
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		return kx022_accel_odr_set(dev, val->val1);
#endif /* KX022_ODR_RUNTIME */
#ifdef KX022_RES_RUNTIME
	case KX022_ATTR_RESOLUTION:
		return kx022_accel_res_set(dev, val->val1);
#endif /* KX022_RES_RUNTIME */
	default:
		LOG_DBG("Accel attribute not supported.");
		return -ENOTSUP;
	}

	return 0;
}

/************************************************************
 * info: kx022_mode use to set pc1 in standby or operating mode
 * input: 0 - standby
 *        1 - operating
 * **********************************************************/

void kx022_mode(const struct device *dev, bool mode)
{
	struct kx022_data *data = dev->data;

	data->hw_tf->update_reg(data, KX022_REG_CNTL1, KX022_MASK_CNTL1_PC1,
				(mode << KX022_CNTL1_PC1_SHIFF));
}

void kx022_motion_detect_config(const struct device *dev, enum sensor_attribute attr,
				const struct sensor_value *val)
{
	struct kx022_data *data = dev->data;
	uint16_t rd_val;

	rd_val = (uint16_t)val->val1;
	switch (attr) {
	case SENSOR_ATTR_WUFF_TH:
		data->hw_tf->write_reg(data, KX022_REG_ATH, rd_val);
		break;
	case SENSOR_ATTR_WUFF_DUR:
		if (rd_val == 0) {
			data->hw_tf->write_reg(data, KX022_REG_WUFC, KX022_WUFC_DUR);
		} else {
			data->hw_tf->write_reg(data, KX022_REG_WUFC, rd_val);
		}
		break;
	default:
		LOG_DBG("Accel attribute not supported.");
		break;
	}
}

void kx022_tilt_position_config(const struct device *dev, enum sensor_attribute attr,
				const struct sensor_value *val)
{
	struct kx022_data *data = dev->data;
	uint16_t rd_val;

	rd_val = (uint16_t)val->val1;

	switch (attr) {
	case SENSOR_ATTR_SLOPE_TH:
		data->hw_tf->write_reg(data, KX022_REG_TILT_ANGLE_LL, rd_val);
		break;

	case SENSOR_ATTR_SLOPE_DUR:
		if (rd_val == 0) {
			data->hw_tf->write_reg(data, KX022_REG_TILT_TIMER,
					       KX022_TILT_TIMES_DURATION);
		} else {
			data->hw_tf->write_reg(data, KX022_REG_TILT_TIMER, rd_val);
		}
		break;
	default:
		LOG_DBG("Accel attribute not supported.");
		break;
	}
}

static int kx022_attr_set(const struct device *dev, enum sensor_channel chan,
			  enum sensor_attribute attr, const struct sensor_value *val)
{
	kx022_mode(dev, KX022_STANDY_MODE);

	switch (chan) {
	case SENSOR_CHAN_ACCEL_XYZ:
		return kx022_accel_config(dev, chan, attr, val);
		kx022_mode(dev, KX022_OPERATING_MODE);
	case SENSOR_CHAN_FREE_FALL:
		kx022_motion_detect_config(dev, attr, val);
		kx022_mode(dev, KX022_OPERATING_MODE);
		break;
	case SENSOR_CHAN_NEAR_FAR:
		kx022_tilt_position_config(dev, attr, val);
		kx022_mode(dev, KX022_OPERATING_MODE);
		break;
	default:
		LOG_WRN("attr_set() not supported on this channel.");
		return -ENOTSUP;
	}

	return 0;
}

static int kx022_sample_fetch_accel(const struct device *dev, uint8_t reg_addr)
{
	struct kx022_data *data = dev->data;
	uint8_t buf[2];
	uint16_t val;

	if (data->hw_tf->read_data(data, reg_addr, buf, sizeof(buf)) < 0) {
		LOG_DBG("failed to read sample");
		return -EIO;
	}

	val = (int16_t)((uint16_t)(buf[0]) | ((uint16_t)(buf[1]) << 8));

	switch (reg_addr) {
	case KX022_REG_XOUT_L:
		data->sample_x = val;
		break;
	case KX022_REG_YOUT_L:
		data->sample_y = val;
		break;
	case KX022_REG_ZOUT_L:
		data->sample_z = val;
		break;
	default:
		LOG_ERR("Invalid register address");
		return -EIO;
	}

	return 0;
}

#define kx022_sample_fetch_accel_x(dev) kx022_sample_fetch_accel(dev, KX022_REG_XOUT_L)
#define kx022_sample_fetch_accel_y(dev) kx022_sample_fetch_accel(dev, KX022_REG_YOUT_L)
#define kx022_sample_fetch_accel_z(dev) kx022_sample_fetch_accel(dev, KX022_REG_ZOUT_L)

static int kx022_sample_fetch_accel_xyz(const struct device *dev)
{
	struct kx022_data *data = dev->data;
	uint8_t buf[6];

	if (data->hw_tf->read_data(data, KX022_REG_XOUT_L, buf, sizeof(buf)) < 0) {
		LOG_DBG("failed to read sample");
		return -EIO;
	}

	data->sample_x = (int16_t)((uint16_t)(buf[0]) | ((uint16_t)(buf[1]) << 8));
	data->sample_y = (int16_t)((uint16_t)(buf[2]) | ((uint16_t)(buf[3]) << 8));
	data->sample_z = (int16_t)((uint16_t)(buf[4]) | ((uint16_t)(buf[5]) << 8));
	return 0;
}

static int kx022_tilt_pos(const struct device *dev)
{
	struct kx022_data *data = dev->data;
	uint8_t rd_data[2];

	data->hw_tf->read_reg(data, KX022_REG_TSCP, &rd_data[0]);
	data->hw_tf->read_reg(data, KX022_REG_TSPP, &rd_data[1]);

	data->sample_tscp = rd_data[0];
	data->sample_tspp = rd_data[1];

	return 0;
}

static int kx022_motion_direction(const struct device *dev)
{
	struct kx022_data *data = dev->data;
	uint8_t rd_data;

	data->hw_tf->read_reg(data, KX022_REG_INS3, &rd_data);

	data->sample_motion_dir = rd_data;

	return 0;
}

static int kx022_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
		kx022_sample_fetch_accel_x(dev);
		break;
	case SENSOR_CHAN_ACCEL_Y:
		kx022_sample_fetch_accel_y(dev);
		break;
	case SENSOR_CHAN_ACCEL_Z:
		kx022_sample_fetch_accel_z(dev);
		break;
	case SENSOR_CHAN_ACCEL_XYZ:
		kx022_sample_fetch_accel_xyz(dev);
		break;
	case SENSOR_CHAN_FREE_FALL:
		kx022_motion_direction(dev);
		break;
	case SENSOR_CHAN_ALL:
		kx022_sample_fetch_accel_xyz(dev);
		kx022_tilt_pos(dev);
		kx022_motion_direction(dev);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static inline void kx022_convert(struct sensor_value *val, int raw_val, float gain)
{
	int64_t dval;

	/* Gain is in mg/LSB */
	/* Convert to m/s^2 */
	dval = ((int64_t)raw_val * gain * SENSOR_G) / 1000;
	val->val1 = dval / 1000000LL;
	val->val2 = dval % 1000000LL;
}
static inline void kx022_tilt_pos_get(struct sensor_value *val, int raw_val)
{
	uint16_t dval;

	dval = (int16_t)raw_val;
	val->val1 = dval;
}

static inline void kx022_motion_dir_get(struct sensor_value *val, int raw_val)
{
	uint16_t dval;

	dval = (int16_t)raw_val;
	val->val1 = dval;
}
static inline int kx022_get_channel(enum sensor_channel chan, struct sensor_value *val,
				    struct kx022_data *data, float gain)
{
	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
		kx022_convert(val, data->sample_x, gain);
		break;
	case SENSOR_CHAN_ACCEL_Y:
		kx022_convert(val, data->sample_y, gain);
		break;
	case SENSOR_CHAN_ACCEL_Z:
		kx022_convert(val, data->sample_z, gain);
		break;
	case SENSOR_CHAN_ACCEL_XYZ:
		kx022_convert(val, data->sample_x, gain);
		kx022_convert(val + 1, data->sample_y, gain);
		kx022_convert(val + 2, data->sample_z, gain);
		break;
	case SENSOR_CHAN_FREE_FALL:
		kx022_motion_dir_get(val, data->sample_motion_dir);
		break;
	case SENSOR_CHAN_NEAR_FAR:
		kx022_tilt_pos_get(val, data->sample_tspp);
		kx022_tilt_pos_get(val + 1, data->sample_tscp);
		break;
	case SENSOR_CHAN_ALL:
		kx022_convert(val, data->sample_x, gain);
		kx022_convert(val + 1, data->sample_y, gain);
		kx022_convert(val + 2, data->sample_z, gain);
		kx022_tilt_pos_get(val + 3, data->sample_tspp);
		kx022_tilt_pos_get(val + 4, data->sample_tscp);
		kx022_motion_dir_get(val + 5, data->sample_motion_dir);

		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int kx022_channel_get(const struct device *dev, enum sensor_channel chan,
			     struct sensor_value *val)
{
	struct kx022_data *data = dev->data;

	return kx022_get_channel(chan, val, data, data->gain);
}

static const struct sensor_driver_api kx022_api_funcs = {
	.attr_set = kx022_attr_set,
#ifdef CONFIG_KX022_TRIGGER
	.trigger_set = kx022_trigger_set,
#endif
	.sample_fetch = kx022_sample_fetch,
	.channel_get = kx022_channel_get,
};

static int kx022_init(const struct device *dev)
{
	const struct kx022_config *const config = dev->config;
	struct kx022_data *data = dev->data;
	uint8_t chip_id;

	data->comm_master = device_get_binding(config->comm_master_dev_name);
	if (!data->comm_master) {
		LOG_DBG("master not found: %s", config->comm_master_dev_name);
		return -EINVAL;
	}
	k_msleep(2000);
	config->bus_init(dev);

	if (data->hw_tf->read_reg(data, KX022_REG_WHO_AM_I, &chip_id) < 0) {
		LOG_DBG("Failed reading chip id");
	}

	if (chip_id != KX022_VAL_WHO_AM_I) {
		LOG_DBG("Invalid chip id 0x%x", chip_id);
	}

	LOG_DBG("Chip id 0x%x", chip_id);

	kx022_mode(dev, KX022_STANDY_MODE);
	/* Reset CNTL1 */
	if (data->hw_tf->write_reg(data, KX022_REG_CNTL1, KX022_VAL_CNTL1_RESET) < 0) {
		LOG_DBG("CNTL1 reset fail");
	}
	/* s/w reset the sensor */
	if (data->hw_tf->update_reg(data, KX022_REG_CNTL2, KX022_MASK_CNTL2_SRST, 1) < 0) {
		LOG_DBG("s/w reset fail");
	}
	/* Make sure the KX022 is stopped before we configure */

	if (data->hw_tf->write_reg(data, KX022_REG_CNTL1, KX022_DEFAULT_CNTL1) < 0) {
		LOG_DBG("Failed CNTL1");
	}
	/* Set KX022 default odr */
	if (data->hw_tf->update_reg(data, KX022_REG_ODCNTL, KX022_MASK_ODCNTL_OSA,
				    KX022_DEFAULT_ODR) < 0) {
		LOG_DBG("Failed setting odr");
	}

#ifdef CONFIG_KX022_TRIGGER
	if (kx022_trigger_init(dev) < 0) {
		LOG_ERR("Failed to initialize triggers.");
	}
#endif

	if (data->hw_tf->write_reg(data, KX022_REG_CNTL3, KX022_DEFAULT_CNTL3) < 0) {
		LOG_DBG("Failed CNTL3");
	}

	if (data->hw_tf->write_reg(data, KX022_REG_TILT_TIMER, KX022_DEFAULT_TILT_TIMER) < 0) {
		LOG_DBG("Failed TILT");
	}

	/* Set Kx022 to Operating Mode */
	kx022_mode(dev, KX022_OPERATING_MODE);

	k_msleep(2000);

	data->gain = KX022_DEFAULT_GAIN;

	return 0;
}

DEVICE_DT_INST_DEFINE(0, kx022_init, NULL, &dev_data, &dev_config, POST_KERNEL,
		      CONFIG_SENSOR_INIT_PRIORITY, &kx022_api_funcs);
