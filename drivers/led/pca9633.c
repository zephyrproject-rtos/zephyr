/*
 * Copyright (c) 2018 Savoir-Faire Linux.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_pca9633

/**
 * @file
 * @brief LED driver for the PCA9633 I2C LED driver (7-bit slave address 0x62)
 */

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/led.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>

#define LOG_LEVEL CONFIG_LED_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(pca9633);

#include "led_context.h"

/* PCA9633 select registers determine the source that drives LED outputs */
#define PCA9633_LED_OFF         0x0     /* LED driver off */
#define PCA9633_LED_ON          0x1     /* LED driver on */
#define PCA9633_LED_PWM         0x2     /* Controlled through PWM */
#define PCA9633_LED_GRP_PWM     0x3     /* Controlled through PWM/GRPPWM */

/* PCA9633 control register */
#define PCA9633_MODE1           0x00
#define PCA9633_MODE2           0x01
#define PCA9633_PWM_BASE        0x02	/* Reg 0x02-0x05 for brightness control LED01-04 */
#define PCA9633_GRPPWM          0x06
#define PCA9633_GRPFREQ         0x07
#define PCA9633_LEDOUT          0x08

/* PCA9633 mode register 1 */
#define PCA9633_MODE1_ALLCAL    0x01    /* All Call Address enabled */
#define PCA9633_MODE1_SLEEP     0x10    /* Sleep Mode */
/* PCA9633 mode register 2 */
#define PCA9633_MODE2_DMBLNK    0x20    /* Enable blinking */

#define PCA9633_MASK            0x03

struct pca9633_config {
	struct i2c_dt_spec i2c;
	bool disable_allcall;
};

struct pca9633_data {
	struct led_data dev_data;
};

static int pca9633_led_blink(const struct device *dev, uint32_t led,
			     uint32_t delay_on, uint32_t delay_off)
{
	struct pca9633_data *data = dev->data;
	const struct pca9633_config *config = dev->config;
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
			       PCA9633_GRPPWM,
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
			       PCA9633_GRPFREQ,
			       gfrq)) {
		LOG_ERR("LED reg write failed");
		return -EIO;
	}

	/* Enable blinking mode */
	if (i2c_reg_update_byte_dt(&config->i2c,
				PCA9633_MODE2,
				PCA9633_MODE2_DMBLNK,
				PCA9633_MODE2_DMBLNK)) {
		LOG_ERR("LED reg update failed");
		return -EIO;
	}

	/* Select the GRPPWM source to drive the LED output */
	if (i2c_reg_update_byte_dt(&config->i2c,
				PCA9633_LEDOUT,
				PCA9633_MASK << (led << 1),
				PCA9633_LED_GRP_PWM << (led << 1))) {
		LOG_ERR("LED reg update failed");
		return -EIO;
	}

	return 0;
}

static int pca9633_led_set_brightness(const struct device *dev, uint32_t led,
				      uint8_t value)
{
	const struct pca9633_config *config = dev->config;
	struct pca9633_data *data = dev->data;
	struct led_data *dev_data = &data->dev_data;
	uint8_t val;

	if (value < dev_data->min_brightness ||
	    value > dev_data->max_brightness) {
		return -EINVAL;
	}

	/* Set the LED brightness value */
	val = (value * 255U) / dev_data->max_brightness;
	if (i2c_reg_write_byte_dt(&config->i2c,
			       PCA9633_PWM_BASE + led,
			       val)) {
		LOG_ERR("LED reg write failed");
		return -EIO;
	}

	/* Set the LED driver to be controlled through its PWMx register. */
	if (i2c_reg_update_byte_dt(&config->i2c,
				PCA9633_LEDOUT,
				PCA9633_MASK << (led << 1),
				PCA9633_LED_PWM << (led << 1))) {
		LOG_ERR("LED reg update failed");
		return -EIO;
	}

	return 0;
}

static inline int pca9633_led_on(const struct device *dev, uint32_t led)
{
	const struct pca9633_config *config = dev->config;

	/* Set LED state to ON */
	if (i2c_reg_update_byte_dt(&config->i2c,
				PCA9633_LEDOUT,
				PCA9633_MASK << (led << 1),
				PCA9633_LED_ON << (led << 1))) {
		LOG_ERR("LED reg update failed");
		return -EIO;
	}

	return 0;
}

static inline int pca9633_led_off(const struct device *dev, uint32_t led)
{
	const struct pca9633_config *config = dev->config;

	/* Set LED state to OFF */
	if (i2c_reg_update_byte_dt(&config->i2c,
				PCA9633_LEDOUT,
				PCA9633_MASK << (led << 1),
				PCA9633_LED_OFF)) {
		LOG_ERR("LED reg update failed");
		return -EIO;
	}

	return 0;
}

static int pca9633_led_init(const struct device *dev)
{
	const struct pca9633_config *config = dev->config;
	struct pca9633_data *data = dev->data;
	struct led_data *dev_data = &data->dev_data;

	if (!device_is_ready(config->i2c.bus)) {
		LOG_ERR("I2C bus is not ready");
		return -ENODEV;
	}

	/*
	 * Take the LED driver out from Sleep mode and disable All Call Address
	 * if specified in DT.
	 */
	if (i2c_reg_update_byte_dt(
		    &config->i2c, PCA9633_MODE1,
		    config->disable_allcall ? PCA9633_MODE1_SLEEP | PCA9633_MODE1_ALLCAL
					    : PCA9633_MODE1_SLEEP,
		    config->disable_allcall ? ~(PCA9633_MODE1_SLEEP | PCA9633_MODE1_ALLCAL)
					    : ~PCA9633_MODE1_SLEEP)) {
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

static DEVICE_API(led, pca9633_led_api) = {
	.blink = pca9633_led_blink,
	.set_brightness = pca9633_led_set_brightness,
	.on = pca9633_led_on,
	.off = pca9633_led_off,
};

#define PCA9633_DEVICE(id)						\
	static const struct pca9633_config pca9633_##id##_cfg = {	\
		.i2c = I2C_DT_SPEC_INST_GET(id),			\
		.disable_allcall = DT_INST_PROP(id, disable_allcall),	\
	};								\
	static struct pca9633_data pca9633_##id##_data;			\
									\
	DEVICE_DT_INST_DEFINE(id, &pca9633_led_init, NULL,		\
			&pca9633_##id##_data,				\
			&pca9633_##id##_cfg, POST_KERNEL,		\
			CONFIG_LED_INIT_PRIORITY,			\
			&pca9633_led_api);

DT_INST_FOREACH_STATUS_OKAY(PCA9633_DEVICE)
