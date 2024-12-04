/*
 * Copyright (c) 2023 Tridonic
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ltr_f216a

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util_macro.h>

LOG_MODULE_REGISTER(ltrf216a, CONFIG_SENSOR_LOG_LEVEL);

/*
 * Driver for I2C illuminance sensor LiteOn LTR-F216A.
 *
 * Datasheet:
 * <https://optoelectronics.liteon.com/upload/download/DS86-2019-0016/LTR-F216A_Final_DS_V1.4.PDF>

 * 7bit Address 0x53
 * 8bit Address 0xA6 read
 * 8bit Address 0xA7 write

 * NOT IMPLEMENTED:
 * - Interrupt
 * - Modifying Gain (using default x3)
 * - Modifying Resolution (using default 100ms)
 * - Modifying Measurement Rate (using default 100ms)
 * - Modifying Window Factor (using default 1)
 */

#define LTRF216A_ALS_RESET_MASK  BIT(4)
#define LTRF216A_ALS_ENABLE_MASK BIT(1)

#define LTRF216A_ALS_DATA_STATUS BIT(3)

/* Part Number ID 7:4 0b1011 = 0xB */
/* Revision ID 3:0 0b0001 = 0x1*/
#define LTRF216A_PART_ID_VALUE 0xB1

#define LTRF216A_MAIN_CTRL        0x00
#define LTRF216A_ALS_MEAS_RES     0x04
#define LTRF216A_ALS_GAIN         0x05
#define LTRF216A_PART_ID          0x06
#define LTRF216A_MAIN_STATUS      0x07
#define LTRF216A_ALS_CLEAR_DATA_0 0x0a
#define LTRF216A_ALS_CLEAR_DATA_1 0x0b
#define LTRF216A_ALS_CLEAR_DATA_2 0x0c
#define LTRF216A_ALS_DATA_0       0x0d
#define LTRF216A_ALS_DATA_1       0x0e
#define LTRF216A_ALS_DATA_2       0x0f
#define LTRF216A_INT_CFG          0x19
#define LTRF216A_INT_PST          0x1a
#define LTRF216A_ALS_THRES_UP_0   0x21
#define LTRF216A_ALS_THRES_UP_1   0x22
#define LTRF216A_ALS_THRES_UP_2   0x23
#define LTRF216A_ALS_THRES_LOW_0  0x24
#define LTRF216A_ALS_THRES_LOW_1  0x25
#define LTRF216A_ALS_THRES_LOW_2  0x26

#define LTRF216A_WIN_FAC 1

#define LTRF216A_NUMBER_DATA_REGISTERS 3

struct ltrf216a_data {
	uint8_t sample[3];
};

struct ltrf216a_config {
	struct i2c_dt_spec i2c;
};

static int ltrf216a_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct ltrf216a_data *drv_data = dev->data;
	const struct ltrf216a_config *config = dev->config;
	uint8_t status;
	int result;
	uint8_t value;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_LIGHT);

	value = LTRF216A_ALS_ENABLE_MASK;
	result = i2c_reg_write_byte_dt(&config->i2c, LTRF216A_MAIN_CTRL, LTRF216A_ALS_ENABLE_MASK);
	if (result != 0) {
		LOG_ERR("ltfr216a: enable failed");
		return result;
	}

	result = i2c_reg_read_byte_dt(&config->i2c, LTRF216A_MAIN_STATUS, &status);
	if (result != 0) {
		LOG_ERR("ltfr216a: read main status failed");
		return -EIO;
	}

	if ((status & LTRF216A_ALS_DATA_STATUS) == 0) {
		LOG_WRN("ltfr216a: main status not ready");
		return -EBUSY;
	}

	if (i2c_burst_read_dt(&config->i2c, LTRF216A_ALS_DATA_0, drv_data->sample,
			      LTRF216A_NUMBER_DATA_REGISTERS) != 0) {
		LOG_ERR("ltfr216a: reading samples failed");
		return -EIO;
	}

	return 0;
}

static int ltrf216a_channel_get(const struct device *dev, enum sensor_channel chan,
				struct sensor_value *val)
{
	struct ltrf216a_data *drv_data = dev->data;

	if (chan != SENSOR_CHAN_LIGHT) {
		return -ENOTSUP;
	}

	uint32_t greendata = sys_get_le24(drv_data->sample);

	/*
	 * 0.45 -> 45 / 100, multiplied by 1000000 for millilux
	 * gain 3 (default), integration time 100ms=1
	 */
	uint64_t microlux = ((uint64_t)greendata * 45000 * LTRF216A_WIN_FAC * 10) / (3 * 1);

	val->val1 = microlux / 1000000;
	val->val2 = microlux % 1000000;

	return 0;
}

static DEVICE_API(sensor, ltrf216a_driver_api) = {
	.sample_fetch = ltrf216a_sample_fetch,
	.channel_get = ltrf216a_channel_get,
};

static int ltrf216a_chip_init(const struct device *dev)
{
	const struct ltrf216a_config *config = dev->config;
	uint8_t value;

	if (!i2c_is_ready_dt(&config->i2c)) {
		LOG_ERR("Bus device is not ready");
		return -ENODEV;
	}

	if (i2c_reg_read_byte_dt(&config->i2c, LTRF216A_PART_ID, &value) != 0) {
		return -EIO;
	}

	if (value != LTRF216A_PART_ID_VALUE) {
		LOG_ERR("Bad manufacturer id 0x%x", value);
		return -ENOTSUP;
	}

	return 0;
}

#define LTRF216A_DEFINE(inst)                                                                      \
	static struct ltrf216a_data ltrf216a_data_##inst;                                          \
                                                                                                   \
	static const struct ltrf216a_config ltrf216a_config_##inst = {                             \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
	};                                                                                         \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, ltrf216a_chip_init, NULL, &ltrf216a_data_##inst,        \
				     &ltrf216a_config_##inst, POST_KERNEL,                         \
				     CONFIG_SENSOR_INIT_PRIORITY, &ltrf216a_driver_api);

DT_INST_FOREACH_STATUS_OKAY(LTRF216A_DEFINE)
