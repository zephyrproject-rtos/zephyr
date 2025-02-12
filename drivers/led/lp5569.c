/*
 * Copyright (c) 2023 Phytec Messtechnik GmbH.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_lp5569

/**
 * @file
 * @brief LP5569 LED controller
 *
 * The LP5569 is a 9-channel LED driver that communicates over I2C.
 */

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/led.h>
#include <zephyr/device.h>
#include <zephyr/pm/device.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lp5569, CONFIG_LED_LOG_LEVEL);

#define LP5569_NUM_LEDS			9

/* General Registers */
#define LP5569_CONFIG			0x00
#define LP5569_CHIP_EN			BIT(6)

#define LP5569_MISC			0x2F
#define LP5569_POWERSAVE_EN		BIT(5)

/* PWM base Register for controlling the duty-cycle */
#define LP5569_LED0_PWM			0x16

struct lp5569_config {
	struct i2c_dt_spec bus;
};

static int lp5569_led_set_brightness(const struct device *dev, uint32_t led,
				     uint8_t brightness)
{
	const struct lp5569_config *config = dev->config;
	uint8_t val;
	int ret;

	if (led >= LP5569_NUM_LEDS || brightness > 100) {
		return -EINVAL;
	}

	/* Map 0-100 % to 0-255 pwm register value */
	val = brightness * 255 / 100;

	ret = i2c_reg_write_byte_dt(&config->bus, LP5569_LED0_PWM + led, val);
	if (ret < 0) {
		LOG_ERR("LED reg update failed");
		return ret;
	}

	return 0;
}

static inline int lp5569_led_on(const struct device *dev, uint32_t led)
{
	/* Set LED brightness to 100 % */
	return lp5569_led_set_brightness(dev, led, 100);
}

static inline int lp5569_led_off(const struct device *dev, uint32_t led)
{
	/* Set LED brightness to 0 % */
	return lp5569_led_set_brightness(dev, led, 0);
}

static int lp5569_enable(const struct device *dev)
{
	const struct lp5569_config *config = dev->config;
	int ret;

	if (!i2c_is_ready_dt(&config->bus)) {
		LOG_ERR("I2C device not ready");
		return -ENODEV;
	}

	ret = i2c_reg_write_byte_dt(&config->bus, LP5569_CONFIG,
				    LP5569_CHIP_EN);
	if (ret < 0) {
		LOG_ERR("Enable LP5569 failed");
		return ret;
	}

	ret = i2c_reg_write_byte_dt(&config->bus, LP5569_MISC,
				    LP5569_POWERSAVE_EN);
	if (ret < 0) {
		LOG_ERR("LED reg update failed");
		return ret;
	}

	return 0;
}

static int lp5569_init(const struct device *dev)
{
	/* If the device is behind a power domain, it will start in
	 * PM_DEVICE_STATE_OFF.
	 */
	if (pm_device_on_power_domain(dev)) {
		pm_device_init_off(dev);
		LOG_INF("Init %s as PM_DEVICE_STATE_OFF", dev->name);
		return 0;
	}

	return lp5569_enable(dev);
}

#ifdef CONFIG_PM_DEVICE
static int lp5569_pm_action(const struct device *dev,
			    enum pm_device_action action)
{
	const struct lp5569_config *config = dev->config;
	int ret;

	switch (action) {
	case PM_DEVICE_ACTION_TURN_ON:
	case PM_DEVICE_ACTION_RESUME:
		ret = lp5569_enable(dev);
		if (ret < 0) {
			LOG_ERR("Enable LP5569 failed");
			return ret;
		}
		break;
	case PM_DEVICE_ACTION_TURN_OFF:
	case PM_DEVICE_ACTION_SUSPEND:
		ret = i2c_reg_update_byte_dt(&config->bus, LP5569_CONFIG,
					   LP5569_CHIP_EN, 0);
		if (ret < 0) {
			LOG_ERR("Disable LP5569 failed");
			return ret;
		}
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}
#endif /* CONFIG_PM_DEVICE */

static const struct led_driver_api lp5569_led_api = {
	.set_brightness = lp5569_led_set_brightness,
	.on = lp5569_led_on,
	.off = lp5569_led_off,
};

#define LP5569_DEFINE(id)						\
	static const struct lp5569_config lp5569_config_##id = {	\
		.bus = I2C_DT_SPEC_INST_GET(id),			\
	};								\
									\
	PM_DEVICE_DT_INST_DEFINE(id, lp5569_pm_action);			\
									\
	DEVICE_DT_INST_DEFINE(id, &lp5569_init,				\
			      PM_DEVICE_DT_INST_GET(id),		\
			      NULL,					\
			      &lp5569_config_##id, POST_KERNEL,		\
			      CONFIG_LED_INIT_PRIORITY,			\
			      &lp5569_led_api);

DT_INST_FOREACH_STATUS_OKAY(LP5569_DEFINE)
