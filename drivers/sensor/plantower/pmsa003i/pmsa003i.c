/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT plantower_pmsa003i

/* sensor pmsa003i.c - Driver for plantower PMSA003I sensor
 * PMSA003I datasheet:
 * https://cdn-shop.adafruit.com/product-files/4632/4505_PMSA003I_series_data_manual_English_V2.6.pdf
 */

#include <errno.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

LOG_MODULE_REGISTER(PMSA003I, CONFIG_SENSOR_LOG_LEVEL);

#define PMSA003I_START_REG       0x00U
#define PMSA003I_FRAME_SIZE      32U
#define PMSA003I_FRAME_LENGTH    28U
#define PMSA003I_CHECKSUM_OFFSET 30U

#define PMSA003I_START_BYTE_1 0x42U
#define PMSA003I_START_BYTE_2 0x4dU

struct pmsa003i_config {
	struct i2c_dt_spec bus;
	struct gpio_dt_spec reset_gpio;
	struct gpio_dt_spec set_gpio;
};

struct pmsa003i_data {
	uint16_t pm_1_0_cf;
	uint16_t pm_2_5_cf;
	uint16_t pm_10_cf;
	uint16_t pm_1_0;
	uint16_t pm_2_5;
	uint16_t pm_10;
	uint16_t pm_0_3_count;
	uint16_t pm_0_5_count;
	uint16_t pm_1_0_count;
	uint16_t pm_2_5_count;
	uint16_t pm_5_0_count;
	uint16_t pm_10_0_count;
};

static int pmsa003i_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct pmsa003i_data *drv_data = dev->data;
	const struct pmsa003i_config *cfg = dev->config;
	uint8_t start_reg = PMSA003I_START_REG;
	uint8_t pmsa003i_receive_buffer[PMSA003I_FRAME_SIZE];
	uint16_t calculated_checksum = 0U;
	uint16_t received_checksum;
	uint16_t frame_length;
	int ret;

	/*
	 * Like the existing PMS7003 driver, PMSA003I always fetches the
	 * complete sample regardless of the requested channel.
	 */
	(void)chan;

	ret = i2c_write_read_dt(&cfg->bus, &start_reg, sizeof(start_reg), pmsa003i_receive_buffer,
				sizeof(pmsa003i_receive_buffer));
	if (ret < 0) {
		LOG_ERR("failed to read sample (%d)", ret);
		return ret;
	}

	if ((pmsa003i_receive_buffer[0] != PMSA003I_START_BYTE_1) ||
	    (pmsa003i_receive_buffer[1] != PMSA003I_START_BYTE_2)) {
		LOG_WRN("invalid start bytes: 0x%02x 0x%02x", pmsa003i_receive_buffer[0],
			pmsa003i_receive_buffer[1]);

		return -EIO;
	}

	frame_length = sys_get_be16(&pmsa003i_receive_buffer[2]);

	if (frame_length != PMSA003I_FRAME_LENGTH) {
		LOG_WRN("invalid frame length: %u", frame_length);
		return -EIO;
	}

	for (size_t i = 0U; i < PMSA003I_CHECKSUM_OFFSET; i++) {
		calculated_checksum += pmsa003i_receive_buffer[i];
	}

	received_checksum = sys_get_be16(&pmsa003i_receive_buffer[PMSA003I_CHECKSUM_OFFSET]);

	if (calculated_checksum != received_checksum) {
		LOG_WRN("invalid checksum: calculated 0x%04x, received 0x%04x", calculated_checksum,
			received_checksum);

		return -EIO;
	}

	drv_data->pm_1_0_cf = sys_get_be16(&pmsa003i_receive_buffer[4]);
	drv_data->pm_2_5_cf = sys_get_be16(&pmsa003i_receive_buffer[6]);
	drv_data->pm_10_cf = sys_get_be16(&pmsa003i_receive_buffer[8]);

	drv_data->pm_1_0 = sys_get_be16(&pmsa003i_receive_buffer[10]);
	drv_data->pm_2_5 = sys_get_be16(&pmsa003i_receive_buffer[12]);
	drv_data->pm_10 = sys_get_be16(&pmsa003i_receive_buffer[14]);

	drv_data->pm_0_3_count = sys_get_be16(&pmsa003i_receive_buffer[16]);
	drv_data->pm_0_5_count = sys_get_be16(&pmsa003i_receive_buffer[18]);
	drv_data->pm_1_0_count = sys_get_be16(&pmsa003i_receive_buffer[20]);
	drv_data->pm_2_5_count = sys_get_be16(&pmsa003i_receive_buffer[22]);
	drv_data->pm_5_0_count = sys_get_be16(&pmsa003i_receive_buffer[24]);
	drv_data->pm_10_0_count = sys_get_be16(&pmsa003i_receive_buffer[26]);

	LOG_DBG("pm1.0_cf = %d", drv_data->pm_1_0_cf);
	LOG_DBG("pm2.5_cf = %d", drv_data->pm_2_5_cf);
	LOG_DBG("pm10_cf = %d", drv_data->pm_10_cf);
	LOG_DBG("pm1.0 = %d", drv_data->pm_1_0);
	LOG_DBG("pm2.5 = %d", drv_data->pm_2_5);
	LOG_DBG("pm10 = %d", drv_data->pm_10);
	LOG_DBG("pm0.3_count = %d", drv_data->pm_0_3_count);
	LOG_DBG("pm0.5_count = %d", drv_data->pm_0_5_count);
	LOG_DBG("pm1.0_count = %d", drv_data->pm_1_0_count);
	LOG_DBG("pm2.5_count = %d", drv_data->pm_2_5_count);
	LOG_DBG("pm5.0_count = %d", drv_data->pm_5_0_count);
	LOG_DBG("pm10_count = %d", drv_data->pm_10_0_count);

	return 0;
}

static int pmsa003i_channel_get(const struct device *dev, enum sensor_channel chan,
				struct sensor_value *val)
{
	struct pmsa003i_data *drv_data = dev->data;

	if (chan == SENSOR_CHAN_PM_1_0) {
		val->val1 = drv_data->pm_1_0;
	} else if (chan == SENSOR_CHAN_PM_2_5) {
		val->val1 = drv_data->pm_2_5;
	} else if (chan == SENSOR_CHAN_PM_10) {
		val->val1 = drv_data->pm_10;
	} else if (chan == SENSOR_CHAN_PM_1_0_CF) {
		val->val1 = drv_data->pm_1_0_cf;
	} else if (chan == SENSOR_CHAN_PM_2_5_CF) {
		val->val1 = drv_data->pm_2_5_cf;
	} else if (chan == SENSOR_CHAN_PM_10_CF) {
		val->val1 = drv_data->pm_10_cf;
	} else if (chan == SENSOR_CHAN_PM_0_3_COUNT) {
		val->val1 = drv_data->pm_0_3_count;
	} else if (chan == SENSOR_CHAN_PM_0_5_COUNT) {
		val->val1 = drv_data->pm_0_5_count;
	} else if (chan == SENSOR_CHAN_PM_1_0_COUNT) {
		val->val1 = drv_data->pm_1_0_count;
	} else if (chan == SENSOR_CHAN_PM_2_5_COUNT) {
		val->val1 = drv_data->pm_2_5_count;
	} else if (chan == SENSOR_CHAN_PM_5_COUNT) {
		val->val1 = drv_data->pm_5_0_count;
	} else if (chan == SENSOR_CHAN_PM_10_COUNT) {
		val->val1 = drv_data->pm_10_0_count;
	} else {
		return -ENOTSUP;
	}

	val->val2 = 0;

	return 0;
}

static DEVICE_API(sensor, pmsa003i_api) = {
	.sample_fetch = &pmsa003i_sample_fetch,
	.channel_get = &pmsa003i_channel_get,
};

static int pmsa003i_init(const struct device *dev)
{
	const struct pmsa003i_config *cfg = dev->config;
	int ret;

	if (!i2c_is_ready_dt(&cfg->bus)) {
		LOG_ERR_DEVICE_NOT_READY(cfg->bus.bus);
		return -ENODEV;
	}

	if (cfg->set_gpio.port != NULL) {
		if (!gpio_is_ready_dt(&cfg->set_gpio)) {
			LOG_ERR_DEVICE_NOT_READY(cfg->set_gpio.port);
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(&cfg->set_gpio, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			LOG_ERR("failed to configure set gpio (%d)", ret);
			return ret;
		}
	}

	if (cfg->reset_gpio.port != NULL) {
		if (!gpio_is_ready_dt(&cfg->reset_gpio)) {
			LOG_ERR_DEVICE_NOT_READY(cfg->reset_gpio.port);
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(&cfg->reset_gpio, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			LOG_ERR("failed to configure reset gpio (%d)", ret);
			return ret;
		}

		k_msleep(1);
		gpio_pin_set_dt(&cfg->reset_gpio, 0);
	}

	return 0;
}

#define PMSA003I_DEFINE(inst)                                                                      \
	static struct pmsa003i_data pmsa003i_data_##inst;                                          \
                                                                                                   \
	static const struct pmsa003i_config pmsa003i_config_##inst = {                             \
		.bus = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.reset_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, {0}),                    \
		.set_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, set_gpios, {0}),                        \
	};                                                                                         \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, &pmsa003i_init, NULL, &pmsa003i_data_##inst,            \
				     &pmsa003i_config_##inst, POST_KERNEL,                         \
				     CONFIG_SENSOR_INIT_PRIORITY, &pmsa003i_api);

DT_INST_FOREACH_STATUS_OKAY(PMSA003I_DEFINE)
