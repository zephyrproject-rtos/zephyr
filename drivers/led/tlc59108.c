/*
 * Copyright (c) 2021 Sky Hero SA
 * Copyright (c) 2018 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_tlc59108

/**
 * @file
 * @brief LED driver for the TLC59108 I2C LED driver
 */

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/led.h>
#include <zephyr/sys/util.h>
#include <zephyr/zephyr.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(tlc59108, CONFIG_LED_LOG_LEVEL);

#include "led_context.h"

/* TLC59108 max supported LED id */
#define TLC59108_MAX_LED 7

/* TLC59108 select registers determine the source that drives LED outputs */
#define TLC59108_LED_OFF         0x0     /* LED driver off */
#define TLC59108_LED_ON          0x1     /* LED driver on */
#define TLC59108_LED_PWM         0x2     /* Controlled through PWM */
#define TLC59108_LED_GRP_PWM     0x3     /* Controlled through PWM/GRPPWM */

/* TLC59108 control register */
#define TLC59108_MODE1           0x00
#define TLC59108_MODE2           0x01
#define TLC59108_PWM_BASE        0x02
#define TLC59108_GRPPWM          0x0A
#define TLC59108_GRPFREQ         0x0B
#define TLC59108_LEDOUT0         0x0C
#define TLC59108_LEDOUT1         0x0D

/* TLC59108 mode register 1 */
#define TLC59108_MODE1_OSC       0x10

/* TLC59108 mode register 2 */
#define TLC59108_MODE2_DMBLNK    0x20    /* Enable blinking */

#define TLC59108_MASK            0x03


struct tlc59108_cfg {
	const struct device *i2c_dev;
	uint16_t i2c_addr;
};

struct tlc59108_data {
	struct led_data dev_data;
};

static int tlc59108_set_ledout(const struct device *dev, uint32_t led,
		uint8_t val)
{
	const struct tlc59108_cfg *config = dev->config;

	if (led < 4) {
		if (i2c_reg_update_byte(config->i2c_dev, config->i2c_addr,
					TLC59108_LEDOUT0,
					TLC59108_MASK << (led << 1),
					val << (led << 1))) {
			LOG_ERR("LED reg 0x%x update failed", TLC59108_LEDOUT0);
			return -EIO;
		}
	} else {
		if (i2c_reg_update_byte(config->i2c_dev, config->i2c_addr,
					TLC59108_LEDOUT1,
					TLC59108_MASK << ((led - 4) << 1),
					val << ((led - 4)  << 1))) {
			LOG_ERR("LED reg 0x%x update failed", TLC59108_LEDOUT1);
			return -EIO;
		}
	}

	return 0;
}

static int tlc59108_led_blink(const struct device *dev, uint32_t led,
		uint32_t delay_on, uint32_t delay_off)
{
	const struct tlc59108_cfg *config = dev->config;
	struct tlc59108_data *data = dev->data;
	struct led_data *dev_data = &data->dev_data;
	uint8_t gdc, gfrq;
	uint32_t period;

	period = delay_on + delay_off;

	if (led > TLC59108_MAX_LED) {
		return -EINVAL;
	}

	if (period < dev_data->min_period || period > dev_data->max_period) {
		return -EINVAL;
	}

	/*
	 * From manual:
	 * duty cycle = (GDC / 256) ->
	 *	(time_on / period) = (GDC / 256) ->
	 *		GDC = ((time_on * 256) / period)
	 */
	gdc = delay_on * 256U / period;
	if (i2c_reg_write_byte(config->i2c_dev, config->i2c_addr,
				TLC59108_GRPPWM,
				gdc)) {
		LOG_ERR("LED reg 0x%x write failed", TLC59108_GRPPWM);
		return -EIO;
	}

	/*
	 * From manual:
	 * period = ((GFRQ + 1) / 24) in seconds.
	 * So, period (in ms) = (((GFRQ + 1) / 24) * 1000) ->
	 *		GFRQ = ((period * 24 / 1000) - 1)
	 */
	gfrq = (period * 24U / 1000) - 1;
	if (i2c_reg_write_byte(config->i2c_dev, config->i2c_addr,
				TLC59108_GRPFREQ,
				gfrq)) {
		LOG_ERR("LED reg 0x%x write failed", TLC59108_GRPFREQ);
		return -EIO;
	}

	/* Enable blinking mode */
	if (i2c_reg_update_byte(config->i2c_dev, config->i2c_addr,
				TLC59108_MODE2,
				TLC59108_MODE2_DMBLNK,
				TLC59108_MODE2_DMBLNK)) {
		LOG_ERR("LED reg 0x%x update failed", TLC59108_MODE2);
		return -EIO;
	}

	/* Select the GRPPWM source to drive the LED output */
	return tlc59108_set_ledout(dev, led, TLC59108_LED_GRP_PWM);
}

static int tlc59108_led_set_brightness(const struct device *dev, uint32_t led,
		uint8_t value)
{
	const struct tlc59108_cfg *config = dev->config;
	struct tlc59108_data *data = dev->data;
	struct led_data *dev_data = &data->dev_data;
	uint8_t val;

	if (led > TLC59108_MAX_LED) {
		return -EINVAL;
	}

	if (value < dev_data->min_brightness ||
	    value > dev_data->max_brightness) {
		return -EINVAL;
	}

	/* Set the LED brightness value */
	val = (value * 255U) / dev_data->max_brightness;
	if (i2c_reg_write_byte(config->i2c_dev, config->i2c_addr,
				TLC59108_PWM_BASE + led,
				val)) {
		LOG_ERR("LED 0x%x reg write failed", TLC59108_PWM_BASE + led);
		return -EIO;
	}

	/* Set the LED driver to be controlled through its PWMx register. */
	return tlc59108_set_ledout(dev, led, TLC59108_LED_PWM);
}

static inline int tlc59108_led_on(const struct device *dev, uint32_t led)
{
	if (led > TLC59108_MAX_LED) {
		return -EINVAL;
	}

	/* Set LED state to ON */
	return tlc59108_set_ledout(dev, led, TLC59108_LED_ON);
}

static inline int tlc59108_led_off(const struct device *dev, uint32_t led)
{
	if (led > TLC59108_MAX_LED) {
		return -EINVAL;
	}

	/* Set LED state to OFF */
	return tlc59108_set_ledout(dev, led, TLC59108_LED_OFF);
}

static int tlc59108_led_init(const struct device *dev)
{
	const struct tlc59108_cfg *config = dev->config;
	struct tlc59108_data *data = dev->data;
	struct led_data *dev_data = &data->dev_data;

	if (!device_is_ready(config->i2c_dev)) {
		LOG_ERR("I2C bus device %s is not ready", config->i2c_dev->name);
		return -ENODEV;
	}

	/* Wake up from sleep mode */
	if (i2c_reg_update_byte(config->i2c_dev, config->i2c_addr,
				TLC59108_MODE1,
				TLC59108_MODE1_OSC, 0)) {
		LOG_ERR("LED reg 0x%x update failed", TLC59108_MODE1);
		return -EIO;
	}

	/* Hardware specific limits */
	dev_data->min_period = 41U;
	dev_data->max_period = 10730U;
	dev_data->min_brightness = 0U;
	dev_data->max_brightness = 100U;

	return 0;
}

static const struct led_driver_api tlc59108_led_api = {
	.blink = tlc59108_led_blink,
	.set_brightness = tlc59108_led_set_brightness,
	.on = tlc59108_led_on,
	.off = tlc59108_led_off,
};

#define TLC59108_DEVICE(id) \
	static const struct tlc59108_cfg tlc59108_##id##_cfg = {	\
		.i2c_dev = DEVICE_DT_GET(DT_INST_BUS(id)),		\
		.i2c_addr     = DT_INST_REG_ADDR(id),			\
	};								\
	static struct tlc59108_data tlc59108_##id##_data;		\
									\
	DEVICE_DT_INST_DEFINE(id, &tlc59108_led_init, NULL,		\
			&tlc59108_##id##_data,				\
			&tlc59108_##id##_cfg, POST_KERNEL,		\
			CONFIG_LED_INIT_PRIORITY,			\
			&tlc59108_led_api);

DT_INST_FOREACH_STATUS_OKAY(TLC59108_DEVICE)
