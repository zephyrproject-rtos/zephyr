/*
 * Copyright (c) 2018 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _ZEPHYR_LED_H
#define _ZEPHYR_LED_H

/**
 * @file
 * @brief Public LED driver APIs
 */

#include <zephyr/types.h>
#include <device.h>

/**
 * @typedef led_api_blink()
 * @brief Callback API for blinking an LED
 *
 * @see led_blink() for argument descriptions.
 */
typedef int (*led_api_blink)(struct device *dev, u32_t led,
			     u32_t delay_on, u32_t delay_off);

/**
 * @typedef led_api_set_brightness()
 * @brief Callback API for setting brightness of an LED
 *
 * @see led_set_brightness() for argument descriptions.
 */
typedef int (*led_api_set_brightness)(struct device *dev, u32_t led,
				      u8_t value);

/**
 * @typedef led_api_set_color()
 * @brief Callback API for setting color of an LED
 *
 * @see led_set_color() for argument descriptions.
 */
typedef int (*led_api_set_color)(struct device *dev, u8_t r, u8_t g, u8_t b);

/**
 * @typedef led_api_fade_brightness()
 * @brief Callback API for fading brightness of an LED
 *
 * @see led_fade_brightness() for argument descriptions.
 */
typedef int (*led_api_fade_brightness)(struct device *dev, u32_t led,
				       u8_t start, u8_t stop, u32_t fade_time);

/**
 * @typedef led_api_on()
 * @brief Callback API for turning on an LED
 *
 * @see led_on() for argument descriptions.
 */
typedef int (*led_api_on)(struct device *dev, u32_t led);

/**
 * @typedef led_api_off()
 * @brief Callback API for turning off an LED
 *
 * @see led_off() for argument descriptions.
 */
typedef int (*led_api_off)(struct device *dev, u32_t led);

/**
 * @brief LED driver API
 *
 * This is the mandatory API any LED driver needs to expose.
 */
struct led_driver_api {
	led_api_blink blink;
	led_api_set_brightness set_brightness;
	led_api_set_color set_color;
	led_api_fade_brightness fade_brightness;
	led_api_on on;
	led_api_off off;
};

/**
 * @brief Blink an LED
 *
 * This routine starts blinking an LED forever with the given time period
 *
 * @param dev LED device
 * @param led LED channel/pin
 * @param delay_on Time period (in milliseconds) an LED should be ON
 * @param delay_off Time period (in milliseconds) an LED should be OFF
 * @return 0 on success, negative on error
 */
__syscall int led_blink(struct device *dev, u32_t led,
			    u32_t delay_on, u32_t delay_off);

static inline int _impl_led_blink(struct device *dev, u32_t led,
			    u32_t delay_on, u32_t delay_off)
{
	const struct led_driver_api *api = dev->driver_api;

	return api->blink(dev, led, delay_on, delay_off);
}

/**
 * @brief Set LED brightness
 *
 * This routine sets the brightness of a LED to the given value.
 * Calling this function after led_blink() won't affect blinking.
 *
 * @param dev LED device
 * @param led LED channel/pin
 * @param value Brightness value to set in percent
 * @return 0 on success, negative on error
 */
__syscall int led_set_brightness(struct device *dev, u32_t led,
				     u8_t value);

static inline int _impl_led_set_brightness(struct device *dev, u32_t led,
				     u8_t value)
{
	const struct led_driver_api *api = dev->driver_api;

	return api->set_brightness(dev, led, value);
}

/**
 * @brief Set LED color
 *
 * This routine sets the color of a RGB LED to the given values.
 *
 * @param dev LED device
 * @param r Red value to be set between 0 and 255
 * @param g Green value to be set between 0 and 255
 * @param b Blue value to be set between 0 and 255
 * @return 0 on success, negative on error
 */
__syscall int led_set_color(struct device *dev, u8_t r, u8_t g, u8_t b);

static inline int _impl_led_set_color(struct device *dev,
				     u8_t r, u8_t g, u8_t b)
{
	const struct led_driver_api *api = dev->driver_api;

	return api->set_color(dev, r, g, b);
}

/**
 * @brief Fade brightness of a LED over given time frame
 *
 * This routine fades the brightness of a LED from the start to stop brightness
 * over the given time.
 *
 * @param dev LED device
 * @param led LED channel/pin
 * @param start Start brightness in percent
 * @param stop Target brightness in percent
 * @param fade_time Time in milliseconds until fading is completed.
 * @return 0 on success, negative on error
 */
__syscall int led_fade_brightness(struct device *dev, u32_t led,
				     u8_t start, u8_t stop, u32_t fade_time);

static inline int _impl_led_fade_brightness(struct device *dev, u32_t led,
				     u8_t start, u8_t stop, u32_t fade_time)
{
	const struct led_driver_api *api = dev->driver_api;

	return api->fade_brightness(dev, led, start, stop, fade_time);
}

/**
 * @brief Turn on an LED
 *
 * This routine turns on an LED
 *
 * @param dev LED device
 * @param led LED channel/pin
 * @return 0 on success, negative on error
 */
__syscall int led_on(struct device *dev, u32_t led);

static inline int _impl_led_on(struct device *dev, u32_t led)
{
	const struct led_driver_api *api = dev->driver_api;

	return api->on(dev, led);
}

/**
 * @brief Turn off an LED
 *
 * This routine turns off an LED
 *
 * @param dev LED device
 * @param led LED channel/pin
 * @return 0 on success, negative on error
 */
__syscall int led_off(struct device *dev, u32_t led);

static inline int _impl_led_off(struct device *dev, u32_t led)
{
	const struct led_driver_api *api = dev->driver_api;

	return api->off(dev, led);
}

#include <syscalls/led.h>

#endif	/* _ZEPHYR_LED_H */
