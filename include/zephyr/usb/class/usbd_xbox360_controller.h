/*
 * Copyright (c) 2026 Adrien RICCIARDI
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief USBD Xbox 360 controller device API header
 */

#ifndef ZEPHYR_INCLUDE_USBD_XBOX360_CONTROLLER_H
#define ZEPHYR_INCLUDE_USBD_XBOX360_CONTROLLER_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief USBD Xbox 360 controller device API
 * @defgroup usbd_xbox360_controller_device USBD Xbox 360 controller device API
 * @ingroup usb
 * @since 4.5
 * @version 0.1.0
 * @{
 */

/**
 * The data sent by the controller to the host to tell which keys and analog controls are pressed.
 *
 * The message format is based on
 * https://www.partsnotincluded.com/understanding-the-xbox-360-wired-controllers-usb-data
 */
struct xbox360_controller_control_surface {
	uint8_t message_type; /**< Always 0 */
	uint8_t length; /**< The size of this structure in bytes */
	uint16_t button_directional_pad_up : 1; /**< Up direction button */
	uint16_t button_directional_pad_down : 1; /**< Down direction button */
	uint16_t button_directional_pad_left : 1; /**< Left direction button */
	uint16_t button_directional_pad_right : 1; /**< Right direction button */
	uint16_t button_start : 1; /**< Start button */
	uint16_t button_back : 1; /**< Back button */
	uint16_t button_l3 : 1; /**< Left stick middle button */
	uint16_t button_r3 : 1; /**< Right stick middle button */
	uint16_t button_lb : 1; /**< Left bumper */
	uint16_t button_rb : 1; /**< Right bumper */
	uint16_t button_xbox : 1; /**< Xbox-shaped button */
	uint16_t reserved_0 : 1; /**< Unused */
	uint16_t button_a : 1; /**< 'A' button */
	uint16_t button_b : 1; /**< 'B' button */
	uint16_t button_x : 1; /**< 'X' button */
	uint16_t button_y : 1; /**< 'Y' button */
	uint8_t trigger_lt; /**< Left analog trigger, 0 is released and 0xFF is fully pressed */
	uint8_t trigger_rt; /**< Right analog trigger, 0 is released and 0xFF is fully pressed */
	int16_t left_joystick_x; /**< Analog left stick X axis, 0 is the centered value */
	int16_t left_joystick_y; /**< Analog left stick Y axis, 0 is the centered value */
	int16_t right_joystick_x; /**< Analog right stick X axis, 0 is the centered value */
	int16_t right_joystick_y; /**< Analog right stick Y axis, 0 is the centered value */
	uint8_t reserved_1[6]; /**< Unused */
} __packed;

/** Application event handlers */
struct xbox360_controller_device_ops {
	/**
	 * The interface ready callback is called with the ready argument set to true when the
	 * corresponding interface is part of the active configuration and the device can
	 * e.g. begin submitting input reports, and with the argument set to false when the
	 * interface is no longer active. This callback is optional.
	 */
	void (*iface_ready)(const struct device *dev, const bool ready);
};

/**
 * @brief Register the Xbox 360 controller device user callbacks.
 *
 * If used, the device user callbacks must be registered before the USB device support
 * is initialized and enabled.
 *
 * @param[in] dev Pointer to the Xbox 360 controller device
 * @param[in] ops Pointer to the Xbox 360 controller device user callbacks
 */
int xbox360_controller_device_register(const struct device *dev,
				       const struct xbox360_controller_device_ops *const ops);

/**
 * @brief Submit a new input report
 *
 * Submit a new control surface (i.e the state of the controller buttons and analog controls)
 * to be sent to the host.
 *
 * @param[in] dev Pointer to the Xbox 360 controller device
 * @param[in] control_surface Pointer to the control surface to submit
 *
 * @return 0 on success, or a negative value if an error occurred.
 */
int xbox360_controller_submit_report(const struct device *dev,
				     struct xbox360_controller_control_surface *control_surface);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_USBD_XBOX360_CONTROLLER_H */
