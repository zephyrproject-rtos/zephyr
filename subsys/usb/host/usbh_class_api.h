/*
 * SPDX-FileCopyrightText: Copyright Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief USB host stack class instances API
 *
 * This file contains the USB host stack class instances API.
 */

#ifndef ZEPHYR_INCLUDE_USBH_CLASS_API_H
#define ZEPHYR_INCLUDE_USBH_CLASS_API_H

#include <zephyr/usb/usbh.h>

/**
 * @brief Initialization of the class implementation
 *
 * This is called for each instance during the initialization phase,
 * for every registered class.
 * It can be used to initialize underlying systems.
 *
 * @param[in] c_data Pointer to USB host class data
 * @return 0 on success, negative error code on failure.
 */
static inline int usbh_class_init(struct usbh_class_data *const c_data)
{
	const struct usbh_class_api *api = c_data->api;

	if (api->init != NULL) {
		return api->init(c_data);
	}

	return -ENOTSUP;
}

/**
 * @brief Request completion event handler
 *
 * Called upon completion of a request made by the host to this class.
 *
 * @param[in] c_data Pointer to USB host class data
 * @param[in] xfer Completed transfer
 * @return 0 on success, negative error code on failure.
 */
static inline int usbh_class_completion_cb(struct usbh_class_data *const c_data,
					   struct uhc_transfer *const xfer)
{
	const struct usbh_class_api *api = c_data->api;

	if (api->completion_cb != NULL) {
		return api->completion_cb(c_data, xfer);
	}

	return -ENOTSUP;
}

/**
 * @brief Device initialization handler
 *
 * Called when a device is connected to the bus.
 * It is called once for every USB function of that device.
 *
 * @param[in] c_data Pointer to USB host class data
 * @param[in] udev USB device connected
 * @param[in] iface The @c bInterfaceNumber or @c bFirstInterface of this class
 * @return 0 on success, negative error code on failure.
 * @return -ENOTSUP if the class is not matching
 */
static inline int usbh_class_probe(struct usbh_class_data *const c_data,
				   struct usb_device *const udev,
				   const uint8_t iface)
{
	const struct usbh_class_api *api = c_data->api;

	if (api->probe != NULL) {
		return api->probe(c_data, udev, iface);
	}

	return -ENOTSUP;
}

/**
 * @brief Device removed handler
 *
 * Called when the device is removed from the bus
 * and it matches the class filters of this instance.
 *
 * @param[in] c_data Pointer to USB host class data
 * @return 0 on success, negative error code on failure.
 */
static inline int usbh_class_removed(struct usbh_class_data *const c_data)
{
	const struct usbh_class_api *api = c_data->api;

	if (api->removed != NULL) {
		return api->removed(c_data);
	}

	return -ENOTSUP;
}

/**
 * @brief Bus suspended handler
 *
 * Called when the host has suspended the bus.
 * It can be used to suspend underlying systems.
 *
 * @param[in] c_data Pointer to USB host class data
 * @return 0 on success, negative error code on failure.
 */
static inline int usbh_class_suspended(struct usbh_class_data *const c_data)
{
	const struct usbh_class_api *api = c_data->api;

	if (api->suspended != NULL) {
		return api->suspended(c_data);
	}

	return 0;
}

/**
 * @brief Bus resumed handler
 *
 * Called when the host resumes its activity on the bus.
 * It can be used to wake-up underlying systems.
 *
 * @param[in] c_data Pointer to USB host class data
 * @return 0 on success, negative error code on failure.
 */
static inline int usbh_class_resumed(struct usbh_class_data *const c_data)
{
	const struct usbh_class_api *api = c_data->api;

	if (api->resumed != NULL) {
		return api->resumed(c_data);
	}

	return 0;
}

#endif /* ZEPHYR_INCLUDE_USBD_CLASS_API_H */
