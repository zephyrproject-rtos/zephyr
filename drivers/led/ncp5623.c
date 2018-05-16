/*
 * Copyright (c) 2018 Workaround GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief NCP5623 LED driver
 *
 * Limitations:
 * - Blinking is not supported.
 * - The driver currently assumes following mapping
 *	LED 1: Green LED
 *	LED 2: Blue LED
 *	LED 3: Red LED
 *   This has to be made configurable in a second step.
 * - The driver ignores the led channel argument currently. It behaves as a
 *   single instance controlling the three LEDs based on the RGB and brightness
 *   values.
 */

#include <zephyr.h>
#include <init.h>
#include <device.h>
#include <i2c.h>
#include <led.h>

#define SYS_LOG_DOMAIN "ncp5623_led"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_LED_LEVEL
#include <logging/sys_log.h>

#include "led_context.h"

#define NCP5623C_I2C_ADDR 0x39
#define NCP5623D_I2C_ADDR 0x38

/*
 * The NCP5623 driver only supports the range 0x00..0x1F for values
 */
#define NCP5623_MAX_ARG_SIZE 0x1F

/* NCP5623 Functions */
#define NCP5623_SHUTDOWN      (0 << 5)
#define NCP5623_CURRENT_CTL   (1 << 5)
#define NCP5623_GREEN_CTL     (2 << 5)
#define NCP5623_BLUE_CTL      (3 << 5)
#define NCP5623_RED_CTL       (4 << 5)
#define NCP5623_STEPS_UP      (5 << 5)
#define NCP5623_STEPS_DOWN    (6 << 5)
#define NCP5623_STEP_TIME_RUN (7 << 5)

/* Fading is done in steps with a duration of a multiple of 8ms */
#define NCP5623_TIME_STEPS_MS 8

/* The chip needs 150 microseconds between two commands for proper operation */
#define NCP5623_COMMAND_PROCESSING_TIME_US 150

struct ncp5623_config {
	u8_t addr;
	char *i2c_dev_name;
};

struct ncp5623_data {
	struct device *i2c;
	struct led_data dev_data;
};

/*
 * @brief correct the 8 bit api values to the 5 bit values this device
 * supports.
 *
 * @param byte The 8 bit value to correct.
 *
 * @return a uint8_t holding a 5 bit value [0x00 .. NCP_MAX_ARG_SIZE].
 *
 * @note Bitshifting the value three positions to the right is corresponds to
 * floor((byte/0xFF)*0x1F) and rescales therefore the API range (0x00 to 0xFF)
 * to the range supported by the driver (0x00 to 0x1F) with minimal rounding
 * errors.
 */
static inline u8_t ncp5623_convert(u8_t byte)
{
	return (u8_t)(byte >> 3);
}

/*
 * @brief Write a key value pair to the NCP5623 chip.
 *
 * @param dev LED device.
 * @param key Register key.
 * @param val Value to write.
 *
 * @return Error code.
 *
 * @retval 0 If successful.
 * @retval -ERRNO on failure.
 */
static int ncp5623_write(struct device *dev, u8_t key, u8_t val)
{
	int ret;
	struct ncp5623_data *data = dev->driver_data;
	const struct ncp5623_config *cfg = dev->config->config_info;

	u8_t buf = key | val;

	ret = i2c_write(data->i2c, &buf, (u32_t)sizeof(buf), cfg->addr);
	k_busy_wait(NCP5623_COMMAND_PROCESSING_TIME_US);
	return ret;
}

static int ncp5623_led_blink(struct device *dev, u32_t led,
			     u32_t delay_on, u32_t delay_off)
{
	return -ENOTSUP;
}

static int ncp5623_led_set_brightness(struct device *dev, u32_t led,
				      u8_t value)
{
	struct ncp5623_data *data = dev->driver_data;
	struct led_data *dev_data = &data->dev_data;
	int ret;

	ARG_UNUSED(led);

	if (value < dev_data->min_brightness ||
			value > dev_data->max_brightness) {
		SYS_LOG_WRN("Brightness 0x%02X is not in the allowed range.",
				value);
		return -EINVAL;
	}

	value = (value * NCP5623_MAX_ARG_SIZE) / dev_data->max_brightness;

	ret = ncp5623_write(dev, NCP5623_CURRENT_CTL, value);
	if (ret) {
		SYS_LOG_ERR("Failed to set brightness 0x%02X with error %d",
				value, ret);
	}

	return ret;
}

static int ncp5623_led_set_color(struct device *dev, u8_t r, u8_t g, u8_t b)
{
	int ret;

	r = ncp5623_convert(r);
	g = ncp5623_convert(g);
	b = ncp5623_convert(b);

	ret = ncp5623_write(dev, NCP5623_RED_CTL, r);
	if (ret) {
		SYS_LOG_ERR("Failed to set red value 0x%02X with error %d",
				r, ret);
		return ret;
	}

	ret = ncp5623_write(dev, NCP5623_GREEN_CTL, g);
	if (ret) {
		SYS_LOG_ERR("Failed to set green value 0x%02X with error %d",
				g, ret);
		return ret;
	}

	ret = ncp5623_write(dev, NCP5623_BLUE_CTL, b);
	if (ret) {
		SYS_LOG_ERR("Failed to set blue value 0x%02X with error %d",
				b, ret);
	}

	return ret;
}

static int ncp5623_led_fade_brightness(struct device *dev, u32_t led,
				       u8_t start, u8_t stop, u32_t fade_time)
{
	struct ncp5623_data *data = dev->driver_data;
	struct led_data *dev_data = &data->dev_data;
	int ret;
	u8_t fade_dir_reg, fade_steps, time_per_step;

	ARG_UNUSED(led);

	ret = ncp5623_led_set_brightness(dev, led, start);
	if (ret) {
		SYS_LOG_ERR("Setting start brightness failed with %d", ret);
		return ret;
	}

	start = (start * NCP5623_MAX_ARG_SIZE) / dev_data->max_brightness;
	stop = (stop * NCP5623_MAX_ARG_SIZE) / dev_data->max_brightness;

	if (stop > start) {
		fade_dir_reg = NCP5623_STEPS_UP;
		fade_steps = stop - start;
	} else if (stop < start) {
		fade_dir_reg = NCP5623_STEPS_DOWN;
		fade_steps = start - stop;
	} else {
		SYS_LOG_INF("Start and Stop brightness are equal.");
		return -EINVAL;
	}

	time_per_step =
		(uint8_t)(fade_time / (fade_steps * NCP5623_TIME_STEPS_MS));

	/*
	 * 0x00 and >0x1E doesn't have effect on the device.
	 * As per specification (NCP5623 page 7), 0x1F is a valid value but
	 * observation did not show expected result.
	 */
	if (time_per_step == 0x00) {
		time_per_step = 0x01;
	} else if (time_per_step >= 0x1F) {
		time_per_step = 0x1E;
	}

	ret = ncp5623_write(dev, fade_dir_reg, stop);
	if (ret) {
		SYS_LOG_ERR("Failed to set target brightness 0x%02X with %d",
				stop, ret);
		return ret;
	}

	ret = ncp5623_write(dev, NCP5623_STEP_TIME_RUN, time_per_step);
	if (ret) {
		SYS_LOG_ERR("Failed to set time per step 0x%02X with %d",
				time_per_step, ret);
		return ret;
	}

	return ret;
}

static int ncp5623_led_on(struct device *dev, u32_t led)
{
	/*
	 * NCP5623 doesn't provide a specific functionality to turn the device
	 * on. This is simply done by increasing the brightness.
	 */
	ARG_UNUSED(dev);
	ARG_UNUSED(led);

	return -ENOTSUP;
}

static int ncp5623_led_off(struct device *dev, u32_t led)
{
	ARG_UNUSED(led);

	if (ncp5623_write(dev, NCP5623_SHUTDOWN, 0)) {
		SYS_LOG_ERR("Failed to turn off LED");
		return -EIO;
	}

	return 0;
}

static int ncp5623_led_init(struct device *dev)
{
	struct ncp5623_data *data = dev->driver_data;
	struct led_data *dev_data = &data->dev_data;
	const struct ncp5623_config *cfg = dev->config->config_info;

	data->i2c = device_get_binding(cfg->i2c_dev_name);
	if (data->i2c == NULL) {
		SYS_LOG_ERR("Failed to get I2C device");
		return -EINVAL;
	}

	dev_data->min_period = 0;
	dev_data->max_period = 0;
	dev_data->min_brightness = 0;
	dev_data->max_brightness = 100;

	SYS_LOG_DBG("NCP5623 LED driver init complete");

	return 0;
}

static const struct led_driver_api ncp5623_led_api = {
	.blink = ncp5623_led_blink,
	.set_brightness = ncp5623_led_set_brightness,
	.set_color = ncp5623_led_set_color,
	.fade_brightness = ncp5623_led_fade_brightness,
	.on = ncp5623_led_on,
	.off = ncp5623_led_off,
};

#if CONFIG_NCP5623C
static struct ncp5623_config ncp5623c_led_config = {
	.addr = NCP5623C_I2C_ADDR,
	.i2c_dev_name = CONFIG_NCP5623C_I2C_MASTER_DEV_NAME,
};

static struct ncp5623_data ncp5623c_led_data;

DEVICE_AND_API_INIT(ncp5623c_led, CONFIG_NCP5623C_DEV_NAME,
		    &ncp5623_led_init, &ncp5623c_led_data,
		    &ncp5623c_led_config, POST_KERNEL, CONFIG_LED_INIT_PRIORITY,
		    &ncp5623_led_api);

#endif /* CONFIG_NCP5623C */

#if CONFIG_NCP5623D
static struct ncp5623_config ncp5623d_led_config = {
	.addr = NCP5623D_I2C_ADDR,
	.i2c_dev_name = CONFIG_NCP5623D_I2C_MASTER_DEV_NAME,
};

static struct ncp5623_data ncp5623d_led_data;

DEVICE_AND_API_INIT(ncp5623d_led, CONFIG_NCP5623D_DEV_NAME,
		    &ncp5623_led_init, &ncp5623d_led_data,
		    &ncp5623d_led_config, POST_KERNEL, CONFIG_LED_INIT_PRIORITY,
		    &ncp5623_led_api);
#endif /* CONFIG_NCP5623D */
