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

/* PCA963X mode register 1 */
#define PCA963X_MODE1_SLEEP     0x10    /* Sleep Mode */
/* PCA963X mode register 2 */
#define PCA963X_MODE2_DMBLNK    0x20    /* Enable blinking */

#define PCA963X_MASK            0x03

struct pca_common_config {
	struct i2c_dt_spec i2c;
	uint8_t pwm_base;
	uint8_t grppwm;
	uint8_t grpfreq;
	uint8_t ledout;
};

struct pca_common_data {
	struct led_data dev_data;
};

static int pca_common_led_blink(const struct device *dev, uint32_t led,
			     uint32_t delay_on, uint32_t delay_off)
{
	struct pca_common_data *data = dev->data;
	const struct pca_common_config *config = dev->config;
	struct led_data *dev_data = &data->dev_data;
	uint8_t gdc, gfrq;
	uint32_t period;

	period = delay_on + delay_off;

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
	struct pca_common_data *data = dev->data;
	struct led_data *dev_data = &data->dev_data;
	uint8_t val;

	if (value < dev_data->min_brightness ||
	    value > dev_data->max_brightness) {
		return -EINVAL;
	}

	/* Set the LED brightness value */
	val = (value * 255U) / dev_data->max_brightness;
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
	struct pca_common_data *data = dev->data;
	struct led_data *dev_data = &data->dev_data;

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
	/* Hardware specific limits */
	dev_data->min_period = 41U;
	dev_data->max_period = 10667U;
	dev_data->min_brightness = 0U;
	dev_data->max_brightness = 100U;

	return 0;
}

static const struct led_driver_api pca_common_led_api = {
	.blink = pca_common_led_blink,
	.set_brightness = pca_common_led_set_brightness,
	.on = pca_common_led_on,
	.off = pca_common_led_off,
};

#define PCA_DEVICE(node_id, prefix, regs)							   \
	static const struct pca_common_config _CONCAT(prefix ## _config_, DT_DEP_ORD(node_id)) = { \
		.i2c = I2C_DT_SPEC_GET(node_id),						   \
		.pwm_base = regs ## _PWM_BASE,							   \
		.grppwm = regs ## _GRPPWM,							   \
		.grpfreq = regs ## _GRPFREQ,							   \
		.ledout = regs ## _LEDOUT,							   \
	};											   \
	static struct pca_common_data _CONCAT(prefix ## _data_, DT_DEP_ORD(node_id));		   \
												   \
	DEVICE_DT_DEFINE(node_id, &pca_common_led_init, NULL,					   \
			&_CONCAT(prefix ## _data_, DT_DEP_ORD(node_id)),			   \
			&_CONCAT(prefix ## _config_, DT_DEP_ORD(node_id)),			   \
			POST_KERNEL, CONFIG_LED_INIT_PRIORITY,					   \
			&pca_common_led_api);

DT_FOREACH_STATUS_OKAY_VARGS(nxp_pca9633, PCA_DEVICE, pca9633, PCA9633)
