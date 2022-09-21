/*
 * Copyright (c) 2022 Elektronikutvecklingsbyrån EUB AB
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT bosch_bma400

#include <drivers/i2c.h>
#include <init.h>
#include <drivers/sensor.h>
#include <sys/__assert.h>
#include <stdint.h>
#include <logging/log.h>

#include "bma400.h"

LOG_MODULE_REGISTER(bma400, CONFIG_SENSOR_LOG_LEVEL);

int16_t bma400_parse_accval(uint8_t *bytes)
{
	int16_t out = bytes[0] + 256 * (bytes[1] & 0x0F);
	if (out > 2047) {
		out -= 4096;
	}
	return out;
}

static int bma400_sample_fetch(const struct device *dev,
		enum sensor_channel chan)
{
	struct bma400_data *drv_data = dev->data;
	int8_t buf[6];

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

	if (i2c_burst_read(drv_data->i2c, drv_data->addr,
				BMA400_REG_ACCEL_DATA, buf, 6) < 0) {
		LOG_ERR("Could not read accel axis data");
		return -EIO;
	}

	drv_data->x_sample = bma400_parse_accval(&buf[0]);
	drv_data->y_sample = bma400_parse_accval(&buf[2]);
	drv_data->z_sample = bma400_parse_accval(&buf[4]);

	int8_t temp;
	if (i2c_reg_read_byte(drv_data->i2c, drv_data->addr,
				BMA400_REG_TEMP_DATA, &temp) < 0) {
		LOG_ERR("Could not read temperature data");
	}
	drv_data->temp_sample = temp;

	return 0;
}

static void bma400_channel_accel_convert(struct sensor_value *val,
		int64_t raw_val)
{
	raw_val = (raw_val * ((1 << (BMA400_RANGE + 1)) * SENSOR_G))
		/ (1 << 11);
	val->val1 = raw_val / 1000000;
	val->val2 = raw_val % 1000000;

	/* Normalize val to make sure val->val2 is positive */
	if (val->val2 < 0) {
		val->val1 -= 1;
		val->val2 += 1000000;
	}
}

static void bma400_temp_convert(struct sensor_value *val, int8_t raw_val)
{
	// 0x7F => 87,5C, 0x80 => -40.0 C
	val->val1 = (24 + (raw_val / 2));

	val->val2 = 0;
	if (raw_val % 2 != 0) {
		val->val2 = 5*10e5;
	}
}

static int bma400_channel_get(const struct device *dev,
		enum sensor_channel chan,
		struct sensor_value *val)
{
	struct bma400_data *drv_data = dev->data;

	/*
	 * See datasheet "Sensor data" section for
	 * more details on processing sample data.
	 */
	if (chan == SENSOR_CHAN_ACCEL_X) {
		bma400_channel_accel_convert(val, drv_data->x_sample);
	} else if (chan == SENSOR_CHAN_ACCEL_Y) {
		bma400_channel_accel_convert(val, drv_data->y_sample);
	} else if (chan == SENSOR_CHAN_ACCEL_Z) {
		bma400_channel_accel_convert(val, drv_data->z_sample);
	} else if (chan == SENSOR_CHAN_ACCEL_XYZ) {
		bma400_channel_accel_convert(val, drv_data->x_sample);
		bma400_channel_accel_convert(val + 1, drv_data->y_sample);
		bma400_channel_accel_convert(val + 2, drv_data->z_sample);
	} else if (chan == SENSOR_CHAN_DIE_TEMP) {
		bma400_temp_convert(val, drv_data->temp_sample);
	} else {
		return -ENOTSUP;
	}

	return 0;
}

static const struct sensor_driver_api bma400_driver_api = {
#if CONFIG_BMA400_TRIGGER
	.attr_set = bma400_attr_set,
	.trigger_set = bma400_trigger_set,
#endif
	.sample_fetch = bma400_sample_fetch,
	.channel_get = bma400_channel_get,
};

int bma400_init(const struct device *dev)
{
	struct bma400_data *drv_data = dev->data;
	uint8_t id = 0U;

	/* read device ID */
	if (i2c_reg_read_byte(drv_data->i2c, drv_data->addr,
				BMA400_REG_CHIP_ID, &id) < 0) {
		LOG_ERR("Could not read chip id");
		return -EIO;
	}

	if (id != BMA400_CHIP_ID) {
		LOG_ERR("Unexpected chip id (%x)", id);
		return -EIO;
	}

	/* Soft reset the device to ensure state is not weird */
	if (i2c_reg_write_byte(drv_data->i2c, drv_data->addr,
				BMA400_REG_COMMAND, BMA400_SOFT_RESET_CMD) < 0) {
		LOG_ERR("Could not soft reset");
		return -EIO;
	}

	k_sleep(K_MSEC(5));

	/* set odr */
	if (i2c_reg_update_byte(drv_data->i2c, drv_data->addr,
				BMA400_REG_ACCEL_CONFIG_1,
				BMA400_ACCEL_ODR_MSK,
				BMA400_ODR) < 0) {
		LOG_ERR("Could not set data rate");
		return -EIO;
	}

	/* set osr */
	if (i2c_reg_update_byte(drv_data->i2c, drv_data->addr,
				BMA400_REG_ACCEL_CONFIG_1,
				BMA400_OSR_MSK,
				BMA400_ACCEL_OSR_SETTING_3 << BMA400_OSR_POS) < 0) {
		LOG_ERR("Could not set OSR to high");
		return -EIO;
	}

	/* set g-range */
	if (i2c_reg_update_byte(drv_data->i2c, drv_data->addr,
				BMA400_REG_ACCEL_CONFIG_1,
				BMA400_ACCEL_RANGE_MSK,
				BMA400_RANGE << BMA400_ACCEL_RANGE_POS) < 0) {
		LOG_ERR("Could not set data g-range");
		return -EIO;
	}

	if (i2c_reg_update_byte(drv_data->i2c, drv_data->addr,
				BMA400_REG_ACCEL_CONFIG_2,
				BMA400_DATA_FILTER_MSK,
				BMA400_DATA_SRC_ACCEL_FILT_2 << BMA400_DATA_FILTER_POS) < 0) {
		LOG_ERR("Could not set data filter");
		return -EIO;
	}

#ifdef CONFIG_BMA400_TRIGGER
	drv_data->dev = dev;

	if (bma400_init_interrupt(dev) < 0) {
		LOG_ERR("Could not initialize interrupts");
		return -EIO;
	}
#endif

	/* enter normal mode */
	if (i2c_reg_update_byte(drv_data->i2c, drv_data->addr,
				BMA400_REG_ACCEL_CONFIG_0,
				BMA400_POWER_MODE_MSK,
				BMA400_MODE_NORMAL) < 0) {
		LOG_ERR("Could not set normal mode");
		return -EIO;
	}

	return 0;
}

struct bma400_data bma400_driver = {
	.i2c = DEVICE_DT_GET(DT_INST_BUS(0)),
	.addr = DT_INST_REG_ADDR(0),
	.range = BMA400_RANGE,

	// FIXME: This is not configurable so should just be a #define
	.resolution = 12,
};

DEVICE_DT_INST_DEFINE(0, bma400_init, NULL, &bma400_driver,
		NULL, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,
		&bma400_driver_api);
