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
 * @since 1.12
 * @version 1.0.0
 * @ingroup io_interfaces
 * @{
 */

#include <errno.h>

#include <zephyr/types.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief LED information structure
 *
 * This structure gathers useful information about LED controller.
 */
struct led_info {
	/** LED label */
	const char *label;
	/** Index of the LED on the controller */
	uint32_t index;
	/** Number of colors per LED */
	uint8_t num_colors;
	/** Mapping of the LED colors */
	const uint8_t *color_mapping;
};

/**
 * @typedef led_api_blink()
 * @brief Callback API for blinking an LED
 *
 * @see led_blink() for argument descriptions.
 */
typedef int (*led_api_blink)(const struct device *dev, uint32_t led,
			     uint32_t delay_on, uint32_t delay_off);

/**
 * @typedef led_api_get_info()
 * @brief Optional API callback to get LED information
 *
 * @see led_get_info() for argument descriptions.
 */
typedef int (*led_api_get_info)(const struct device *dev, uint32_t led,
				const struct led_info **info);

/**
 * @typedef led_api_set_brightness()
 * @brief Callback API for setting brightness of an LED
 *
 * @see led_set_brightness() for argument descriptions.
 */
typedef int (*led_api_set_brightness)(const struct device *dev, uint32_t led,
				      uint8_t value);
/**
 * @typedef led_api_set_color()
 * @brief Optional API callback to set the colors of a LED.
 *
 * @see led_set_color() for argument descriptions.
 */
typedef int (*led_api_set_color)(const struct device *dev, uint32_t led,
				 uint8_t num_colors, const uint8_t *color);

/**
 * @typedef led_api_on()
 * @brief Callback API for turning on an LED
 *
 * @see led_on() for argument descriptions.
 */
typedef int (*led_api_on)(const struct device *dev, uint32_t led);

/**
 * @typedef led_api_off()
 * @brief Callback API for turning off an LED
 *
 * @see led_off() for argument descriptions.
 */
typedef int (*led_api_off)(const struct device *dev, uint32_t led);

/**
 * @typedef led_api_write_channels()
 * @brief Callback API for writing a strip of LED channels
 *
 * @see led_api_write_channels() for arguments descriptions.
 */
typedef int (*led_api_write_channels)(const struct device *dev,
				      uint32_t start_channel,
				      uint32_t num_channels,
				      const uint8_t *buf);

/**
 * @brief LED driver API
 */
__subsystem struct led_driver_api {
	/* Mandatory callbacks. */
	led_api_on on;
	led_api_off off;
	/* Optional callbacks. */
	led_api_blink blink;
	led_api_get_info get_info;
	led_api_set_brightness set_brightness;
	led_api_set_color set_color;
	led_api_write_channels write_channels;
};

/**
 * @brief Blink an LED
 *
 * This optional routine starts blinking a LED forever with the given time
 * period.
 *
 * @param dev LED device
 * @param led LED number
 * @param delay_on Time period (in milliseconds) an LED should be ON
 * @param delay_off Time period (in milliseconds) an LED should be OFF
 * @return 0 on success, negative on error
 */
__syscall int led_blink(const struct device *dev, uint32_t led,
			    uint32_t delay_on, uint32_t delay_off);

static inline int z_impl_led_blink(const struct device *dev, uint32_t led,
				   uint32_t delay_on, uint32_t delay_off)
{
	const struct led_driver_api *api =
		(const struct led_driver_api *)dev->api;

	if (api->blink == NULL) {
		return -ENOSYS;
	}
	return api->blink(dev, led, delay_on, delay_off);
}

/**
 * @brief Get LED information
 *
 * This optional routine provides information about a LED.
 *
 * @param dev LED device
 * @param led LED number
 * @param info Pointer to a pointer filled with LED information
 * @return 0 on success, negative on error
 */
__syscall int led_get_info(const struct device *dev, uint32_t led,
			   const struct led_info **info);

static inline int z_impl_led_get_info(const struct device *dev, uint32_t led,
				      const struct led_info **info)
{
	const struct led_driver_api *api =
		(const struct led_driver_api *)dev->api;

	if (api->get_info == NULL) {
		*info = NULL;
		return -ENOSYS;
	}
	return api->get_info(dev, led, info);
}

/**
 * @brief Set LED brightness
 *
 * This optional routine sets the brightness of a LED to the given value.
 * Calling this function after led_blink() won't affect blinking.
 *
 * LEDs which can only be turned on or off may provide this function.
 * These should simply turn the LED on if @p value is nonzero, and off
 * if @p value is zero.
 *
 * @param dev LED device
 * @param led LED number
 * @param value Brightness value to set in percent
 * @return 0 on success, negative on error
 */
__syscall int led_set_brightness(const struct device *dev, uint32_t led,
				     uint8_t value);

static inline int z_impl_led_set_brightness(const struct device *dev,
					    uint32_t led,
					    uint8_t value)
{
	const struct led_driver_api *api =
		(const struct led_driver_api *)dev->api;

	if (api->set_brightness == NULL) {
		return -ENOSYS;
	}
	return api->set_brightness(dev, led, value);
}

/**
 * @brief Write/update a strip of LED channels
 *
 * This optional routine writes a strip of LED channels to the given array of
 * levels. Therefore it can be used to configure several LEDs at the same time.
 *
 * Calling this function after led_blink() won't affect blinking.
 *
 * @param dev LED device
 * @param start_channel Absolute number (i.e. not relative to a LED) of the
 *        first channel to update.
 * @param num_channels The number of channels to write/update.
 * @param buf array of values to configure the channels with. num_channels
 *        entries must be provided.
 * @return 0 on success, negative on error
 */
__syscall int led_write_channels(const struct device *dev,
				 uint32_t start_channel,
				 uint32_t num_channels, const uint8_t *buf);

static inline int
z_impl_led_write_channels(const struct device *dev, uint32_t start_channel,
			  uint32_t num_channels, const uint8_t *buf)
{
	const struct led_driver_api *api =
		(const struct led_driver_api *)dev->api;

	if (api->write_channels == NULL) {
		return -ENOSYS;
	}
	return api->write_channels(dev, start_channel, num_channels, buf);
}

/**
 * @brief Set a single LED channel
 *
 * This optional routine sets a single LED channel to the given value.
 *
 * Calling this function after led_blink() won't affect blinking.
 *
 * @param dev LED device
 * @param channel Absolute channel number (i.e. not relative to a LED)
 * @param value Value to configure the channel with
 * @return 0 on success, negative on error
 */
__syscall int led_set_channel(const struct device *dev,
			      uint32_t channel, uint8_t value);

static inline int z_impl_led_set_channel(const struct device *dev,
					 uint32_t channel, uint8_t value)
{
	return z_impl_led_write_channels(dev, channel, 1, &value);
}

/**
 * @brief Set LED color
 *
 * This routine configures all the color channels of a LED with the given
 * color array.
 *
 * Calling this function after led_blink() won't affect blinking.
 *
 * @param dev LED device
 * @param led LED number
 * @param num_colors Number of colors in the array.
 * @param color Array of colors. It must be ordered following the color
 *        mapping of the LED controller. See the color_mapping member
 *        in struct led_info.
 * @return 0 on success, negative on error
 */
__syscall int led_set_color(const struct device *dev, uint32_t led,
			    uint8_t num_colors, const uint8_t *color);

static inline int z_impl_led_set_color(const struct device *dev, uint32_t led,
				       uint8_t num_colors, const uint8_t *color)
{
	const struct led_driver_api *api =
		(const struct led_driver_api *)dev->api;

	if (api->set_color == NULL) {
		return -ENOSYS;
	}
	return api->set_color(dev, led, num_colors, color);
}

/**
 * @brief Turn on an LED
 *
 * This routine turns on an LED
 *
 * @param dev LED device
 * @param led LED number
 * @return 0 on success, negative on error
 */
__syscall int led_on(const struct device *dev, uint32_t led);

static inline int z_impl_led_on(const struct device *dev, uint32_t led)
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
 * @param led LED number
 * @return 0 on success, negative on error
 */
__syscall int led_off(const struct device *dev, uint32_t led);

static inline int z_impl_led_off(const struct device *dev, uint32_t led)
{
	const struct led_driver_api *api =
		(const struct led_driver_api *)dev->api;

	return api->off(dev, led);
}

/*
 * LED DT helpers.
 */

/**
 * @brief Container for an LED information specified in devicetree.
 *
 * This type contains a pointer to and LED device and an LED index.
 *
 * @see LED_DT_SPEC_GET
 * @see LED_DT_SPEC_GET_OR
 */
struct led_dt_spec {
	/** LED device instance. */
	const struct device *dev;
	/** Index of the LED on the controller. */
	uint32_t index;
};

/**
 * @brief Set LED brightness from a led_dt_spec.
 *
 * @param spec LED device specification from devicetree.
 * @param value Brightness value to set in percent.
 * @return 0 on success, negative on error.
 *
 * @see led_set_brightness()
 */
static inline int led_set_brightness_dt(const struct led_dt_spec *spec,
					uint8_t value)
{
	return led_set_brightness(spec->dev, spec->index, value);
}

/**
 * @brief Turn on an LED from a struct led_dt_spec.
 *
 * @param spec LED device specification from devicetree.
 * @return 0 on success, negative on error.
 *
 * @see led_on()
 */
static inline int led_on_dt(const struct led_dt_spec *spec)
{
	return led_on(spec->dev, spec->index);
}

/**
 * @brief Turn off an LED from a struct led_dt_spec.
 *
 * @param spec LED device specification from devicetree.
 * @return 0 on success, negative on error.
 *
 * @see led_off()
 */
static inline int led_off_dt(const struct led_dt_spec *spec)
{
	return led_off(spec->dev, spec->index);
}

/**
 * @brief Validate that the LED device is ready.
 *
 * @param spec LED specification from devicetree.
 *
 * @retval true If the LED device is ready for use.
 * @retval false If the LED device is not ready for use.
 */
static inline bool led_is_ready_dt(const struct led_dt_spec *spec)
{
	return device_is_ready(spec->dev);
}

/**
 * @brief Static initializer for a struct led_dt_spec
 *
 * This returns a static initializer for a struct led_dt_spec given a devicetree
 * node identifier.
 *
 * Example devicetree fragment:
 *
 * @code{.dts}
 *    leds {
 *        compatible = "gpio-leds";
 *        led0: led_0 {
 *            ...
 *        };
 *    };
 * @endcode
 *
 * Example usage:
 *
 * @code{.c}
 *    const struct led_dt_spec spec = LED_DT_SPEC_GET(DT_NODELABEL(led0));
 *
 *    // Initializes 'spec' to:
 *    // {
 *    //         .dev = DEVICE_DT_GET(DT_PARENT(led0)),
 *    //         .index = 0,
 *    // }
 * @endcode
 *
 * The device (dev) must still be checked for readiness, e.g. using
 * device_is_ready().
 *
 * @param node_id Devicetree node identifier.
 *
 * @return Static initializer for a struct led_dt_spec for the property.
 */
#define LED_DT_SPEC_GET(node_id)                                               \
	{                                                                      \
		.dev = DEVICE_DT_GET(DT_PARENT(node_id)),                      \
		.index = DT_NODE_CHILD_IDX(node_id),                           \
	}

/**
 * @brief Like LED_DT_SPEC_GET(), with a fallback value if the node does not exist.
 *
 * @param node_id Devicetree node identifier.
 *
 * @return Static initializer for a struct led_dt_spec for the property.
 *
 * @see LED_DT_SPEC_GET
 */
#define LED_DT_SPEC_GET_OR(node_id, default_value)                             \
	COND_CODE_1(DT_NODE_EXISTS(node_id),                                   \
		    (LED_DT_SPEC_GET(node_id)),                                \
		    (default_value))

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#include <zephyr/syscalls/led.h>

#endif	/* ZEPHYR_INCLUDE_DRIVERS_LED_H_ */
