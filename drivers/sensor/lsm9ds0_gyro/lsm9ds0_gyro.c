/* lsm9ds0_gyro.c - Driver for LSM9DS0 gyroscope sensor */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sensor.h>
#include <kernel.h>
#include <device.h>
#include <init.h>
#include <misc/byteorder.h>
#include <misc/__assert.h>

#include <gpio.h>

#include "lsm9ds0_gyro.h"

static inline int lsm9ds0_gyro_power_ctrl(struct device *dev, int power,
					  int x_en, int y_en, int z_en)
{
	struct lsm9ds0_gyro_data *data = dev->driver_data;
	const struct lsm9ds0_gyro_config *config = dev->config->config_info;
	uint8_t state = (power << LSM9DS0_GYRO_SHIFT_CTRL_REG1_G_PD) |
			(x_en << LSM9DS0_GYRO_SHIFT_CTRL_REG1_G_XEN) |
			(y_en << LSM9DS0_GYRO_SHIFT_CTRL_REG1_G_YEN) |
			(z_en << LSM9DS0_GYRO_SHIFT_CTRL_REG1_G_ZEN);

	return i2c_reg_update_byte(data->i2c_master, config->i2c_slave_addr,
				   LSM9DS0_GYRO_REG_CTRL_REG1_G,
				   LSM9DS0_GYRO_MASK_CTRL_REG1_G_PD |
				   LSM9DS0_GYRO_MASK_CTRL_REG1_G_XEN |
				   LSM9DS0_GYRO_MASK_CTRL_REG1_G_YEN |
				   LSM9DS0_GYRO_MASK_CTRL_REG1_G_ZEN,
				   state);
}

static int lsm9ds0_gyro_set_fs_raw(struct device *dev, uint8_t fs)
{
	struct lsm9ds0_gyro_data *data = dev->driver_data;
	const struct lsm9ds0_gyro_config *config = dev->config->config_info;

	if (i2c_reg_update_byte(data->i2c_master, config->i2c_slave_addr,
				LSM9DS0_GYRO_REG_CTRL_REG4_G,
				LSM9DS0_GYRO_MASK_CTRL_REG4_G_FS,
				fs << LSM9DS0_GYRO_SHIFT_CTRL_REG4_G_FS) < 0) {
		return -EIO;
	}

#if defined(CONFIG_LSM9DS0_GYRO_FULLSCALE_RUNTIME)
	data->fs = fs;
#endif

	return 0;
}

#if defined(CONFIG_LSM9DS0_GYRO_FULLSCALE_RUNTIME)
static const struct {
	int fs;
	uint8_t reg_val;
} lsm9ds0_gyro_fs_table[] = { {245, 0},
			      {500, 1},
			      {2000, 2} };

static int lsm9ds0_gyro_set_fs(struct device *dev, int fs)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(lsm9ds0_gyro_fs_table); ++i) {
		if (fs <= lsm9ds0_gyro_fs_table[i].fs) {
			return lsm9ds0_gyro_set_fs_raw(dev, lsm9ds0_gyro_fs_table[i].reg_val);
		}
	}

	return -ENOTSUP;
}
#endif

static inline int lsm9ds0_gyro_set_odr_raw(struct device *dev, uint8_t odr)
{
	struct lsm9ds0_gyro_data *data = dev->driver_data;
	const struct lsm9ds0_gyro_config *config = dev->config->config_info;

	return i2c_reg_update_byte(data->i2c_master, config->i2c_slave_addr,
				   LSM9DS0_GYRO_REG_CTRL_REG1_G,
				   LSM9DS0_GYRO_MASK_CTRL_REG1_G_DR,
				   odr << LSM9DS0_GYRO_SHIFT_CTRL_REG1_G_BW);
}

#if defined(CONFIG_LSM9DS0_GYRO_SAMPLING_RATE_RUNTIME)
static const struct {
	int freq;
	uint8_t reg_val;
} lsm9ds0_gyro_samp_freq_table[] = { {95, 0},
				     {190, 1},
				     {380, 2},
				     {760, 3} };

static int lsm9ds0_gyro_set_odr(struct device *dev, int odr)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(lsm9ds0_gyro_samp_freq_table); ++i) {
		if (odr <= lsm9ds0_gyro_samp_freq_table[i].freq) {
			return lsm9ds0_gyro_set_odr_raw(dev,
							lsm9ds0_gyro_samp_freq_table[i].
							reg_val);
		}
	}

	return -ENOTSUP;
}
#endif

static int lsm9ds0_gyro_sample_fetch(struct device *dev,
				     enum sensor_channel chan)
{
	struct lsm9ds0_gyro_data *data = dev->driver_data;
	const struct lsm9ds0_gyro_config *config = dev->config->config_info;
	uint8_t x_l, x_h, y_l, y_h, z_l, z_h;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL ||
			chan == SENSOR_CHAN_GYRO_XYZ);

	if (i2c_reg_read_byte(data->i2c_master, config->i2c_slave_addr,
			      LSM9DS0_GYRO_REG_OUT_X_L_G, &x_l) < 0 ||
	    i2c_reg_read_byte(data->i2c_master, config->i2c_slave_addr,
			      LSM9DS0_GYRO_REG_OUT_X_H_G, &x_h) < 0 ||
	    i2c_reg_read_byte(data->i2c_master, config->i2c_slave_addr,
			      LSM9DS0_GYRO_REG_OUT_Y_L_G, &y_l) < 0 ||
	    i2c_reg_read_byte(data->i2c_master, config->i2c_slave_addr,
			      LSM9DS0_GYRO_REG_OUT_Y_H_G, &y_h) < 0 ||
	    i2c_reg_read_byte(data->i2c_master, config->i2c_slave_addr,
			      LSM9DS0_GYRO_REG_OUT_Z_L_G, &z_l) < 0 ||
	    i2c_reg_read_byte(data->i2c_master, config->i2c_slave_addr,
			      LSM9DS0_GYRO_REG_OUT_Z_H_G, &z_h) < 0) {
		SYS_LOG_DBG("failed to read sample");
		return -EIO;
	}

	data->sample_x = (int16_t)((uint16_t)(x_l) | ((uint16_t)(x_h) << 8));
	data->sample_y = (int16_t)((uint16_t)(y_l) | ((uint16_t)(y_h) << 8));
	data->sample_z = (int16_t)((uint16_t)(z_l) | ((uint16_t)(z_h) << 8));

#if defined(CONFIG_LSM9DS0_GYRO_FULLSCALE_RUNTIME)
	data->sample_fs = data->fs;
#endif

	return 0;
}

static inline void lsm9ds0_gyro_convert(struct sensor_value *val, int raw_val,
					float numerator)
{
	double dval;

	dval = (double)(raw_val) * numerator / 1000.0 * DEG2RAD;
	val->val1 = (int32_t)dval;
	val->val2 = ((int32_t)(dval * 1000000)) % 1000000;
}

static inline int lsm9ds0_gyro_get_channel(enum sensor_channel chan,
					   struct sensor_value *val,
					   struct lsm9ds0_gyro_data *data,
					   float numerator)
{
	switch (chan) {
	case SENSOR_CHAN_GYRO_X:
		lsm9ds0_gyro_convert(val, data->sample_x, numerator);
		break;
	case SENSOR_CHAN_GYRO_Y:
		lsm9ds0_gyro_convert(val, data->sample_y, numerator);
		break;
	case SENSOR_CHAN_GYRO_Z:
		lsm9ds0_gyro_convert(val, data->sample_z, numerator);
		break;
	case SENSOR_CHAN_GYRO_XYZ:
		lsm9ds0_gyro_convert(val, data->sample_x, numerator);
		lsm9ds0_gyro_convert(val + 1, data->sample_y, numerator);
		lsm9ds0_gyro_convert(val + 2, data->sample_z, numerator);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int lsm9ds0_gyro_channel_get(struct device *dev,
				    enum sensor_channel chan,
				    struct sensor_value *val)
{
	struct lsm9ds0_gyro_data *data = dev->driver_data;

#if defined(CONFIG_LSM9DS0_GYRO_FULLSCALE_RUNTIME)
	switch (data->sample_fs) {
	case 0:
		return lsm9ds0_gyro_get_channel(chan, val, data, 8.75f);
	case 1:
		return lsm9ds0_gyro_get_channel(chan, val, data, 17.50f);
	default:
		return lsm9ds0_gyro_get_channel(chan, val, data, 70.0f);
	}
#elif defined(CONFIG_LSM9DS0_GYRO_FULLSCALE_245)
	return lsm9ds0_gyro_get_channel(chan, val, data, 8.75f);
#elif defined(CONFIG_LSM9DS0_GYRO_FULLSCALE_500)
	return lsm9ds0_gyro_get_channel(chan, val, data, 17.50f);
#elif defined(CONFIG_LSM9DS0_GYRO_FULLSCALE_2000)
	return lsm9ds0_gyro_get_channel(chan, val, data, 70.0f);
#endif
	return 0;
}

#if defined(LSM9DS0_GYRO_SET_ATTR)
static int lsm9ds0_gyro_attr_set(struct device *dev,
				 enum sensor_channel chan,
				 enum sensor_attribute attr,
				 const struct sensor_value *val)
{
	switch (attr) {
#if defined(CONFIG_LSM9DS0_GYRO_FULLSCALE_RUNTIME)
	case SENSOR_ATTR_FULL_SCALE:
		if (lsm9ds0_gyro_set_fs(dev, sensor_rad_to_degrees(val)) < 0) {
			SYS_LOG_DBG("full-scale value not supported");
			return -EIO;
		}
		break;
#endif
#if defined(CONFIG_LSM9DS0_GYRO_SAMPLING_RATE_RUNTIME)
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		if (lsm9ds0_gyro_set_odr(dev, val->val1) < 0) {
			SYS_LOG_DBG("sampling frequency value not supported");
			return -EIO;
		}
		break;
#endif
	default:
		return -ENOTSUP;
	}

	return 0;
}
#endif

static const struct sensor_driver_api lsm9ds0_gyro_api_funcs = {
	.sample_fetch = lsm9ds0_gyro_sample_fetch,
	.channel_get = lsm9ds0_gyro_channel_get,
#if defined(LSM9DS0_GYRO_SET_ATTR)
	.attr_set = lsm9ds0_gyro_attr_set,
#endif
#if defined(CONFIG_LSM9DS0_GYRO_TRIGGER_DRDY)
	.trigger_set = lsm9ds0_gyro_trigger_set,
#endif
};

static int lsm9ds0_gyro_init_chip(struct device *dev)
{
	struct lsm9ds0_gyro_data *data = dev->driver_data;
	const struct lsm9ds0_gyro_config *config = dev->config->config_info;
	uint8_t chip_id;

	if (lsm9ds0_gyro_power_ctrl(dev, 0, 0, 0, 0) < 0) {
		SYS_LOG_DBG("failed to power off device");
		return -EIO;
	}

	if (lsm9ds0_gyro_power_ctrl(dev, 1, 1, 1, 1) < 0) {
		SYS_LOG_DBG("failed to power on device");
		return -EIO;
	}

	if (i2c_reg_read_byte(data->i2c_master, config->i2c_slave_addr,
			      LSM9DS0_GYRO_REG_WHO_AM_I_G, &chip_id) < 0) {
		SYS_LOG_DBG("failed reading chip id");
		goto err_poweroff;
	}
	if (chip_id != LSM9DS0_GYRO_VAL_WHO_AM_I_G) {
		SYS_LOG_DBG("invalid chip id 0x%x", chip_id);
		goto err_poweroff;
	}
	SYS_LOG_DBG("chip id 0x%x", chip_id);

	if (lsm9ds0_gyro_set_fs_raw(dev, LSM9DS0_GYRO_DEFAULT_FULLSCALE) < 0) {
		SYS_LOG_DBG("failed to set full-scale");
		goto err_poweroff;
	}

	if (lsm9ds0_gyro_set_odr_raw(dev, LSM9DS0_GYRO_DEFAULT_SAMPLING_RATE)
				     < 0) {
		SYS_LOG_DBG("failed to set sampling rate");
		goto err_poweroff;
	}

	if (i2c_reg_update_byte(data->i2c_master, config->i2c_slave_addr,
				LSM9DS0_GYRO_REG_CTRL_REG4_G,
				LSM9DS0_GYRO_MASK_CTRL_REG4_G_BDU |
				LSM9DS0_GYRO_MASK_CTRL_REG4_G_BLE,
				(1 << LSM9DS0_GYRO_SHIFT_CTRL_REG4_G_BDU) |
				(0 << LSM9DS0_GYRO_SHIFT_CTRL_REG4_G_BLE))
				< 0) {
		SYS_LOG_DBG("failed to set BDU and BLE");
		goto err_poweroff;
	}

	return 0;

err_poweroff:
	lsm9ds0_gyro_power_ctrl(dev, 0, 0, 0, 0);
	return -EIO;
}

static int lsm9ds0_gyro_init(struct device *dev)
{
	const struct lsm9ds0_gyro_config * const config =
					   dev->config->config_info;
	struct lsm9ds0_gyro_data *data = dev->driver_data;

	data->i2c_master = device_get_binding(config->i2c_master_dev_name);
	if (!data->i2c_master) {
		SYS_LOG_DBG("i2c master not found: %s",
			    config->i2c_master_dev_name);
		return -EINVAL;
	}

	if (lsm9ds0_gyro_init_chip(dev) < 0) {
		SYS_LOG_DBG("failed to initialize chip");
		return -EIO;
	}

#if defined(CONFIG_LSM9DS0_GYRO_TRIGGER_DRDY)
	if (lsm9ds0_gyro_init_interrupt(dev) < 0) {
		SYS_LOG_DBG("failed to initialize interrupts");
		return -EIO;
	}

	data->dev = dev;
#endif

	dev->driver_api = &lsm9ds0_gyro_api_funcs;

	return 0;
}

static const struct lsm9ds0_gyro_config lsm9ds0_gyro_config = {
	.i2c_master_dev_name = CONFIG_LSM9DS0_GYRO_I2C_MASTER_DEV_NAME,
	.i2c_slave_addr = LSM9DS0_GYRO_I2C_ADDR,
#if defined(CONFIG_LSM9DS0_GYRO_TRIGGER_DRDY)
	.gpio_drdy_dev_name = CONFIG_LSM9DS0_GYRO_GPIO_DRDY_DEV_NAME,
	.gpio_drdy_int_pin = CONFIG_LSM9DS0_GYRO_GPIO_DRDY_INT_PIN,
#endif
};

static struct lsm9ds0_gyro_data lsm9ds0_gyro_data;

DEVICE_INIT(lsm9ds0_gyro, CONFIG_LSM9DS0_GYRO_DEV_NAME, lsm9ds0_gyro_init,
	    &lsm9ds0_gyro_data, &lsm9ds0_gyro_config, POST_KERNEL,
	    CONFIG_SENSOR_INIT_PRIORITY);
