/*
 * Copyright (c) 2022 Esco Medical Aps.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_pca9956

/**
 * @file
 * @brief LED driver for the PCA9956 I2C LED driver (The 7-bit slave address
 *  is determined by the quinary input pads AD0, AD1 and AD2)
 */

#include <drivers/i2c.h>
#include <drivers/gpio.h>
#include <drivers/led.h>
#include <sys/util.h>
#include <zephyr.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(pca9956, CONFIG_LED_LOG_LEVEL);

#include "led_context.h"

/* PCA9956 select LDRx registers determine the source that drives LED outputs */
#define PCA9956_LED_OFF		0x0 /* LED driver off */
#define PCA9956_LED_ON		0x1 /* LED driver on */
#define PCA9956_LED_PWM		0x2 /* Controlled through PWM */
#define PCA9956_LED_GRP_PWM	0x3 /* Controlled through PWM/GRPPWM */

/* PCA9956 control register */
#define PCA9956_MODE1		0x00
#define PCA9956_MODE2		0x01
#define PCA9956_PWM_BASE	0x0A /* Reg 0x0A-0x21 for brightness control LED01-24 */
#define PCA9956_GRPPWM		0x08
#define PCA9956_GRPFREQ		0x09
#define PCA9956_LEDOUT0		0x02
#define PCA9956_IREFALL		0x40

/* PCA9956 mode register 1 */
#define PCA9956_MODE1_SLEEP	0x10 /* Sleep Mode */
/* PCA9956 mode register 2 */
#define PCA9956_MODE2_DMBLNK	0x20 /* Enable blinking */

#define PCA9956_STATE_CTRL_MASK	GENMASK(1, 0)

#define PCA9956_MAX_LED_GROUP	6 /* LEDOUT0-LEDOUT5 */

#define PCA9956_LED_PER_GROUP	4

/* Brightness limits in percent */
#define PCA9956_MIN_BRIGHTNESS	0
#define PCA9956_MAX_BRIGHTNESS	100

/* The minimum blinking period is 67 ms, frequency 15 Hz */
#define PCA9956_MIN_BLINK_PERIOD 67

/*
 * From manual:
 * General brightness for the 24 outputs is controlled through 256 linear steps from 00h
 *(0 % duty cycle = LED output off) to FFh (99.6 % duty cycle = maximum brightness).
 * period = ((GFRQ + 1) / 15.26) in seconds.
 * So, period (in ms) = (((255 + 1) / 15.26 * 1000) = 16775.884 . We round it to 16776 ms.
 */
#define PCA9956_MAX_BLINK_PERIOD 16776

struct pca9956_config {
	struct i2c_dt_spec i2c;
	struct gpio_dt_spec output_enable;
	struct gpio_dt_spec reset;
	uint8_t default_iref;
};

static int pca9956_set_led_output_state_reg(const struct pca9956_config *config, uint32_t led,
					    uint8_t val)
{
	/* Reg 0x02-0x07 for output state registers LEDOUT0-LEDOUT5 */
	uint8_t led_group = led / PCA9956_LED_PER_GROUP;
	uint8_t reg_addr = PCA9956_LEDOUT0 + led_group;

	/* Total 24 LED output state controls and 6 LED Groups. 4 LED output state controls in
	 * each LED Group and 2 bits for each output state control in each led group.
	 */
	uint8_t led_output_state_control = (led % PCA9956_LED_PER_GROUP) << 1;

	uint8_t mask = PCA9956_STATE_CTRL_MASK << led_output_state_control;
	uint8_t value = val << led_output_state_control;

	if (i2c_reg_update_byte_dt(&config->i2c, reg_addr, mask, value)) {
		LOG_ERR("LED reg update failed");
		return -EIO;
	}

	return 0;
}

static int pca9956_led_blink(const struct device *dev, uint32_t led, uint32_t delay_on,
			     uint32_t delay_off)
{
	const struct pca9956_config *config = dev->config;
	uint8_t gdc;
	uint8_t gfrq;
	uint32_t period;

	period = delay_on + delay_off;

	if (period < PCA9956_MIN_BLINK_PERIOD || period > PCA9956_MAX_BLINK_PERIOD) {
		return -EINVAL;
	}

	/*
	 * From manual:
	 * duty cycle = (GDC / 256) ->
	 *	(time_on / period) = (GDC / 256) ->
	 *		GDC = ((time_on * 256) / period)
	 */
	gdc = delay_on * 256U / period;
	if (i2c_reg_write_byte_dt(&config->i2c, PCA9956_GRPPWM, gdc)) {
		LOG_ERR("LED reg write failed");
		return -EIO;
	}

	/*
	 * From manual:
	 * period = ((GFRQ + 1) / 15.26) in seconds.
	 * So, period (in ms) = (((GFRQ + 1) / 15.26 * 1000) ->
	 *		 GFRQ = ((period * 15.26 / 1000) - 1)
	 */
	gfrq = (period * 15.26f / 1000) - 1;
	if (i2c_reg_write_byte_dt(&config->i2c, PCA9956_GRPFREQ, gfrq)) {
		LOG_ERR("LED reg write failed");
		return -EIO;
	}

	/* Enable blinking mode */
	if (i2c_reg_update_byte_dt(&config->i2c, PCA9956_MODE2, PCA9956_MODE2_DMBLNK,
				   PCA9956_MODE2_DMBLNK)) {
		LOG_ERR("LED reg update failed");
		return -EIO;
	}

	/* Select the GRPPWM source to drive the LED outpout */
	return pca9956_set_led_output_state_reg(config, led, PCA9956_LED_GRP_PWM);
}

static int pca9956_led_set_brightness(const struct device *dev, uint32_t led, uint8_t value)
{
	const struct pca9956_config *config = dev->config;
	uint8_t val;

	if (value < PCA9956_MIN_BRIGHTNESS || value > PCA9956_MAX_BRIGHTNESS) {
		return -EINVAL;
	}

	/* Set the LED brightness value */
	val = (value * 255U) / PCA9956_MAX_BRIGHTNESS;
	if (i2c_reg_write_byte_dt(&config->i2c, PCA9956_PWM_BASE + led, val)) {
		LOG_ERR("LED reg write failed");
		return -EIO;
	}

	/* Set the LED driver to be controlled through its PWMx register. */
	return pca9956_set_led_output_state_reg(config, led, PCA9956_LED_PWM);
}

static inline int pca9956_led_on(const struct device *dev, uint32_t led)
{
	const struct pca9956_config *config = dev->config;

	return pca9956_set_led_output_state_reg(config, led, PCA9956_LED_ON);
}

static inline int pca9956_led_off(const struct device *dev, uint32_t led)
{
	const struct pca9956_config *config = dev->config;

	return pca9956_set_led_output_state_reg(config, led, PCA9956_LED_OFF);
}

static int pca9956_led_init(const struct device *dev)
{
	const struct pca9956_config *config = dev->config;
	int i;

	if (!device_is_ready(config->i2c.bus)) {
		LOG_ERR("I2C bus is not ready");
		return -ENODEV;
	}

	/* Take the LED driver out from Sleep mode. */
	if (i2c_reg_update_byte_dt(&config->i2c, PCA9956_MODE1, PCA9956_MODE1_SLEEP,
				   ~PCA9956_MODE1_SLEEP)) {
		LOG_ERR("LED reg update failed");
		return -EIO;
	}

	if (config->output_enable.port) {
		if (!device_is_ready(config->output_enable.port)) {
			LOG_ERR("%s: GPIO device not ready", config->output_enable.port->name);
			return -ENODEV;
		}

		/* Set output enable pin active*/
		if (gpio_pin_configure_dt(&config->output_enable, GPIO_OUTPUT_ACTIVE)) {
			LOG_ERR("Cannot configure GPIO");
			return -EIO;
		}
	}

	if (config->reset.port) {
		if (!device_is_ready(config->reset.port)) {
			LOG_ERR("%s: GPIO device not ready", config->reset.port->name);
			return -ENODEV;
		}

		/* Set reset pin inactive*/
		if (gpio_pin_configure_dt(&config->reset, GPIO_OUTPUT_INACTIVE)) {
			LOG_ERR("Cannot configure GPIO");
			return -EIO;
		}
	}

	/* Turn off all the leds by setting 0x00 to all the bits in LED output state registers
	 * (LEDOUT0-LEDOUT5)
	 */
	for (i = 0; i < PCA9956_MAX_LED_GROUP; i++) {
		if (i2c_reg_write_byte_dt(&config->i2c, PCA9956_LEDOUT0 + i, PCA9956_LED_OFF)) {
			LOG_ERR("LED reg update failed");
			return -EIO;
		}
	}

	/* Set the output current */
	if (i2c_reg_write_byte_dt(&config->i2c, PCA9956_IREFALL, config->default_iref)) {
		LOG_ERR("LED reg write failed");
		return -EIO;
	}

	return 0;
}

static const struct led_driver_api pca9956_led_api = {
	.blink = pca9956_led_blink,
	.set_brightness = pca9956_led_set_brightness,
	.on = pca9956_led_on,
	.off = pca9956_led_off,
};

#define PCA9956_DEVICE(id)									\
	static const struct pca9956_config pca9956_##id##_cfg = {				\
		.i2c = I2C_DT_SPEC_INST_GET(id),						\
		.output_enable = GPIO_DT_SPEC_INST_GET_OR(id, output_enable_gpios, { 0 }),	\
		.reset = GPIO_DT_SPEC_INST_GET_OR(id, reset_gpios, { 0 }),			\
		.default_iref = DT_INST_PROP(id, default_iref)					\
	};											\
												\
	DEVICE_DT_INST_DEFINE(id, &pca9956_led_init, NULL, NULL,				\
			      &pca9956_##id##_cfg, POST_KERNEL, CONFIG_LED_INIT_PRIORITY,	\
			      &pca9956_led_api);

DT_INST_FOREACH_STATUS_OKAY(PCA9956_DEVICE)
