/*
 * Copyright (c) 2017 Intel Corporation
 * Copyright (c) 2018 Phytec Messtechnik GmbH
 *
 *SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT avago_apds9960

/* @file
 * @brief driver for APDS9960 ALS/RGB/gesture/proximity sensor
 */

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

#include "apds9960.h"

LOG_MODULE_REGISTER(APDS9960, CONFIG_SENSOR_LOG_LEVEL);

static void apds9960_handle_cb(struct apds9960_data *drv_data)
{
	apds9960_setup_int(drv_data->dev->config, false);

#ifdef CONFIG_APDS9960_TRIGGER
	k_work_submit(&drv_data->work);
#else
	k_sem_give(&drv_data->data_sem);
#endif
}

static void apds9960_gpio_callback(const struct device *dev,
				   struct gpio_callback *cb, uint32_t pins)
{
	struct apds9960_data *drv_data =
		CONTAINER_OF(cb, struct apds9960_data, gpio_cb);

	apds9960_handle_cb(drv_data);
}

static int apds9960_sample_fetch(const struct device *dev,
				 enum sensor_channel chan)
{
	const struct apds9960_config *config = dev->config;
	struct apds9960_data *data = dev->data;
	uint8_t tmp;

	if (chan != SENSOR_CHAN_ALL) {
		LOG_ERR("Unsupported sensor channel");
		return -ENOTSUP;
	}

#ifndef CONFIG_APDS9960_TRIGGER
	apds9960_setup_int(config, true);

#ifdef CONFIG_APDS9960_ENABLE_ALS
	tmp = APDS9960_ENABLE_PON | APDS9960_ENABLE_AIEN;
#else
	tmp = APDS9960_ENABLE_PON | APDS9960_ENABLE_PIEN;
#endif
	if (i2c_reg_update_byte_dt(&config->i2c,
				APDS9960_ENABLE_REG, tmp, tmp)) {
		LOG_ERR("Power on bit not set.");
		return -EIO;
	}

	k_sem_take(&data->data_sem, K_FOREVER);
#endif

	if (i2c_reg_read_byte_dt(&config->i2c,
			      APDS9960_STATUS_REG, &tmp)) {
		return -EIO;
	}

	LOG_DBG("status: 0x%x", tmp);
	if (tmp & APDS9960_STATUS_PINT) {
		if (i2c_reg_read_byte_dt(&config->i2c,
				      APDS9960_PDATA_REG, &data->pdata)) {
			return -EIO;
		}
	}

	if (tmp & APDS9960_STATUS_AINT) {
		if (i2c_burst_read_dt(&config->i2c,
				   APDS9960_CDATAL_REG,
				   (uint8_t *)&data->sample_crgb,
				   sizeof(data->sample_crgb))) {
			return -EIO;
		}

	}

#ifndef CONFIG_APDS9960_TRIGGER
	if (i2c_reg_update_byte_dt(&config->i2c,
				APDS9960_ENABLE_REG,
				APDS9960_ENABLE_PON,
				0)) {
		return -EIO;
	}
#endif

	if (i2c_reg_write_byte_dt(&config->i2c,
			       APDS9960_AICLEAR_REG, 0)) {
		return -EIO;
	}

	return 0;
}

static int apds9960_channel_get(const struct device *dev,
				enum sensor_channel chan,
				struct sensor_value *val)
{
	struct apds9960_data *data = dev->data;

	switch (chan) {
#ifdef CONFIG_APDS9960_ENABLE_ALS
	case SENSOR_CHAN_LIGHT:
		val->val1 = sys_le16_to_cpu(data->sample_crgb[0]);
		val->val2 = 0;
		break;
	case SENSOR_CHAN_RED:
		val->val1 = sys_le16_to_cpu(data->sample_crgb[1]);
		val->val2 = 0;
		break;
	case SENSOR_CHAN_GREEN:
		val->val1 = sys_le16_to_cpu(data->sample_crgb[2]);
		val->val2 = 0;
		break;
	case SENSOR_CHAN_BLUE:
		val->val1 = sys_le16_to_cpu(data->sample_crgb[3]);
		val->val2 = 0;
		break;
#endif
	case SENSOR_CHAN_PROX:
		val->val1 = data->pdata;
		val->val2 = 0;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int apds9960_proxy_setup(const struct device *dev)
{
	const struct apds9960_config *config = dev->config;

	if (i2c_reg_write_byte_dt(&config->i2c,
			       APDS9960_POFFSET_UR_REG,
			       APDS9960_DEFAULT_POFFSET_UR)) {
		LOG_ERR("Default offset UR not set ");
		return -EIO;
	}

	if (i2c_reg_write_byte_dt(&config->i2c,
			       APDS9960_POFFSET_DL_REG,
			       APDS9960_DEFAULT_POFFSET_DL)) {
		LOG_ERR("Default offset DL not set ");
		return -EIO;
	}

	if (i2c_reg_write_byte_dt(&config->i2c,
			       APDS9960_PPULSE_REG,
			       config->ppcount)) {
		LOG_ERR("Default pulse count not set ");
		return -EIO;
	}

	if (i2c_reg_update_byte_dt(&config->i2c,
				APDS9960_CONTROL_REG,
				APDS9960_CONTROL_LDRIVE,
				APDS9960_DEFAULT_LDRIVE)) {
		LOG_ERR("LED Drive Strength not set");
		return -EIO;
	}

	if (i2c_reg_update_byte_dt(&config->i2c,
				APDS9960_CONFIG2_REG,
				APDS9960_PLED_BOOST_300,
				config->pled_boost)) {
		LOG_ERR("LED Drive Strength not set");
		return -EIO;
	}

	if (i2c_reg_update_byte_dt(&config->i2c,
				APDS9960_CONTROL_REG, APDS9960_CONTROL_PGAIN,
				(config->pgain & APDS9960_PGAIN_8X))) {
		LOG_ERR("Gain is not set");
		return -EIO;
	}

	if (i2c_reg_write_byte_dt(&config->i2c,
			       APDS9960_PILT_REG, APDS9960_DEFAULT_PILT)) {
		LOG_ERR("Low threshold not set");
		return -EIO;
	}

	if (i2c_reg_write_byte_dt(&config->i2c,
			       APDS9960_PIHT_REG, APDS9960_DEFAULT_PIHT)) {
		LOG_ERR("High threshold not set");
		return -EIO;
	}

	if (i2c_reg_update_byte_dt(&config->i2c,
				APDS9960_ENABLE_REG, APDS9960_ENABLE_PEN,
				APDS9960_ENABLE_PEN)) {
		LOG_ERR("Proximity mode is not enabled");
		return -EIO;
	}

	return 0;
}

#ifdef CONFIG_APDS9960_ENABLE_ALS
static int apds9960_ambient_setup(const struct device *dev)
{
	const struct apds9960_config *config = dev->config;
	uint16_t th;

	/* ADC value */
	if (i2c_reg_write_byte_dt(&config->i2c,
			       APDS9960_ATIME_REG, APDS9960_DEFAULT_ATIME)) {
		LOG_ERR("Default integration time not set for ADC");
		return -EIO;
	}

	/* ALS Gain */
	if (i2c_reg_update_byte_dt(&config->i2c,
				APDS9960_CONTROL_REG,
				APDS9960_CONTROL_AGAIN,
				(config->again & APDS9960_AGAIN_64X))) {
		LOG_ERR("Ambient Gain is not set");
		return -EIO;
	}

	th = sys_cpu_to_le16(APDS9960_DEFAULT_AILT);
	if (i2c_burst_write_dt(&config->i2c,
			    APDS9960_INT_AILTL_REG,
			    (uint8_t *)&th, sizeof(th))) {
		LOG_ERR("ALS low threshold not set");
		return -EIO;
	}

	th = sys_cpu_to_le16(APDS9960_DEFAULT_AIHT);
	if (i2c_burst_write_dt(&config->i2c,
			    APDS9960_INT_AIHTL_REG,
			    (uint8_t *)&th, sizeof(th))) {
		LOG_ERR("ALS low threshold not set");
		return -EIO;
	}

	/* Enable ALS */
	if (i2c_reg_update_byte_dt(&config->i2c,
				APDS9960_ENABLE_REG, APDS9960_ENABLE_AEN,
				APDS9960_ENABLE_AEN)) {
		LOG_ERR("ALS is not enabled");
		return -EIO;
	}

	return 0;
}
#endif

static int apds9960_sensor_setup(const struct device *dev)
{
	const struct apds9960_config *config = dev->config;
	uint8_t chip_id;

	if (i2c_reg_read_byte_dt(&config->i2c,
			      APDS9960_ID_REG, &chip_id)) {
		LOG_ERR("Failed reading chip id");
		return -EIO;
	}

	if (!((chip_id == APDS9960_ID_1) || (chip_id == APDS9960_ID_2))) {
		LOG_ERR("Invalid chip id 0x%x", chip_id);
		return -EIO;
	}

	/* Disable all functions and interrupts */
	if (i2c_reg_write_byte_dt(&config->i2c,
			       APDS9960_ENABLE_REG, 0)) {
		LOG_ERR("ENABLE register is not cleared");
		return -EIO;
	}

	if (i2c_reg_write_byte_dt(&config->i2c,
			       APDS9960_AICLEAR_REG, 0)) {
		return -EIO;
	}

	/* Disable gesture interrupt */
	if (i2c_reg_write_byte_dt(&config->i2c,
			       APDS9960_GCONFIG4_REG, 0)) {
		LOG_ERR("GCONFIG4 register is not cleared");
		return -EIO;
	}

	if (i2c_reg_write_byte_dt(&config->i2c,
			       APDS9960_WTIME_REG, APDS9960_DEFAULT_WTIME)) {
		LOG_ERR("Default wait time not set");
		return -EIO;
	}

	if (i2c_reg_write_byte_dt(&config->i2c,
			       APDS9960_CONFIG1_REG,
			       APDS9960_DEFAULT_CONFIG1)) {
		LOG_ERR("Default WLONG not set");
		return -EIO;
	}

	if (i2c_reg_write_byte_dt(&config->i2c,
			       APDS9960_CONFIG2_REG,
			       APDS9960_DEFAULT_CONFIG2)) {
		LOG_ERR("Configuration Register Two not set");
		return -EIO;
	}

	if (i2c_reg_write_byte_dt(&config->i2c,
			       APDS9960_CONFIG3_REG,
			       APDS9960_DEFAULT_CONFIG3)) {
		LOG_ERR("Configuration Register Three not set");
		return -EIO;
	}

	if (i2c_reg_write_byte_dt(&config->i2c,
			       APDS9960_PERS_REG,
			       APDS9960_DEFAULT_PERS)) {
		LOG_ERR("Interrupt persistence not set");
		return -EIO;
	}

	if (apds9960_proxy_setup(dev)) {
		LOG_ERR("Failed to setup proximity functionality");
		return -EIO;
	}

#ifdef CONFIG_APDS9960_ENABLE_ALS
	if (apds9960_ambient_setup(dev)) {
		LOG_ERR("Failed to setup ambient light functionality");
		return -EIO;
	}
#endif

	return 0;
}

static int apds9960_init_interrupt(const struct device *dev)
{
	const struct apds9960_config *config = dev->config;
	struct apds9960_data *drv_data = dev->data;

	if (!gpio_is_ready_dt(&config->int_gpio)) {
		LOG_ERR("%s: device %s is not ready", dev->name,
			config->int_gpio.port->name);
		return -ENODEV;
	}

	gpio_pin_configure_dt(&config->int_gpio, GPIO_INPUT | config->int_gpio.dt_flags);

	gpio_init_callback(&drv_data->gpio_cb,
			   apds9960_gpio_callback,
			   BIT(config->int_gpio.pin));

	if (gpio_add_callback(config->int_gpio.port, &drv_data->gpio_cb) < 0) {
		LOG_DBG("Failed to set gpio callback!");
		return -EIO;
	}

	drv_data->dev = dev;

#ifdef CONFIG_APDS9960_TRIGGER
	drv_data->work.handler = apds9960_work_cb;
	if (i2c_reg_update_byte_dt(&config->i2c,
				APDS9960_ENABLE_REG,
				APDS9960_ENABLE_PON,
				APDS9960_ENABLE_PON)) {
		LOG_ERR("Power on bit not set.");
		return -EIO;
	}

#else
	k_sem_init(&drv_data->data_sem, 0, K_SEM_MAX_LIMIT);
#endif
	apds9960_setup_int(config, true);

	if (gpio_pin_get_dt(&config->int_gpio) > 0) {
		apds9960_handle_cb(drv_data);
	}

	return 0;
}

#ifdef CONFIG_PM_DEVICE
static int apds9960_pm_action(const struct device *dev,
			      enum pm_device_action action)
{
	const struct apds9960_config *config = dev->config;
	int ret = 0;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		if (i2c_reg_update_byte_dt(&config->i2c,
					APDS9960_ENABLE_REG,
					APDS9960_ENABLE_PON,
					APDS9960_ENABLE_PON)) {
			ret = -EIO;
		}
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		if (i2c_reg_update_byte_dt(&config->i2c,
					APDS9960_ENABLE_REG,
					APDS9960_ENABLE_PON, 0)) {
			ret = -EIO;
		}

		if (i2c_reg_write_byte_dt(&config->i2c,
				       APDS9960_AICLEAR_REG, 0)) {
			ret = -EIO;
		}
		break;
	default:
		return -ENOTSUP;
	}

	return ret;
}
#endif

static int apds9960_init(const struct device *dev)
{
	const struct apds9960_config *config = dev->config;
	struct apds9960_data *data = dev->data;

	/* Initialize time 5.7ms */
	k_sleep(K_MSEC(6));

	if (!device_is_ready(config->i2c.bus)) {
		LOG_ERR("Bus device is not ready");
		return -EINVAL;
	}

	(void)memset(data->sample_crgb, 0, sizeof(data->sample_crgb));
	data->pdata = 0U;

	if (apds9960_sensor_setup(dev) < 0) {
		LOG_ERR("Failed to setup device!");
		return -EIO;
	}

	if (apds9960_init_interrupt(dev) < 0) {
		LOG_ERR("Failed to initialize interrupt!");
		return -EIO;
	}

	return 0;
}

static const struct sensor_driver_api apds9960_driver_api = {
	.sample_fetch = &apds9960_sample_fetch,
	.channel_get = &apds9960_channel_get,
#ifdef CONFIG_APDS9960_TRIGGER
	.attr_set = apds9960_attr_set,
	.trigger_set = apds9960_trigger_set,
#endif
};

static const struct apds9960_config apds9960_config = {
	.i2c = I2C_DT_SPEC_INST_GET(0),
	.int_gpio = GPIO_DT_SPEC_INST_GET(0, int_gpios),
#if CONFIG_APDS9960_PGAIN_8X
	.pgain = APDS9960_PGAIN_8X,
#elif CONFIG_APDS9960_PGAIN_4X
	.pgain = APDS9960_PGAIN_4X,
#elif CONFIG_APDS9960_PGAIN_2X
	.pgain = APDS9960_PGAIN_2X,
#else
	.pgain = APDS9960_PGAIN_1X,
#endif
#if CONFIG_APDS9960_AGAIN_64X
	.again = APDS9960_AGAIN_64X,
#elif CONFIG_APDS9960_AGAIN_16X
	.again = APDS9960_AGAIN_16X,
#elif CONFIG_APDS9960_AGAIN_4X
	.again = APDS9960_AGAIN_4X,
#else
	.again = APDS9960_AGAIN_1X,
#endif
#if CONFIG_APDS9960_PPULSE_LENGTH_32US
	.ppcount = APDS9960_PPULSE_LENGTH_32US |
		   (CONFIG_APDS9960_PPULSE_COUNT - 1),
#elif CONFIG_APDS9960_PPULSE_LENGTH_16US
	.ppcount = APDS9960_PPULSE_LENGTH_16US |
		   (CONFIG_APDS9960_PPULSE_COUNT - 1),
#elif CONFIG_APDS9960_PPULSE_LENGTH_8US
	.ppcount = APDS9960_PPULSE_LENGTH_8US |
		   (CONFIG_APDS9960_PPULSE_COUNT - 1),
#else
	.ppcount = APDS9960_PPULSE_LENGTH_4US |
		   (CONFIG_APDS9960_PPULSE_COUNT - 1),
#endif
#if CONFIG_APDS9960_PLED_BOOST_300PCT
	.pled_boost = APDS9960_PLED_BOOST_300,
#elif CONFIG_APDS9960_PLED_BOOST_200PCT
	.pled_boost = APDS9960_PLED_BOOST_200,
#elif CONFIG_APDS9960_PLED_BOOST_150PCT
	.pled_boost = APDS9960_PLED_BOOST_150,
#else
	.pled_boost = APDS9960_PLED_BOOST_100,
#endif
};

static struct apds9960_data apds9960_data;

PM_DEVICE_DT_INST_DEFINE(0, apds9960_pm_action);

SENSOR_DEVICE_DT_INST_DEFINE(0, apds9960_init,
	      PM_DEVICE_DT_INST_GET(0), &apds9960_data, &apds9960_config,
	      POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, &apds9960_driver_api);
