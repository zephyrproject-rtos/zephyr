/*
 * Copyright (c) 2018 Savoir-Faire Linux.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief LED driver for the PCA9633 I2C LED driver (7-bit slave address 0x62)
 */

#include <i2c.h>
#include <led.h>
#include <misc/util.h>
#include <zephyr.h>

#define SYS_LOG_LEVEL CONFIG_SYS_LOG_LED_LEVEL
#include <logging/sys_log.h>

#include "led_context.h"

/* PCA9633 select registers determine the source that drives LED outputs */
#define PCA9633_LED_OFF         0x0     /* LED driver off */
#define PCA9633_LED_ON          0x1     /* LED driver on */
#define PCA9633_LED_PWM         0x2     /* Controlled through PWM */
#define PCA9633_LED_GRP_PWM     0x3     /* Controlled through PWM/GRPPWM */

/* PCA9633 control register */
#define PCA9633_MODE1           0x00
#define PCA9633_MODE2           0x01
#define PCA9633_PWM_BASE        0x02
#define PCA9633_GRPPWM          0x06
#define PCA9633_GRPFREQ         0x07
#define PCA9633_LEDOUT          0x08

/* PCA9633 mode register 2 */
#define PCA9633_MODE2_DMBLNK    0x20    /* Enable blinking */

#define PCA9633_MASK            0x03

struct pca9633_data {
	struct device *i2c;
	struct led_data dev_data;
};

static int pca9633_led_blink(struct device *dev, u32_t led,
			     u32_t delay_on, u32_t delay_off)
{
	struct pca9633_data *data = dev->driver_data;
	struct led_data *dev_data = &data->dev_data;
	u8_t gdc, gfrq;
	u32_t period;

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
	gdc = delay_on * 256 / period;
	if (i2c_reg_write_byte(data->i2c, CONFIG_PCA9633_I2C_ADDRESS,
			       PCA9633_GRPPWM,
			       gdc)) {
		SYS_LOG_ERR("LED reg write failed");
		return -EIO;
	}

	/*
	 * From manual:
	 * period = ((GFRQ + 1) / 24) in seconds.
	 * So, period (in ms) = (((GFRQ + 1) / 24) * 1000) ->
	 *		GFRQ = ((period * 24 / 1000) - 1)
	 */
	gfrq = (period * 24 / 1000) - 1;
	if (i2c_reg_write_byte(data->i2c, CONFIG_PCA9633_I2C_ADDRESS,
			       PCA9633_GRPFREQ,
			       gfrq)) {
		SYS_LOG_ERR("LED reg write failed");
		return -EIO;
	}

	/* Enable blinking mode */
	if (i2c_reg_update_byte(data->i2c, CONFIG_PCA9633_I2C_ADDRESS,
				PCA9633_MODE2,
				PCA9633_MODE2_DMBLNK,
				PCA9633_MODE2_DMBLNK)) {
		SYS_LOG_ERR("LED reg update failed");
		return -EIO;
	}

	/* Select the GRPPWM source to drive the LED outpout */
	if (i2c_reg_update_byte(data->i2c, CONFIG_PCA9633_I2C_ADDRESS,
				PCA9633_LEDOUT,
				PCA9633_MASK << (led << 1),
				PCA9633_LED_GRP_PWM << (led << 1))) {
		SYS_LOG_ERR("LED reg update failed");
		return -EIO;
	}

	return 0;
}

static int pca9633_led_set_brightness(struct device *dev, u32_t led,
				      u8_t value)
{
	struct pca9633_data *data = dev->driver_data;
	struct led_data *dev_data = &data->dev_data;
	u8_t val;

	if (value < dev_data->min_brightness ||
	    value > dev_data->max_brightness) {
		return -EINVAL;
	}

	/* Set the LED brightness value */
	val = (value * 255) / dev_data->max_brightness;
	if (i2c_reg_write_byte(data->i2c, CONFIG_PCA9633_I2C_ADDRESS,
			       PCA9633_PWM_BASE + led,
			       val)) {
		SYS_LOG_ERR("LED reg write failed");
		return -EIO;
	}

	/* Set the LED driver to be controlled through its PWMx register. */
	if (i2c_reg_update_byte(data->i2c, CONFIG_PCA9633_I2C_ADDRESS,
				PCA9633_LEDOUT,
				PCA9633_MASK << (led << 1),
				PCA9633_LED_PWM << (led << 1))) {
		SYS_LOG_ERR("LED reg update failed");
		return -EIO;
	}

	return 0;
}

static inline int pca9633_led_on(struct device *dev, u32_t led)
{
	struct pca9633_data *data = dev->driver_data;

	/* Set LED state to ON */
	if (i2c_reg_update_byte(data->i2c, CONFIG_PCA9633_I2C_ADDRESS,
				PCA9633_LEDOUT,
				PCA9633_MASK << (led << 1),
				PCA9633_LED_ON << (led << 1))) {
		SYS_LOG_ERR("LED reg update failed");
		return -EIO;
	}

	return 0;
}

static inline int pca9633_led_off(struct device *dev, u32_t led)
{
	struct pca9633_data *data = dev->driver_data;

	/* Set LED state to OFF */
	if (i2c_reg_update_byte(data->i2c, CONFIG_PCA9633_I2C_ADDRESS,
				PCA9633_LEDOUT,
				PCA9633_MASK << (led << 1),
				PCA9633_LED_OFF)) {
		SYS_LOG_ERR("LED reg update failed");
		return -EIO;
	}

	return 0;
}

static int pca9633_led_init(struct device *dev)
{
	struct pca9633_data *data = dev->driver_data;
	struct led_data *dev_data = &data->dev_data;

	data->i2c = device_get_binding(CONFIG_PCA9633_I2C_MASTER_DEV_NAME);
	if (data->i2c == NULL) {
		SYS_LOG_DBG("Failed to get I2C device");
		return -EINVAL;
	}

	/* Hardware specific limits */
	dev_data->min_period = 41;
	dev_data->max_period = 10667;
	dev_data->min_brightness = 0;
	dev_data->max_brightness = 100;

	return 0;
}

static struct pca9633_data pca9633_led_data;

static const struct led_driver_api pca9633_led_api = {
	.blink = pca9633_led_blink,
	.set_brightness = pca9633_led_set_brightness,
	.on = pca9633_led_on,
	.off = pca9633_led_off,
};

DEVICE_AND_API_INIT(pca9633_led, CONFIG_PCA9633_DEV_NAME,
		    &pca9633_led_init, &pca9633_led_data,
		    NULL, POST_KERNEL, CONFIG_LED_INIT_PRIORITY,
		    &pca9633_led_api);
