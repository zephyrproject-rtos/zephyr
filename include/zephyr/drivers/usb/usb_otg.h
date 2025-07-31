/*
 * Copyright (c) 2024 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_USB_OTG_H_
#define ZEPHYR_INCLUDE_DRIVERS_USB_OTG_H_

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief USB OTG Controller Interface
 * @defgroup usb_otg_interface USB OTG Interface
 * @ingroup io_interfaces
 * @{
 */

/**
 * @brief USB OTG role
 */
enum usb_otg_role {
	USB_OTG_ROLE_NONE,
	USB_OTG_ROLE_HOST,
	USB_OTG_ROLE_DEVICE,
};

/**
 * @brief USB OTG events
 */
enum usb_otg_event {
	USB_OTG_EVENT_NONE,
	USB_OTG_EVENT_VBUS_VALID,
	USB_OTG_EVENT_VBUS_INVALID,
	USB_OTG_EVENT_ID_LOW,     /* ID pin grounded - host mode */
	USB_OTG_EVENT_ID_HIGH,    /* ID pin floating - device mode */
	USB_OTG_EVENT_ROLE_CHANGED,
};

/**
 * @brief USB OTG event callback
 *
 * @param dev OTG device
 * @param event OTG event that occurred
 * @param role Current role after the event
 */
typedef void (*usb_otg_event_cb_t)(const struct device *dev,
				   enum usb_otg_event event,
				   enum usb_otg_role role);

/**
 * @brief USB OTG controller API
 */
struct usb_otg_api {
	int (*init)(const struct device *dev);
	int (*deinit)(const struct device *dev);
	int (*set_role)(const struct device *dev, enum usb_otg_role role);
	enum usb_otg_role (*get_role)(const struct device *dev);
	int (*register_callback)(const struct device *dev, usb_otg_event_cb_t cb);
	int (*enable)(const struct device *dev);
	int (*disable)(const struct device *dev);
};

/**
 * @brief Initialize USB OTG controller
 *
 * @param dev OTG device
 * @return 0 on success, negative errno on failure
 */
static inline int usb_otg_init(const struct device *dev)
{
	const struct usb_otg_api *api = dev->api;

	if (api->init == NULL) {
		return -ENOSYS;
	}

	return api->init(dev);
}

/**
 * @brief Deinitialize USB OTG controller
 *
 * @param dev OTG device
 * @return 0 on success, negative errno on failure
 */
static inline int usb_otg_deinit(const struct device *dev)
{
	const struct usb_otg_api *api = dev->api;

	if (api->deinit == NULL) {
		return -ENOSYS;
	}

	return api->deinit(dev);
}

/**
 * @brief Set USB OTG role
 *
 * @param dev OTG device
 * @param role Desired role
 * @return 0 on success, negative errno on failure
 */
static inline int usb_otg_set_role(const struct device *dev,
				   enum usb_otg_role role)
{
	const struct usb_otg_api *api = dev->api;

	if (api->set_role == NULL) {
		return -ENOSYS;
	}

	return api->set_role(dev, role);
}

/**
 * @brief Get current USB OTG role
 *
 * @param dev OTG device
 * @return Current role
 */
static inline enum usb_otg_role usb_otg_get_role(const struct device *dev)
{
	const struct usb_otg_api *api = dev->api;

	if (api->get_role == NULL) {
		return USB_OTG_ROLE_NONE;
	}

	return api->get_role(dev);
}

/**
 * @brief Register USB OTG event callback
 *
 * @param dev OTG device
 * @param cb Callback function
 * @return 0 on success, negative errno on failure
 */
static inline int usb_otg_register_callback(const struct device *dev,
					    usb_otg_event_cb_t cb)
{
	const struct usb_otg_api *api = dev->api;

	if (api->register_callback == NULL) {
		return -ENOSYS;
	}

	return api->register_callback(dev, cb);
}

/**
 * @brief Enable USB OTG controller
 *
 * @param dev OTG device
 * @return 0 on success, negative errno on failure
 */
static inline int usb_otg_enable(const struct device *dev)
{
	const struct usb_otg_api *api = dev->api;

	if (api->enable == NULL) {
		return -ENOSYS;
	}

	return api->enable(dev);
}

/**
 * @brief Disable USB OTG controller
 *
 * @param dev OTG device
 * @return 0 on success, negative errno on failure
 */
static inline int usb_otg_disable(const struct device *dev)
{
	const struct usb_otg_api *api = dev->api;

	if (api->disable == NULL) {
		return -ENOSYS;
	}

	return api->disable(dev);
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_USB_OTG_H_ */