/*
 * Copyright (c) 2017 Intel Corporation
 * Copyright (c) 2018 Phytec Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT avago_apds9253

/* @file
 * @brief driver for APDS9253 ALS/RGB/
 */

#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>

#include "apds9253.h"

#define BYTES_PER_VALUE   3
#define VALUES_PER_SAMPLE 4

LOG_MODULE_REGISTER(APDS9253, CONFIG_SENSOR_LOG_LEVEL);

static inline void apds9253_setup_int(const struct apds9253_config *cfg, bool enable)
{
	gpio_flags_t flags = enable ? GPIO_INT_EDGE_TO_ACTIVE : GPIO_INT_DISABLE;

	gpio_pin_interrupt_configure_dt(&cfg->int_gpio, flags);
}

static void apds9253_handle_cb(struct apds9253_data *drv_data)
{
	apds9253_setup_int(drv_data->dev->config, false);
}

static void apds9253_gpio_callback(const struct device *dev, struct gpio_callback *cb,
				   uint32_t pins)
{
	struct apds9253_data *drv_data = CONTAINER_OF(cb, struct apds9253_data, gpio_cb);

	apds9253_handle_cb(drv_data);
}

static uint32_t get_value_from_buf(int idx, const uint8_t *buf)
{
	size_t offset = BYTES_PER_VALUE * idx;

	return sys_get_le24(&buf[offset]);
}

static int apds9253_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct apds9253_config *config = dev->config;
	struct apds9253_data *data = dev->data;
	uint8_t tmp;
	uint8_t buf[BYTES_PER_VALUE * VALUES_PER_SAMPLE];

	if (chan != SENSOR_CHAN_ALL) {
		LOG_ERR("Unsupported sensor channel");
		return -ENOTSUP;
	}

	if (i2c_reg_update_byte_dt(&config->i2c, APDS9253_MAIN_CTRL_REG, APDS9253_MAIN_CTRL_LS_EN,
				   APDS9253_MAIN_CTRL_LS_EN)) {
		LOG_ERR("Power on bit not set.");
		return -EIO;
	}

	if (config->interrupt_enabled) {
		k_sem_take(&data->data_sem, K_FOREVER);
	}

	if (i2c_reg_read_byte_dt(&config->i2c, APDS9253_MAIN_STATUS_REG, &tmp)) {
		return -EIO;
	}

	LOG_DBG("status: 0x%x", tmp);

	if (tmp & APDS9253_MAIN_STATUS_LS_STATUS) {
		if (i2c_burst_read_dt(&config->i2c, APDS9253_LS_DATA_BASE, buf, sizeof(buf))) {
			return -EIO;
		}

		for (int i = 0; i < VALUES_PER_SAMPLE; ++i) {
			data->sample_crgb[i] = get_value_from_buf(i, buf);
		}

		LOG_DBG("IR 0x%x GREEN 0x%x BLUE 0x%x RED 0x%x\n", data->sample_crgb[0],
			data->sample_crgb[1], data->sample_crgb[2], data->sample_crgb[3]);
	}

	return 0;
}

static int apds9253_channel_get(const struct device *dev, enum sensor_channel chan,
				struct sensor_value *val)
{
	struct apds9253_data *data = dev->data;

	val->val2 = 0;

	switch (chan) {
	case SENSOR_CHAN_IR:
		val->val1 = sys_le32_to_cpu(data->sample_crgb[0]);
		break;
	case SENSOR_CHAN_GREEN:
		val->val1 = sys_le32_to_cpu(data->sample_crgb[1]);
		break;
	case SENSOR_CHAN_BLUE:
		val->val1 = sys_le32_to_cpu(data->sample_crgb[2]);
		break;
	case SENSOR_CHAN_RED:
		val->val1 = sys_le32_to_cpu(data->sample_crgb[3]);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int apds9253_sensor_setup(const struct device *dev)
{
	const struct apds9253_config *config = dev->config;
	uint8_t chip_id;

	if (i2c_reg_read_byte_dt(&config->i2c, APDS9253_PART_ID, &chip_id)) {
		LOG_ERR("Failed reading chip id");
		return -EIO;
	}

	if ((chip_id & APDS9253_PART_ID_ID_MASK) != APDS9253_DEVICE_PART_ID) {
		LOG_ERR("Invalid chip id 0x%x", chip_id);
		return -EIO;
	}

	if (i2c_reg_update_byte_dt(&config->i2c, APDS9253_LS_GAIN_REG, APDS9253_LS_GAIN_MASK,
				   (config->ls_gain & APDS9253_LS_GAIN_MASK))) {
		LOG_ERR("Light sensor gain is not set");
		return -EIO;
	}

	if (i2c_reg_update_byte_dt(&config->i2c, APDS9253_LS_MEAS_RATE_REG,
				   APDS9253_LS_MEAS_RATE_RES_MASK,
				   (config->ls_resolution & APDS9253_LS_MEAS_RATE_RES_MASK))) {
		LOG_ERR("Light sensor resolution is not set");
		return -EIO;
	}

	if (i2c_reg_update_byte_dt(&config->i2c, APDS9253_LS_MEAS_RATE_REG,
				   APDS9253_LS_MEAS_RATE_MES_MASK,
				   (config->ls_rate & APDS9253_LS_MEAS_RATE_MES_MASK))) {
		LOG_ERR("Light sensor rate is not set");
		return -EIO;
	}

	if (i2c_reg_update_byte_dt(&config->i2c, APDS9253_MAIN_CTRL_REG,
				   APDS9253_MAIN_CTRL_RGB_MODE, APDS9253_MAIN_CTRL_RGB_MODE)) {
		LOG_ERR("Enable RGB mode failed");
		return -EIO;
	}

	return 0;
}

static int apds9253_init_interrupt(const struct device *dev)
{
	const struct apds9253_config *config = dev->config;
	struct apds9253_data *drv_data = dev->data;
	int ret = 0;

	if (!gpio_is_ready_dt(&config->int_gpio)) {
		LOG_ERR("%s: device %s is not ready", dev->name, config->int_gpio.port->name);
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&config->int_gpio, GPIO_INPUT);
	if (!ret) {
		LOG_ERR("Failed to configure gpio direction");
		return ret;
	}

	gpio_init_callback(&drv_data->gpio_cb, apds9253_gpio_callback, BIT(config->int_gpio.pin));

	if (gpio_add_callback(config->int_gpio.port, &drv_data->gpio_cb) < 0) {
		LOG_ERR("Failed to set gpio callback!");
		return -EIO;
	}

	drv_data->dev = dev;

	k_sem_init(&drv_data->data_sem, 0, K_SEM_MAX_LIMIT);
	apds9253_setup_int(config, true);

	if (gpio_pin_get_dt(&config->int_gpio) > 0) {
		apds9253_handle_cb(drv_data);
	}

	return 0;
}

static int apds9253_init(const struct device *dev)
{
	const struct apds9253_config *config = dev->config;

	/* Initialize time 500 us */
	k_msleep(1);

	if (!i2c_is_ready_dt(&config->i2c)) {
		LOG_ERR("Bus device is not ready");
		return -EINVAL;
	}

	if (apds9253_sensor_setup(dev) < 0) {
		LOG_ERR("Failed to setup device!");
		return -EIO;
	}

	if (config->interrupt_enabled && apds9253_init_interrupt(dev) < 0) {
		LOG_ERR("Failed to initialize interrupt!");
		return -EIO;
	}

	return 0;
}

static DEVICE_API(sensor, apds9253_driver_api) = {
	.sample_fetch = &apds9253_sample_fetch,
	.channel_get = &apds9253_channel_get,
};

#define APDS9253_INIT(n)                                                                           \
	static struct apds9253_data apds9253_data_##n = {                                          \
		.sample_crgb = {0},                                                                \
		.pdata = 0U,                                                                       \
	};                                                                                         \
                                                                                                   \
	static const struct apds9253_config apds9253_config_##n = {                                \
		.i2c = I2C_DT_SPEC_INST_GET(n),                                                    \
		.interrupt_enabled = DT_INST_NODE_HAS_PROP(n, int_gpios),                          \
		.int_gpio = GPIO_DT_SPEC_INST_GET_OR(n, int_gpios, {}),                            \
		.ls_rate = DT_INST_PROP(n, rate),                                                  \
		.ls_resolution = DT_INST_PROP(n, resolution),                                      \
		.ls_gain = DT_INST_PROP(n, gain),                                                  \
	};                                                                                         \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(n, apds9253_init, NULL, &apds9253_data_##n,                   \
				     &apds9253_config_##n, POST_KERNEL,                            \
				     CONFIG_SENSOR_INIT_PRIORITY, &apds9253_driver_api);

DT_INST_FOREACH_STATUS_OKAY(APDS9253_INIT)
