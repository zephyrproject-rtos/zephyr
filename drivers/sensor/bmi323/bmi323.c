/*
 * Copyright (c) 2023 Trackunit Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bmi323.h"
#include "bmi323_spi.h"

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bosch_bmi323);

#define DT_DRV_COMPAT bosch_bmi323

/* Value taken from BMI323 Datasheet section 5.8.1 */
#define IMU_BOSCH_FEATURE_ENGINE_STARTUP_CONFIG (0x012C)

#define IMU_BOSCH_DIE_TEMP_OFFSET_MICRO_DEG_CELCIUS (23000000LL)
#define IMU_BOSCH_DIE_TEMP_MICRO_DEG_CELCIUS_LSB    (1953L)

typedef void (*bosch_bmi323_gpio_callback_ptr)(const struct device *dev, struct gpio_callback *cb,
					       uint32_t pins);

struct bosch_bmi323_config {
	const struct bosch_bmi323_bus *bus;
	const struct gpio_dt_spec int_gpio;

	const bosch_bmi323_gpio_callback_ptr int_gpio_callback;
};

struct bosch_bmi323_data {
	struct k_mutex lock;

	struct sensor_value acc_samples[3];
	struct sensor_value gyro_samples[3];
	struct sensor_value temperature;

	bool acc_samples_valid;
	bool gyro_samples_valid;
	bool temperature_valid;

	uint32_t acc_full_scale;
	uint32_t gyro_full_scale;

	struct gpio_callback gpio_callback;
	const struct sensor_trigger *trigger;
	sensor_trigger_handler_t trigger_handler;
	struct k_work callback_work;
	const struct device *dev;
};

static int bosch_bmi323_bus_init(const struct device *dev)
{
	const struct bosch_bmi323_config *config = (const struct bosch_bmi323_config *)dev->config;

	const struct bosch_bmi323_bus *bus = config->bus;

	return bus->api->init(bus->context);
}

static int bosch_bmi323_bus_read_words(const struct device *dev, uint8_t offset, uint16_t *words,
				       uint16_t words_count)
{
	const struct bosch_bmi323_config *config = (const struct bosch_bmi323_config *)dev->config;

	const struct bosch_bmi323_bus *bus = config->bus;

	return bus->api->read_words(bus->context, offset, words, words_count);
}

static int bosch_bmi323_bus_write_words(const struct device *dev, uint8_t offset, uint16_t *words,
					uint16_t words_count)
{
	const struct bosch_bmi323_config *config = (const struct bosch_bmi323_config *)dev->config;

	const struct bosch_bmi323_bus *bus = config->bus;

	return bus->api->write_words(bus->context, offset, words, words_count);
}

static int32_t bosch_bmi323_lsb_from_fullscale(int64_t fullscale)
{
	return (fullscale * 1000) / INT16_MAX;
}

/* lsb is the value of one 1/1000000 LSB */
static int64_t bosch_bmi323_value_to_micro(int16_t value, int32_t lsb)
{
	return ((int64_t)value) * lsb;
}

/* lsb is the value of one 1/1000000 LSB */
static void bosch_bmi323_value_to_sensor_value(struct sensor_value *result, int16_t value,
					       int32_t lsb)
{
	int64_t ll_value = (int64_t)value * lsb;
	int32_t int_part = (int32_t)(ll_value / 1000000);
	int32_t frac_part = (int32_t)(ll_value % 1000000);

	result->val1 = int_part;
	result->val2 = frac_part;
}

static int64_t bosch_bmi323_sensor_value_to_milli(const struct sensor_value *val)
{
	return ((int64_t)val->val1 * 1000) + val->val2 / 1000;
}

static void bosch_bmi323_sensor_value_from_micro(struct sensor_value *result, int64_t micro)
{
	int32_t int_part = (int32_t)(micro / 1000000);
	int32_t frac_part = (int32_t)(micro % 1000000);

	result->val1 = int_part;
	result->val2 = frac_part;
}

static bool bosch_bmi323_value_is_valid(int16_t value)
{
	return ((uint16_t)value == 0x8000) ? false : true;
}

static int bosch_bmi323_validate_chip_id(const struct device *dev)
{
	uint16_t sensor_id;
	int ret;

	ret = bosch_bmi323_bus_read_words(dev, 0, &sensor_id, 1);

	if (ret < 0) {
		return ret;
	}

	if ((sensor_id & 0xFF) != 0x43) {
		return -ENODEV;
	}

	return 0;
}

static int bosch_bmi323_soft_reset(const struct device *dev)
{
	uint16_t cmd;
	int ret;

	cmd = IMU_BOSCH_BMI323_REG_VALUE(CMD, CMD, SOFT_RESET);

	ret = bosch_bmi323_bus_write_words(dev, IMU_BOSCH_BMI323_REG_CMD, &cmd, 1);

	if (ret < 0) {
		return ret;
	}

	k_usleep(1500);

	return 0;
}

static int bosch_bmi323_enable_feature_engine(const struct device *dev)
{
	uint16_t buf;
	int ret;

	buf = IMU_BOSCH_FEATURE_ENGINE_STARTUP_CONFIG;

	ret = bosch_bmi323_bus_write_words(dev, IMU_BOSCH_BMI323_REG_FEATURE_IO2, &buf, 1);

	if (ret < 0) {
		return ret;
	}

	buf = IMU_BOSCH_BMI323_REG_VALUE(FEATURE_IO_STATUS, STATUS, SET);

	ret = bosch_bmi323_bus_write_words(dev, IMU_BOSCH_BMI323_REG_FEATURE_IO_STATUS, &buf, 1);

	if (ret < 0) {
		return ret;
	}

	buf = IMU_BOSCH_BMI323_REG_VALUE(FEATURE_CTRL, ENABLE, EN);

	return bosch_bmi323_bus_write_words(dev, IMU_BOSCH_BMI323_REG_FEATURE_CTRL, &buf, 1);
}

static int bosch_bmi323_driver_api_set_acc_odr(const struct device *dev,
					       const struct sensor_value *val)
{
	int ret;
	uint16_t acc_conf;
	int64_t odr = bosch_bmi323_sensor_value_to_milli(val);

	ret = bosch_bmi323_bus_read_words(dev, IMU_BOSCH_BMI323_REG_ACC_CONF, &acc_conf, 1);

	if (ret < 0) {
		return ret;
	}

	acc_conf &= ~IMU_BOSCH_BMI323_REG_MASK(ACC_CONF, ODR);

	if (odr <= 782) {
		acc_conf |= IMU_BOSCH_BMI323_REG_VALUE(ACC_CONF, ODR, HZ0P78125);
	} else if (odr <= 1563) {
		acc_conf |= IMU_BOSCH_BMI323_REG_VALUE(ACC_CONF, ODR, HZ1P5625);
	} else if (odr <= 3125) {
		acc_conf |= IMU_BOSCH_BMI323_REG_VALUE(ACC_CONF, ODR, HZ3P125);
	} else if (odr <= 6250) {
		acc_conf |= IMU_BOSCH_BMI323_REG_VALUE(ACC_CONF, ODR, HZ6P25);
	} else if (odr <= 12500) {
		acc_conf |= IMU_BOSCH_BMI323_REG_VALUE(ACC_CONF, ODR, HZ12P5);
	} else if (odr <= 25000) {
		acc_conf |= IMU_BOSCH_BMI323_REG_VALUE(ACC_CONF, ODR, HZ25);
	} else if (odr <= 50000) {
		acc_conf |= IMU_BOSCH_BMI323_REG_VALUE(ACC_CONF, ODR, HZ50);
	} else if (odr <= 100000) {
		acc_conf |= IMU_BOSCH_BMI323_REG_VALUE(ACC_CONF, ODR, HZ100);
	} else if (odr <= 200000) {
		acc_conf |= IMU_BOSCH_BMI323_REG_VALUE(ACC_CONF, ODR, HZ200);
	} else if (odr <= 400000) {
		acc_conf |= IMU_BOSCH_BMI323_REG_VALUE(ACC_CONF, ODR, HZ400);
	} else if (odr <= 800000) {
		acc_conf |= IMU_BOSCH_BMI323_REG_VALUE(ACC_CONF, ODR, HZ800);
	} else if (odr <= 1600000) {
		acc_conf |= IMU_BOSCH_BMI323_REG_VALUE(ACC_CONF, ODR, HZ1600);
	} else if (odr <= 3200000) {
		acc_conf |= IMU_BOSCH_BMI323_REG_VALUE(ACC_CONF, ODR, HZ3200);
	} else {
		acc_conf |= IMU_BOSCH_BMI323_REG_VALUE(ACC_CONF, ODR, HZ6400);
	}

	return bosch_bmi323_bus_write_words(dev, IMU_BOSCH_BMI323_REG_ACC_CONF, &acc_conf, 1);
}

static int bosch_bmi323_driver_api_set_acc_full_scale(const struct device *dev,
						      const struct sensor_value *val)
{
	struct bosch_bmi323_data *data = (struct bosch_bmi323_data *)dev->data;
	int ret;
	uint16_t acc_conf;
	int64_t fullscale = bosch_bmi323_sensor_value_to_milli(val);

	ret = bosch_bmi323_bus_read_words(dev, IMU_BOSCH_BMI323_REG_ACC_CONF, &acc_conf, 1);

	if (ret < 0) {
		return ret;
	}

	acc_conf &= ~IMU_BOSCH_BMI323_REG_MASK(ACC_CONF, RANGE);

	if (fullscale <= 2000) {
		acc_conf |= IMU_BOSCH_BMI323_REG_VALUE(ACC_CONF, RANGE, G2);
	} else if (fullscale <= 4000) {
		acc_conf |= IMU_BOSCH_BMI323_REG_VALUE(ACC_CONF, RANGE, G4);
	} else if (fullscale <= 8000) {
		acc_conf |= IMU_BOSCH_BMI323_REG_VALUE(ACC_CONF, RANGE, G8);
	} else {
		acc_conf |= IMU_BOSCH_BMI323_REG_VALUE(ACC_CONF, RANGE, G16);
	}

	data->acc_full_scale = 0;

	return bosch_bmi323_bus_write_words(dev, IMU_BOSCH_BMI323_REG_ACC_CONF, &acc_conf, 1);
}

static int bosch_bmi323_driver_api_set_acc_feature_mask(const struct device *dev,
							const struct sensor_value *val)
{
	int ret;
	uint16_t acc_conf;

	ret = bosch_bmi323_bus_read_words(dev, IMU_BOSCH_BMI323_REG_ACC_CONF, &acc_conf, 1);

	if (ret < 0) {
		return ret;
	}

	acc_conf &= ~IMU_BOSCH_BMI323_REG_MASK(ACC_CONF, MODE);

	if (val->val1) {
		acc_conf |= IMU_BOSCH_BMI323_REG_VALUE(ACC_CONF, MODE, HPWR);
	} else {
		acc_conf |= IMU_BOSCH_BMI323_REG_VALUE(ACC_CONF, MODE, DIS);
	}

	return bosch_bmi323_bus_write_words(dev, IMU_BOSCH_BMI323_REG_ACC_CONF, &acc_conf, 1);
}

static int bosch_bmi323_driver_api_set_gyro_odr(const struct device *dev,
						const struct sensor_value *val)
{
	int ret;
	uint16_t gyro_conf;
	int64_t odr = bosch_bmi323_sensor_value_to_milli(val);

	ret = bosch_bmi323_bus_read_words(dev, IMU_BOSCH_BMI323_REG_GYRO_CONF, &gyro_conf, 1);

	if (ret < 0) {
		return ret;
	}

	gyro_conf &= ~IMU_BOSCH_BMI323_REG_MASK(GYRO_CONF, ODR);

	if (odr <= 782) {
		gyro_conf |= IMU_BOSCH_BMI323_REG_VALUE(GYRO_CONF, ODR, HZ0P78125);
	} else if (odr <= 1563) {
		gyro_conf |= IMU_BOSCH_BMI323_REG_VALUE(GYRO_CONF, ODR, HZ1P5625);
	} else if (odr <= 3125) {
		gyro_conf |= IMU_BOSCH_BMI323_REG_VALUE(GYRO_CONF, ODR, HZ3P125);
	} else if (odr <= 6250) {
		gyro_conf |= IMU_BOSCH_BMI323_REG_VALUE(GYRO_CONF, ODR, HZ6P25);
	} else if (odr <= 12500) {
		gyro_conf |= IMU_BOSCH_BMI323_REG_VALUE(GYRO_CONF, ODR, HZ12P5);
	} else if (odr <= 25000) {
		gyro_conf |= IMU_BOSCH_BMI323_REG_VALUE(GYRO_CONF, ODR, HZ25);
	} else if (odr <= 50000) {
		gyro_conf |= IMU_BOSCH_BMI323_REG_VALUE(GYRO_CONF, ODR, HZ50);
	} else if (odr <= 100000) {
		gyro_conf |= IMU_BOSCH_BMI323_REG_VALUE(GYRO_CONF, ODR, HZ100);
	} else if (odr <= 200000) {
		gyro_conf |= IMU_BOSCH_BMI323_REG_VALUE(GYRO_CONF, ODR, HZ200);
	} else if (odr <= 400000) {
		gyro_conf |= IMU_BOSCH_BMI323_REG_VALUE(GYRO_CONF, ODR, HZ400);
	} else if (odr <= 800000) {
		gyro_conf |= IMU_BOSCH_BMI323_REG_VALUE(GYRO_CONF, ODR, HZ800);
	} else if (odr <= 1600000) {
		gyro_conf |= IMU_BOSCH_BMI323_REG_VALUE(GYRO_CONF, ODR, HZ1600);
	} else if (odr <= 3200000) {
		gyro_conf |= IMU_BOSCH_BMI323_REG_VALUE(GYRO_CONF, ODR, HZ3200);
	} else {
		gyro_conf |= IMU_BOSCH_BMI323_REG_VALUE(GYRO_CONF, ODR, HZ6400);
	}

	return bosch_bmi323_bus_write_words(dev, IMU_BOSCH_BMI323_REG_GYRO_CONF, &gyro_conf, 1);
}

static int bosch_bmi323_driver_api_set_gyro_full_scale(const struct device *dev,
						       const struct sensor_value *val)
{
	struct bosch_bmi323_data *data = (struct bosch_bmi323_data *)dev->data;
	int ret;
	uint16_t gyro_conf;
	int32_t fullscale = bosch_bmi323_sensor_value_to_milli(val);

	ret = bosch_bmi323_bus_read_words(dev, IMU_BOSCH_BMI323_REG_GYRO_CONF, &gyro_conf, 1);

	if (ret < 0) {
		return ret;
	}

	gyro_conf &= ~IMU_BOSCH_BMI323_REG_MASK(GYRO_CONF, RANGE);

	if (fullscale <= 125000) {
		gyro_conf |= IMU_BOSCH_BMI323_REG_VALUE(GYRO_CONF, RANGE, DPS125);
	} else if (fullscale <= 250000) {
		gyro_conf |= IMU_BOSCH_BMI323_REG_VALUE(GYRO_CONF, RANGE, DPS250);
	} else if (fullscale <= 500000) {
		gyro_conf |= IMU_BOSCH_BMI323_REG_VALUE(GYRO_CONF, RANGE, DPS500);
	} else if (fullscale <= 1000000) {
		gyro_conf |= IMU_BOSCH_BMI323_REG_VALUE(GYRO_CONF, RANGE, DPS1000);
	} else {
		gyro_conf |= IMU_BOSCH_BMI323_REG_VALUE(GYRO_CONF, RANGE, DPS2000);
	}

	data->gyro_full_scale = 0;

	return bosch_bmi323_bus_write_words(dev, IMU_BOSCH_BMI323_REG_GYRO_CONF, &gyro_conf, 1);
}

static int bosch_bmi323_driver_api_set_gyro_feature_mask(const struct device *dev,
							 const struct sensor_value *val)
{
	int ret;
	uint16_t gyro_conf;

	ret = bosch_bmi323_bus_read_words(dev, IMU_BOSCH_BMI323_REG_GYRO_CONF, &gyro_conf, 1);

	if (ret < 0) {
		return ret;
	}

	gyro_conf &= ~IMU_BOSCH_BMI323_REG_MASK(GYRO_CONF, MODE);

	if (val->val1) {
		gyro_conf |= IMU_BOSCH_BMI323_REG_VALUE(GYRO_CONF, MODE, HPWR);
	} else {
		gyro_conf |= IMU_BOSCH_BMI323_REG_VALUE(GYRO_CONF, MODE, DIS);
	}

	return bosch_bmi323_bus_write_words(dev, IMU_BOSCH_BMI323_REG_GYRO_CONF, &gyro_conf, 1);
}

static int bosch_bmi323_driver_api_attr_set(const struct device *dev, enum sensor_channel chan,
					    enum sensor_attribute attr,
					    const struct sensor_value *val)
{
	struct bosch_bmi323_data *data = (struct bosch_bmi323_data *)dev->data;
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);

	switch (chan) {
	case SENSOR_CHAN_ACCEL_XYZ:
		switch (attr) {
		case SENSOR_ATTR_SAMPLING_FREQUENCY:
			ret = bosch_bmi323_driver_api_set_acc_odr(dev, val);

			break;

		case SENSOR_ATTR_FULL_SCALE:
			ret = bosch_bmi323_driver_api_set_acc_full_scale(dev, val);

			break;

		case SENSOR_ATTR_FEATURE_MASK:
			ret = bosch_bmi323_driver_api_set_acc_feature_mask(dev, val);

			break;

		default:
			ret = -ENODEV;

			break;
		}

		break;

	case SENSOR_CHAN_GYRO_XYZ:
		switch (attr) {
		case SENSOR_ATTR_SAMPLING_FREQUENCY:
			ret = bosch_bmi323_driver_api_set_gyro_odr(dev, val);

			break;

		case SENSOR_ATTR_FULL_SCALE:
			ret = bosch_bmi323_driver_api_set_gyro_full_scale(dev, val);

			break;

		case SENSOR_ATTR_FEATURE_MASK:
			ret = bosch_bmi323_driver_api_set_gyro_feature_mask(dev, val);

			break;

		default:
			ret = -ENODEV;

			break;
		}

		break;

	default:
		ret = -ENODEV;

		break;
	}

	k_mutex_unlock(&data->lock);

	return ret;
}

static int bosch_bmi323_driver_api_get_acc_odr(const struct device *dev, struct sensor_value *val)
{
	uint16_t acc_conf;
	int ret;

	ret = bosch_bmi323_bus_read_words(dev, IMU_BOSCH_BMI323_REG_ACC_CONF, &acc_conf, 1);

	if (ret < 0) {
		return ret;
	}

	switch (IMU_BOSCH_BMI323_REG_VALUE_GET_FIELD(acc_conf, ACC_CONF, ODR)) {
	case IMU_BOSCH_BMI323_REG_ACC_CONF_ODR_VAL_HZ0P78125:
		val->val1 = 0;
		val->val2 = 781250;
		break;
	case IMU_BOSCH_BMI323_REG_ACC_CONF_ODR_VAL_HZ1P5625:
		val->val1 = 1;
		val->val2 = 562500;
		break;
	case IMU_BOSCH_BMI323_REG_ACC_CONF_ODR_VAL_HZ3P125:
		val->val1 = 3;
		val->val2 = 125000;
		break;
	case IMU_BOSCH_BMI323_REG_ACC_CONF_ODR_VAL_HZ6P25:
		val->val1 = 6;
		val->val2 = 250000;
		break;
	case IMU_BOSCH_BMI323_REG_ACC_CONF_ODR_VAL_HZ12P5:
		val->val1 = 12;
		val->val2 = 500000;
		break;
	case IMU_BOSCH_BMI323_REG_ACC_CONF_ODR_VAL_HZ25:
		val->val1 = 25;
		val->val2 = 0;
		break;
	case IMU_BOSCH_BMI323_REG_ACC_CONF_ODR_VAL_HZ50:
		val->val1 = 50;
		val->val2 = 0;
		break;
	case IMU_BOSCH_BMI323_REG_ACC_CONF_ODR_VAL_HZ100:
		val->val1 = 100;
		val->val2 = 0;
		break;
	case IMU_BOSCH_BMI323_REG_ACC_CONF_ODR_VAL_HZ200:
		val->val1 = 200;
		val->val2 = 0;
		break;
	case IMU_BOSCH_BMI323_REG_ACC_CONF_ODR_VAL_HZ400:
		val->val1 = 400;
		val->val2 = 0;
		break;
	case IMU_BOSCH_BMI323_REG_ACC_CONF_ODR_VAL_HZ800:
		val->val1 = 800;
		val->val2 = 0;
		break;
	case IMU_BOSCH_BMI323_REG_ACC_CONF_ODR_VAL_HZ1600:
		val->val1 = 1600;
		val->val2 = 0;
		break;
	case IMU_BOSCH_BMI323_REG_ACC_CONF_ODR_VAL_HZ3200:
		val->val1 = 3200;
		val->val2 = 0;
		break;
	case IMU_BOSCH_BMI323_REG_ACC_CONF_ODR_VAL_HZ6400:
		val->val1 = 6400;
		val->val2 = 0;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int bosch_bmi323_driver_api_get_acc_full_scale(const struct device *dev,
						      struct sensor_value *val)
{
	uint16_t acc_conf;
	int ret;

	ret = bosch_bmi323_bus_read_words(dev, IMU_BOSCH_BMI323_REG_ACC_CONF, &acc_conf, 1);

	if (ret < 0) {
		return ret;
	}

	switch (IMU_BOSCH_BMI323_REG_VALUE_GET_FIELD(acc_conf, ACC_CONF, RANGE)) {
	case IMU_BOSCH_BMI323_REG_ACC_CONF_RANGE_VAL_G2:
		val->val1 = 2;
		val->val2 = 0;
		break;
	case IMU_BOSCH_BMI323_REG_ACC_CONF_RANGE_VAL_G4:
		val->val1 = 4;
		val->val2 = 0;
		break;
	case IMU_BOSCH_BMI323_REG_ACC_CONF_RANGE_VAL_G8:
		val->val1 = 8;
		val->val2 = 0;
		break;
	case IMU_BOSCH_BMI323_REG_ACC_CONF_RANGE_VAL_G16:
		val->val1 = 16;
		val->val2 = 0;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int bosch_bmi323_driver_api_get_acc_feature_mask(const struct device *dev,
							struct sensor_value *val)
{
	uint16_t acc_conf;
	int ret;

	ret = bosch_bmi323_bus_read_words(dev, IMU_BOSCH_BMI323_REG_ACC_CONF, &acc_conf, 1);

	if (ret < 0) {
		return ret;
	}

	if (IMU_BOSCH_BMI323_REG_VALUE_GET_FIELD(acc_conf, ACC_CONF, MODE)) {
		val->val1 = 1;
		val->val2 = 0;
	} else {
		val->val1 = 0;
		val->val2 = 0;
	}

	return 0;
}

static int bosch_bmi323_driver_api_get_gyro_odr(const struct device *dev, struct sensor_value *val)
{
	uint16_t gyro_conf;
	int ret;

	ret = bosch_bmi323_bus_read_words(dev, IMU_BOSCH_BMI323_REG_GYRO_CONF, &gyro_conf, 1);

	if (ret < 0) {
		return ret;
	}

	switch (IMU_BOSCH_BMI323_REG_VALUE_GET_FIELD(gyro_conf, GYRO_CONF, ODR)) {
	case IMU_BOSCH_BMI323_REG_GYRO_CONF_ODR_VAL_HZ0P78125:
		val->val1 = 0;
		val->val2 = 781250;
		break;
	case IMU_BOSCH_BMI323_REG_GYRO_CONF_ODR_VAL_HZ1P5625:
		val->val1 = 1;
		val->val2 = 562500;
		break;
	case IMU_BOSCH_BMI323_REG_GYRO_CONF_ODR_VAL_HZ3P125:
		val->val1 = 3;
		val->val2 = 125000;
		break;
	case IMU_BOSCH_BMI323_REG_GYRO_CONF_ODR_VAL_HZ6P25:
		val->val1 = 6;
		val->val2 = 250000;
		break;
	case IMU_BOSCH_BMI323_REG_GYRO_CONF_ODR_VAL_HZ12P5:
		val->val1 = 12;
		val->val2 = 500000;
		break;
	case IMU_BOSCH_BMI323_REG_GYRO_CONF_ODR_VAL_HZ25:
		val->val1 = 25;
		val->val2 = 0;
		break;
	case IMU_BOSCH_BMI323_REG_GYRO_CONF_ODR_VAL_HZ50:
		val->val1 = 50;
		val->val2 = 0;
		break;
	case IMU_BOSCH_BMI323_REG_GYRO_CONF_ODR_VAL_HZ100:
		val->val1 = 100;
		val->val2 = 0;
		break;
	case IMU_BOSCH_BMI323_REG_GYRO_CONF_ODR_VAL_HZ200:
		val->val1 = 200;
		val->val2 = 0;
		break;
	case IMU_BOSCH_BMI323_REG_GYRO_CONF_ODR_VAL_HZ400:
		val->val1 = 400;
		val->val2 = 0;
		break;
	case IMU_BOSCH_BMI323_REG_GYRO_CONF_ODR_VAL_HZ800:
		val->val1 = 800;
		val->val2 = 0;
		break;
	case IMU_BOSCH_BMI323_REG_GYRO_CONF_ODR_VAL_HZ1600:
		val->val1 = 1600;
		val->val2 = 0;
		break;
	case IMU_BOSCH_BMI323_REG_GYRO_CONF_ODR_VAL_HZ3200:
		val->val1 = 3200;
		val->val2 = 0;
		break;
	case IMU_BOSCH_BMI323_REG_GYRO_CONF_ODR_VAL_HZ6400:
		val->val1 = 6400;
		val->val2 = 0;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int bosch_bmi323_driver_api_get_gyro_full_scale(const struct device *dev,
						       struct sensor_value *val)
{
	uint16_t gyro_conf;
	int ret;

	ret = bosch_bmi323_bus_read_words(dev, IMU_BOSCH_BMI323_REG_GYRO_CONF, &gyro_conf, 1);

	if (ret < 0) {
		return ret;
	}

	switch (IMU_BOSCH_BMI323_REG_VALUE_GET_FIELD(gyro_conf, GYRO_CONF, RANGE)) {
	case IMU_BOSCH_BMI323_REG_GYRO_CONF_RANGE_VAL_DPS125:
		val->val1 = 125;
		val->val2 = 0;
		break;
	case IMU_BOSCH_BMI323_REG_GYRO_CONF_RANGE_VAL_DPS250:
		val->val1 = 250;
		val->val2 = 0;
		break;
	case IMU_BOSCH_BMI323_REG_GYRO_CONF_RANGE_VAL_DPS500:
		val->val1 = 500;
		val->val2 = 0;
		break;
	case IMU_BOSCH_BMI323_REG_GYRO_CONF_RANGE_VAL_DPS1000:
		val->val1 = 1000;
		val->val2 = 0;
		break;
	case IMU_BOSCH_BMI323_REG_GYRO_CONF_RANGE_VAL_DPS2000:
		val->val1 = 2000;
		val->val2 = 0;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int bosch_bmi323_driver_api_get_gyro_feature_mask(const struct device *dev,
							 struct sensor_value *val)
{
	uint16_t gyro_conf;
	int ret;

	ret = bosch_bmi323_bus_read_words(dev, IMU_BOSCH_BMI323_REG_GYRO_CONF, &gyro_conf, 1);

	if (ret < 0) {
		return ret;
	}

	if (IMU_BOSCH_BMI323_REG_VALUE_GET_FIELD(gyro_conf, GYRO_CONF, MODE)) {
		val->val1 = 1;
		val->val2 = 0;
	} else {
		val->val1 = 0;
		val->val2 = 0;
	}

	return 0;
}

static int bosch_bmi323_driver_api_attr_get(const struct device *dev, enum sensor_channel chan,
					    enum sensor_attribute attr, struct sensor_value *val)
{
	struct bosch_bmi323_data *data = (struct bosch_bmi323_data *)dev->data;
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);

	switch (chan) {
	case SENSOR_CHAN_ACCEL_XYZ:
		switch (attr) {
		case SENSOR_ATTR_SAMPLING_FREQUENCY:
			ret = bosch_bmi323_driver_api_get_acc_odr(dev, val);

			break;

		case SENSOR_ATTR_FULL_SCALE:
			ret = bosch_bmi323_driver_api_get_acc_full_scale(dev, val);

			break;

		case SENSOR_ATTR_FEATURE_MASK:
			ret = bosch_bmi323_driver_api_get_acc_feature_mask(dev, val);

			break;

		default:
			ret = -ENODEV;

			break;
		}

		break;

	case SENSOR_CHAN_GYRO_XYZ:
		switch (attr) {
		case SENSOR_ATTR_SAMPLING_FREQUENCY:
			ret = bosch_bmi323_driver_api_get_gyro_odr(dev, val);

			break;

		case SENSOR_ATTR_FULL_SCALE:
			ret = bosch_bmi323_driver_api_get_gyro_full_scale(dev, val);

			break;

		case SENSOR_ATTR_FEATURE_MASK:
			ret = bosch_bmi323_driver_api_get_gyro_feature_mask(dev, val);

			break;

		default:
			ret = -ENODEV;

			break;
		}

		break;

	default:
		ret = -ENODEV;
		break;
	}

	k_mutex_unlock(&data->lock);

	return ret;
}

static int bosch_bmi323_driver_api_trigger_set_acc_drdy(const struct device *dev)
{
	uint16_t buf[2];

	buf[0] = 0;
	buf[1] = IMU_BOSCH_BMI323_REG_VALUE(INT_MAP2, ACC_DRDY_INT, INT1);

	return bosch_bmi323_bus_write_words(dev, IMU_BOSCH_BMI323_REG_INT_MAP1, buf, 2);
}

static int bosch_bmi323_driver_api_trigger_set_acc_motion(const struct device *dev)
{
	uint16_t buf[2];
	int ret;

	buf[0] = IMU_BOSCH_BMI323_REG_VALUE(INT_MAP1, MOTION_OUT, INT1);
	buf[1] = 0;

	ret = bosch_bmi323_bus_write_words(dev, IMU_BOSCH_BMI323_REG_INT_MAP1, buf, 2);

	if (ret < 0) {
		return ret;
	}

	buf[0] = 0;

	ret = bosch_bmi323_bus_write_words(dev, IMU_BOSCH_BMI323_REG_FEATURE_IO0, buf, 1);

	if (ret < 0) {
		return ret;
	}

	buf[0] = IMU_BOSCH_BMI323_REG_VALUE(FEATURE_IO0, MOTION_X_EN, EN) |
		 IMU_BOSCH_BMI323_REG_VALUE(FEATURE_IO0, MOTION_Y_EN, EN) |
		 IMU_BOSCH_BMI323_REG_VALUE(FEATURE_IO0, MOTION_Z_EN, EN);

	ret = bosch_bmi323_bus_write_words(dev, IMU_BOSCH_BMI323_REG_FEATURE_IO0, buf, 1);

	if (ret < 0) {
		return ret;
	}

	buf[0] = 1;

	return bosch_bmi323_bus_write_words(dev, IMU_BOSCH_BMI323_REG_FEATURE_IO_STATUS, buf, 1);
}

static int bosch_bmi323_driver_api_trigger_set(const struct device *dev,
					       const struct sensor_trigger *trig,
					       sensor_trigger_handler_t handler)
{
	struct bosch_bmi323_data *data = (struct bosch_bmi323_data *)dev->data;
	int ret = -ENODEV;

	k_mutex_lock(&data->lock, K_FOREVER);

	data->trigger = trig;
	data->trigger_handler = handler;

	switch (trig->chan) {
	case SENSOR_CHAN_ACCEL_XYZ:
		switch (trig->type) {
		case SENSOR_TRIG_DATA_READY:
			ret = bosch_bmi323_driver_api_trigger_set_acc_drdy(dev);

			break;

		case SENSOR_TRIG_MOTION:
			ret = bosch_bmi323_driver_api_trigger_set_acc_motion(dev);

			break;

		default:
			break;
		}

		break;

	default:
		break;
	}

	k_mutex_unlock(&data->lock);

	return ret;
}

static int bosch_bmi323_driver_api_fetch_acc_samples(const struct device *dev)
{
	struct bosch_bmi323_data *data = (struct bosch_bmi323_data *)dev->data;
	struct sensor_value full_scale;
	int16_t *buf = (int16_t *)data->acc_samples;
	int ret;
	int32_t lsb;

	if (data->acc_full_scale == 0) {
		ret = bosch_bmi323_driver_api_get_acc_full_scale(dev, &full_scale);

		if (ret < 0) {
			return ret;
		}

		data->acc_full_scale = bosch_bmi323_sensor_value_to_milli(&full_scale);
	}

	ret = bosch_bmi323_bus_read_words(dev, IMU_BOSCH_BMI323_REG_ACC_DATA_X, (uint16_t *)buf, 3);

	if (ret < 0) {
		return ret;
	}

	if ((bosch_bmi323_value_is_valid(buf[0]) == false) ||
	    (bosch_bmi323_value_is_valid(buf[1]) == false) ||
	    (bosch_bmi323_value_is_valid(buf[2]) == false)) {
		return -ENODATA;
	}

	lsb = bosch_bmi323_lsb_from_fullscale(data->acc_full_scale);

	/* Reuse vector backwards to avoid overwriting the raw values */
	bosch_bmi323_value_to_sensor_value(&data->acc_samples[2], buf[2], lsb);
	bosch_bmi323_value_to_sensor_value(&data->acc_samples[1], buf[1], lsb);
	bosch_bmi323_value_to_sensor_value(&data->acc_samples[0], buf[0], lsb);

	data->acc_samples_valid = true;

	return 0;
}

static int bosch_bmi323_driver_api_fetch_gyro_samples(const struct device *dev)
{
	struct bosch_bmi323_data *data = (struct bosch_bmi323_data *)dev->data;
	struct sensor_value full_scale;
	int16_t *buf = (int16_t *)data->gyro_samples;
	int ret;
	int32_t lsb;

	if (data->gyro_full_scale == 0) {
		ret = bosch_bmi323_driver_api_get_gyro_full_scale(dev, &full_scale);

		if (ret < 0) {
			return ret;
		}

		data->gyro_full_scale = bosch_bmi323_sensor_value_to_milli(&full_scale);
	}

	ret = bosch_bmi323_bus_read_words(dev, IMU_BOSCH_BMI323_REG_GYRO_DATA_X, (uint16_t *)buf,
					  3);

	if (ret < 0) {
		return ret;
	}

	if ((bosch_bmi323_value_is_valid(buf[0]) == false) ||
	    (bosch_bmi323_value_is_valid(buf[1]) == false) ||
	    (bosch_bmi323_value_is_valid(buf[2]) == false)) {
		return -ENODATA;
	}

	lsb = bosch_bmi323_lsb_from_fullscale(data->gyro_full_scale);

	/* Reuse vector backwards to avoid overwriting the raw values */
	bosch_bmi323_value_to_sensor_value(&data->gyro_samples[2], buf[2], lsb);
	bosch_bmi323_value_to_sensor_value(&data->gyro_samples[1], buf[1], lsb);
	bosch_bmi323_value_to_sensor_value(&data->gyro_samples[0], buf[0], lsb);

	data->gyro_samples_valid = true;

	return 0;
}

static int bosch_bmi323_driver_api_fetch_temperature(const struct device *dev)
{
	struct bosch_bmi323_data *data = (struct bosch_bmi323_data *)dev->data;
	int16_t buf;
	int64_t micro;
	int ret;

	ret = bosch_bmi323_bus_read_words(dev, IMU_BOSCH_BMI323_REG_TEMP_DATA, &buf, 1);

	if (ret < 0) {
		return ret;
	}

	if (bosch_bmi323_value_is_valid(buf) == false) {
		return -ENODATA;
	}

	micro = bosch_bmi323_value_to_micro(buf, IMU_BOSCH_DIE_TEMP_MICRO_DEG_CELCIUS_LSB);

	micro += IMU_BOSCH_DIE_TEMP_OFFSET_MICRO_DEG_CELCIUS;

	bosch_bmi323_sensor_value_from_micro(&data->temperature, micro);

	data->temperature_valid = true;

	return 0;
}

static int bosch_bmi323_driver_api_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct bosch_bmi323_data *data = (struct bosch_bmi323_data *)dev->data;
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);

	switch (chan) {
	case SENSOR_CHAN_ACCEL_XYZ:
		ret = bosch_bmi323_driver_api_fetch_acc_samples(dev);

		break;

	case SENSOR_CHAN_GYRO_XYZ:
		ret = bosch_bmi323_driver_api_fetch_gyro_samples(dev);

		break;

	case SENSOR_CHAN_DIE_TEMP:
		ret = bosch_bmi323_driver_api_fetch_temperature(dev);

		break;

	case SENSOR_CHAN_ALL:
		ret = bosch_bmi323_driver_api_fetch_acc_samples(dev);

		if (ret < 0) {
			break;
		}

		ret = bosch_bmi323_driver_api_fetch_gyro_samples(dev);

		if (ret < 0) {
			break;
		}

		ret = bosch_bmi323_driver_api_fetch_temperature(dev);

		break;

	default:
		ret = -ENODEV;

		break;
	}

	k_mutex_unlock(&data->lock);

	return ret;
}

static int bosch_bmi323_driver_api_channel_get(const struct device *dev, enum sensor_channel chan,
					       struct sensor_value *val)
{
	struct bosch_bmi323_data *data = (struct bosch_bmi323_data *)dev->data;
	int ret = 0;

	k_mutex_lock(&data->lock, K_FOREVER);

	switch (chan) {
	case SENSOR_CHAN_ACCEL_XYZ:
		if (data->acc_samples_valid == false) {
			ret = -ENODATA;

			break;
		}

		memcpy(val, data->acc_samples, sizeof(data->acc_samples));

		break;

	case SENSOR_CHAN_GYRO_XYZ:
		if (data->gyro_samples_valid == false) {
			ret = -ENODATA;

			break;
		}

		memcpy(val, data->gyro_samples, sizeof(data->gyro_samples));

		break;

	case SENSOR_CHAN_DIE_TEMP:
		if (data->temperature_valid == false) {
			ret = -ENODATA;

			break;
		}

		(*val) = data->temperature;

		break;

	default:
		ret = -ENODEV;

		break;
	}

	k_mutex_unlock(&data->lock);

	return ret;
}

static const struct sensor_driver_api bosch_bmi323_api = {
	.attr_set = bosch_bmi323_driver_api_attr_set,
	.attr_get = bosch_bmi323_driver_api_attr_get,
	.trigger_set = bosch_bmi323_driver_api_trigger_set,
	.sample_fetch = bosch_bmi323_driver_api_sample_fetch,
	.channel_get = bosch_bmi323_driver_api_channel_get,
};

static void bosch_bmi323_irq_callback(const struct device *dev)
{
	struct bosch_bmi323_data *data = (struct bosch_bmi323_data *)dev->data;

	k_work_submit(&data->callback_work);
}

static int bosch_bmi323_init_irq(const struct device *dev)
{
	struct bosch_bmi323_data *data = (struct bosch_bmi323_data *)dev->data;
	struct bosch_bmi323_config *config = (struct bosch_bmi323_config *)dev->config;
	int ret;

	ret = gpio_pin_configure_dt(&config->int_gpio, GPIO_INPUT);

	if (ret < 0) {
		return ret;
	}

	gpio_init_callback(&data->gpio_callback, config->int_gpio_callback,
			   BIT(config->int_gpio.pin));

	ret = gpio_add_callback(config->int_gpio.port, &data->gpio_callback);

	if (ret < 0) {
		return ret;
	}

	return gpio_pin_interrupt_configure_dt(&config->int_gpio, GPIO_INT_EDGE_TO_ACTIVE);
}

static int bosch_bmi323_init_int1(const struct device *dev)
{
	uint16_t buf;

	buf = IMU_BOSCH_BMI323_REG_VALUE(IO_INT_CTRL, INT1_LVL, ACT_HIGH) |
	      IMU_BOSCH_BMI323_REG_VALUE(IO_INT_CTRL, INT1_OD, PUSH_PULL) |
	      IMU_BOSCH_BMI323_REG_VALUE(IO_INT_CTRL, INT1_OUTPUT_EN, EN);

	return bosch_bmi323_bus_write_words(dev, IMU_BOSCH_BMI323_REG_IO_INT_CTRL, &buf, 1);
}

static void bosch_bmi323_irq_callback_handler(struct k_work *item)
{
	struct bosch_bmi323_data *data =
		CONTAINER_OF(item, struct bosch_bmi323_data, callback_work);

	k_mutex_lock(&data->lock, K_FOREVER);

	if (data->trigger_handler != NULL) {
		data->trigger_handler(data->dev, data->trigger);
	}

	k_mutex_unlock(&data->lock);
}

static int bosch_bmi323_pm_resume(const struct device *dev)
{
	int ret;

	ret = bosch_bmi323_bus_init(dev);

	if (ret < 0) {
		LOG_WRN("Failed to init bus");

		return ret;
	}

	ret = bosch_bmi323_validate_chip_id(dev);

	if (ret < 0) {
		LOG_WRN("Failed to validate chip id");

		return ret;
	}

	ret = bosch_bmi323_soft_reset(dev);

	if (ret < 0) {
		LOG_WRN("Failed to soft reset chip");

		return ret;
	}

	ret = bosch_bmi323_bus_init(dev);

	if (ret < 0) {
		LOG_WRN("Failed to re-init bus");

		return ret;
	}

	ret = bosch_bmi323_enable_feature_engine(dev);

	if (ret < 0) {
		LOG_WRN("Failed to enable feature engine");

		return ret;
	}

	ret = bosch_bmi323_init_int1(dev);

	if (ret < 0) {
		LOG_WRN("Failed to enable INT1");
	}

	return ret;
}

#ifdef CONFIG_PM_DEVICE
static int bosch_bmi323_pm_suspend(const struct device *dev)
{
	/* Soft reset device to put it into suspend */
	return bosch_bmi323_soft_reset(dev);
}
#endif /* CONFIG_PM_DEVICE */

#ifdef CONFIG_PM_DEVICE
static int bosch_bmi323_pm_action(const struct device *dev, enum pm_device_action action)
{
	struct bosch_bmi323_data *data = (struct bosch_bmi323_data *)dev->data;
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		ret = bosch_bmi323_pm_resume(dev);

		break;

	case PM_DEVICE_ACTION_SUSPEND:
		ret = bosch_bmi323_pm_suspend(dev);

		break;

	default:
		ret = -ENOTSUP;

		break;
	}

	k_mutex_unlock(&data->lock);

	return ret;
}
#endif /* CONFIG_PM_DEVICE */

static int bosch_bmi323_init(const struct device *dev)
{
	struct bosch_bmi323_data *data = (struct bosch_bmi323_data *)dev->data;
	int ret;

	k_mutex_init(&data->lock);

	k_work_init(&data->callback_work, bosch_bmi323_irq_callback_handler);

	data->dev = dev;

	ret = bosch_bmi323_init_irq(dev);

	if (ret < 0) {
		LOG_WRN("Failed to init irq");

		return ret;
	}

#ifndef CONFIG_PM_DEVICE_RUNTIME
	ret = bosch_bmi323_pm_resume(dev);

	if (ret < 0) {
		LOG_WRN("Failed to initialize device");
	}
#else
	pm_device_init_suspended(dev);

	ret = pm_device_runtime_enable(dev);

	if (ret < 0) {
		LOG_WRN("Failed to enable device pm runtime");
	}
#endif /* CONFIG_PM_DEVICE_RUNTIME */

	return ret;
}

/*
 * Currently only support for the SPI bus is implemented. This shall be updated to
 * select the appropriate bus once I2C is implemented.
 */
#define BMI323_DEVICE_BUS(inst)                                                                    \
	BUILD_ASSERT(DT_INST_ON_BUS(inst, spi), "Unimplemented bus");                              \
	BMI323_DEVICE_SPI_BUS(inst)

#define BMI323_DEVICE(inst)                                                                        \
	static struct bosch_bmi323_data bosch_bmi323_data_##inst;                                  \
                                                                                                   \
	BMI323_DEVICE_BUS(inst);                                                                   \
                                                                                                   \
	static void bosch_bmi323_irq_callback##inst(const struct device *dev,                      \
						    struct gpio_callback *cb, uint32_t pins)       \
	{                                                                                          \
		bosch_bmi323_irq_callback(DEVICE_DT_INST_GET(inst));                               \
	}                                                                                          \
                                                                                                   \
	static const struct bosch_bmi323_config bosch_bmi323_config_##inst = {                     \
		.bus = &bosch_bmi323_bus_api##inst,                                                \
		.int_gpio = GPIO_DT_SPEC_INST_GET(inst, int_gpios),                                \
		.int_gpio_callback = bosch_bmi323_irq_callback##inst,                              \
	};                                                                                         \
                                                                                                   \
	PM_DEVICE_DT_INST_DEFINE(inst, bosch_bmi323_pm_action);                                    \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, bosch_bmi323_init, PM_DEVICE_DT_INST_GET(inst),         \
				     &bosch_bmi323_data_##inst, &bosch_bmi323_config_##inst,       \
				     POST_KERNEL, 99, &bosch_bmi323_api);

DT_INST_FOREACH_STATUS_OKAY(BMI323_DEVICE)
