/*
 * Copyright (c) 2021, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT invensense_mpu9250

#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>
#include <zephyr/devicetree.h>

#include "mpu9250.h"

#ifdef CONFIG_MPU9250_MAGN_EN
#include "ak8963.h"
#endif

LOG_MODULE_REGISTER(MPU9250, CONFIG_SENSOR_LOG_LEVEL);


#define MPU9250_REG_CHIP_ID		0x75
#define MPU9250_CHIP_ID			0x71

#define MPU9250_REG_SR_DIV		0x19

#define MPU9250_REG_CONFIG		0x1A
#define MPU9250_GYRO_DLPF_MAX		7

#define MPU9250_REG_GYRO_CFG		0x1B
#define MPU9250_GYRO_FS_SHIFT		3
#define MPU9250_GYRO_FS_MAX		3

#define MPU9250_REG_ACCEL_CFG		0x1C
#define MPU9250_ACCEL_FS_SHIFT		3
#define MPU9250_ACCEL_FS_MAX		3

#define MPU9250_REG_ACCEL_CFG2		0x1D
#define MPU9250_ACCEL_DLPF_MAX		7

#define MPU9250_REG_DATA_START		0x3B

#define MPU0259_TEMP_SENSITIVITY	334
#define MPU9250_TEMP_OFFSET		21

#define MPU9250_REG_PWR_MGMT1		0x6B
#define MPU9250_SLEEP_EN		BIT(6)


#ifdef CONFIG_MPU9250_MAGN_EN
#define MPU9250_READ_BUF_SIZE 11
#else
#define MPU9250_READ_BUF_SIZE 7
#endif


/* see "Accelerometer Measurements" section from register map description */
static void mpu9250_convert_accel(struct sensor_value *val, int16_t raw_val,
				  uint16_t sensitivity_shift)
{
	int64_t conv_val;

	conv_val = ((int64_t)raw_val * SENSOR_G) >> sensitivity_shift;
	val->val1 = conv_val / 1000000;
	val->val2 = conv_val % 1000000;
}

/* see "Gyroscope Measurements" section from register map description */
static void mpu9250_convert_gyro(struct sensor_value *val, int16_t raw_val,
				 uint16_t sensitivity_x10)
{
	int64_t conv_val;

	conv_val = ((int64_t)raw_val * SENSOR_PI * 10) /
		   (sensitivity_x10 * 180U);
	val->val1 = conv_val / 1000000;
	val->val2 = conv_val % 1000000;
}

/* see "Temperature Measurement" section from register map description */
static inline void mpu9250_convert_temp(struct sensor_value *val,
					int16_t raw_val)
{
	/* Temp[*C] = (raw / sensitivity) + offset */
	val->val1 = (raw_val / MPU0259_TEMP_SENSITIVITY) + MPU9250_TEMP_OFFSET;
	val->val2 = (((int64_t)(raw_val % MPU0259_TEMP_SENSITIVITY) * 1000000)
				/ MPU0259_TEMP_SENSITIVITY);

	if (val->val2 < 0) {
		val->val1--;
		val->val2 += 1000000;
	} else if (val->val2 >= 1000000) {
		val->val1++;
		val->val2 -= 1000000;
	}
}

static int mpu9250_channel_get(const struct device *dev,
			       enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct mpu9250_data *drv_data = dev->data;
#ifdef CONFIG_MPU9250_MAGN_EN
	int ret;
#endif

	switch (chan) {
	case SENSOR_CHAN_ACCEL_XYZ:
		mpu9250_convert_accel(val, drv_data->accel_x,
				      drv_data->accel_sensitivity_shift);
		mpu9250_convert_accel(val + 1, drv_data->accel_y,
				      drv_data->accel_sensitivity_shift);
		mpu9250_convert_accel(val + 2, drv_data->accel_z,
				      drv_data->accel_sensitivity_shift);
		break;
	case SENSOR_CHAN_ACCEL_X:
		mpu9250_convert_accel(val, drv_data->accel_x,
				      drv_data->accel_sensitivity_shift);
		break;
	case SENSOR_CHAN_ACCEL_Y:
		mpu9250_convert_accel(val, drv_data->accel_y,
				      drv_data->accel_sensitivity_shift);
		break;
	case SENSOR_CHAN_ACCEL_Z:
		mpu9250_convert_accel(val, drv_data->accel_z,
				      drv_data->accel_sensitivity_shift);
		break;
	case SENSOR_CHAN_GYRO_XYZ:
		mpu9250_convert_gyro(val, drv_data->gyro_x,
				     drv_data->gyro_sensitivity_x10);
		mpu9250_convert_gyro(val + 1, drv_data->gyro_y,
				     drv_data->gyro_sensitivity_x10);
		mpu9250_convert_gyro(val + 2, drv_data->gyro_z,
				     drv_data->gyro_sensitivity_x10);
		break;
	case SENSOR_CHAN_GYRO_X:
		mpu9250_convert_gyro(val, drv_data->gyro_x,
				     drv_data->gyro_sensitivity_x10);
		break;
	case SENSOR_CHAN_GYRO_Y:
		mpu9250_convert_gyro(val, drv_data->gyro_y,
				     drv_data->gyro_sensitivity_x10);
		break;
	case SENSOR_CHAN_GYRO_Z:
		mpu9250_convert_gyro(val, drv_data->gyro_z,
				     drv_data->gyro_sensitivity_x10);
		break;
#ifdef CONFIG_MPU9250_MAGN_EN
	case SENSOR_CHAN_MAGN_XYZ:
		ret = ak8963_convert_magn(val, drv_data->magn_x,
					  drv_data->magn_scale_x,
					  drv_data->magn_st2);
		if (ret < 0) {
			return ret;
		}
		ret = ak8963_convert_magn(val + 1, drv_data->magn_y,
					  drv_data->magn_scale_y,
					  drv_data->magn_st2);
		if (ret < 0) {
			return ret;
		}
		ret = ak8963_convert_magn(val + 2, drv_data->magn_z,
					  drv_data->magn_scale_z,
					  drv_data->magn_st2);
		return ret;
	case SENSOR_CHAN_MAGN_X:
		return ak8963_convert_magn(val, drv_data->magn_x,
				    drv_data->magn_scale_x,
				    drv_data->magn_st2);
	case SENSOR_CHAN_MAGN_Y:
		return ak8963_convert_magn(val, drv_data->magn_y,
				    drv_data->magn_scale_y,
				    drv_data->magn_st2);
	case SENSOR_CHAN_MAGN_Z:
		return ak8963_convert_magn(val, drv_data->magn_z,
				    drv_data->magn_scale_z,
				    drv_data->magn_st2);
	case SENSOR_CHAN_DIE_TEMP:
		mpu9250_convert_temp(val, drv_data->temp);
		break;
#endif
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int mpu9250_sample_fetch(const struct device *dev,
				enum sensor_channel chan)
{
	struct mpu9250_data *drv_data = dev->data;
	const struct mpu9250_config *cfg = dev->config;
	int16_t buf[MPU9250_READ_BUF_SIZE];
	int ret;

	ret = i2c_burst_read_dt(&cfg->i2c,
				MPU9250_REG_DATA_START, (uint8_t *)buf,
				sizeof(buf));
	if (ret < 0) {
		LOG_ERR("Failed to read data sample.");
		return ret;
	}

	drv_data->accel_x = sys_be16_to_cpu(buf[0]);
	drv_data->accel_y = sys_be16_to_cpu(buf[1]);
	drv_data->accel_z = sys_be16_to_cpu(buf[2]);
	drv_data->temp = sys_be16_to_cpu(buf[3]);
	drv_data->gyro_x = sys_be16_to_cpu(buf[4]);
	drv_data->gyro_y = sys_be16_to_cpu(buf[5]);
	drv_data->gyro_z = sys_be16_to_cpu(buf[6]);
#ifdef CONFIG_MPU9250_MAGN_EN
	drv_data->magn_x = sys_be16_to_cpu(buf[7]);
	drv_data->magn_y = sys_be16_to_cpu(buf[8]);
	drv_data->magn_z = sys_be16_to_cpu(buf[9]);
	drv_data->magn_st2 = ((uint8_t *)buf)[20];
	LOG_DBG("magn_st2: %u", drv_data->magn_st2);
#endif

	return 0;
}

static const struct sensor_driver_api mpu9250_driver_api = {
#if CONFIG_MPU9250_TRIGGER
	.trigger_set = mpu9250_trigger_set,
#endif
	.sample_fetch = mpu9250_sample_fetch,
	.channel_get = mpu9250_channel_get,
};

/* measured in degrees/sec x10 to avoid floating point */
static const uint16_t mpu9250_gyro_sensitivity_x10[] = {
	1310, 655, 328, 164
};

static int mpu9250_init(const struct device *dev)
{
	struct mpu9250_data *drv_data = dev->data;
	const struct mpu9250_config *cfg = dev->config;
	uint8_t id;
	int ret;

	if (!device_is_ready(cfg->i2c.bus)) {
		LOG_ERR("I2C dev %s not ready", cfg->i2c.bus->name);
		return -ENODEV;
	}

	/* check chip ID */
	ret = i2c_reg_read_byte_dt(&cfg->i2c, MPU9250_REG_CHIP_ID, &id);
	if (ret < 0) {
		LOG_ERR("Failed to read chip ID.");
		return ret;
	}

	if (id != MPU9250_CHIP_ID) {
		LOG_ERR("Invalid chip ID.");
		return -ENOTSUP;
	}

	/* wake up chip */
	ret = i2c_reg_update_byte_dt(&cfg->i2c,
				     MPU9250_REG_PWR_MGMT1,
				     MPU9250_SLEEP_EN, 0);
	if (ret < 0) {
		LOG_ERR("Failed to wake up chip.");
		return ret;
	}

	if (cfg->accel_fs > MPU9250_ACCEL_FS_MAX) {
		LOG_ERR("Accel FS is too big: %d", cfg->accel_fs);
		return -EINVAL;
	}

	ret = i2c_reg_write_byte_dt(&cfg->i2c, MPU9250_REG_ACCEL_CFG,
				    cfg->accel_fs << MPU9250_ACCEL_FS_SHIFT);
	if (ret < 0) {
		LOG_ERR("Failed to write accel full-scale range.");
		return ret;
	}
	drv_data->accel_sensitivity_shift = 14 - cfg->accel_fs;

	if (cfg->gyro_fs > MPU9250_GYRO_FS_MAX) {
		LOG_ERR("Gyro FS is too big: %d", cfg->accel_fs);
		return -EINVAL;
	}

	ret = i2c_reg_write_byte_dt(&cfg->i2c, MPU9250_REG_GYRO_CFG,
				    cfg->gyro_fs << MPU9250_GYRO_FS_SHIFT);
	if (ret < 0) {
		LOG_ERR("Failed to write gyro full-scale range.");
		return ret;
	}

	if (cfg->gyro_dlpf > MPU9250_GYRO_DLPF_MAX) {
		LOG_ERR("Gyro DLPF is too big: %d", cfg->gyro_dlpf);
		return -EINVAL;
	}

	ret = i2c_reg_write_byte_dt(&cfg->i2c, MPU9250_REG_CONFIG,
				    cfg->gyro_dlpf);
	if (ret < 0) {
		LOG_ERR("Failed to write gyro digital LPF settings.");
		return ret;
	}

	if (cfg->accel_dlpf > MPU9250_ACCEL_DLPF_MAX) {
		LOG_ERR("Accel DLPF is too big: %d", cfg->accel_dlpf);
		return -EINVAL;
	}

	ret = i2c_reg_write_byte_dt(&cfg->i2c, MPU9250_REG_ACCEL_CFG2,
				    cfg->gyro_dlpf);
	if (ret < 0) {
		LOG_ERR("Failed to write accel digital LPF settings.");
		return ret;
	}

	ret = i2c_reg_write_byte_dt(&cfg->i2c, MPU9250_REG_SR_DIV,
				    cfg->gyro_sr_div);
	if (ret < 0) {
		LOG_ERR("Failed to write gyro ODR divider.");
		return ret;
	}

	drv_data->gyro_sensitivity_x10 =
				mpu9250_gyro_sensitivity_x10[cfg->gyro_fs];

#ifdef CONFIG_MPU9250_MAGN_EN
	ret = ak8963_init(dev);
	if (ret < 0) {
		LOG_ERR("Failed to initialize AK8963.");
		return ret;
	}
#endif

#ifdef CONFIG_MPU9250_TRIGGER
	ret = mpu9250_init_interrupt(dev);
	if (ret < 0) {
		LOG_ERR("Failed to initialize interrupts.");
		return ret;
	}
#endif

	return 0;
}


#define INIT_MPU9250_INST(inst)						\
	static struct mpu9250_data mpu9250_data_##inst;			\
	static const struct mpu9250_config mpu9250_cfg_##inst = {	\
	.i2c = I2C_DT_SPEC_INST_GET(inst),				\
	.gyro_sr_div = DT_INST_PROP(inst, gyro_sr_div),			\
	.gyro_dlpf = DT_INST_ENUM_IDX(inst, gyro_dlpf),			\
	.gyro_fs = DT_INST_ENUM_IDX(inst, gyro_fs),			\
	.accel_fs = DT_INST_ENUM_IDX(inst, accel_fs),			\
	.accel_dlpf = DT_INST_ENUM_IDX(inst, accel_dlpf),		\
	IF_ENABLED(CONFIG_MPU9250_TRIGGER,				\
		  (.int_pin = GPIO_DT_SPEC_INST_GET(inst, irq_gpios)))	\
	};								\
									\
	DEVICE_DT_INST_DEFINE(inst, mpu9250_init, NULL,			\
			      &mpu9250_data_##inst, &mpu9250_cfg_##inst,\
			      POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,	\
			      &mpu9250_driver_api);

DT_INST_FOREACH_STATUS_OKAY(INIT_MPU9250_INST)
