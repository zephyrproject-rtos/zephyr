/* Bosch BMI160 inertial measurement unit driver
 *
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * http://ae-bst.resource.bosch.com/media/_tech/media/datasheets/BST-BMI160-DS000-07.pdf
 */

#include <zephyr/init.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/logging/log.h>

#include "bmi160.h"

LOG_MODULE_DECLARE(BMI160, CONFIG_SENSOR_LOG_LEVEL);

#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 0
#warning "BMI160 driver enabled without any devices"
#endif

#if !defined(CONFIG_BMI160_ACCEL_PMU_SUSPEND)
/*
 * Accelerometer offset scale, taken from pg. 79, converted to micro m/s^2:
 *	3.9 * 9.80665 * 1000
 */
#define BMI160_ACC_OFS_LSB		38246
static int bmi160_acc_ofs_set(const struct device *dev,
			      enum sensor_channel chan,
			      const struct sensor_value *ofs)
{
	uint8_t reg_addr[] = {
		BMI160_REG_OFFSET_ACC_X,
		BMI160_REG_OFFSET_ACC_Y,
		BMI160_REG_OFFSET_ACC_Z
	};
	int i;
	int32_t ofs_u;
	int8_t reg_val;

	/* we need the offsets for all axis */
	if (chan != SENSOR_CHAN_ACCEL_XYZ) {
		return -ENOTSUP;
	}

	for (i = 0; i < BMI160_AXES; i++, ofs++) {
		/* convert offset to micro m/s^2 */
		ofs_u = ofs->val1 * 1000000ULL + ofs->val2;
		reg_val = ofs_u / BMI160_ACC_OFS_LSB;

		if (bmi160_byte_write(dev, reg_addr[i], reg_val) < 0) {
			return -EIO;
		}
	}

	/* activate accel HW compensation */
	return bmi160_reg_field_update(dev, BMI160_REG_OFFSET_EN, BMI160_ACC_OFS_EN_POS,
				       BIT(BMI160_ACC_OFS_EN_POS), 1);
}

static int bmi160_acc_calibrate(const struct device *dev, enum sensor_channel chan,
				const struct sensor_value *xyz_calib_value)
{
	struct bmi160_data *data = dev->data;
	uint8_t foc_pos[] = {
		BMI160_FOC_ACC_X_POS,
		BMI160_FOC_ACC_Y_POS,
		BMI160_FOC_ACC_Z_POS,
	};
	int i;
	uint8_t reg_val = 0U;

	/* Calibration has to be done in normal mode. */
	if (data->pmu_sts.acc != BMI160_PMU_NORMAL) {
		return -ENOTSUP;
	}

	/*
	 * Hardware calibration is done knowing the expected values on all axis.
	 */
	if (chan != SENSOR_CHAN_ACCEL_XYZ) {
		return -ENOTSUP;
	}

	for (i = 0; i < BMI160_AXES; i++, xyz_calib_value++) {
		int32_t accel_g;
		uint8_t accel_val;

		accel_g = sensor_ms2_to_g(xyz_calib_value);
		if (accel_g == 0) {
			accel_val = 3U;
		} else if (accel_g == 1) {
			accel_val = 1U;
		} else if (accel_g == -1) {
			accel_val = 2U;
		} else {
			accel_val = 0U;
		}
		reg_val |= (accel_val << foc_pos[i]);
	}

	if (bmi160_do_calibration(dev, reg_val) < 0) {
		return -EIO;
	}

	/* activate accel HW compensation */
	return bmi160_reg_field_update(dev, BMI160_REG_OFFSET_EN, BMI160_ACC_OFS_EN_POS,
				       BIT(BMI160_ACC_OFS_EN_POS), 1);
}

static int bmi160_acc_config(const struct device *dev, enum sensor_channel chan,
			     enum sensor_attribute attr, const struct sensor_value *val)
{
	switch (attr) {
#if defined(CONFIG_BMI160_ACCEL_RANGE_RUNTIME)
	case SENSOR_ATTR_FULL_SCALE:
		return bmi160_acc_range_set(dev, sensor_ms2_to_g(val));
#endif
#if defined(CONFIG_BMI160_ACCEL_ODR_RUNTIME)
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		return bmi160_acc_odr_set(dev, val->val1, val->val2 / 1000);
#endif
	case SENSOR_ATTR_OFFSET:
		return bmi160_acc_ofs_set(dev, chan, val);
	case SENSOR_ATTR_CALIB_TARGET:
		return bmi160_acc_calibrate(dev, chan, val);
#if defined(CONFIG_BMI160_TRIGGER)
	case SENSOR_ATTR_SLOPE_TH:
	case SENSOR_ATTR_SLOPE_DUR:
		return bmi160_acc_slope_config(dev, attr, val);
#endif
	default:
		LOG_DBG("Accel attribute not supported.");
		return -ENOTSUP;
	}

	return 0;
}
#endif /* !defined(CONFIG_BMI160_ACCEL_PMU_SUSPEND) */

#if !defined(CONFIG_BMI160_GYRO_PMU_SUSPEND)
/*
 * Gyro offset scale, taken from pg. 79, converted to micro rad/s:
 *		0.061 * (pi / 180) * 1000000, where pi = 3.141592
 */
#define BMI160_GYR_OFS_LSB 1065
static int bmi160_gyr_ofs_set(const struct device *dev, enum sensor_channel chan,
			      const struct sensor_value *ofs)
{
	struct {
		uint8_t lsb_addr;
		uint8_t msb_pos;
	} ofs_desc[] = {
		{ BMI160_REG_OFFSET_GYR_X, BMI160_GYR_MSB_OFS_X_POS },
		{ BMI160_REG_OFFSET_GYR_Y, BMI160_GYR_MSB_OFS_Y_POS },
		{ BMI160_REG_OFFSET_GYR_Z, BMI160_GYR_MSB_OFS_Z_POS },
	};
	int i;
	int32_t ofs_u;
	int16_t val;

	/* we need the offsets for all axis */
	if (chan != SENSOR_CHAN_GYRO_XYZ) {
		return -ENOTSUP;
	}

	for (i = 0; i < BMI160_AXES; i++, ofs++) {
		/* convert offset to micro rad/s */
		ofs_u = ofs->val1 * 1000000ULL + ofs->val2;

		val = ofs_u / BMI160_GYR_OFS_LSB;

		/*
		 * The gyro offset is a 10 bit two-complement value. Make sure
		 * the passed value is within limits.
		 */
		if (val < -512 || val > 512) {
			return -EINVAL;
		}

		/* write the LSB */
		if (bmi160_byte_write(dev, ofs_desc[i].lsb_addr, val & 0xff) < 0) {
			return -EIO;
		}

		/* write the MSB */
		if (bmi160_reg_field_update(dev, BMI160_REG_OFFSET_EN, ofs_desc[i].msb_pos,
					    0x3 << ofs_desc[i].msb_pos, (val >> 8) & 0x3) < 0) {
			return -EIO;
		}
	}

	/* activate gyro HW compensation */
	return bmi160_reg_field_update(dev, BMI160_REG_OFFSET_EN, BMI160_GYR_OFS_EN_POS,
				       BIT(BMI160_GYR_OFS_EN_POS), 1);
}

static int bmi160_gyr_calibrate(const struct device *dev, enum sensor_channel chan)
{
	struct bmi160_data *data = dev->data;

	ARG_UNUSED(chan);

	/* Calibration has to be done in normal mode. */
	if (data->pmu_sts.gyr != BMI160_PMU_NORMAL) {
		return -ENOTSUP;
	}

	if (bmi160_do_calibration(dev, BIT(BMI160_FOC_GYR_EN_POS)) < 0) {
		return -EIO;
	}

	/* activate gyro HW compensation */
	return bmi160_reg_field_update(dev, BMI160_REG_OFFSET_EN, BMI160_GYR_OFS_EN_POS,
				       BIT(BMI160_GYR_OFS_EN_POS), 1);
}

static int bmi160_gyr_config(const struct device *dev, enum sensor_channel chan,
			     enum sensor_attribute attr, const struct sensor_value *val)
{
	switch (attr) {
#if defined(CONFIG_BMI160_GYRO_RANGE_RUNTIME)
	case SENSOR_ATTR_FULL_SCALE:
		return bmi160_gyr_range_set(dev, sensor_rad_to_degrees(val));
#endif
#if defined(CONFIG_BMI160_GYRO_ODR_RUNTIME)
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		return bmi160_gyr_odr_set(dev, val->val1, val->val2 / 1000);
#endif
	case SENSOR_ATTR_OFFSET:
		return bmi160_gyr_ofs_set(dev, chan, val);

	case SENSOR_ATTR_CALIB_TARGET:
		return bmi160_gyr_calibrate(dev, chan);

	default:
		LOG_DBG("Gyro attribute not supported.");
		return -ENOTSUP;
	}

	return 0;
}
#endif /* !defined(CONFIG_BMI160_GYRO_PMU_SUSPEND) */

static int bmi160_attr_set(const struct device *dev, enum sensor_channel chan,
			   enum sensor_attribute attr,
			   const struct sensor_value *val)
{
	switch (chan) {
#if !defined(CONFIG_BMI160_GYRO_PMU_SUSPEND)
	case SENSOR_CHAN_GYRO_X:
	case SENSOR_CHAN_GYRO_Y:
	case SENSOR_CHAN_GYRO_Z:
	case SENSOR_CHAN_GYRO_XYZ:
		return bmi160_gyr_config(dev, chan, attr, val);
#endif
#if !defined(CONFIG_BMI160_ACCEL_PMU_SUSPEND)
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ:
		return bmi160_acc_config(dev, chan, attr, val);
#endif
	default:
		LOG_DBG("attr_set() not supported on this channel.");
		return -ENOTSUP;
	}

	return 0;
}

static int bmi160_sample_fetch_impl(const struct device *dev,
				    enum sensor_channel chan)
{
	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);
	return bmi160_sample_fetch(dev);
}

static void bmi160_to_fixed_point(int16_t raw_val, uint16_t scale,
				  struct sensor_value *val)
{
	int32_t converted_val;

	/*
	 * maximum converted value we can get is: max(raw_val) * max(scale)
	 *	max(raw_val) = +/- 2^15
	 *	max(scale) = 4785
	 *	max(converted_val) = 156794880 which is less than 2^31
	 */
	converted_val = raw_val * scale;
	val->val1 = converted_val / 1000000;
	val->val2 = converted_val % 1000000;
}

static void bmi160_channel_convert(enum sensor_channel chan,
				   uint16_t scale,
				   uint16_t *raw_xyz,
				   struct sensor_value *val)
{
	int i;
	uint8_t ofs_start, ofs_stop;

	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_GYRO_X:
		ofs_start = ofs_stop = 0U;
		break;
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_GYRO_Y:
		ofs_start = ofs_stop = 1U;
		break;
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_GYRO_Z:
		ofs_start = ofs_stop = 2U;
		break;
	default:
		ofs_start = 0U; ofs_stop = 2U;
		break;
	}

	for (i = ofs_start; i <= ofs_stop ; i++, val++) {
		bmi160_to_fixed_point(raw_xyz[i], scale, val);
	}
}

#if !defined(CONFIG_BMI160_GYRO_PMU_SUSPEND)
static inline void bmi160_gyr_channel_get(const struct device *dev,
					  enum sensor_channel chan,
					  struct sensor_value *val)
{
	struct bmi160_data *data = dev->data;

	bmi160_channel_convert(chan, data->scale.gyr, data->sample.gyr, val);
}
#endif

#if !defined(CONFIG_BMI160_ACCEL_PMU_SUSPEND)
static inline void bmi160_acc_channel_get(const struct device *dev,
					  enum sensor_channel chan,
					  struct sensor_value *val)
{
	struct bmi160_data *data = dev->data;

	bmi160_channel_convert(chan, data->scale.acc, data->sample.acc, val);
}
#endif

static int bmi160_temp_channel_get(const struct device *dev,
				   struct sensor_value *val)
{
	uint16_t temp_raw = 0U;
	int32_t temp_micro = 0;
	struct bmi160_data *data = dev->data;

	if (data->pmu_sts.raw == 0U) {
		return -EINVAL;
	}

	if (bmi160_word_read(dev, BMI160_REG_TEMPERATURE0, &temp_raw) < 0) {
		return -EIO;
	}

	/* the scale is 1/2^9/LSB = 1953 micro degrees */
	temp_micro = BMI160_TEMP_OFFSET * 1000000ULL + temp_raw * 1953ULL;

	val->val1 = temp_micro / 1000000ULL;
	val->val2 = temp_micro % 1000000ULL;

	return 0;
}

static int bmi160_channel_get(const struct device *dev,
			      enum sensor_channel chan,
			      struct sensor_value *val)
{
	switch (chan) {
#if !defined(CONFIG_BMI160_GYRO_PMU_SUSPEND)
	case SENSOR_CHAN_GYRO_X:
	case SENSOR_CHAN_GYRO_Y:
	case SENSOR_CHAN_GYRO_Z:
	case SENSOR_CHAN_GYRO_XYZ:
		bmi160_gyr_channel_get(dev, chan, val);
		return 0;
#endif
#if !defined(CONFIG_BMI160_ACCEL_PMU_SUSPEND)
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ:
		bmi160_acc_channel_get(dev, chan, val);
		return 0;
#endif
	case SENSOR_CHAN_DIE_TEMP:
		return bmi160_temp_channel_get(dev, val);
	default:
		LOG_DBG("Channel not supported.");
		return -ENOTSUP;
	}

	return 0;
}

static const struct sensor_driver_api bmi160_api = {
	.attr_set = bmi160_attr_set,
#ifdef CONFIG_BMI160_TRIGGER
	.trigger_set = bmi160_trigger_set,
#endif
	.sample_fetch = bmi160_sample_fetch_impl,
	.channel_get = bmi160_channel_get,
};

#if defined(CONFIG_BMI160_TRIGGER)
#define BMI160_TRIGGER_CFG(inst) \
	.interrupt = GPIO_DT_SPEC_INST_GET(inst, int_gpios),
#else
#define BMI160_TRIGGER_CFG(inst)
#endif

#define BMI160_DEVICE_INIT(inst)					\
	SENSOR_DEVICE_DT_INST_DEFINE(inst, bmi160_init, NULL,		\
			      &bmi160_data_##inst, &bmi160_cfg_##inst,	\
			      POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,	\
			      &bmi160_api);

/* Instantiation macros used when a device is on a SPI bus */
#define BMI160_DEFINE_SPI(inst)						   \
	static struct bmi160_data bmi160_data_##inst;			   \
	static const struct bmi160_cfg bmi160_cfg_##inst = {		   \
		.bus.spi = SPI_DT_SPEC_INST_GET(inst, SPI_WORD_SET(8), 0), \
		.bus_io = &bmi160_bus_spi_io,				   \
		BMI160_TRIGGER_CFG(inst)				   \
	};								   \
	BMI160_DEVICE_INIT(inst)

/* Instantiation macros used when a device is on an I2C bus */
#define BMI160_CONFIG_I2C(inst)			       \
	{					       \
		.bus.i2c = I2C_DT_SPEC_INST_GET(inst), \
		.bus_io = &bmi160_bus_i2c_io,	       \
	}

#define BMI160_DEFINE_I2C(inst)							    \
	static struct bmi160_data bmi160_data_##inst;				    \
	static const struct bmi160_cfg bmi160_cfg_##inst = BMI160_CONFIG_I2C(inst); \
	BMI160_DEVICE_INIT(inst)

/*
 * Main instantiation macro. Use of COND_CODE_1() selects the right
 * bus-specific macro at preprocessor time.
 */
#define BMI160_DEFINE(inst)						\
	COND_CODE_1(DT_INST_ON_BUS(inst, spi),				\
		    (BMI160_DEFINE_SPI(inst)),				\
		    (BMI160_DEFINE_I2C(inst)))

DT_INST_FOREACH_STATUS_OKAY(BMI160_DEFINE)
