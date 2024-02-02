/*
 * Copyright (c) 2024 Javad Rahimipetroudi <javad.rahimipetroudi@mind.be>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_tlc59731

/**
 * @file
 * @brief LED driver for the TLC59731 LED driver.
 *
 * TLC59731 is a 3-Channel, 8-Bit, PWM LED Driver
 * With Single-Wire Interface (EasySet)
 *
 */

#include <zephyr/drivers/led.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(tlc59731, CONFIG_LED_STRIP_LOG_LEVEL);

#include "../led/led_context.h"

/* Pulse timing */
#define DELAY     0x01 /* us */
#define T_CYCLE_0 0x04 /* us */
#define T_CYCLE_1 0x01 /* us */

/* Threshould levels */
#define HIGH 0x01
#define LOW  0x00

/* RGB LED constants */
#define RED      0x00
#define GREEN    0x01
#define BLUE     0x02
#define ALL_LEDS 0x03

#define NUM_COLORS 0x03

/* Write command */
#define WR 0x3A

struct tlc59731_cfg {
	struct gpio_dt_spec sdi_gpio;
	struct led_data led_data;
};

struct tlc59731_data {
	uint8_t r;
	uint8_t g;
	uint8_t b;
};

/*****************************/
static inline int rgb_pulse(const struct gpio_dt_spec *led_dev)
{
	int32_t fret = 0;

	fret = gpio_pin_set_dt(led_dev, HIGH);
	if (fret != 0) {
		return fret;
	}

	fret = gpio_pin_set_dt(led_dev, LOW);
	if (fret != 0) {
		return fret;
	}

	return fret;
}

static int rgb_eos(void)
{
	k_busy_wait(4 * (DELAY + T_CYCLE_0));
	return 0;
}

static int rgb_gslatch(void)
{
	k_busy_wait(8 * (DELAY + T_CYCLE_0));

	return 0;
}

static int rgb_write_bit(const struct gpio_dt_spec *led_dev, uint8_t data)
{
	rgb_pulse(led_dev);

	k_busy_wait(DELAY);

	if (data) {
		rgb_pulse(led_dev);
		k_busy_wait(T_CYCLE_1);
	} else {
		k_busy_wait(T_CYCLE_0);
	}

	return 0;
}

static int rgb_write_data(const struct gpio_dt_spec *led_dev, uint8_t data)
{
	int8_t idx = 7;

	while (idx >= 0) {
		rgb_write_bit(led_dev, data & BIT((idx--)));
	}

	return 0;
}

static int rgb_write_led(const struct gpio_dt_spec *led_dev, struct tlc59731_data *rgb_data)
{
	rgb_write_data(led_dev, WR);
	rgb_write_data(led_dev, rgb_data->r);
	rgb_write_data(led_dev, rgb_data->g);
	rgb_write_data(led_dev, rgb_data->b);

	rgb_eos();
	rgb_gslatch();
	return 0;
}

static inline int rgb_fill_latch(struct tlc59731_data *rgb_lt, const uint8_t led,
				 const uint8_t value)
{
	int32_t err = 0;

	switch (led) {
	case RED:
		rgb_lt->r = value;
		break;

	case GREEN:
		rgb_lt->g = value;
		break;

	case BLUE:
		rgb_lt->b = value;
		break;

	case ALL_LEDS:
		rgb_lt->r = value;
		rgb_lt->g = value;
		rgb_lt->b = value;
		break;

	default:
		LOG_ERR("Illegal RGB number/value");
		err = -EINVAL;
		break;
	}

	return err;
}

static int tlc59731_latch_and_write(const struct device *dev, const uint8_t led,
				    const uint16_t nval)
{
	int32_t err = 0;
	const struct tlc59731_cfg *tlc_conf = dev->config;
	const struct gpio_dt_spec *led_gpio = &tlc_conf->sdi_gpio;
	struct tlc59731_data *rgb_latch = dev->data;

	/* First change latch data */
	err = rgb_fill_latch(rgb_latch, led, nval);
	if (err) {
		LOG_ERR("%s: GPIO latch error", dev->name);
		goto scape;
	}
	/* Then let it go */
	err = rgb_write_led(led_gpio, rgb_latch);
	if (err) {
		LOG_ERR("%s: GPIO write error", dev->name);
	}
scape:
	return err;
}

/**********************************/

static int tlc59731_led_set_brightness(const struct device *dev, const uint32_t led,
				       const uint8_t value)
{
	int32_t err = 0;
	uint8_t real_val;
	const struct tlc59731_cfg *tlc_conf = dev->config;
	const struct led_data *led_data = &tlc_conf->led_data;

	if ((value < 0) || (value > 100)) {
		LOG_ERR("%s: Illegal brighness value %d. It should be in [0-100] range", dev->name,
			err);
		err = -EINVAL;
		goto scape;
	}

	real_val = (value * (led_data->max_brightness - led_data->min_brightness)) / 100;

	err = tlc59731_latch_and_write(dev, ALL_LEDS, real_val);
	if (err) {
		LOG_ERR("%s: RGB write error: %d", dev->name, err);
	}

scape:
	return err;
}

static inline int tlc59731_led_on(const struct device *dev, uint32_t led)
{
	int32_t err = 0;
	const struct tlc59731_cfg *tlc_conf = dev->config;
	const struct led_data *led_data = &tlc_conf->led_data;

	err = tlc59731_latch_and_write(dev, led, led_data->max_brightness);
	if (err) {
		LOG_ERR("%s: RGB write error: %d", dev->name, err);
	}

	return err;
}

static inline int tlc59731_led_off(const struct device *dev, uint32_t led)
{
	int32_t err = 0;
	const struct tlc59731_cfg *tlc_conf = dev->config;
	const struct led_data *led_data = &tlc_conf->led_data;

	err = tlc59731_latch_and_write(dev, led, led_data->min_brightness);
	if (err) {
		LOG_ERR("%s: RGB write error: %d", dev->name, err);
	}

	return err;
}

static int tlc59731_led_set_color(const struct device *dev, uint32_t led, uint8_t num_colors,
				  const uint8_t *color)
{
	int32_t err = 0;

	if (num_colors != NUM_COLORS) {
		err = -EINVAL;
		LOG_ERR("%s: Invalid number of color arguments: %d != 3", dev->name, num_colors);
		goto scape;
	}

	if (color == NULL) {
		err = -EINVAL;
		LOG_ERR("%s: Invalid color arguments", dev->name);
		goto scape;
	}

	err = tlc59731_latch_and_write(dev, RED, color[RED]);
	if (err) {
		LOG_ERR("%s: RGB write error: %d", dev->name, err);
		goto scape;
	}

	err = tlc59731_latch_and_write(dev, GREEN, color[GREEN]);
	if (err) {
		LOG_ERR("%s: RGB write error: %d", dev->name, err);
		goto scape;
	}

	err = tlc59731_latch_and_write(dev, BLUE, color[BLUE]);
	if (err) {
		LOG_ERR("%s: RGB write error: %d", dev->name, err);
	}

scape:
	return err;
}
static int tlc59731_led_init(const struct device *dev)
{
	const struct tlc59731_cfg *tlc_conf = dev->config;
	const struct gpio_dt_spec *led = &tlc_conf->sdi_gpio;
	const struct led_data *led_data = &tlc_conf->led_data;
	int32_t err = 0;

	if (!device_is_ready(led->port)) {
		LOG_ERR("%s: no LEDs found (DT child nodes missing)", dev->name);
		err = -ENODEV;
	}
	LOG_DBG("%s: Min/Max Brightness: %d/%d. Min/Max Period: %d/%d", dev->name,
		led_data->min_brightness, led_data->max_brightness, led_data->min_period,
		led_data->max_period);

	err = gpio_pin_configure_dt(led, GPIO_OUTPUT_ACTIVE);
	if (err < 0) {
		LOG_ERR("%s: Unable to setup SDI port", dev->name);

		err = -EIO;
	}

	err = gpio_pin_set_dt(led, LOW);
	if (err < 0) {
		LOG_ERR("%s: Unable to set the SDI-GPIO)", dev->name);
		err = -EIO;
	}
	gpio_pin_set_dt(led, HIGH);
	gpio_pin_set_dt(led, LOW);

	k_busy_wait((DELAY + T_CYCLE_0));

	return err;
}

static const struct led_driver_api tlc59731_led_api = {
	.set_brightness = tlc59731_led_set_brightness,
	.set_color = tlc59731_led_set_color,
	.on = tlc59731_led_on,
	.off = tlc59731_led_off,
};

#define TLC59731_DEVICE(i)                                                                         \
	static struct tlc59731_cfg tlc59731_cfg_##i = {                                            \
		.sdi_gpio = GPIO_DT_SPEC_INST_GET(i, gpios),                                       \
		.led_data.min_period = DT_INST_PROP_OR(i, min_period, 0x00),                       \
		.led_data.max_period = DT_INST_PROP_OR(i, max_period, 0x00),                       \
		.led_data.min_brightness = DT_INST_PROP_OR(i, min_brightness, 0x00),               \
		.led_data.max_brightness = DT_INST_PROP_OR(i, max_brightness, 0xFF),               \
	};                                                                                         \
	struct tlc59731_data data_##i;                                                             \
	DEVICE_DT_INST_DEFINE(i, &tlc59731_led_init, NULL, &data_##i, &tlc59731_cfg_##i,           \
			      POST_KERNEL, CONFIG_LED_INIT_PRIORITY, &tlc59731_led_api);

DT_INST_FOREACH_STATUS_OKAY(TLC59731_DEVICE)
