/*
 * Copyright 2024 PHYTEC America, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT invensense_icm40627

#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/byteorder.h>
#include "icm40627.h"
#include "icm40627_reg.h"
#include "icm40627_trigger.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ICM40627, CONFIG_SENSOR_LOG_LEVEL);

/*
 * Gyro FS to scaling factor mapping.
 * See datasheet section 3.1 for details
 */
static const uint16_t icm40627_gyro_sensitivity_x10[] = {
	164,   /* BIT_GYRO_UI_FS_2000 */
	328,   /* BIT_GYRO_UI_FS_1000 */
	655,   /* BIT_GYRO_UI_FS_500 */
	1310,  /* BIT_GYRO_UI_FS_250 */
	2620,  /* BIT_GYRO_UI_FS_125 */
	5243,  /* BIT_GYRO_UI_FS_62_5 */
	10486, /* BIT_GYRO_UI_FS_31_25 */
	20972, /* BIT_GYRO_UI_FS_15_625 */
};

static int icm40627_set_accel_fs(const struct device *dev, uint16_t fs)
{
	const struct icm40627_config *cfg = dev->config;
	struct icm40627_data *data = dev->data;
	uint8_t temp;

	if ((fs > 16) || (fs < 2)) {
		LOG_ERR("Unsupported range");
		return -ENOTSUP;
	}

	if (fs > 8) {
		temp = BIT_ACCEL_UI_FS_16;
	} else if (fs > 4) {
		temp = BIT_ACCEL_UI_FS_8;
	} else if (fs > 2) {
		temp = BIT_ACCEL_UI_FS_4;
	} else {
		temp = BIT_ACCEL_UI_FS_2;
	}

	data->accel_sensitivity_shift = MIN_ACCEL_SENS_SHIFT;

	return cfg->bus_io->update(&cfg->bus, REG_ACCEL_CONFIG0, (uint8_t)MASK_ACCEL_UI_FS_SEL,
				   temp);
}

static int icm40627_set_gyro_fs(const struct device *dev, uint16_t fs)
{
	const struct icm40627_config *cfg = dev->config;
	struct icm40627_data *data = dev->data;
	uint8_t temp;

	if ((fs > 2000) || (fs < 250)) {
		LOG_ERR("Unsupported range");
		return -ENOTSUP;
	}

	if (fs > 1000) {
		temp = BIT_GYRO_UI_FS_2000;
	} else if (fs > 500) {
		temp = BIT_GYRO_UI_FS_1000;
	} else if (fs > 250) {
		temp = BIT_GYRO_UI_FS_500;
	} else if (fs > 125) {
		temp = BIT_GYRO_UI_FS_250;
	} else if (fs > 62) {
		temp = BIT_GYRO_UI_FS_125;
	} else if (fs > 31) {
		temp = BIT_GYRO_UI_FS_62;
	} else if (fs > 15) {
		temp = BIT_GYRO_UI_FS_31;
	} else {
		temp = BIT_GYRO_UI_FS_15;
	}

	data->gyro_sensitivity_x10 = icm40627_gyro_sensitivity_x10[temp];

	return cfg->bus_io->update(&cfg->bus, REG_GYRO_CONFIG0, (uint8_t)MASK_GYRO_UI_FS_SEL, temp);
}

static int icm40627_set_accel_odr(const struct device *dev, uint16_t rate)
{
	const struct icm40627_config *cfg = dev->config;
	uint8_t temp;

	if ((rate > 8000) || (rate < 1)) {
		LOG_ERR("Unsupported frequency");
		return -ENOTSUP;
	}

	if (rate > 4000) {
		temp = BIT_ACCEL_ODR_8000;
	} else if (rate > 2000) {
		temp = BIT_ACCEL_ODR_4000;
	} else if (rate > 1000) {
		temp = BIT_ACCEL_ODR_2000;
	} else if (rate > 500) {
		temp = BIT_ACCEL_ODR_1000;
	} else if (rate > 200) {
		temp = BIT_ACCEL_ODR_500;
	} else if (rate > 100) {
		temp = BIT_ACCEL_ODR_200;
	} else if (rate > 50) {
		temp = BIT_ACCEL_ODR_100;
	} else if (rate > 25) {
		temp = BIT_ACCEL_ODR_50;
	} else if (rate > 12) {
		temp = BIT_ACCEL_ODR_25;
	} else if (rate > 6) {
		temp = BIT_ACCEL_ODR_12;
	} else if (rate > 3) {
		temp = BIT_ACCEL_ODR_6;
	} else if (rate > 1) {
		temp = BIT_ACCEL_ODR_3;
	} else {
		temp = BIT_ACCEL_ODR_1;
	}

	return cfg->bus_io->update(&cfg->bus, REG_ACCEL_CONFIG0, (uint8_t)MASK_ACCEL_ODR, temp);
}

static int icm40627_set_gyro_odr(const struct device *dev, uint16_t rate)
{
	const struct icm40627_config *cfg = dev->config;
	uint8_t temp;

	if ((rate > 8000) || (rate < 12)) {
		LOG_ERR("Unsupported frequency");
		return -ENOTSUP;
	}

	if (rate > 4000) {
		temp = BIT_GYRO_ODR_8000;
	} else if (rate > 2000) {
		temp = BIT_GYRO_ODR_4000;
	} else if (rate > 1000) {
		temp = BIT_GYRO_ODR_2000;
	} else if (rate > 500) {
		temp = BIT_GYRO_ODR_1000;
	} else if (rate > 200) {
		temp = BIT_GYRO_ODR_500;
	} else if (rate > 100) {
		temp = BIT_GYRO_ODR_200;
	} else if (rate > 50) {
		temp = BIT_GYRO_ODR_100;
	} else if (rate > 25) {
		temp = BIT_GYRO_ODR_50;
	} else if (rate > 12) {
		temp = BIT_GYRO_ODR_25;
	} else {
		temp = BIT_GYRO_ODR_12;
	}

	return cfg->bus_io->update(&cfg->bus, REG_GYRO_CONFIG0, (uint8_t)MASK_GYRO_ODR, temp);
}

static int icm40627_sensor_init(const struct device *dev)
{
	int res;
	uint8_t value;
	const struct icm40627_config *cfg = dev->config;

	/* start up time for register read/write after POR is 1ms and supply ramp time is 3ms */
	k_msleep(3);

	/* perform a soft reset to ensure a clean slate, reset bit will auto-clear */
	res = cfg->bus_io->write(&cfg->bus, REG_DEVICE_CONFIG, BIT_SOFT_RESET);

	if (res) {
		LOG_ERR("write REG_SIGNAL_PATH_RESET failed");
		return res;
	}

	/* wait for soft reset to take effect */
	k_msleep(SOFT_RESET_TIME_MS);

	/* always use internal RC oscillator */
	res = cfg->bus_io->write(&cfg->bus, REG_INTF_CONFIG1,
				 (uint8_t)FIELD_PREP(MASK_CLKSEL, BIT_CLKSEL_INT_RC));

	if (res) {
		return res;
	}

	/* clear reset done int flag */
	res = cfg->bus_io->read(&cfg->bus, REG_INT_STATUS, &value, 1);

	if (res) {
		return res;
	}

	if (FIELD_GET(BIT_STATUS_RESET_DONE_INT, value) != 1) {
		LOG_ERR("unexpected RESET_DONE_INT value, %i", value);
		return -EINVAL;
	}

	res = cfg->bus_io->read(&cfg->bus, REG_WHO_AM_I, &value, 1);

	if (res) {
		return res;
	}

	if (value != WHO_AM_I_ICM40627) {
		LOG_ERR("invalid WHO_AM_I value, was %i but expected %i", value, WHO_AM_I_ICM40627);
		return -EINVAL;
	}

	LOG_DBG("device id: 0x%02X", value);

	return 0;
}

static int icm40627_turn_on_sensor(const struct device *dev)
{
	struct icm40627_data *data = dev->data;
	const struct icm40627_config *cfg = dev->config;
	uint8_t value;
	int res;

	value = FIELD_PREP(MASK_ACCEL_MODE, BIT_ACCEL_MODE_LNM) |
		FIELD_PREP(MASK_GYRO_MODE, BIT_GYRO_MODE_LNM);

	res = cfg->bus_io->update(&cfg->bus, REG_PWR_MGMT0,
				  (uint8_t)(MASK_ACCEL_MODE | MASK_GYRO_MODE), value);

	if (res) {
		return res;
	}

	res = icm40627_set_accel_fs(dev, data->accel_fs);

	if (res) {
		return res;
	}

	res = icm40627_set_accel_odr(dev, data->accel_hz);

	if (res) {
		return res;
	}

	res = icm40627_set_gyro_fs(dev, data->gyro_fs);

	if (res) {
		return res;
	}

	res = icm40627_set_gyro_odr(dev, data->gyro_hz);

	if (res) {
		return res;
	}

	/*
	 * Accelerometer sensor need at least 10ms startup time
	 * Gyroscope sensor need at least 30ms startup time
	 */
	k_msleep(100);

	return 0;
}

static void icm40627_convert_accel(struct sensor_value *val, int16_t raw_val,
				   uint16_t sensitivity_shift)
{
	/* see datasheet section 3.2 for details */
	int64_t conv_val = ((int64_t)raw_val * SENSOR_G) >> sensitivity_shift;

	val->val1 = conv_val / 1000000LL;
	val->val2 = conv_val % 1000000LL;
}

static void icm40627_convert_gyro(struct sensor_value *val, int16_t raw_val,
				  uint16_t sensitivity_x10)
{
	/* see datasheet section 3.1 for details */
	int64_t conv_val = ((int64_t)raw_val * SENSOR_PI * 10) / (sensitivity_x10 * 180LL);

	val->val1 = conv_val / 1000000LL;
	val->val2 = conv_val % 1000000LL;
}

static inline void icm40627_convert_temp(struct sensor_value *val, int16_t raw_val)
{
	/* see datasheet section 14.6 for details */
	val->val1 = (((int64_t)raw_val * 100) / 13248) + 25;
	val->val2 = ((((int64_t)raw_val * 100) % 13248) * 1000000) / 13248;

	if (val->val2 < 0) {
		val->val1--;
		val->val2 += 1000000;
	} else if (val->val2 >= 1000000) {
		val->val1++;
		val->val2 -= 1000000;
	}
}

static int icm40627_channel_get(const struct device *dev, enum sensor_channel chan,
				struct sensor_value *val)
{
	int res = 0;
	const struct icm40627_data *data = dev->data;

	icm40627_lock(dev);

	switch (chan) {
	case SENSOR_CHAN_ACCEL_XYZ:
		icm40627_convert_accel(&val[0], data->accel_x, data->accel_sensitivity_shift);
		icm40627_convert_accel(&val[1], data->accel_y, data->accel_sensitivity_shift);
		icm40627_convert_accel(&val[2], data->accel_z, data->accel_sensitivity_shift);
		break;
	case SENSOR_CHAN_ACCEL_X:
		icm40627_convert_accel(val, data->accel_x, data->accel_sensitivity_shift);
		break;
	case SENSOR_CHAN_ACCEL_Y:
		icm40627_convert_accel(val, data->accel_y, data->accel_sensitivity_shift);
		break;
	case SENSOR_CHAN_ACCEL_Z:
		icm40627_convert_accel(val, data->accel_z, data->accel_sensitivity_shift);
		break;
	case SENSOR_CHAN_GYRO_XYZ:
		icm40627_convert_gyro(&val[0], data->gyro_x, data->gyro_sensitivity_x10);
		icm40627_convert_gyro(&val[1], data->gyro_y, data->gyro_sensitivity_x10);
		icm40627_convert_gyro(&val[2], data->gyro_z, data->gyro_sensitivity_x10);
		break;
	case SENSOR_CHAN_GYRO_X:
		icm40627_convert_gyro(val, data->gyro_x, data->gyro_sensitivity_x10);
		break;
	case SENSOR_CHAN_GYRO_Y:
		icm40627_convert_gyro(val, data->gyro_y, data->gyro_sensitivity_x10);
		break;
	case SENSOR_CHAN_GYRO_Z:
		icm40627_convert_gyro(val, data->gyro_z, data->gyro_sensitivity_x10);
		break;
	case SENSOR_CHAN_DIE_TEMP:
		icm40627_convert_temp(val, data->temp);
		break;
	default:
		res = -ENOTSUP;
		break;
	}

	icm40627_unlock(dev);

	return res;
}

static int icm40627_sample_fetch_accel(const struct device *dev)
{
	const struct icm40627_config *cfg = dev->config;
	struct icm40627_data *data = dev->data;
	uint8_t buffer[ACCEL_DATA_SIZE];

	int res = cfg->bus_io->read(&cfg->bus, REG_ACCEL_DATA_X1, buffer, ACCEL_DATA_SIZE);

	if (res) {
		return res;
	}

	data->accel_x = (int16_t)sys_get_be16(&buffer[0]);
	data->accel_y = (int16_t)sys_get_be16(&buffer[2]);
	data->accel_z = (int16_t)sys_get_be16(&buffer[4]);

	return 0;
}

static int icm40627_sample_fetch_gyro(const struct device *dev)
{
	const struct icm40627_config *cfg = dev->config;
	struct icm40627_data *data = dev->data;
	uint8_t buffer[GYRO_DATA_SIZE];

	int res = cfg->bus_io->read(&cfg->bus, REG_GYRO_DATA_X1, buffer, GYRO_DATA_SIZE);

	if (res) {
		return res;
	}

	data->gyro_x = (int16_t)sys_get_be16(&buffer[0]);
	data->gyro_y = (int16_t)sys_get_be16(&buffer[2]);
	data->gyro_z = (int16_t)sys_get_be16(&buffer[4]);

	return 0;
}

static int icm40627_sample_fetch_temp(const struct device *dev)
{
	const struct icm40627_config *cfg = dev->config;
	struct icm40627_data *data = dev->data;
	uint8_t buffer[TEMP_DATA_SIZE];

	int res = cfg->bus_io->read(&cfg->bus, REG_TEMP_DATA1, buffer, TEMP_DATA_SIZE);

	if (res) {
		return res;
	}

	data->temp = (int16_t)sys_get_be16(&buffer[0]);

	return 0;
}

static int icm40627_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	uint8_t status;
	const struct icm40627_config *cfg = dev->config;

	icm40627_lock(dev);

	int res = cfg->bus_io->read(&cfg->bus, REG_INT_STATUS, &status, 1);

	if (res) {
		goto cleanup;
	}

	if (!FIELD_GET(BIT_INT_STATUS_DATA_RDY_INT, status)) {
		res = -EBUSY;
		goto cleanup;
	}

	switch (chan) {
	case SENSOR_CHAN_ALL:
		res |= icm40627_sample_fetch_accel(dev);
		res |= icm40627_sample_fetch_gyro(dev);
		res |= icm40627_sample_fetch_temp(dev);
		break;
	case SENSOR_CHAN_ACCEL_XYZ:
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
		res = icm40627_sample_fetch_accel(dev);
		break;
	case SENSOR_CHAN_GYRO_XYZ:
	case SENSOR_CHAN_GYRO_X:
	case SENSOR_CHAN_GYRO_Y:
	case SENSOR_CHAN_GYRO_Z:
		res = icm40627_sample_fetch_gyro(dev);
		break;
	case SENSOR_CHAN_DIE_TEMP:
		res = icm40627_sample_fetch_temp(dev);
		break;
	default:
		res = -ENOTSUP;
		break;
	}

cleanup:
	icm40627_unlock(dev);
	return res;
}

static int icm40627_attr_set(const struct device *dev, enum sensor_channel chan,
			     enum sensor_attribute attr, const struct sensor_value *val)
{
	int res = 0;
	struct icm40627_data *data = dev->data;

	__ASSERT_NO_MSG(val != NULL);

	icm40627_lock(dev);

	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ:
		if (attr == SENSOR_ATTR_SAMPLING_FREQUENCY) {
			res = icm40627_set_accel_odr(dev, data->accel_hz);

			if (res) {
				LOG_ERR("Incorrect sampling value");
			} else {
				data->accel_hz = val->val1;
			}
		} else if (attr == SENSOR_ATTR_FULL_SCALE) {
			res = icm40627_set_accel_fs(dev, data->accel_fs);

			if (res) {
				LOG_ERR("Incorrect fullscale value");
			} else {
				data->accel_fs = val->val1;
			}
		} else {
			LOG_ERR("Unsupported attribute");
			res = -ENOTSUP;
		}
		break;

	case SENSOR_CHAN_GYRO_X:
	case SENSOR_CHAN_GYRO_Y:
	case SENSOR_CHAN_GYRO_Z:
	case SENSOR_CHAN_GYRO_XYZ:
		if (attr == SENSOR_ATTR_SAMPLING_FREQUENCY) {
			res = icm40627_set_gyro_odr(dev, data->gyro_hz);

			if (res) {
				LOG_ERR("Incorrect sampling value");
			} else {
				data->gyro_hz = val->val1;
			}
		} else if (attr == SENSOR_ATTR_FULL_SCALE) {
			res = icm40627_set_gyro_fs(dev, data->gyro_fs);

			if (res) {
				LOG_ERR("Incorrect fullscale value");
			} else {
				data->gyro_fs = val->val1;
			}
		} else {
			LOG_ERR("Unsupported attribute");
			res = -EINVAL;
		}
		break;

	default:
		LOG_ERR("Unsupported channel");
		res = -EINVAL;
		break;
	}

	icm40627_unlock(dev);

	return res;
}

static int icm40627_attr_get(const struct device *dev, enum sensor_channel chan,
			     enum sensor_attribute attr, struct sensor_value *val)
{
	const struct icm40627_data *data = dev->data;
	int res = 0;

	__ASSERT_NO_MSG(val != NULL);

	icm40627_lock(dev);

	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ:
		if (attr == SENSOR_ATTR_SAMPLING_FREQUENCY) {
			val->val1 = data->accel_hz;
		} else if (attr == SENSOR_ATTR_FULL_SCALE) {
			val->val1 = data->accel_fs;
		} else {
			LOG_ERR("Unsupported attribute");
			res = -EINVAL;
		}
		break;

	case SENSOR_CHAN_GYRO_X:
	case SENSOR_CHAN_GYRO_Y:
	case SENSOR_CHAN_GYRO_Z:
	case SENSOR_CHAN_GYRO_XYZ:
		if (attr == SENSOR_ATTR_SAMPLING_FREQUENCY) {
			val->val1 = data->gyro_hz;
		} else if (attr == SENSOR_ATTR_FULL_SCALE) {
			val->val1 = data->gyro_fs;
		} else {
			LOG_ERR("Unsupported attribute");
			res = -EINVAL;
		}
		break;

	default:
		LOG_ERR("Unsupported channel");
		res = -EINVAL;
		break;
	}

	icm40627_unlock(dev);

	return res;
}

static inline int icm40627_bus_check(const struct device *dev)
{
	const struct icm40627_config *cfg = dev->config;

	return cfg->bus_io->check(&cfg->bus);
}

static int icm40627_init(const struct device *dev)
{
	struct icm40627_data *data = dev->data;

	if (icm40627_bus_check(dev) < 0) {
		LOG_ERR("Bus is not ready");
		return -ENODEV;
	}

	data->accel_x = 0;
	data->accel_y = 0;
	data->accel_z = 0;
	data->gyro_x = 0;
	data->gyro_y = 0;
	data->gyro_z = 0;
	data->temp = 0;

	if (icm40627_sensor_init(dev)) {
		LOG_ERR("could not initialize sensor");
		return -EIO;
	}

#ifdef CONFIG_ICM40627_TRIGGER
	if (icm40627_trigger_init(dev)) {
		LOG_ERR("Failed to initialize interrupts.");
		return -EIO;
	}
#endif

	int res = icm40627_turn_on_sensor(dev);

#ifdef CONFIG_ICM40627_TRIGGER
	if (icm40627_trigger_enable_interrupt(dev)) {
		LOG_ERR("Failed to enable interrupts");
		return -EIO;
	}
#endif

	return res;
}

#ifndef CONFIG_ICM40627_TRIGGER

void icm40627_lock(const struct device *dev)
{
	ARG_UNUSED(dev);
}

void icm40627_unlock(const struct device *dev)
{
	ARG_UNUSED(dev);
}

#endif

static const struct sensor_driver_api icm40627_driver_api = {
#ifdef CONFIG_ICM40627_TRIGGER
	.trigger_set = icm40627_trigger_set,
#endif
	.sample_fetch = icm40627_sample_fetch,
	.channel_get = icm40627_channel_get,
	.attr_set = icm40627_attr_set,
	.attr_get = icm40627_attr_get,
};

/* Initializes the bus members for an instance on an I2C bus. */
#define ICM40627_CONFIG_I2C(inst)                                                                  \
	.bus.i2c = I2C_DT_SPEC_INST_GET(inst), .bus_io = &icm40627_bus_io_i2c,

#define ICM40627_INIT(inst)                                                                        \
	static struct icm40627_data icm40627_driver_##inst = {                                     \
		.accel_hz = DT_INST_PROP(inst, accel_hz),                                          \
		.accel_fs = DT_INST_PROP(inst, accel_fs),                                          \
		.gyro_hz = DT_INST_PROP(inst, gyro_hz),                                            \
		.gyro_fs = DT_INST_PROP(inst, gyro_fs),                                            \
	};                                                                                         \
                                                                                                   \
	static const struct icm40627_config icm40627_cfg_##inst = {                                \
		COND_CODE_1(DT_INST_ON_BUS(inst, spi),                                             \
			(ICM40627_CONFIG_SPI(inst)),                                               \
			(ICM40627_CONFIG_I2C(inst))) .gpio_int =               \
						     GPIO_DT_SPEC_INST_GET_OR(inst, int_gpios,     \
									      {0}),                \
	};                                                                                         \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, icm40627_init, NULL, &icm40627_driver_##inst,           \
				     &icm40627_cfg_##inst, POST_KERNEL,                            \
				     CONFIG_SENSOR_INIT_PRIORITY, &icm40627_driver_api);

DT_INST_FOREACH_STATUS_OKAY(ICM40627_INIT)
