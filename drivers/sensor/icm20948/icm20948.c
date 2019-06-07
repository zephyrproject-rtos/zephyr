/*
 * SPDX-License-Identifier: Apache-2.0
 */


#include <init.h>
#include <sensor.h>
#include <misc/byteorder.h>
#include <kernel.h>
#include <misc/__assert.h>
#include <logging/log.h>

#include "icm20948.h"

#ifdef DT_TDK_ICM20948_BUS_I2C
#include <i2c.h>
#elif defined DT_TDK_ICM20948_BUS_SPI
#include <spi.h>
#endif


#define LOG_LEVEL CONFIG_SENSOR_LOG_LEVEL
LOG_MODULE_REGISTER(ICM20948);

static inline int icm20948_set_accel_fs(struct device *dev, enum icm20948_accel_fs accel_fs)
{
	struct icm20948_data *data = (struct icm20948_data *)dev->driver_data;

	/* set default fullscale range for gyro */
	if (data->hw_tf->update_reg(data, ICM20948_REG_GYRO_CONFIG_1,
				    ICM20948_ACCEL_MASK,
				    accel_fs)) {
		LOG_DBG("failed to set acceleromter full-scale");
		return -EIO;
	}
	data->accel_fs = accel_fs;
	return 0;
}

static inline int icm20948_set_gyro_fs(struct device *dev, enum icm20948_gyro_fs gyro_fs)
{
	struct icm20948_data *data = (struct icm20948_data *)dev->driver_data;

	/* set default fullscale range for acc */
	if (data->hw_tf->update_reg(data, ICM20948_REG_ACCEL_CONFIG,
				    ICM20948_ACCEL_MASK,
				    gyro_fs)) {
		return -EIO;
	}
	data->gyro_fs = gyro_fs;
	return 0;
}

#ifdef CONFIG_ICM20948_ACCEL_RANGE_RUNTIME
static const u16_t icm20948_accel_fs_map[] = { 2, 4, 8, 16 };
static int icm20948_accel_range_set(struct device *dev, s32_t range)
{
	for (size_t i = 0; i < ARRAY_SIZE(icm20948_accel_fs_map); i++) {
		if (range == icm20948_accel_fs_map[i]) {
			return icm20948_set_accel_fs(dev, i);
		}
	}
	return -EINVAL;
};
#endif


#ifdef CONFIG_ICM20948_GYRO_RANGE_RUNTIME
static const u16_t icm20948_gyro_fs_map[] = { 250, 500, 1000, 2000 };
static int icm20948_gyro_range_set(struct device *dev, s32_t range)
{
	for (size_t i = 0; i < ARRAY_SIZE(icm20948_gyro_fs_map); i++) {
		if (range == icm20948_gyro_fs_map[i]) {
			return icm20948_set_gyro_fs(dev, i);
		}
	}
	return -EINVAL;
}
#endif



static int icm20948_accel_config(struct device *dev, enum sensor_channel chan,
				 enum sensor_attribute attr,
				 const struct sensor_value *val)
{
	switch (attr) {
#ifdef CONFIG_ICM20948_ACCEL_RANGE_RUNTIME
	case SENSOR_ATTR_FULL_SCALE:
		return icm20948_accel_range_set(dev, sensor_ms2_to_g(val));
#endif
	default:
		LOG_WRN("attr_set() not supported on this channel.");
		return -ENOTSUP;
	}
}

static int icm20948_gyro_config(struct device *dev, enum sensor_channel chan,
				enum sensor_attribute attr,
				const struct sensor_value *val)
{
	switch (attr) {
#ifdef CONFIG_ICM20948_GYRO_RANGE_RUNTIME
	case SENSOR_ATTR_FULL_SCALE:
		return icm20948_gyro_range_set(dev, sensor_rad_to_degrees(val));
#endif
	default:
		LOG_WRN("attr_set() not supported on this channel.");
		return -ENOTSUP;
	}
}

static int icm20948_attr_set(struct device *dev, enum sensor_channel chan,
			     enum sensor_attribute attr,
			     const struct sensor_value *val)
{
	switch (chan) {
	case SENSOR_CHAN_GYRO_X:
	case SENSOR_CHAN_GYRO_Y:
	case SENSOR_CHAN_GYRO_Z:
	case SENSOR_CHAN_GYRO_XYZ:
		return icm20948_accel_config(dev, chan, attr, val);
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ:
		return icm20948_gyro_config(dev, chan, attr, val);
	default:
		LOG_WRN("attr_set() not supported on this channel.");
		return -ENOTSUP;
	}
	return 0;
}

static inline void icm20948_accel_convert(struct sensor_value *val, int raw_val,
					  enum icm20948_accel_fs sensitivity)
{
	/* Sensitivity is exposed in LSB/g */
	double dval = raw_val / ((double)(16384 >> sensitivity));

	val->val1 = (s32_t)dval;
	val->val2 = (((s32_t)(dval * 1000)) % 1000) * 1000;
}


static inline void icm20948_gyro_convert(struct sensor_value *val, int raw_val,
					 enum icm20948_gyro_fs sensitivity)
{
	/* Sensitivity is exposed in LSB/dps */
	double dval = raw_val / ((double)(131.0 / (sensitivity + 1)));

	val->val1 = (s32_t)dval;
	val->val2 = (((s32_t)(dval * 1000)) % 1000) * 1000;
}


static int icm20948_channel_get(struct device *dev, enum sensor_channel chan,
				struct sensor_value *val)
{
	struct icm20948_data *data = dev->driver_data;

	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
		icm20948_accel_convert(val, data->acc[0], data->accel_fs);
		break;
	case SENSOR_CHAN_ACCEL_Y:
		icm20948_accel_convert(val, data->acc[1], data->accel_fs);
		break;
	case SENSOR_CHAN_ACCEL_Z:
		icm20948_accel_convert(val, data->acc[2], data->accel_fs);
		break;
	case SENSOR_CHAN_ACCEL_XYZ:
		icm20948_accel_convert(val, data->acc[0], data->accel_fs);
		icm20948_accel_convert(val + 1, data->acc[1], data->accel_fs);
		icm20948_accel_convert(val + 2, data->acc[2], data->accel_fs);
		break;
	case SENSOR_CHAN_GYRO_X:
		icm20948_gyro_convert(val, data->gyro[0], data->gyro_fs);
		break;
	case SENSOR_CHAN_GYRO_Y:
		icm20948_gyro_convert(val, data->gyro[1], data->gyro_fs);
		break;
	case SENSOR_CHAN_GYRO_Z:
		icm20948_gyro_convert(val, data->gyro[2], data->gyro_fs);
		break;
	case SENSOR_CHAN_GYRO_XYZ:
		icm20948_gyro_convert(val, data->gyro[0], data->gyro_fs);
		icm20948_gyro_convert(val, data->gyro[1], data->gyro_fs);
		icm20948_gyro_convert(val, data->gyro[2], data->gyro_fs);
		break;
	case SENSOR_CHAN_DIE_TEMP:
	case SENSOR_CHAN_AMBIENT_TEMP:
		break;
	default:
		break;
	}
	return 0;
}



static int icm20948_sample_fetch(struct device *dev, enum sensor_channel chan)
{
	struct icm20948_data *data = dev->driver_data;

	union {
		u8_t raw[6];
		struct {
			s16_t axis[3];
		};
	} buf __aligned(2);

	union {
		u8_t raw[2];
		struct {
			s16_t temp;
		};
	} buf2 __aligned(2);


	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ: {
		if (data->hw_tf->read_data(data, ICM20948_REG_ACCEL_XOUT_H_SH,
					   buf.raw, sizeof(buf))) {
			LOG_DBG("Failed to fetch raw data samples from accel");
			return -EIO;
		}
		data->acc[0] = sys_be16_to_cpu(buf.axis[0]);
		data->acc[1] = sys_be16_to_cpu(buf.axis[1]);
		data->acc[2] = sys_be16_to_cpu(buf.axis[2]);
	}
	break;
	case SENSOR_CHAN_GYRO_X:
	case SENSOR_CHAN_GYRO_Y:
	case SENSOR_CHAN_GYRO_Z:
	case SENSOR_CHAN_GYRO_XYZ: {

		if (data->hw_tf->read_data(data, ICM20948_REG_GYRO_XOUT_H_SH,
					   buf.raw, sizeof(buf))) {
			LOG_DBG("Failed to fetch raw data samples from gyro");
			return -EIO;
		}
		data->gyro[0] = sys_be16_to_cpu(buf.axis[0]);
		data->gyro[1] = sys_be16_to_cpu(buf.axis[1]);
		data->gyro[2] = sys_be16_to_cpu(buf.axis[2]);
		return 0;
	}
	break;
	case SENSOR_CHAN_AMBIENT_TEMP:
		data->temp = sys_be16_to_cpu(buf2.temp);
		break;
	case SENSOR_CHAN_ALL: {
		if (icm20948_sample_fetch(dev, SENSOR_CHAN_ACCEL_XYZ)) {
			return -EIO;
		}
		if (icm20948_sample_fetch(dev, SENSOR_CHAN_GYRO_XYZ)) {
			return -EIO;
		}
		if (icm20948_sample_fetch(dev, SENSOR_CHAN_AMBIENT_TEMP)) {
			return -EIO;
		}
	}
	break;
	default:
		return -ENOTSUP;
	}
	return 0;
}

int icm20948_init(struct device *dev)
{
	struct icm20948_data *data = (struct icm20948_data *)dev->driver_data;

	data->bus = device_get_binding(DT_TDK_ICM20948_0_BUS_NAME);
	data->bank = 0;
	if (!data->bus) {
		LOG_DBG("master not found: %s", DT_TDK_ICM20948_0_BUS_NAME);
		return -EINVAL;
	}

	#if defined(DT_TDK_ICM20948_BUS_SPI)
	icm20948_spi_init(dev);
	#elif defined(DT_TDK_ICM20948_BUS_I2C)
	icm20948_i2c_init(dev);
	#else
	#error "BUS MACRO NOT DEFINED IN DTS"
	#endif

	u8_t tmp;

	/* verify chip ID */
	if (data->hw_tf->read_reg(data, ICM20948_REG_WHO_AM_I, &tmp)) {
		LOG_ERR("Failed to read chip ID");
		return -EIO;
	}

	if (tmp != ICM20948_WHO_AM_I) {
		LOG_ERR("Invalid Chip ID Expects 0x%x -- 0x%x", ICM20948_WHO_AM_I, tmp);
	}

	/* set gyro and accel config */
	icm20948_set_gyro_fs(dev, ICM20948_GYRO_FS_DEFAULT);
	icm20948_set_accel_fs(dev, ICM20948_ACCEL_FS_DEFAULT);

	/* set default fullscale range for gyro */
	if (data->hw_tf->update_reg(data, ICM20948_REG_GYRO_CONFIG_1,
				    ICM20948_GYRO_MASK,
				    ICM20948_GYRO_FS_DEFAULT)) {
		return -EIO;
	}

	/* wake up chip */
	if (data->hw_tf->update_reg(data, ICM20948_REG_PWR_MGMT_1, BIT(6), 0)) {
		LOG_ERR("Failed to wake up chip.");
		return -EIO;
	}

	return 0;
}

struct icm20948_data icm20948_data;

static const struct sensor_driver_api icm20948_driver_api = {
	.attr_set = icm20948_attr_set,
	.sample_fetch = icm20948_sample_fetch,
	.channel_get = icm20948_channel_get
};

DEVICE_AND_API_INIT(icm20948, DT_TDK_ICM20948_0_LABEL, icm20948_init,
		    &icm20948_data, NULL, POST_KERNEL,
		    CONFIG_SENSOR_INIT_PRIORITY, &icm20948_driver_api);
