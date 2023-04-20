/*
 * Copyright (c) 2023 PHYTEC Messtechnik GmbH
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

static void tmd2620_gpio_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	LOG_DBG("Interrupt Callback was called");

	struct tmd2620_data *data = CONTAINER_OF(cb, struct tmd2620_data, gpio_cb);

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
	int ret;

	LOG_DBG("Configuring Interrupt.");

	if (!gpio_is_ready_dt(&config->int_gpio)) {
		LOG_ERR("Interrupt GPIO device not ready");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&config->int_gpio, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Failed to configure interrupt pin");
		return ret;
	}

	gpio_init_callback(&data->gpio_cb, tmd2620_gpio_callback, BIT(config->int_gpio.pin));

	ret = gpio_add_callback(config->int_gpio.port, &data->gpio_cb);
	if (ret < 0) {
		LOG_ERR("Failed to set GPIO callback");
		return ret;
	}

	data->dev = dev;

#ifdef CONFIG_TMD2620_TRIGGER
	data->work.handler = tmd2620_work_cb;
#else
	k_sem_init(&data->data_sem, 0, K_SEM_MAX_LIMIT);
#endif
	return 0;
}

static int tmd2620_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	LOG_DBG("Fetching Sample...");
	struct tmd2620_data *data = dev->data;
	const struct tmd2620_config *config = dev->config;
	uint8_t tmp;
	int ret;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_PROX) {
		LOG_ERR("Unsupported sensor channel");
		return -ENOTSUP;
	}

#ifndef CONFIG_TMD2620_TRIGGER
	/* enabling interrupt */
	ret = i2c_reg_update_byte_dt(&config->i2c, TMD2620_INTENAB_REG, TMD2620_INTENAB_PIEN,
					TMD2620_INTENAB_PIEN);
	if (ret < 0) {
		LOG_ERR("Failed enabling interrupt.");
		return ret;
	}

	tmd2620_setup_int(config, true);

	/* Enabling proximity and powering up device */
	tmp = TMD2620_ENABLE_PEN | TMD2620_ENABLE_PON;

	ret = i2c_reg_update_byte_dt(&config->i2c, TMD2620_ENABLE_REG, tmp, tmp);
	if (ret < 0) {
		LOG_ERR("Failed enabling device.");
		return ret;
	}

	LOG_DBG("waiting for semaphore..");

	k_sem_take(&data->data_sem, K_FOREVER);
#endif
	ret = i2c_reg_read_byte_dt(&config->i2c, TMD2620_STATUS_REG, &tmp);
	if (ret < 0) {
		LOG_ERR("Failed reading status register.");
		return ret;
	}

	LOG_DBG("Status register: 0x%x", tmp);
	if (tmp & TMD2620_STATUS_PINT) {
		LOG_DBG("Proximity interrupt detected.");

		ret = i2c_reg_read_byte_dt(&config->i2c, TMD2620_PDATA_REG, &data->pdata);
		if (ret < 0) {
			LOG_ERR("Failed reading proximity data.");
			return ret;
		}
	}

#ifndef CONFIG_TMD2620_TRIGGER
	tmp = TMD2620_ENABLE_PEN | TMD2620_ENABLE_PON;

	/* Disabling proximity and powering down device */
	ret = i2c_reg_update_byte_dt(&config->i2c, TMD2620_ENABLE_REG, tmp, 0);
	if (ret < 0) {
		LOG_ERR("Failed powering down device.");
		return ret;
	}
#endif
	/* clearing interrupt flag */
	ret = i2c_reg_update_byte_dt(&config->i2c, TMD2620_STATUS_REG, TMD2620_STATUS_PINT,
				     TMD2620_STATUS_PINT);
	if (ret < 0) {
		LOG_ERR("Failed clearing interrupt flag.");
		return ret;
	}

	return 0;
}

static int tmd2620_channel_get(const struct device *dev, enum sensor_channel chan,
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
	int ret;

	/* trying to read the id twice, as the sensor does not answer the first request */
	/* because of this no return code is checked in this line */
	i2c_reg_read_byte_dt(&config->i2c, TMD2620_ID_REG, &chip_id);

	ret = i2c_reg_read_byte_dt(&config->i2c, TMD2620_ID_REG, &chip_id);
	if (ret < 0) {
		LOG_ERR("Failed reading chip id");
		return ret;
	}

	if (chip_id != TMD2620_CHIP_ID) {
		LOG_ERR("Chip id is invalid! Device @%02x is no TMD2620!", config->i2c.addr);
		return -EIO;
	}

	ret = i2c_reg_write_byte_dt(&config->i2c, TMD2620_ENABLE_REG, 0);
	if (ret < 0) {
		LOG_ERR("ENABLE Register was not cleared");
		return ret;
	}

	tmp = config->wait_time_factor;
	ret = i2c_reg_write_byte_dt(&config->i2c, TMD2620_WTIME_REG, tmp);
	if (ret < 0) {
		LOG_ERR("Failed setting wait time");
		return ret;
	}

	tmp = config->proximity_low_threshold;
	ret = i2c_reg_write_byte_dt(&config->i2c, TMD2620_PILT_REG, tmp);
	if (ret < 0) {
		LOG_ERR("Failed setting PILT");
		return ret;
	}

	tmp = config->proximity_high_threshold;
	ret = i2c_reg_write_byte_dt(&config->i2c, TMD2620_PIHT_REG, (255 - tmp));
	if (ret < 0) {
		LOG_ERR("Failed setting PIHT");
		return ret;
	}

#ifdef CONFIG_TMD2620_TRIGGER
	tmp = (config->proximity_interrupt_filter << 3);
	ret = i2c_reg_write_byte_dt(&config->i2c, TMD2620_PERS_REG, tmp);
	if (ret < 0) {
		LOG_ERR("Failed setting PERS");
		return ret;
	}
#endif

	if (config->wait_long) {
		tmp = TMD2620_CFG0_WLONG;
	} else {
		tmp = 0;
	}

	ret = i2c_reg_write_byte_dt(&config->i2c, TMD2620_CFG0_REG, tmp);
	if (ret < 0) {
		LOG_ERR("Failed setting CFG0");
		return ret;
	}

	switch (config->proximity_pulse_length) {
	case 4:
		tmp = TMD2620_PCFG0_PPULSE_LEN_4US;
		break;
	case 8:
		tmp = TMD2620_PCFG0_PPULSE_LEN_8US;
		break;
	case 16:
		tmp = TMD2620_PCFG0_PPULSE_LEN_16US;
		break;
	case 32:
		tmp = TMD2620_PCFG0_PPULSE_LEN_32US;
		break;
	default:
		LOG_ERR("Invalid proximity pulse length");
		return -EINVAL;
	}

	tmp |= config->proximity_pulse_count;
	ret = i2c_reg_write_byte_dt(&config->i2c, TMD2620_PCFG0_REG, tmp);
	if (ret < 0) {
		LOG_ERR("Failed setting PPULSE");
		return ret;
	}

	switch (config->proximity_gain) {
	case 1:
		tmp = TMD2620_PCFG1_PGAIN_X1;
		break;
	case 2:
		tmp = TMD2620_PCFG1_PGAIN_X2;
		break;
	case 4:
		tmp = TMD2620_PCFG1_PGAIN_X4;
		break;
	case 8:
		tmp = TMD2620_PCFG1_PGAIN_X8;
		break;
	default:
		LOG_ERR("Invalid proximity gain");
		return -EINVAL;
	}

	tmp |= config->proximity_led_drive_strength;
	ret = i2c_reg_write_byte_dt(&config->i2c, TMD2620_PCFG1_REG, tmp);
	if (ret < 0) {
		LOG_ERR("Failed setting PCGF1");
		return ret;
	}

	tmp = TMD2620_CFG3_INT_READ_CLEAR;
	ret = i2c_reg_write_byte_dt(&config->i2c, TMD2620_CFG3_REG, tmp);
	if (ret < 0) {
		LOG_ERR("Failed setting CFG3");
		return ret;
	}

	tmp = 1; /* enable interrupt */
	ret = i2c_reg_write_byte_dt(&config->i2c, TMD2620_INTENAB_REG, tmp);
	if (ret < 0) {
		LOG_ERR("Failed setting INTENAB");
		return ret;
	}

	if (config->enable_wait_mode) {
		ret = i2c_reg_update_byte_dt(&config->i2c, TMD2620_ENABLE_REG, TMD2620_ENABLE_WEN,
					     TMD2620_ENABLE_WEN);
		if (ret < 0) {
			LOG_ERR("Failed enabling wait mode");
			return ret;
		}
	}

	return 0;
}

static int tmd2620_init(const struct device *dev)
{
	const struct tmd2620_config *config = dev->config;
	struct tmd2620_data *data = dev->data;
	int ret;
#ifdef CONFIG_TMD2620_TRIGGER
	uint8_t tmp;
#endif

	if (!i2c_is_ready_dt(&config->i2c)) {
		LOG_ERR("I2C bus not ready!");
		return -ENODEV;
	}

	data->pdata = 0U;

	ret = tmd2620_sensor_setup(dev);
	if (ret < 0) {
		LOG_ERR("Failed to configure device");
		return ret;
	}

	LOG_DBG("Device setup complete");

	ret = tmd2620_configure_interrupt(dev);
	if (ret < 0) {
		LOG_ERR("Failed configuring interrupt!");
		return ret;
	}

#ifdef CONFIG_TMD2620_TRIGGER
	tmp = TMD2620_ENABLE_PEN | TMD2620_ENABLE_PON;
	ret = i2c_reg_update_byte_dt(&config->i2c, TMD2620_ENABLE_REG, tmp, tmp);
	if (ret < 0) {
		LOG_ERR("Failed enabling device.");
		return ret;
	}
#endif
	LOG_DBG("Driver init complete.");

	return 0;
}

#ifdef CONFIG_PM_DEVICE
static int tmd2620_pm_action(const struct device *dev, enum pm_device_action action)
{
	const struct tmd2620_config *config = dev->config;
	int ret;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		ret = i2c_reg_update_byte_dt(&config->i2c, TMD2620_ENABLE_REG,
				TMD2620_ENABLE_PON, TMD2620_ENABLE_PON);
		if (ret < 0) {
			LOG_ERR("Failed enabling sensor.");
			return ret;
		}
		break;

	case PM_DEVICE_ACTION_SUSPEND:
		ret = i2c_reg_update_byte_dt(&config->i2c, TMD2620_ENABLE_REG,
				TMD2620_ENABLE_PON, 0);
		if (ret < 0) {
			LOG_ERR("Failed suspending sensor.");
			return ret;
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

#define TMD2620_INIT_INST(n)                                                                       \
	struct tmd2620_data tmd2620_data_##n;                                                      \
	static const struct tmd2620_config tmd2620_config_##n = {                                  \
		.i2c = I2C_DT_SPEC_INST_GET(n),                                                    \
		.int_gpio = GPIO_DT_SPEC_INST_GET(n, int_gpios),                                   \
		.proximity_gain = DT_INST_PROP(n, proximity_gain),                                 \
		.proximity_pulse_length = DT_INST_PROP(n, proximity_pulse_length),                 \
		.proximity_pulse_count = DT_INST_PROP(n, proximity_pulse_count),                   \
		.proximity_high_threshold = DT_INST_PROP(n, proximity_high_threshold),             \
		.proximity_low_threshold = DT_INST_PROP(n, proximity_low_threshold),               \
		.proximity_led_drive_strength = DT_INST_PROP(n, proximity_led_drive_strength),     \
		.proximity_interrupt_filter = DT_INST_PROP(n, proximity_interrupt_filter),         \
		.enable_wait_mode = DT_INST_PROP(n, enable_wait_mode),                             \
		.wait_time_factor = DT_INST_PROP(n, wait_time_factor),                             \
		.wait_long = DT_INST_PROP(n, wait_long),                                           \
	};                                                                                         \
                                                                                                   \
	PM_DEVICE_DT_INST_DEFINE(n, tmd2620_pm_action);                                            \
	SENSOR_DEVICE_DT_INST_DEFINE(n, tmd2620_init, PM_DEVICE_DT_INST_GET(n), &tmd2620_data_##n, \
				     &tmd2620_config_##n, POST_KERNEL,                             \
				     CONFIG_SENSOR_INIT_PRIORITY, &tmd2620_driver_api);

DT_INST_FOREACH_STATUS_OKAY(TMD2620_INIT_INST);
