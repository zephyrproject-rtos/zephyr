/*
 * Copyright (c) 2018 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public LED driver APIs
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_LED_H_
#define ZEPHYR_INCLUDE_DRIVERS_LED_H_

/**
 * @brief LED Interface
 * @defgroup led_interface LED Interface
 * @ingroup io_interfaces
 * @{
 */

#include <zephyr/types.h>
#include <device.h>

/**
 * @typedef led_api_blink()
 * @brief Callback API for blinking an LED
 *
 * @see led_blink() for argument descriptions.
 */
typedef int (*led_api_blink)(struct device *dev, uint32_t led,
			     uint32_t delay_on, uint32_t delay_off);

/**
 * @typedef led_api_set_brightness()
 * @brief Callback API for setting brightness of an LED
 *
 * @see led_set_brightness() for argument descriptions.
 */
typedef int (*led_api_set_brightness)(struct device *dev, uint32_t led,
				      uint8_t value);
/**
 * @typedef led_api_on()
 * @brief Callback API for turning on an LED
 *
 * @see led_on() for argument descriptions.
 */
typedef int (*led_api_on)(struct device *dev, uint32_t led);

/**
 * @typedef led_api_off()
 * @brief Callback API for turning off an LED
 *
 * @see led_off() for argument descriptions.
 */
typedef int (*led_api_off)(struct device *dev, uint32_t led);

/**
 * @brief LED driver API
 *
 * This is the mandatory API any LED driver needs to expose.
 */
__subsystem struct led_driver_api {
	led_api_blink blink;
	led_api_set_brightness set_brightness;
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
__syscall int led_blink(struct device *dev, uint32_t led,
			    uint32_t delay_on, uint32_t delay_off);

static inline int z_impl_led_blink(struct device *dev, uint32_t led,
			    uint32_t delay_on, uint32_t delay_off)
{
	const struct led_driver_api *api =
		(const struct led_driver_api *)dev->api;

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
__syscall int led_set_brightness(struct device *dev, uint32_t led,
				     uint8_t value);

static inline int z_impl_led_set_brightness(struct device *dev, uint32_t led,
				     uint8_t value)
{
	const struct led_driver_api *api =
		(const struct led_driver_api *)dev->api;

	return api->set_brightness(dev, led, value);
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
__syscall int led_on(struct device *dev, uint32_t led);

static inline int z_impl_led_on(struct device *dev, uint32_t led)
{
	const struct led_driver_api *api =
		(const struct led_driver_api *)dev->api;

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
__syscall int led_off(struct device *dev, uint32_t led);

static inline int z_impl_led_off(struct device *dev, uint32_t led)
{
	const struct led_driver_api *api =
		(const struct led_driver_api *)dev->api;

	return api->off(dev, led);
}

/**
 * @}
 */

#include <syscalls/led.h>

#endif	/* ZEPHYR_INCLUDE_DRIVERS_LED_H_ */
