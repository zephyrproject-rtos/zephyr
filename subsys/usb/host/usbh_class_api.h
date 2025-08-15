/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
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
 *
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
 * @param[in] err Result of the transfer. 0 if the transfer was successful.
 *
 * @return 0 on success, negative error code on failure.
 */
static inline int usbh_class_request(struct usbh_class_data *const c_data,
				     struct uhc_transfer *const xfer, int err)
{
	const struct usbh_class_api *api = c_data->api;

	if (api->request != NULL) {
		return api->request(c_data, xfer, err);
	}

	return -ENOTSUP;
}

/**
 * @brief Device connected handler
 *
 * Called when a device is connected to the bus
 * and it matches the class filters of this instance.
 *
 * @param[in] c_data Pointer to USB host class data
 * @param[in] desc_start_addr Pointer to the start of the descriptor
 * @param[in] desc_end_addr Pointer after the end of the USB descriptor
 *
 * @return 0 on success, negative error code on failure.
 */
static inline int usbh_class_connected(struct usbh_class_data *const c_data,
				       void *const desc_start_addr,
				       void *const desc_end_addr)
{
	const struct usbh_class_api *api = c_data->api;

	if (api->connected != NULL) {
		return api->connected(c_data, desc_start_addr, desc_end_addr);
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
 *
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
 * @brief Bus remote wakeup handler
 *
 * Called when the device trigger a remote wakeup to the host.
 * and it matches the class filters.
 *
 * @param[in] c_data Pointer to USB host class data
 *
 * @return 0 on success, negative error code on failure.
 */
static inline int usbh_class_rwup(struct usbh_class_data *const c_data)
{
	const struct usbh_class_api *api = c_data->api;

	if (api->rwup != NULL) {
		return api->rwup(c_data);
	}

	return -ENOTSUP;
}

/**
 * @brief Bus suspended handler
 *
 * Called when the host has suspending the bus.
 * It can be used to suspend underlying systems.
 *
 * @param[in] c_data Pointer to USB host class data
 *
 * @return 0 on success, negative error code on failure.
 */
static inline int usbh_class_suspended(struct usbh_class_data *const c_data)
{
	const struct usbh_class_api *api = c_data->api;

	if (api->suspended != NULL) {
		return api->suspended(c_data);
	}

	return -ENOTSUP;
}

/**
 * @brief Bus resumed handler
 *
 * Called when the host resumes the activity on a bus.
 * It can be used to wake-up underlying systems.
 *
 * @param[in] c_data Pointer to USB host class data
 *
 * @return 0 on success, negative error code on failure.
 */
static inline int usbh_class_resumed(struct usbh_class_data *const c_data)
{
	const struct usbh_class_api *api = c_data->api;

	if (api->resumed != NULL) {
		return api->resumed(c_data);
	}

	return -ENOTSUP;
}

#endif /* ZEPHYR_INCLUDE_USBD_CLASS_API_H */
