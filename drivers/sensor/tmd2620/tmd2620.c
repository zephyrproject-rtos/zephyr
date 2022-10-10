/*
 * Copyright (c) 2022 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ams_tmd2620

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <string.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>
#include <stdio.h>

#include "tmd2620.h"

LOG_MODULE_REGISTER(TMD2620, CONFIG_SENSOR_LOG_LEVEL);

static void tmd2620_gpio_callback(const struct device *dev,
					struct gpio_callback *cb,
					uint32_t pins)
{

	LOG_DBG("Interrupt Callback was called");

	struct tmd2620_data *data =
		CONTAINER_OF(cb, struct tmd2620_data, gpio_cb);

	tmd2620_setup_int(data->dev->config, false);

#ifdef CONFIG_TMD2620_TRIGGER
	k_work_submit(&data->work);
#else
	k_sem_give(&data->data_sem);
#endif
}

static int tmd2620_configure_interrupt(const struct device *dev)
{
	struct tmd2620_data *data = dev->data;
	const struct tmd2620_config *config = dev->config;

	LOG_DBG("Configuring Interrupt.");

	if (!device_is_ready(config->int_gpio.port)) {
		LOG_ERR("Interrupt GPIO device not ready");
		return -EIO;
	}

	gpio_pin_configure_dt(&config->int_gpio,
				GPIO_INPUT | config->int_gpio.dt_flags);

	gpio_init_callback(&data->gpio_cb,
				tmd2620_gpio_callback,
				BIT(config->int_gpio.pin));

	if (gpio_add_callback(config->int_gpio.port, &data->gpio_cb) < 0) {
		LOG_ERR("Failed to set GPIO callback");
		return -EIO;
	}

	data->dev = dev;

#ifdef CONFIG_TMD2620_TRIGGER
	data->work.handler = tmd2620_work_cb;

#else
	k_sem_init(&data->data_sem, 0, K_SEM_MAX_LIMIT);
#endif

	return 0;
}

static int tmd2620_sample_fetch(const struct device *dev,
				enum sensor_channel chan)
{

	LOG_DBG("Fetching Sample...");
	struct tmd2620_data *data = dev->data;
	const struct tmd2620_config *config = dev->config;
	uint8_t tmp;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_PROX) {
		LOG_ERR("Unsupported sensor channel");
		return -ENOTSUP;
	}

#ifndef CONFIG_TMD2620_TRIGGER

	/* enabling interrupt */
	if (i2c_reg_update_byte_dt(&config->i2c,
					TMD2620_INTENAB_REG,
					TMD2620_INTENAB_PIEN,
					TMD2620_INTENAB_PIEN)) {

		LOG_ERR("Failed enabling interrupt.");
		return -EIO;
	}

	tmd2620_setup_int(config, true);

	/* Enabling proximity and powering up device */
	tmp = TMD2620_ENABLE_PEN | TMD2620_ENABLE_PON;

	if (i2c_reg_update_byte_dt(&config->i2c,
					TMD2620_ENABLE_REG,
					tmp,
					tmp)) {

		LOG_ERR("Failed enabling device.");
		return -EIO;
	}

	LOG_DBG("waiting for semaphore..");

	k_sem_take(&data->data_sem, K_FOREVER);

#endif

	if (i2c_reg_read_byte_dt(&config->i2c,
					TMD2620_STATUS_REG, &tmp)) {

		LOG_ERR("Failed reading status register.");
		return -EIO;
	}

	LOG_DBG("Status register: 0x%x", tmp);
	if (tmp & TMD2620_STATUS_PINT) {
		LOG_DBG("Proximity interrupt detected.");

		if (i2c_reg_read_byte_dt(&config->i2c,
					TMD2620_PDATA_REG, &data->pdata)) {

			LOG_ERR("Failed reading proximity data.");
			return -EIO;
		}
	}

#ifndef CONFIG_TMD2620_TRIGGER

	tmp = TMD2620_ENABLE_PEN | TMD2620_ENABLE_PON;

	/* Disabling proximity and powering down device */
	if (i2c_reg_update_byte_dt(&config->i2c,
					TMD2620_ENABLE_REG,
					tmp,
					0)) {

		LOG_ERR("Failed powering down device.");
		return -EIO;
	}

#endif

	/* clearing interrupt flag */
	if (i2c_reg_update_byte_dt(&config->i2c,
					TMD2620_STATUS_REG,
					TMD2620_STATUS_PINT,
					TMD2620_STATUS_PINT)) {

		LOG_ERR("Failed clearing interrupt flag.");
		return -EIO;
	}

	return 0;
}

static int tmd2620_channel_get(const struct device *dev,
				enum sensor_channel chan,
				struct sensor_value *val)
{

	struct tmd2620_data *data = dev->data;

	if (chan == SENSOR_CHAN_PROX) {

		/* inverting sensor data to fit Zephyr */
		val->val1 = (256 - data->pdata);
		val->val2 = 0;

		return 0;
	}

	return -ENOTSUP;
}

static int tmd2620_sensor_setup(const struct device *dev)
{
	const struct tmd2620_config *config = dev->config;
	uint8_t chip_id;
	uint8_t tmp;

	i2c_reg_read_byte_dt(&config->i2c, TMD2620_ID_REG, &chip_id);

	if (i2c_reg_read_byte_dt(&config->i2c, TMD2620_ID_REG, &chip_id)) {
		LOG_ERR("Failed reading chip id");
		return -EIO;
	}

	if (chip_id != TMD2620_CHIP_ID) {
		LOG_ERR("Chip id is invalid! Device @%02x is no TMD2620!",
			config->i2c.addr);
		return -EIO;
	}

	if (i2c_reg_write_byte_dt(&config->i2c, TMD2620_ENABLE_REG, 0)) {
		LOG_ERR("ENABLE Register was not cleared");
		return -EIO;
	}

	tmp = DT_INST_PROP(0, wait_time);
	if (i2c_reg_write_byte_dt(&config->i2c, TMD2620_WTIME_REG, tmp)) {
		LOG_ERR("Failed setting wait time");
		return -EIO;
	}

	tmp = DT_INST_PROP(0, proximity_low_threshold);
	if (i2c_reg_write_byte_dt(&config->i2c, TMD2620_PILT_REG, tmp)) {
		LOG_ERR("Failed setting PILT");
		return -EIO;
	}

	tmp = DT_INST_PROP(0, proximity_high_threshold);
	if (i2c_reg_write_byte_dt(&config->i2c,
				TMD2620_PIHT_REG,
				(255 - tmp))) {
		LOG_ERR("Failed setting PIHT");
		return -EIO;
	}

#ifdef CONFIG_TMD2620_TRIGGER
	tmp = (DT_INST_PROP(0, proximity_interrupt_filter) << 3);

	if (i2c_reg_write_byte_dt(&config->i2c, TMD2620_PERS_REG, tmp)) {
		LOG_ERR("Failed setting PERS");
		return -EIO;
	}
#endif

#if DT_INST_PROP(0, wait_long)
	tmp = TMD2620_CONFIG_WLONG;
#else
	tmp = 0;
#endif

	if (i2c_reg_write_byte_dt(&config->i2c, TMD2620_CFG0_REG, tmp)) {
		LOG_ERR("Failed setting CFG0");
		return -EIO;
	}

#if DT_INST_PROP(0, proximity_pulse_length) == 4
	tmp = TMD2620_PCFG0_PPULSE_LEN_4US;
#elif DT_INST_PROP(0, proximity_pulse_length) == 8
	tmp = TMD2620_PCFG0_PPULSE_LEN_8US;
#elif DT_INST_PROP(0, proximity_pulse_length) == 16
	tmp = TMD2620_PCFG0_PPULSE_LEN_16US;
#elif DT_INST_PROP(0, proximity_pulse_length) == 32
	tmp = TMD2620_PCFG0_PPULSE_LEN_32US;
#endif

	tmp = tmp | DT_INST_PROP(0, proximity_pulse_count);

	if (i2c_reg_write_byte_dt(&config->i2c,
					TMD2620_PCFG0_REG,
					tmp)) {
		LOG_ERR("Failed setting PPULSE");
		return -EIO;
	}

#if DT_INST_PROP(0, proximity_gain) == 1
	tmp = TMD2620_PCFG1_PGAIN_X1;
#elif DT_INST_PROP(0, proximity_gain) == 2
	tmp = TMD2620_PCFG1_PGAIN_X2;
#elif DT_INST_PROP(0, proximity_gain) == 4
	tmp = TMD2620_PCFG1_PGAIN_X4;
#elif DT_INST_PROP(0, proximity_gain) == 8
	tmp = TMD2620_PCFG1_PGAIN_X8;
#endif

	tmp = tmp | DT_INST_PROP(0, proximity_led_drive_strength);
	if (i2c_reg_write_byte_dt(&config->i2c, TMD2620_PCFG1_REG, tmp)) {
		LOG_ERR("Failed setting PCGF1");
		return -EIO;
	}

	tmp = TMD2620_CFG3_INT_READ_CLEAR;
	if (i2c_reg_write_byte_dt(&config->i2c, TMD2620_CFG3_REG, tmp)) {
		LOG_ERR("Failed setting CFG3");
		return -EIO;
	}

	tmp = 1; /* enable interrupt */
	if (i2c_reg_write_byte_dt(&config->i2c, TMD2620_INTENAB_REG, tmp)) {
		LOG_ERR("Failed setting INTENAB");
		return -EIO;
	}


#if DT_INST_PROP(0, enable_wait_mode)
	if (i2c_reg_update_byte_dt(&config->i2c,
			TMD2620_ENABLE_REG,
			TMD2620_ENABLE_WEN,
			TMD2620_ENABLE_WEN)) {
		LOG_ERR("Failed enabling wait mode");
	}
#endif

	return 0;
}

static int tmd2620_init(const struct device *dev)
{
	const struct tmd2620_config *config = dev->config;
	struct tmd2620_data *data = dev->data;
#ifdef CONFIG_TMD2620_TRIGGER
	uint8_t tmp;
#endif

	if (!device_is_ready(config->i2c.bus)) {
		LOG_ERR("I2C bus not ready!");
		return -EIO;
	}

	data->pdata = 0U;

	if (tmd2620_sensor_setup(dev)) {
		LOG_ERR("Failed to configure device");
		return -EIO;
	}

	LOG_DBG("Device setup complete");

	if (tmd2620_configure_interrupt(dev) < 0) {
		LOG_ERR("Failed configuring interrupt!");
		return -EIO;
	}

#ifdef CONFIG_TMD2620_TRIGGER
	tmp = TMD2620_ENABLE_PEN | TMD2620_ENABLE_PON;

	if (i2c_reg_update_byte_dt(&config->i2c,
					TMD2620_ENABLE_REG,
					tmp,
					tmp)) {

		LOG_ERR("Failed enabling device.");
		return -EIO;
	}
#endif

	LOG_DBG("Driver init complete.");

	return 0;
}

#ifdef CONFIG_PM_DEVICE
static int tmd2620_pm_action(const struct device *dev,
				enum pm_device_action action)
{

	const struct tmd2620_config *config = dev->config;
	uint8_t en_bits = TMD2620_ENABLE_PON;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		if (i2c_reg_update_byte_dt(&config->i2c,
						TMD2620_ENABLE_REG,
						en_bits,
						en_bits)) {

			LOG_ERR("Failed enabling sensor.");
			return -EIO;
		}
		break;

	case PM_DEVICE_ACTION_SUSPEND:
		if (i2c_reg_update_byte_dt(&config->i2c,
						TMD2620_ENABLE_REG,
						en_bits,
						0)) {

			LOG_ERR("Failed suspending sensor.");
			return -EIO;
		}
		break;

	default:
		return -ENOTSUP;
	}

	return 0;
}

#endif

static const struct sensor_driver_api tmd2620_driver_api = {
	.sample_fetch = tmd2620_sample_fetch,
	.channel_get = tmd2620_channel_get,
#ifdef CONFIG_TMD2620_TRIGGER
	.attr_set = tmd2620_attr_set,
	.trigger_set = tmd2620_trigger_set,
#endif
};

#define TMD2620_INIT_INST(n) \
	struct tmd2620_data tmd2620_data_##n; \
	static struct tmd2620_config tmd2620_config_##n = { \
		.i2c = I2C_DT_SPEC_INST_GET(n), \
		.int_gpio = GPIO_DT_SPEC_INST_GET(n, int_gpios), \
		.inst = n, \
	}; \
	\
	PM_DEVICE_DT_INST_DEFINE(n, tmd2620_pm_action); \
	DEVICE_DT_INST_DEFINE(n, tmd2620_init, PM_DEVICE_DT_INST_GET(n), \
			&tmd2620_data_##n, &tmd2620_config_##n, POST_KERNEL,\
			CONFIG_SENSOR_INIT_PRIORITY, &tmd2620_driver_api);\

DT_INST_FOREACH_STATUS_OKAY(TMD2620_INIT_INST);
