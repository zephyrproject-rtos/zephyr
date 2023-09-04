/*
 * Copyright (c) 2023 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ams_tmd2635

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

#include "tmd2635.h"

LOG_MODULE_REGISTER(TMD2635, CONFIG_SENSOR_LOG_LEVEL);

static void tmd2635_gpio_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	LOG_DBG("Interrupt Callback was called");

	struct tmd2635_data *data = CONTAINER_OF(cb, struct tmd2635_data, gpio_cb);

	tmd2635_setup_int(data->dev->config, false);

#ifdef CONFIG_TMD2635_TRIGGER
	k_work_submit(&data->work);
#else
	k_sem_give(&data->data_sem);
#endif
}

static int tmd2635_configure_interrupt(const struct device *dev)
{
	struct tmd2635_data *data = dev->data;
	const struct tmd2635_config *config = dev->config;
	int ret;

	LOG_INF("Configuring Interrupt.");

	if (!gpio_is_ready_dt(&config->int_gpio)) {
		LOG_ERR("Interrupt GPIO device not ready");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&config->int_gpio, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Failed to configure interrupt pin");
		return ret;
	}

	gpio_init_callback(&data->gpio_cb, tmd2635_gpio_callback, BIT(config->int_gpio.pin));

	ret = gpio_add_callback(config->int_gpio.port, &data->gpio_cb);
	if (ret < 0) {
		LOG_ERR("Failed to set GPIO callback");
		return ret;
	}

	data->dev = dev;

#ifdef CONFIG_TMD2635_TRIGGER
	data->work.handler = tmd2635_work_cb;
#else
	k_sem_init(&data->data_sem, 0, K_SEM_MAX_LIMIT);
#endif
	return 0;
}

static int tmd2635_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	LOG_INF("Fetching Sample...");
	struct tmd2635_data *data = dev->data;
	const struct tmd2635_config *config = dev->config;
	uint8_t tmp;
	int ret;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_PROX) {
		LOG_ERR("Unsupported sensor channel");
		return -ENOTSUP;
	}

#ifndef CONFIG_TMD2635_TRIGGER
	LOG_INF("not define CONFIG_TMD2635_TRIGGER...");
	/* enabling interrupt */
	ret = i2c_reg_update_byte_dt(&config->i2c, TMD2635_INTENAB_REG, TMD2635_INTENAB_PIEN,
				     TMD2635_INTENAB_PIEN);
	if (ret < 0) {
		LOG_ERR("Failed enabling interrupt.");
		return ret;
	}

	tmd2635_setup_int(config, true);

	/* Enabling proximity and powering up device */
	tmp = TMD2635_ENABLE_PEN | TMD2635_ENABLE_PON;

	ret = i2c_reg_update_byte_dt(&config->i2c, TMD2635_ENABLE_REG, tmp, tmp);
	if (ret < 0) {
		LOG_ERR("Failed enabling device.");
		return ret;
	}

	LOG_INF("waiting for semaphore..");

	k_sem_take(&data->data_sem, K_FOREVER);
#endif

	ret = i2c_reg_read_byte_dt(&config->i2c, TMD2635_STATUS_REG, &tmp);
	if (ret < 0) {
		LOG_ERR("Failed reading status register.");
		return ret;
	}

	LOG_INF("Status register: 0x%x", tmp);
	if (tmp & TMD2635_STATUS_PINT) {
		ret = i2c_reg_read_byte_dt(&config->i2c, TMD2635_PDATAL_REG, &data->pdata_low);
		if (ret < 0) {
			LOG_ERR("Failed reading proximity data.");
			return ret;
		}
		ret = i2c_reg_read_byte_dt(&config->i2c, TMD2635_PDATAH_REG, &data->pdata_high);
		if (ret < 0) {
			LOG_ERR("Failed reading proximity data.");
			return ret;
		}
	}

#ifndef CONFIG_TMD2635_TRIGGER
	tmp = TMD2635_ENABLE_PEN | TMD2635_ENABLE_PON;

	/* Disabling proximity and powering down device */
	ret = i2c_reg_update_byte_dt(&config->i2c, TMD2635_ENABLE_REG, tmp, 0);
	if (ret < 0) {
		LOG_ERR("Failed powering down device.");
		return ret;
	}
#endif
	/* clearing interrupt flag */
	ret = i2c_reg_update_byte_dt(&config->i2c, TMD2635_STATUS_REG, TMD2635_STATUS_PINT,
				     TMD2635_STATUS_PINT);
	if (ret < 0) {
		LOG_ERR("Failed clearing interrupt flag.");
		return ret;
	}

	return 0;
}

static int tmd2635_channel_get(const struct device *dev, enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct tmd2635_data *data = dev->data;

	if (chan == SENSOR_CHAN_PROX) {
		/* inverting sensor data to fit Zephyr */
		val->val1 = (data->pdata_high);
		val->val2 = data->pdata_low;

		return 0;
	}

	return -ENOTSUP;
}

static int tmd2635_sensor_calibration(const struct device *dev, uint8_t calib_value,
				      uint8_t calibcfg_value)
{
	const struct tmd2635_config *config = dev->config;
	uint8_t ret;
	uint8_t calibra_status;
	uint8_t status;
	ret = i2c_reg_write_byte_dt(&config->i2c, TMD2635_CALIB_REG, calib_value);
	if (ret < 0) {
		LOG_ERR("Failed configure CALIB register");
		return ret;
	}

	ret = i2c_reg_write_byte_dt(&config->i2c, TMD2635_CALIBCFG_REG, calibcfg_value);
	if (ret < 0) {
		LOG_ERR("Failed configure CALIBCFG register");
		return ret;
	}

	ret = i2c_reg_read_byte_dt(&config->i2c, TMD2635_CALIBSTAT_REG, &calibra_status);
	if (ret < 0) {
		LOG_ERR("Failed reading calibra status");
		return ret;
	}

	return calibra_status;
}

static int tmd2635_sensor_softrest(const struct device *dev)
{
	const struct tmd2635_config *config = dev->config;

	uint8_t ret = i2c_reg_write_byte_dt(&config->i2c, TMD2635_SOFTREST_REG, 0X01);
	if (ret < 0) {
		LOG_ERR("SOFTREST Register was not cleared");
		return ret;
	}

	return 0;
}

static int tmd2635_sensor_setup(const struct device *dev)
{
	const struct tmd2635_config *config = dev->config;
	uint8_t chip_id;
	uint8_t tmp;
	int ret;

	/* trying to read the id twice, as the sensor does not answer the first request */
	/* because of this no return code is checked in this line */
	i2c_reg_read_byte_dt(&config->i2c, TMD2635_ID_REG, &chip_id);

	ret = i2c_reg_read_byte_dt(&config->i2c, TMD2635_ID_REG, &chip_id);
	if (ret < 0) {
		LOG_ERR("Failed reading chip id");
		return ret;
	}

	if (chip_id != TMD2635_CHIP_ID) {
		LOG_ERR("Chip id is invalid! Device @%02x is no TMD2635!", config->i2c.addr);
		return -EIO;
	}

	tmp = TMD2635_ENABLE_PEN | TMD2635_ENABLE_PON;
	ret = i2c_reg_write_byte_dt(&config->i2c, TMD2635_ENABLE_REG, tmp);
	if (ret < 0) {
		LOG_ERR("ENABLE Register was not cleared");
		return ret;
	}

	tmp = config->wait_time_factor;
	ret = i2c_reg_write_byte_dt(&config->i2c, TMD2635_PWTIME_REG, tmp);
	if (ret < 0) {
		LOG_ERR("Failed setting wait time");
		return ret;
	}

	uint16_t pl_tmp = config->proximity_low_threshold;
	ret = i2c_reg_write_byte_dt(&config->i2c, TMD2635_PILTL_REG, (pl_tmp & 0xff));
	if (ret < 0) {
		LOG_ERR("Failed setting PILT");
		return ret;
	}

	ret = i2c_reg_write_byte_dt(&config->i2c, TMD2635_PILTH_REG, ((pl_tmp >> 8) & 0xff));
	if (ret < 0) {
		LOG_ERR("Failed setting PILT");
		return ret;
	}

	uint16_t t_tmp = config->proximity_high_threshold;
	LOG_INF("[tmd2635]:tmp: %d+%d>%d ...", t_tmp, (t_tmp & 0xff), (t_tmp >> 8) & 0xff);
	ret = i2c_reg_write_byte_dt(&config->i2c, TMD2635_PIHTL_REG, (t_tmp & 0xff));
	if (ret < 0) {
		LOG_ERR("Failed setting PIHTL");
		return ret;
	}

	ret = i2c_reg_write_byte_dt(&config->i2c, TMD2635_PIHTH_REG, ((t_tmp >> 8) & 0xff));
	if (ret < 0) {
		LOG_ERR("Failed setting PIHTH");
		return ret;
	}

#ifdef CONFIG_TMD2635_TRIGGER
	tmp = (config->proximity_interrupt_filter << 3);
	ret = i2c_reg_write_byte_dt(&config->i2c, TMD2635_PERS_REG, 0X0F);
	if (ret < 0) {
		LOG_ERR("Failed setting PERS");
		return ret;
	}
#endif

	if (config->wait_long) {
		tmp = TMD2635_CFG0_PWLONG;
	} else {
		tmp = 0;
	}

	ret = i2c_reg_write_byte_dt(&config->i2c, TMD2635_CFG0_REG, tmp);
	if (ret < 0) {
		LOG_ERR("Failed setting CFG0");
		return ret;
	}

	switch (config->proximity_gain) {
	case 1:
		tmp = TMD2635_PCFG0_PGAIN_X1;
		break;
	case 2:
		tmp = TMD2635_PCFG0_PGAIN_X2;
		break;
	case 4:
		tmp = TMD2635_PCFG0_PGAIN_X4;
		break;
	case 8:
		tmp = TMD2635_PCFG0_PGAIN_X8;
		break;
	default:
		LOG_ERR("Invalid proximity gain");
		return -EINVAL;
	}

	tmp |= config->proximity_pulse_count;
	ret = i2c_reg_write_byte_dt(&config->i2c, TMD2635_PCFG0_REG, tmp);
	if (ret < 0) {
		LOG_ERR("Failed setting PPULSE");
		return ret;
	}

	switch (config->proximity_pulse_length) {
	case 1:
		tmp = TMD2635_PCFG1_PPULSE_LEN_1US;
		break;
	case 2:
		tmp = TMD2635_PCFG1_PPULSE_LEN_2US;
		break;
	case 4:
		tmp = TMD2635_PCFG1_PPULSE_LEN_4US;
		break;
	case 8:
		tmp = TMD2635_PCFG1_PPULSE_LEN_8US;
		break;
	case 16:
		tmp = TMD2635_PCFG1_PPULSE_LEN_12US;
		break;
	case 24:
		tmp = TMD2635_PCFG1_PPULSE_LEN_24US;
		break;
		break;
	case 32:
		tmp = TMD2635_PCFG1_PPULSE_LEN_32US;
		break;
	default:
		LOG_ERR("Invalid proximity pulse length");
		return -EINVAL;
	}

	tmp |= config->proximity_led_drive_strength;
	ret = i2c_reg_write_byte_dt(&config->i2c, TMD2635_PCFG1_REG, tmp);
	if (ret < 0) {
		LOG_ERR("Failed setting PCGF1");
		return ret;
	}

	tmp = TMD2635_CFG3_INT_READ_CLEAR;
	ret = i2c_reg_write_byte_dt(&config->i2c, TMD2635_CFG3_REG, tmp);
	if (ret < 0) {
		LOG_ERR("Failed setting CFG3");
		return ret;
	}

	tmp = config->proximity_sample_duration;
	ret = i2c_reg_write_byte_dt(&config->i2c, TMD2635_PRATE_REG, tmp);
	if (ret < 0) {
		LOG_ERR("Failed setting PRATE");
		return ret;
	}

	tmp = TMD2635_INTENAB_PIEN |
	      TMD2635_INTENAB_CIEN; /* enable interrupt & Calibration Interrupt Enable &  */
	ret = i2c_reg_write_byte_dt(&config->i2c, TMD2635_INTENAB_REG, tmp);
	if (ret < 0) {
		LOG_ERR("Failed setting INTENAB");
		return ret;
	}

	tmp = TMD2635_CFG8_PDSELECT_BOTH;  /* select near and far photodiode */
	ret = i2c_reg_write_byte_dt(&config->i2c, TMD2635_CFG8_REG, tmp);
	if (ret < 0) {
		LOG_ERR("Failed setting select photodiode");
		return ret;
	}

	if (config->enable_wait_mode) {
		ret = i2c_reg_update_byte_dt(&config->i2c, TMD2635_ENABLE_REG, TMD2635_ENABLE_PWEN,
					     TMD2635_ENABLE_PWEN);
		if (ret < 0) {
			LOG_ERR("Failed enabling wait mode");
			return ret;
		}
	}

	return 0;
}

static int tmd2635_init(const struct device *dev)
{
	const struct tmd2635_config *config = dev->config;
	struct tmd2635_data *data = dev->data;
	int ret;
#ifdef CONFIG_TMD2635_TRIGGER
	uint8_t tmp;
#endif

	if (!i2c_is_ready_dt(&config->i2c)) {
		LOG_ERR("I2C bus not ready!");
		return -ENODEV;
	}

	data->pdata_high = 0U;
	data->pdata_low = 0u;

	ret = tmd2635_sensor_setup(dev);
	if (ret < 0) {
		LOG_ERR("Failed to configure device");
		return ret;
	}

	LOG_INF("Device setup complete");

	ret = tmd2635_configure_interrupt(dev);
	if (ret < 0) {
		LOG_ERR("Failed configuring interrupt!");
		return ret;
	}

#ifdef CONFIG_TMD2635_TRIGGER
	tmp = TMD2635_ENABLE_PEN | TMD2635_ENABLE_PON;
	ret = i2c_reg_update_byte_dt(&config->i2c, TMD2635_ENABLE_REG, tmp, tmp);
	if (ret < 0) {
		LOG_ERR("Failed enabling device.");
		return ret;
	}
#endif
	LOG_INF("Driver init complete.");

	return 0;
}

#ifdef CONFIG_PM_DEVICE
static int tmd2635_pm_action(const struct device *dev, enum pm_device_action action)
{
	const struct tmd2635_config *config = dev->config;
	int ret;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		ret = i2c_reg_update_byte_dt(&config->i2c, TMD2635_ENABLE_REG, TMD2635_ENABLE_PON,
					     TMD2635_ENABLE_PON);
		if (ret < 0) {
			LOG_ERR("Failed enabling sensor.");
			return ret;
		}
		break;

	case PM_DEVICE_ACTION_SUSPEND:
		ret = i2c_reg_update_byte_dt(&config->i2c, TMD2635_ENABLE_REG, TMD2635_ENABLE_PON,
					     0);
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

static const struct sensor_driver_api tmd2635_driver_api = {
	.sample_fetch = tmd2635_sample_fetch,
	.channel_get = tmd2635_channel_get,
#ifdef CONFIG_TMD2635_TRIGGER
	.attr_set = tmd2635_attr_set,
	.trigger_set = tmd2635_trigger_set,
#endif
};

#define TMD2635_INIT_INST(n)                                                                       \
	struct tmd2635_data tmd2635_data_##n;                                                      \
	static const struct tmd2635_config tmd2635_config_##n = {                                  \
		.i2c = I2C_DT_SPEC_INST_GET(n),                                                    \
		.int_gpio = GPIO_DT_SPEC_INST_GET(n, int_gpios),                                   \
		.proximity_gain = DT_INST_PROP(n, proximity_gain),                                 \
		.proximity_pulse_length = DT_INST_PROP(n, proximity_pulse_length),                 \
		.proximity_pulse_count = DT_INST_PROP(n, proximity_pulse_count),                   \
		.proximity_high_threshold = DT_INST_PROP(n, proximity_high_threshold),             \
		.proximity_low_threshold = DT_INST_PROP(n, proximity_low_threshold),               \
		.proximity_led_drive_strength = DT_INST_PROP(n, proximity_led_drive_strength),     \
		.proximity_interrupt_filter = DT_INST_PROP(n, proximity_interrupt_filter),         \
		.proximity_sample_duration = DT_INST_PROP(n, proximity_sample_duration),           \
		.enable_wait_mode = DT_INST_PROP(n, enable_wait_mode),                             \
		.wait_time_factor = DT_INST_PROP(n, wait_time_factor),                             \
		.wait_long = DT_INST_PROP(n, wait_long),                                           \
	};                                                                                         \
                                                                                                   \
	PM_DEVICE_DT_INST_DEFINE(n, tmd2635_pm_action);                                            \
	SENSOR_DEVICE_DT_INST_DEFINE(n, tmd2635_init, PM_DEVICE_DT_INST_GET(n), &tmd2635_data_##n, \
				     &tmd2635_config_##n, POST_KERNEL,                             \
				     CONFIG_SENSOR_INIT_PRIORITY, &tmd2635_driver_api);

DT_INST_FOREACH_STATUS_OKAY(TMD2635_INIT_INST);
