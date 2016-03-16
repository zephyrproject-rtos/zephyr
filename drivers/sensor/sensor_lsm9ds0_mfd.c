/* sensor_lsm9ds0_mfd.c - Driver for LSM9DS0 accelerometer, magnetometer
 * and temperature (MFD) sensor driver
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <sensor.h>
#include <nanokernel.h>
#include <device.h>
#include <init.h>
#include <i2c.h>
#include <misc/byteorder.h>

#include <gpio.h>

#include "sensor_lsm9ds0_mfd.h"

#ifdef CONFIG_SENSOR_DEBUG
#include <misc/printk.h>
#define sensor_dbg(fmt, ...) printk("lsm9ds0_mfd: " fmt, ##__VA_ARGS__)
#else
#define sensor_dbg(fmt, ...) do { } while (0)
#endif /* CONFIG_SENSOR_DEBUG */

static inline int lsm9ds0_mfd_reboot_memory(struct device *dev)
{
	struct lsm9ds0_mfd_data *data = dev->driver_data;
	struct lsm9ds0_mfd_config *config = dev->config->config_info;

	if (i2c_reg_update_byte(data->i2c_master, config->i2c_slave_addr,
				LSM9DS0_MFD_REG_CTRL_REG0_XM,
				LSM9DS0_MFD_MASK_CTRL_REG0_XM_BOOT,
				1 << LSM9DS0_MFD_SHIFT_CTRL_REG0_XM_BOOT)
				!= 0) {
		return -EIO;
	}

	sys_thread_busy_wait(50 * USEC_PER_MSEC);

	return 0;
}

#if !defined(LSM9DS0_MFD_ACCEL_DISABLED)
static inline int lsm9ds0_mfd_accel_set_odr_raw(struct device *dev, uint8_t odr)
{
	struct lsm9ds0_mfd_data *data = dev->driver_data;
	struct lsm9ds0_mfd_config *config = dev->config->config_info;

	return i2c_reg_update_byte(data->i2c_master, config->i2c_slave_addr,
				   LSM9DS0_MFD_REG_CTRL_REG1_XM,
				   LSM9DS0_MFD_MASK_CTRL_REG1_XM_AODR,
				   odr << LSM9DS0_MFD_SHIFT_CTRL_REG1_XM_AODR);
}

#if defined(CONFIG_LSM9DS0_MFD_ACCEL_SAMPLING_RATE_RUNTIME)
static const struct {
	int freq_int;
	int freq_micro;
} lsm9ds0_mfd_accel_odr_map[] = { {0, 0},
				  {3, 125000},
				  {6, 250000},
				  {12, 500000},
				  {25, 0},
				  {50, 0},
				  {100, 0},
				  {200, 0},
				  {400, 0},
				  {800, 0},
				  {1600, 0} };

static int lsm9ds0_mfd_accel_set_odr(struct device *dev,
				     const struct sensor_value *val)
{
	uint8_t i;

	for (i = 0; i < ARRAY_SIZE(lsm9ds0_mfd_accel_odr_map); ++i) {
		if (val->val1 < lsm9ds0_mfd_accel_odr_map[i].freq_int ||
		    (val->val1 == lsm9ds0_mfd_accel_odr_map[i].freq_int &&
		     val->val2 <= lsm9ds0_mfd_accel_odr_map[i].freq_micro)) {
			return lsm9ds0_mfd_accel_set_odr_raw(dev, i);
		}
	}

	return -ENOTSUP;
}
#endif

static inline int lsm9ds0_mfd_accel_set_fs_raw(struct device *dev, uint8_t fs)
{
	struct lsm9ds0_mfd_data *data = dev->driver_data;
	struct lsm9ds0_mfd_config *config = dev->config->config_info;

	if (i2c_reg_update_byte(data->i2c_master, config->i2c_slave_addr,
				LSM9DS0_MFD_REG_CTRL_REG2_XM,
				LSM9DS0_MFD_MASK_CTRL_REG2_XM_AFS,
				fs << LSM9DS0_MFD_SHIFT_CTRL_REG2_XM_AFS)
				!= 0) {
		return -EIO;
	}

#if defined(CONFIG_LSM9DS0_MFD_ACCEL_FULL_SCALE_RUNTIME)
	data->accel_fs = fs;
#endif

	return 0;
}

#if defined(CONFIG_LSM9DS0_MFD_ACCEL_FULL_SCALE_RUNTIME)
static const struct {
	int fs;
} lsm9ds0_mfd_accel_fs_map[] = { {2},
				 {4},
				 {6},
				 {8},
				 {16} };

static int lsm9ds0_mfd_accel_set_fs(struct device *dev, int val)
{
	uint8_t i;

	for (i = 0; i < ARRAY_SIZE(lsm9ds0_mfd_accel_fs_map); ++i) {
		if (val <= lsm9ds0_mfd_accel_fs_map[i].fs) {
			return lsm9ds0_mfd_accel_set_fs_raw(dev, i);
		}
	}

	return -ENOTSUP;
}
#endif
#endif

#if !defined(LSM9DS0_MFD_MAGN_DISABLED)
static inline int lsm9ds0_mfd_magn_set_odr_raw(struct device *dev, uint8_t odr)
{
	struct lsm9ds0_mfd_data *data = dev->driver_data;
	struct lsm9ds0_mfd_config *config = dev->config->config_info;

	return i2c_reg_update_byte(data->i2c_master, config->i2c_slave_addr,
				   LSM9DS0_MFD_REG_CTRL_REG5_XM,
				   LSM9DS0_MFD_MASK_CTRL_REG5_XM_M_ODR,
				   odr << LSM9DS0_MFD_SHIFT_CTRL_REG5_XM_M_ODR);
}

#if defined(CONFIG_LSM9DS0_MFD_MAGN_SAMPLING_RATE_RUNTIME)
static const struct {
	int freq_int;
	int freq_micro;
} lsm9ds0_mfd_magn_odr_map[] = { {0, 0},
				 {3, 125000},
				 {6, 250000},
				 {12, 500000},
				 {25, 0},
				 {50, 0},
				 {100, 0} };

static int lsm9ds0_mfd_magn_set_odr(struct device *dev,
				    const struct sensor_value *val)
{
	uint8_t i;

	for (i = 0; i < ARRAY_SIZE(lsm9ds0_mfd_accel_odr_map); ++i) {
		if (val->val1 < lsm9ds0_mfd_accel_odr_map[i].freq_int ||
		    (val->val1 == lsm9ds0_mfd_accel_odr_map[i].freq_int &&
		     val->val2 <= lsm9ds0_mfd_accel_odr_map[i].freq_micro)) {
			return lsm9ds0_mfd_magn_set_odr_raw(dev, i);
		}
	}

	return -ENOTSUP;
}
#endif

static inline int lsm9ds0_mfd_magn_set_fs_raw(struct device *dev, uint8_t fs)
{
	struct lsm9ds0_mfd_data *data = dev->driver_data;
	struct lsm9ds0_mfd_config *config = dev->config->config_info;

	if (i2c_reg_update_byte(data->i2c_master, config->i2c_slave_addr,
				LSM9DS0_MFD_REG_CTRL_REG6_XM,
				LSM9DS0_MFD_MASK_CTRL_REG6_XM_MFS,
				fs << LSM9DS0_MFD_SHIFT_CTRL_REG6_XM_MFS)
				!= 0) {
		return -EIO;
	}

#if defined(CONFIG_LSM9DS0_MFD_MAGN_FULL_SCALE_RUNTIME)
	data->magn_fs = fs;
#endif

	return 0;
}

#if defined(CONFIG_LSM9DS0_MFD_MAGN_FULL_SCALE_RUNTIME)
static const struct {
	int fs;
} lsm9ds0_mfd_magn_fs_map[] = { {2},
				{4},
				{8},
				{12} };

static int lsm9ds0_mfd_magn_set_fs(struct device *dev,
				   const struct sensor_value *val)
{
	uint8_t i;

	for (i = 0; i < ARRAY_SIZE(lsm9ds0_mfd_magn_fs_map); ++i) {
		if (val->val1 <= lsm9ds0_mfd_magn_fs_map[i].fs) {
			return lsm9ds0_mfd_magn_set_fs_raw(dev, i);
		}
	}

	return -ENOTSUP;
}
#endif
#endif

#if !defined(LSM9DS0_MFD_ACCEL_DISABLED)
static inline int lsm9ds0_mfd_sample_fetch_accel(struct device *dev)
{
	struct lsm9ds0_mfd_data *data = dev->driver_data;
	struct lsm9ds0_mfd_config *config = dev->config->config_info;
	uint8_t out_l, out_h;

#if defined(CONFIG_LSM9DS0_MFD_ACCEL_ENABLE_X)
	if (i2c_reg_read_byte(data->i2c_master, config->i2c_slave_addr,
			      LSM9DS0_MFD_REG_OUT_X_L_A, &out_l) != 0 ||
	    i2c_reg_read_byte(data->i2c_master, config->i2c_slave_addr,
			      LSM9DS0_MFD_REG_OUT_X_H_A, &out_h) != 0) {
		sensor_dbg("failed to read accel sample (X axis)\n");
		return -EIO;
	}

	data->sample_accel_x = (int16_t)((uint16_t)(out_l) |
			       ((uint16_t)(out_h) << 8));
#endif

#if defined(CONFIG_LSM9DS0_MFD_ACCEL_ENABLE_Y)
	if (i2c_reg_read_byte(data->i2c_master, config->i2c_slave_addr,
			      LSM9DS0_MFD_REG_OUT_Y_L_A, &out_l) != 0 ||
	    i2c_reg_read_byte(data->i2c_master, config->i2c_slave_addr,
			      LSM9DS0_MFD_REG_OUT_Y_H_A, &out_h) != 0) {
		sensor_dbg("failed to read accel sample (Y axis)\n");
		return -EIO;
	}

	data->sample_accel_y = (int16_t)((uint16_t)(out_l) |
			       ((uint16_t)(out_h) << 8));
#endif

#if defined(CONFIG_LSM9DS0_MFD_ACCEL_ENABLE_Z)
	if (i2c_reg_read_byte(data->i2c_master, config->i2c_slave_addr,
			      LSM9DS0_MFD_REG_OUT_Z_L_A, &out_l) != 0 ||
	    i2c_reg_read_byte(data->i2c_master, config->i2c_slave_addr,
			      LSM9DS0_MFD_REG_OUT_Z_H_A, &out_h) != 0) {
		sensor_dbg("failed to read accel sample (Z axis)\n");
		return -EIO;
	}

	data->sample_accel_z = (int16_t)((uint16_t)(out_l) |
			       ((uint16_t)(out_h) << 8));
#endif

#if defined(CONFIG_LSM9DS0_MFD_ACCEL_FULL_SCALE_RUNTIME)
	data->sample_accel_fs = data->accel_fs;
#endif

	return 0;
}
#endif

#if !defined(LSM9DS0_MFD_MAGN_DISABLED)
static inline int lsm9ds0_mfd_sample_fetch_magn(struct device *dev)
{
	struct lsm9ds0_mfd_data *data = dev->driver_data;
	struct lsm9ds0_mfd_config *config = dev->config->config_info;
	uint8_t out_l, out_h;

	if (i2c_reg_read_byte(data->i2c_master, config->i2c_slave_addr,
			      LSM9DS0_MFD_REG_OUT_X_L_M, &out_l) != 0 ||
	    i2c_reg_read_byte(data->i2c_master, config->i2c_slave_addr,
			      LSM9DS0_MFD_REG_OUT_X_H_M, &out_h) != 0) {
		sensor_dbg("failed to read magn sample (X axis)\n");
		return -EIO;
	}

	data->sample_magn_x = (int16_t)((uint16_t)(out_l) |
			      ((uint16_t)(out_h) << 8));

	if (i2c_reg_read_byte(data->i2c_master, config->i2c_slave_addr,
			      LSM9DS0_MFD_REG_OUT_Y_L_M, &out_l) != 0 ||
	    i2c_reg_read_byte(data->i2c_master, config->i2c_slave_addr,
			      LSM9DS0_MFD_REG_OUT_Y_H_M, &out_h) != 0) {
		sensor_dbg("failed to read magn sample (Y axis)\n");
		return -EIO;
	}

	data->sample_magn_y = (int16_t)((uint16_t)(out_l) |
			      ((uint16_t)(out_h) << 8));

	if (i2c_reg_read_byte(data->i2c_master, config->i2c_slave_addr,
			      LSM9DS0_MFD_REG_OUT_Z_L_M, &out_l) != 0 ||
	    i2c_reg_read_byte(data->i2c_master, config->i2c_slave_addr,
			      LSM9DS0_MFD_REG_OUT_Z_H_M, &out_h) != 0) {
		sensor_dbg("failed to read magn sample (Z axis)\n");
		return -EIO;
	}

	data->sample_magn_z = (int16_t)((uint16_t)(out_l) |
			      ((uint16_t)(out_h) << 8));

#if defined(CONFIG_LSM9DS0_MFD_MAGN_FULL_SCALE_RUNTIME)
	data->sample_magn_fs = data->magn_fs;
#endif

	return 0;
}
#endif

#if !defined(LSM9DS0_MFD_TEMP_DISABLED)
static inline int lsm9ds0_mfd_sample_fetch_temp(struct device *dev)
{
	struct lsm9ds0_mfd_data *data = dev->driver_data;
	struct lsm9ds0_mfd_config *config = dev->config->config_info;
	uint8_t out_l, out_h;

	if (i2c_reg_read_byte(data->i2c_master, config->i2c_slave_addr,
			      LSM9DS0_MFD_REG_OUT_TEMP_L_XM, &out_l) != 0 ||
	    i2c_reg_read_byte(data->i2c_master, config->i2c_slave_addr,
			      LSM9DS0_MFD_REG_OUT_TEMP_H_XM, &out_h) != 0) {
		sensor_dbg("failed to read temperature sample\n");
		return -EIO;
	}

	data->sample_temp = (int16_t)((uint16_t)(out_l) |
			    ((uint16_t)(out_h) << 8));

	return 0;
}
#endif

static inline int lsm9ds0_mfd_sample_fetch_all(struct device *dev)
{
#if !defined(LSM9DS0_MFD_ACCEL_DISABLED)
	if (lsm9ds0_mfd_sample_fetch_accel(dev) != 0) {
		return -EIO;
	}
#endif

#if !defined(LSM9DS0_MFD_MAGN_DISABLED)
	if (lsm9ds0_mfd_sample_fetch_magn(dev) != 0) {
		return -EIO;
	}
#endif

#if !defined(LSM9DS0_MFD_TEMP_DISABLED)
	if (lsm9ds0_mfd_sample_fetch_temp(dev) != 0) {
		return -EIO;
	}
#endif

	return 0;
}

static int lsm9ds0_mfd_sample_fetch(struct device *dev,
				    enum sensor_channel chan)
{
	switch (chan) {
#if !defined(LSM9DS0_MFD_ACCEL_DISABLED)
	case SENSOR_CHAN_ACCEL_ANY:
		return lsm9ds0_mfd_sample_fetch_accel(dev);
#endif
#if !defined(LSM9DS0_MFD_MAGN_DISABLED)
	case SENSOR_CHAN_MAGN_ANY:
		return lsm9ds0_mfd_sample_fetch_magn(dev);
#endif
#if !defined(LSM9DS0_MFD_TEMP_DISABLED)
	case SENSOR_CHAN_TEMP:
		return lsm9ds0_mfd_sample_fetch_temp(dev);
#endif
	case SENSOR_CHAN_ALL:
		return lsm9ds0_mfd_sample_fetch_all(dev);
	default:
		return -EINVAL;
	}

	return 0;
}

#if !defined(LSM9DS0_MFD_ACCEL_DISABLED)
static inline int lsm9ds0_mfd_channel_get_accel(struct device *dev,
						int raw_val,
						struct sensor_value *val)
{
#if defined(CONFIG_LSM9DS0_MFD_ACCEL_FULL_SCALE_RUNTIME)
	struct lsm9ds0_mfd_data *data = dev->driver_data;
#endif

	val->type = SENSOR_VALUE_TYPE_DOUBLE;

#if defined(CONFIG_LSM9DS0_MFD_ACCEL_FULL_SCALE_RUNTIME)
	switch (data->sample_accel_fs) {
	case 0:
#endif
#if defined(CONFIG_LSM9DS0_MFD_ACCEL_FULL_SCALE_RUNTIME) || \
	defined(CONFIG_LSM9DS0_MFD_ACCEL_FULL_SCALE_2)
		val->dval = (double)(raw_val) * 2.0 * 9.807 / 32767.0;
#endif
#if defined(CONFIG_LSM9DS0_MFD_ACCEL_FULL_SCALE_RUNTIME)
		break;
	case 1:
#endif
#if defined(CONFIG_LSM9DS0_MFD_ACCEL_FULL_SCALE_RUNTIME) || \
	defined(CONFIG_LSM9DS0_MFD_ACCEL_FULL_SCALE_4)
		val->dval = (double)(raw_val) * 4.0 * 9.807 / 32767.0;
#endif
#if defined(CONFIG_LSM9DS0_MFD_ACCEL_FULL_SCALE_RUNTIME)
		break;
	case 2:
#endif
#if defined(CONFIG_LSM9DS0_MFD_ACCEL_FULL_SCALE_RUNTIME) || \
	defined(CONFIG_LSM9DS0_MFD_ACCEL_FULL_SCALE_6)
		val->dval = (double)(raw_val) * 6.0 * 9.807 / 32767.0;
#endif
#if defined(CONFIG_LSM9DS0_MFD_ACCEL_FULL_SCALE_RUNTIME)
		break;
	case 3:
#endif
#if defined(CONFIG_LSM9DS0_MFD_ACCEL_FULL_SCALE_RUNTIME) || \
	defined(CONFIG_LSM9DS0_MFD_ACCEL_FULL_SCALE_8)
		val->dval = (double)(raw_val) * 8.0 * 9.807 / 32767.0;
#endif
#if defined(CONFIG_LSM9DS0_MFD_ACCEL_FULL_SCALE_RUNTIME)
		break;
	case 4:
#endif
#if defined(CONFIG_LSM9DS0_MFD_ACCEL_FULL_SCALE_RUNTIME) || \
	defined(CONFIG_LSM9DS0_MFD_ACCEL_FULL_SCALE_16)
		val->dval = (double)(raw_val) * 16.0 * 9.807 / 32767.0;
#endif
#if defined(CONFIG_LSM9DS0_MFD_ACCEL_FULL_SCALE_RUNTIME)
		break;
	default:
		return -ENOTSUP;
	}
#endif

	return 0;
}
#endif

#if !defined(LSM9DS0_MFD_MAGN_DISABLED)
static inline int lsm9ds0_mfd_channel_get_magn(struct device *dev,
					       int raw_val,
					       struct sensor_value *val)
{
#if defined(CONFIG_LSM9DS0_MFD_MAGN_FULL_SCALE_RUNTIME)
	struct lsm9ds0_mfd_data *data = dev->driver_data;
#endif

	val->type = SENSOR_VALUE_TYPE_DOUBLE;

#if defined(CONFIG_LSM9DS0_MFD_MAGN_FULL_SCALE_RUNTIME)
	switch (data->sample_magn_fs) {
	case 0:
#endif
#if defined(CONFIG_LSM9DS0_MFD_MAGN_FULL_SCALE_RUNTIME) || \
	defined(CONFIG_LSM9DS0_MFD_MAGN_FULL_SCALE_2)
		val->dval = raw_val * 2.0 / 32767.0;
#endif
#if defined(CONFIG_LSM9DS0_MFD_MAGN_FULL_SCALE_RUNTIME)
		break;
	case 1:
#endif
#if defined(CONFIG_LSM9DS0_MFD_MAGN_FULL_SCALE_RUNTIME) || \
	defined(CONFIG_LSM9DS0_MFD_MAGN_FULL_SCALE_4)
		val->dval = raw_val * 4.0 / 32767.0;
#endif
#if defined(CONFIG_LSM9DS0_MFD_MAGN_FULL_SCALE_RUNTIME)
		break;
	case 2:
#endif
#if defined(CONFIG_LSM9DS0_MFD_MAGN_FULL_SCALE_RUNTIME) || \
	defined(CONFIG_LSM9DS0_MFD_MAGN_FULL_SCALE_8)
		val->dval = raw_val * 8.0 / 32767.0;
#endif
#if defined(CONFIG_LSM9DS0_MFD_MAGN_FULL_SCALE_RUNTIME)
		break;
	case 3:
#endif
#if defined(CONFIG_LSM9DS0_MFD_MAGN_FULL_SCALE_RUNTIME) || \
	defined(CONFIG_LSM9DS0_MFD_MAGN_FULL_SCALE_12)
		val->dval = raw_val * 12.0 / 32767.0;
#endif
#if defined(CONFIG_LSM9DS0_MFD_MAGN_FULL_SCALE_RUNTIME)
		break;
	default:
		return -ENOTSUP;
	}
#endif

	return 0;
}
#endif

static int lsm9ds0_mfd_channel_get(struct device *dev,
				   enum sensor_channel chan,
				   struct sensor_value *val)
{
	struct lsm9ds0_mfd_data *data = dev->driver_data;

	switch (chan) {
#if !defined(LSM9DS0_MFD_ACCEL_DISABLED)
	case SENSOR_CHAN_ACCEL_X:
		return lsm9ds0_mfd_channel_get_accel(dev, data->sample_accel_x,
						     val);
	case SENSOR_CHAN_ACCEL_Y:
		return lsm9ds0_mfd_channel_get_accel(dev, data->sample_accel_y,
						     val);
	case SENSOR_CHAN_ACCEL_Z:
		return lsm9ds0_mfd_channel_get_accel(dev, data->sample_accel_z,
						     val);
#endif
#if !defined(LSM9DS0_MFD_MAGN_DISABLED)
	case SENSOR_CHAN_MAGN_X:
		return lsm9ds0_mfd_channel_get_magn(dev, data->sample_magn_x,
						    val);
	case SENSOR_CHAN_MAGN_Y:
		return lsm9ds0_mfd_channel_get_magn(dev, data->sample_magn_y,
						    val);
	case SENSOR_CHAN_MAGN_Z:
		return lsm9ds0_mfd_channel_get_magn(dev, data->sample_magn_z,
						    val);
#endif
#if !defined(LSM9DS0_MFD_TEMP_DISABLED)
	case SENSOR_CHAN_TEMP:
		val->type = SENSOR_VALUE_TYPE_DOUBLE;
		val->dval = data->sample_temp;
		return 0;
#endif
	default:
		return -ENOTSUP;
	}
}

#if defined(LSM9DS0_MFD_ATTR_SET_ACCEL)
static inline int lsm9ds0_mfd_attr_set_accel(struct device *dev,
					     enum sensor_attribute attr,
					     const struct sensor_value *val)
{
	switch (attr) {
#if defined(CONFIG_LSM9DS0_MFD_ACCEL_SAMPLING_RATE_RUNTIME)
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		if (val->type != SENSOR_VALUE_TYPE_INT &&
		    val->type != SENSOR_VALUE_TYPE_INT_PLUS_MICRO) {
			return -EINVAL;
		}

		return lsm9ds0_mfd_accel_set_odr(dev, val);
#endif
#if defined(CONFIG_LSM9DS0_MFD_ACCEL_FULL_SCALE_RUNTIME)
	case SENSOR_ATTR_FULL_SCALE:
		if (val->type != SENSOR_VALUE_TYPE_INT &&
		    val->type != SENSOR_VALUE_TYPE_INT_PLUS_MICRO) {
			return -EINVAL;
		}

		return lsm9ds0_mfd_accel_set_fs(dev, sensor_ms2_to_g(val));
#endif
	default:
		return -ENOTSUP;
	}

	return 0;
}
#endif

#if defined(LSM9DS0_MFD_ATTR_SET_MAGN)
static inline int lsm9ds0_mfd_attr_set_magn(struct device *dev,
					     enum sensor_attribute attr,
					     const struct sensor_value *val)
{
	switch (attr) {
#if defined(CONFIG_LSM9DS0_MFD_MAGN_SAMPLING_RATE_RUNTIME)
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		if (val->type != SENSOR_VALUE_TYPE_INT &&
		    val->type != SENSOR_VALUE_TYPE_INT_PLUS_MICRO) {
			return -EINVAL;
		}

		return lsm9ds0_mfd_magn_set_odr(dev, val);
#endif
#if defined(CONFIG_LSM9DS0_MFD_MAGN_FULL_SCALE_RUNTIME)
	case SENSOR_ATTR_FULL_SCALE:
		if (val->type != SENSOR_VALUE_TYPE_INT &&
		    val->type != SENSOR_VALUE_TYPE_INT_PLUS_MICRO) {
			return -EINVAL;
		}

		return lsm9ds0_mfd_magn_set_fs(dev, val);
#endif
	default:
		return -ENOTSUP;
	}

	return 0;
}
#endif

#if defined(LSM9DS0_MFD_ATTR_SET)
static int lsm9ds0_mfd_attr_set(struct device *dev,
				enum sensor_channel chan,
				enum sensor_attribute attr,
				const struct sensor_value *val)
{

	switch (chan) {
#if defined(LSM9DS0_MFD_ATTR_SET_ACCEL)
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_ANY:
		return lsm9ds0_mfd_attr_set_accel(dev, attr, val);
#endif
#if defined(LSM9DS0_MFD_ATTR_SET_MAGN)
	case SENSOR_CHAN_MAGN_X:
	case SENSOR_CHAN_MAGN_Y:
	case SENSOR_CHAN_MAGN_Z:
	case SENSOR_CHAN_MAGN_ANY:
		return lsm9ds0_mfd_attr_set_magn(dev, attr, val);
#endif
	default:
		return -ENOTSUP;
	}

	return 0;
}
#endif

static struct sensor_driver_api lsm9ds0_mfd_api_funcs = {
	.sample_fetch = lsm9ds0_mfd_sample_fetch,
	.channel_get = lsm9ds0_mfd_channel_get,
#if defined(LSM9DS0_MFD_ATTR_SET)
	.attr_set = lsm9ds0_mfd_attr_set,
#endif
};

static int lsm9ds0_mfd_init_chip(struct device *dev)
{
	struct lsm9ds0_mfd_data *data = dev->driver_data;
	struct lsm9ds0_mfd_config *config = dev->config->config_info;
	uint8_t chip_id;

	if (lsm9ds0_mfd_reboot_memory(dev) != 0) {
		sensor_dbg("failed to reset device\n");
		return -EIO;
	}

	if (i2c_reg_read_byte(data->i2c_master, config->i2c_slave_addr,
			      LSM9DS0_MFD_REG_WHO_AM_I_XM, &chip_id) != 0) {
		sensor_dbg("failed reading chip id\n");
		return -EIO;
	}

	if (chip_id != LSM9DS0_MFD_VAL_WHO_AM_I_XM) {
		sensor_dbg("invalid chip id 0x%x\n", chip_id);
		return -EIO;
	}

	sensor_dbg("chip id 0x%x\n", chip_id);

#if !defined(LSM9DS0_MFD_ACCEL_DISABLED)
	if (i2c_reg_update_byte(data->i2c_master, config->i2c_slave_addr,
				LSM9DS0_MFD_REG_CTRL_REG1_XM,
				LSM9DS0_MFD_MASK_CTRL_REG1_XM_BDU |
				LSM9DS0_MFD_MASK_CTRL_REG1_XM_AODR,
				(1 << LSM9DS0_MFD_SHIFT_CTRL_REG1_XM_BDU) |
				(LSM9DS0_MFD_ACCEL_DEFAULT_AODR <<
				LSM9DS0_MFD_SHIFT_CTRL_REG1_XM_AODR))) {
		sensor_dbg("failed to set AODR and BDU\n");
		return -EIO;
	}

	if (lsm9ds0_mfd_accel_set_fs_raw(dev, LSM9DS0_MFD_ACCEL_DEFAULT_FS)) {
		sensor_dbg("failed to set accelerometer full-scale\n");
		return -EIO;
	}

	if (i2c_reg_update_byte(data->i2c_master, config->i2c_slave_addr,
				LSM9DS0_MFD_REG_CTRL_REG1_XM,
				LSM9DS0_MFD_MASK_CTRL_REG1_XM_AXEN |
				LSM9DS0_MFD_MASK_CTRL_REG1_XM_AYEN |
				LSM9DS0_MFD_MASK_CTRL_REG1_XM_AZEN,
				(LSM9DS0_MFD_ACCEL_ENABLE_X <<
				LSM9DS0_MFD_SHIFT_CTRL_REG1_XM_AXEN) |
				(LSM9DS0_MFD_ACCEL_ENABLE_Y <<
				LSM9DS0_MFD_SHIFT_CTRL_REG1_XM_AYEN) |
				(LSM9DS0_MFD_ACCEL_ENABLE_Z <<
				LSM9DS0_MFD_SHIFT_CTRL_REG1_XM_AZEN)) != 0) {
		sensor_dbg("failed to set accelerometer axis enable bits\n");
		return -EIO;
	}

#elif !defined(LSM9DS0_MFD_MAGN_DISABLED)
	if (i2c_reg_update_byte(data->i2c_master, config->i2c_slave_addr,
				LSM9DS0_MFD_REG_CTRL_REG1_XM,
				LSM9DS0_MFD_MASK_CTRL_REG1_XM_BDU,
				1 << LSM9DS0_MFD_SHIFT_CTRL_REG1_XM_BDU)
				!= 0) {
		sensor_dbg("failed to set BDU\n");
		return -EIO;
	}
#endif

#if !defined(LSM9DS0_MFD_MAGN_DISABLED)
	if (i2c_reg_update_byte(data->i2c_master, config->i2c_slave_addr,
				LSM9DS0_MFD_REG_CTRL_REG7_XM,
				LSM9DS0_MFD_MASK_CTRL_REG7_XM_MD,
				(0 << LSM9DS0_MFD_SHIFT_CTRL_REG7_XM_MD))
				!= 0) {
		sensor_dbg("failed to power on magnetometer\n");
		return -EIO;
	}

	if (lsm9ds0_mfd_magn_set_odr_raw(dev, LSM9DS0_MFD_MAGN_DEFAULT_M_ODR)) {
		sensor_dbg("failed to set magnetometer sampling rate\n");
		return -EIO;
	}

	if (lsm9ds0_mfd_magn_set_fs_raw(dev, LSM9DS0_MFD_MAGN_DEFAULT_FS)) {
		sensor_dbg("failed to set magnetometer full-scale\n");
		return -EIO;
	}
#endif

#if !defined(LSM9DS0_MFD_TEMP_DISABLED)
	if (i2c_reg_update_byte(data->i2c_master, config->i2c_slave_addr,
				LSM9DS0_MFD_REG_CTRL_REG5_XM,
				LSM9DS0_MFD_MASK_CTRL_REG5_XM_TEMP_EN,
				1 << LSM9DS0_MFD_SHIFT_CTRL_REG5_XM_TEMP_EN)
				!= 0) {
		sensor_dbg("failed to power on temperature sensor\n");
		return -EIO;
	}
#endif

	return 0;
}

int lsm9ds0_mfd_init(struct device *dev)
{
	const struct lsm9ds0_mfd_config * const config =
				dev->config->config_info;
	struct lsm9ds0_mfd_data *data = dev->driver_data;

	data->i2c_master = device_get_binding(config->i2c_master_dev_name);
	if (!data->i2c_master) {
		sensor_dbg("i2c master not found: %s\n",
			   config->i2c_master_dev_name);
		return -EINVAL;
	}

	if (lsm9ds0_mfd_init_chip(dev) != 0) {
		sensor_dbg("failed to initialize chip\n");
		return -EIO;
	}

	dev->driver_api = &lsm9ds0_mfd_api_funcs;

	return 0;
}

static struct lsm9ds0_mfd_config lsm9ds0_mfd_config = {
	.i2c_master_dev_name = CONFIG_LSM9DS0_MFD_I2C_MASTER_DEV_NAME,
	.i2c_slave_addr = LSM9DS0_MFD_I2C_ADDR,
};

static struct lsm9ds0_mfd_data lsm9ds0_mfd_data;

DEVICE_INIT(lsm9ds0_mfd, CONFIG_LSM9DS0_MFD_DEV_NAME, lsm9ds0_mfd_init,
	    &lsm9ds0_mfd_data, &lsm9ds0_mfd_config, NANOKERNEL,
	    CONFIG_LSM9DS0_MFD_INIT_PRIORITY);
