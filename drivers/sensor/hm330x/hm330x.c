/*
 * Copyright (c) 2023 Benjamin Cabé <benjamin@zephyrproject.org>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT seeed_hm330x

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>

LOG_MODULE_REGISTER(HM3301, CONFIG_SENSOR_LOG_LEVEL);

#define HM330X_SELECT_COMM_CMD	0X88

#define HM330X_PM_1_0_ATM	10
#define HM330X_PM_2_5_ATM	12
#define HM330X_PM_10_ATM	14

#define HM330X_FRAME_LEN	29

struct hm330x_data {
	uint16_t pm_1_0_sample;
	uint16_t pm_2_5_sample;
	uint16_t pm_10_sample;
};

struct hm330x_config {
	struct i2c_dt_spec i2c;
};

static int hm330x_sample_fetch(const struct device *dev,
			       enum sensor_channel chan)
{
	struct hm330x_data *drv_data = dev->data;
	const struct hm330x_config *config = dev->config;
	uint8_t buf[HM330X_FRAME_LEN];
	uint8_t checksum = 0;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

	if (i2c_burst_read_dt(&config->i2c,
			      0, buf, HM330X_FRAME_LEN) < 0) {
		return -EIO;
	}

	drv_data->pm_1_0_sample = (buf[HM330X_PM_1_0_ATM] << 8) | buf[HM330X_PM_1_0_ATM + 1];
	drv_data->pm_2_5_sample = (buf[HM330X_PM_2_5_ATM] << 8) | buf[HM330X_PM_2_5_ATM + 1];
	drv_data->pm_10_sample = (buf[HM330X_PM_10_ATM] << 8) | buf[HM330X_PM_10_ATM + 1];

	for (int i = 0; i < HM330X_FRAME_LEN - 1; i++) {
		checksum += buf[i];
	}
	if (checksum != buf[HM330X_FRAME_LEN - 1]) {
		LOG_ERR("Checksum error");
		return -EBADMSG;
	}

	return 0;
}

static int hm330x_channel_get(const struct device *dev,
			      enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct hm330x_data *drv_data = dev->data;

	if (chan == SENSOR_CHAN_PM_1_0) {
		val->val1 = drv_data->pm_1_0_sample;
		val->val2 = 0;
	} else if (chan == SENSOR_CHAN_PM_2_5) {
		val->val1 = drv_data->pm_2_5_sample;
		val->val2 = 0;
	} else if (chan == SENSOR_CHAN_PM_10) {
		val->val1 = drv_data->pm_10_sample;
		val->val2 = 0;
	} else {
		return -ENOTSUP;
	}

	return 0;
}

static const struct sensor_driver_api hm330x_driver_api = {
	.sample_fetch = hm330x_sample_fetch,
	.channel_get = hm330x_channel_get
};

int hm330x_init(const struct device *dev)
{
	const struct hm330x_config *config = dev->config;

	if (!i2c_is_ready_dt(&config->i2c)) {
		LOG_ERR("I2C bus device not ready");
		return -ENODEV;
	}

	/** Enable I2C communications (module defaults to UART) */
	if (i2c_reg_write_byte_dt(&config->i2c, 0, HM330X_SELECT_COMM_CMD) < 0) {
		LOG_ERR("Failed to switch to I2C");
		return -EIO;
	}

	return 0;
}

#define HM330X_DEFINE(inst)									\
	static struct hm330x_data hm330x_data_##inst;						\
												\
	static const struct hm330x_config hm330x_config##inst = {				\
		.i2c = I2C_DT_SPEC_INST_GET(inst)						\
	};											\
												\
	SENSOR_DEVICE_DT_INST_DEFINE(inst, hm330x_init, NULL, &hm330x_data_##inst,		\
			      &hm330x_config##inst, POST_KERNEL,				\
			      CONFIG_SENSOR_INIT_PRIORITY, &hm330x_driver_api);			\

DT_INST_FOREACH_STATUS_OKAY(HM330X_DEFINE)
