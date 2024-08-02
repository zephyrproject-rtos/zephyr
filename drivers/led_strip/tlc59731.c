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
 * The EasySet protocol is based on short pulses and the time between
 * them. At least one pulse must be sent every T_CYCLE, which can be
 * between 1.67us and 50us. We want to go as fast as possible, but
 * delays under 1us don't work very well, so we settle on 5us for the
 * cycle time.
 * A pulse must be high for at least 14ns. In practice, turning a GPIO on
 * and immediately off again already takes longer than that, so no delay
 * is needed there.
 * A zero is represented by no additional pulses within a cycle.
 * A one is represented by an additional pulse between 275ns and 2.5us
 * (half a cycle) after the first one. We need at least some delay to get to
 * 275ns, but because of the limited granularity of k_busy_wait we use a
 * full 1us. After the pulse, we wait an additional T_CYCLE_1 to complete
 * the cycle. This time can be slightly shorter because the second pulse
 * already closes the cycle.
 * Finally we need to keep the line low for T_H0 to complete the address
 * for a single chip, and T_H1 to complete the write for all chips.
 */

#include <zephyr/drivers/led_strip.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(tlc59731, CONFIG_LED_STRIP_LOG_LEVEL);

/* Pulse timing */
#define TLC59731_DELAY     0x01 /* us */
#define TLC59731_T_CYCLE_0 0x04 /* us */
#define TLC59731_T_CYCLE_1 0x01 /* us */
#define TLC59731_T_H0      (4 * TLC59731_T_CYCLE_0)
#define TLC59731_T_H1      (8 * TLC59731_T_CYCLE_0)
/* Threshould levels */
#define TLC59731_HIGH      0x01
#define TLC59731_LOW       0x00

/* Write command */
#define TLC59731_WR 0x3A

struct tlc59731_cfg {
	struct gpio_dt_spec sdi_gpio;
	size_t length;
};

static inline int rgb_pulse(const struct gpio_dt_spec *led_dev)
{
	int fret = 0;

	fret = gpio_pin_set_dt(led_dev, TLC59731_HIGH);
	if (fret != 0) {
		return fret;
	}

	fret = gpio_pin_set_dt(led_dev, TLC59731_LOW);
	if (fret != 0) {
		return fret;
	}

	return fret;
}

static int rgb_write_bit(const struct gpio_dt_spec *led_dev, uint8_t data)
{
	rgb_pulse(led_dev);

	k_busy_wait(TLC59731_DELAY);

	if (data) {
		rgb_pulse(led_dev);
		k_busy_wait(TLC59731_T_CYCLE_1);
	} else {
		k_busy_wait(TLC59731_T_CYCLE_0);
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

static int tlc59731_led_set_color(const struct device *dev, struct led_rgb *pixel)
{

	const struct tlc59731_cfg *tlc_conf = dev->config;
	const struct gpio_dt_spec *led_gpio = &tlc_conf->sdi_gpio;

	rgb_write_data(led_gpio, TLC59731_WR);
	rgb_write_data(led_gpio, pixel->r);
	rgb_write_data(led_gpio, pixel->g);
	rgb_write_data(led_gpio, pixel->b);

	return 0;
}

static int tlc59731_gpio_update_rgb(const struct device *dev, struct led_rgb *pixels,
				    size_t num_pixels)
{
	size_t i;
	int err = 0;

	for (i = 0; i < num_pixels; i++) {
		err = tlc59731_led_set_color(dev, &pixels[i]);
		if (err) {
			break;
		}
	}

	return err;
}

static size_t tlc59731_length(const struct device *dev)
{
	const struct tlc59731_cfg *config = dev->config;

	return config->length;
}

static const struct led_strip_driver_api tlc59731_gpio_api = {
	.update_rgb = tlc59731_gpio_update_rgb,
	.length = tlc59731_length,
};

static int tlc59731_gpio_init(const struct device *dev)
{
	const struct tlc59731_cfg *tlc_conf = dev->config;
	const struct gpio_dt_spec *led = &tlc_conf->sdi_gpio;
	int err = 0;

	if (!device_is_ready(led->port)) {
		LOG_ERR("%s: no LEDs found (DT child nodes missing)", dev->name);
		err = -ENODEV;
		goto scape;
	}

	err = gpio_pin_configure_dt(led, GPIO_OUTPUT_ACTIVE);
	if (err < 0) {
		LOG_ERR("%s: Unable to setup SDI port", dev->name);
		err = -EIO;
		goto scape;
	}

	err = gpio_pin_set_dt(led, TLC59731_LOW);
	if (err < 0) {
		LOG_ERR("%s: Unable to set the SDI-GPIO)", dev->name);
		err = -EIO;
		goto scape;
	}

	gpio_pin_set_dt(led, TLC59731_HIGH);
	gpio_pin_set_dt(led, TLC59731_LOW);

	k_busy_wait((TLC59731_DELAY + TLC59731_T_CYCLE_0));
scape:
	return err;
}

#define TLC59731_DEVICE(i)                                                                         \
	static struct tlc59731_cfg tlc59731_cfg_##i = {                                            \
		.sdi_gpio = GPIO_DT_SPEC_INST_GET(i, gpios),                                       \
		.length = DT_INST_PROP(i, chain_length),                                           \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(i, tlc59731_gpio_init, NULL, NULL, &tlc59731_cfg_##i, POST_KERNEL,   \
			      CONFIG_LED_STRIP_INIT_PRIORITY, &tlc59731_gpio_api);
DT_INST_FOREACH_STATUS_OKAY(TLC59731_DEVICE)
