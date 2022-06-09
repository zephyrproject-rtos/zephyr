/*
 * Copyright (c) 2018 Savoir-Faire Linux.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief LED driver for the PCA963x I2C LED drivers
 */

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/led.h>
#include <zephyr/sys/util.h>
#include <zephyr/zephyr.h>

#define LOG_LEVEL CONFIG_LED_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(pca_common);

#include "led_context.h"

#define _DT_PROP_ANY_OR(id, compat, prop) DT_PROP(DT_INST(id, compat), prop) ||
#define DT_PROP_ANY(compat, prop) \
	DT_FOREACH_STATUS_OKAY_VARGS(compat, _DT_PROP_ANY_OR, compat, prop) 0

/* PCA963X select registers determine the source that drives LED outputs */
#define PCA963X_LED_OFF         0x0     /* LED driver off */
#define PCA963X_LED_ON          0x1     /* LED driver on */
#define PCA963X_LED_PWM         0x2     /* Controlled through PWM */
#define PCA963X_LED_GRP_PWM     0x3     /* Controlled through PWM/GRPPWM */

/* PCA963X control register */
#define PCA963X_MODE1           0x00
#define PCA963X_MODE2           0x01

/* PCA9633 control register */
#define PCA9633_PWM_BASE        0x02	/* Reg 0x02-0x05 for brightness control LED01-04 */
#define PCA9633_GRPPWM          0x06
#define PCA9633_GRPFREQ         0x07
#define PCA9633_LEDOUT          0x08

/* PCA9634 control register */
#define PCA9634_PWM_BASE        0x02	/* Reg 0x02-0x09 for brightness control LED01-08 */
#define PCA9634_GRPPWM          0x0A
#define PCA9634_GRPFREQ         0x0B
#define PCA9634_LEDOUT          0x0C

/* PCA9956 control register */
#define PCA9956_PWM_BASE        0x0A	/* Reg 0x0B-0x21 for brightness control LED01-24 */
#define PCA9956_GRPPWM          0x08
#define PCA9956_GRPFREQ         0x09
#define PCA9956_LEDOUT          0x02

/* PCA963X mode register 1 */
#define PCA963X_MODE1_SLEEP     0x10    /* Sleep Mode */
/* PCA963X mode register 2 */
#define PCA963X_MODE2_INVRT	0x10	/* Enable inverted output / external driver */
#define PCA963X_MODE2_DMBLNK    0x20    /* Enable blinking */

#define PCA963X_MASK            0x03

/* PCA963X parameters */
#define PCA963X_MIN_PERIOD	41U
#define PCA963X_MAX_PERIOD	10667U
#define PCA963X_MIN_BRIGHTNESS  0U
#define PCA963X_MAX_BRIGHTNESS  100U

/* PCA9956 parameters */
#define PCA9956_MIN_PERIOD	67U
#define PCA9956_MAX_PERIOD	16776U
#define PCA9956_MIN_BRIGHTNESS	0U
#define PCA9956_MAX_BRIGHTNESS	100U

/* TLC59108 parameters */
#define TLC59108_MIN_PERIOD	41U
#define TLC59108_MAX_PERIOD	10730U
#define TLC59108_MIN_BRIGHTNESS	0U
#define TLC59108_MAX_BRIGHTNESS	100U

struct pca_common_config {
	struct i2c_dt_spec i2c;
	struct led_data led_data;
	uint8_t pwm_base;
	uint8_t grppwm;
	uint8_t grpfreq;
	uint8_t ledout;
#if DT_PROP_ANY(nxp_pca9634, external_driver)
	bool external_driver;
#endif
};

static int pca_common_led_blink(const struct device *dev, uint32_t led,
			     uint32_t delay_on, uint32_t delay_off)
{
	const struct pca_common_config *config = dev->config;
	const struct led_data *led_data = &config->led_data;
	uint8_t gdc, gfrq;
	uint32_t period;

	period = delay_on + delay_off;

	if (period < led_data->min_period || period > led_data->max_period) {
		return -EINVAL;
	}

	/*
	 * From manual:
	 * duty cycle = (GDC / 256) ->
	 *	(time_on / period) = (GDC / 256) ->
	 *		GDC = ((time_on * 256) / period)
	 */
	gdc = delay_on * 256U / period;
	if (i2c_reg_write_byte_dt(&config->i2c,
			       config->grppwm,
			       gdc)) {
		LOG_ERR("LED reg write failed");
		return -EIO;
	}

	/*
	 * From manual:
	 * period = ((GFRQ + 1) / 24) in seconds.
	 * So, period (in ms) = (((GFRQ + 1) / 24) * 1000) ->
	 *		GFRQ = ((period * 24 / 1000) - 1)
	 */
	gfrq = (period * 24U / 1000) - 1;
	if (i2c_reg_write_byte_dt(&config->i2c,
			       config->grpfreq,
			       gfrq)) {
		LOG_ERR("LED reg write failed");
		return -EIO;
	}

	/* Enable blinking mode */
	if (i2c_reg_update_byte_dt(&config->i2c,
				PCA963X_MODE2,
				PCA963X_MODE2_DMBLNK,
				PCA963X_MODE2_DMBLNK)) {
		LOG_ERR("LED reg update failed");
		return -EIO;
	}

	/* Select the GRPPWM source to drive the LED output */
	if (i2c_reg_update_byte_dt(&config->i2c,
				config->ledout + (led / 4),
				PCA963X_MASK << ((led % 4) << 1),
				PCA963X_LED_GRP_PWM << ((led % 4) << 1))) {
		LOG_ERR("LED reg update failed");
		return -EIO;
	}

	return 0;
}

static int pca_common_led_set_brightness(const struct device *dev, uint32_t led,
				      uint8_t value)
{
	const struct pca_common_config *config = dev->config;
	const struct led_data *led_data = &config->led_data;
	uint8_t val;

	if (value < led_data->min_brightness ||
	    value > led_data->max_brightness) {
		return -EINVAL;
	}

	/* Set the LED brightness value */
	val = (value * 255U) / led_data->max_brightness;
	if (i2c_reg_write_byte_dt(&config->i2c,
			       config->pwm_base + led,
			       val)) {
		LOG_ERR("LED reg write failed");
		return -EIO;
	}

	/* Set the LED driver to be controlled through its PWMx register. */
	if (i2c_reg_update_byte_dt(&config->i2c,
				config->ledout + (led / 4),
				PCA963X_MASK << ((led % 4) << 1),
				PCA963X_LED_PWM << ((led % 4) << 1))) {
		LOG_ERR("LED reg update failed");
		return -EIO;
	}

	return 0;
}

static inline int pca_common_led_on(const struct device *dev, uint32_t led)
{
	const struct pca_common_config *config = dev->config;

	/* Set LED state to ON */
	if (i2c_reg_update_byte_dt(&config->i2c,
				config->ledout + (led / 4),
				PCA963X_MASK << ((led % 4) << 1),
				PCA963X_LED_ON << ((led % 4) << 1))) {
		LOG_ERR("LED reg update failed");
		return -EIO;
	}

	return 0;
}

static inline int pca_common_led_off(const struct device *dev, uint32_t led)
{
	const struct pca_common_config *config = dev->config;

	/* Set LED state to OFF */
	if (i2c_reg_update_byte_dt(&config->i2c,
				config->ledout + (led / 4),
				PCA963X_MASK << ((led % 4) << 1),
				PCA963X_LED_OFF)) {
		LOG_ERR("LED reg update failed");
		return -EIO;
	}

	return 0;
}

static int pca_common_led_init(const struct device *dev)
{
	const struct pca_common_config *config = dev->config;

	if (!device_is_ready(config->i2c.bus)) {
		LOG_ERR("I2C bus is not ready");
		return -ENODEV;
	}

	/* Take the LED driver out from Sleep mode. */
	if (i2c_reg_update_byte_dt(&config->i2c,
				PCA963X_MODE1,
				PCA963X_MODE1_SLEEP,
				~PCA963X_MODE1_SLEEP)) {
		LOG_ERR("LED reg update failed");
		return -EIO;
	}

#if DT_PROP_ANY(nxp_pca9634, external_driver)
	/* If there is external driver, set invrt bit */
	if (config->external_driver) {
		if (i2c_reg_update_byte_dt(&config->i2c,
					PCA963X_MODE2,
					PCA963X_MODE2_INVRT,
					PCA963X_MODE2_INVRT)) {
			LOG_ERR("LED reg update failed");
			return -EIO;
		}
	}
#endif

	return 0;
}

static const struct led_driver_api pca_common_led_api = {
	.blink = pca_common_led_blink,
	.set_brightness = pca_common_led_set_brightness,
	.on = pca_common_led_on,
	.off = pca_common_led_off,
};

#define PCA_DEVICE(node_id, prefix, regs, params)						   \
	static const struct pca_common_config _CONCAT(prefix ## _config_, DT_DEP_ORD(node_id)) = { \
		.i2c = I2C_DT_SPEC_GET(node_id),						   \
		.led_data = {									   \
			.min_period = params ## _MIN_PERIOD,					   \
			.max_period = params ## _MAX_PERIOD,					   \
			.min_brightness = params ## _MIN_BRIGHTNESS,				   \
			.max_brightness = params ## _MAX_BRIGHTNESS,				   \
		},										   \
		.pwm_base = regs ## _PWM_BASE,							   \
		.grppwm = regs ## _GRPPWM,							   \
		.grpfreq = regs ## _GRPFREQ,							   \
		.ledout = regs ## _LEDOUT,							   \
COND_CODE_1(DT_PROP_ANY(nxp9634, external_driver), (						   \
		.external_driver = DT_PROP(node_id, external_driver),				   \
), ())												   \
	};											   \
												   \
	DEVICE_DT_DEFINE(node_id, &pca_common_led_init, NULL,					   \
			NULL, &_CONCAT(prefix ## _config_, DT_DEP_ORD(node_id)),		   \
			POST_KERNEL, CONFIG_LED_INIT_PRIORITY,					   \
			&pca_common_led_api);

DT_FOREACH_STATUS_OKAY_VARGS(nxp_pca9633, PCA_DEVICE, pca9633, PCA9633, PCA963X)
DT_FOREACH_STATUS_OKAY_VARGS(nxp_pca9634, PCA_DEVICE, pca9634, PCA9634, PCA963X)
DT_FOREACH_STATUS_OKAY_VARGS(nxp_pca9956, PCA_DEVICE, pca9956, PCA9956, PCA9956)
DT_FOREACH_STATUS_OKAY_VARGS(ti_tlc59108, PCA_DEVICE, tlc59108, PCA9634, TLC59108)
