/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief USB device stack class instances API
 *
 * This file contains the USB device stack class instances API.
 */

#ifndef ZEPHYR_INCLUDE_USBD_CLASS_API_H
#define ZEPHYR_INCLUDE_USBD_CLASS_API_H

#include <zephyr/usb/usbd.h>

/**
 * @brief Endpoint request completion event handler
 *
 * This is the event handler for all endpoint accommodated
 * by a class instance.
 *
 * @param[in] c_data Pointer to USB device class data
 * @param[in] buf Control Request Data buffer
 * @param[in] err Result of the transfer. 0 if the transfer was successful.
 */
static inline int usbd_class_request(struct usbd_class_data *const c_data,
				     struct net_buf *const buf,
				     int err)
{
	const struct usbd_class_api *api = c_data->api;

	if (api->request != NULL) {
		return api->request(c_data, buf, err);
	}

	return -ENOTSUP;
}


/**
 * @brief USB control request handler
 *
 * Common handler for all control request.
 * Regardless requests recipient, interface or endpoint,
 * the USB device core will identify proper class instance
 * and call this handler.
 * For the vendor type request USBD_VENDOR_REQ macro must be used
 * to identify the class, if more than one class instance is
 * present, only the first one will be called.
 *
 * The execution of the handler must not block.
 *
 * @param[in] c_data Pointer to USB device class data
 * @param[in] setup Pointer to USB Setup Packet
 * @param[in] buf Control Request Data buffer
 *
 * @return 0 on success, other values on fail.
 */
static inline int usbd_class_control_to_host(struct usbd_class_data *const c_data,
					     struct usb_setup_packet *const setup,
					     struct net_buf *const buf)
{
	const struct usbd_class_api *api = c_data->api;

	if (api->control_to_host != NULL) {
		return api->control_to_host(c_data, setup, buf);
	}

	errno = -ENOTSUP;
	return 0;
}

/**
 * @brief USB control request handler
 *
 * Common handler for all control request.
 * Regardless requests recipient, interface or endpoint,
 * the USB device core will identify proper class instance
 * and call this handler.
 * For the vendor type request USBD_VENDOR_REQ macro must be used
 * to identify the class, if more than one class instance is
 * present, only the first one will be called.
 *
 * The execution of the handler must not block.
 *
 * @param[in] c_data Pointer to USB device class data
 * @param[in] setup Pointer to USB Setup Packet
 * @param[in] buf Control Request Data buffer
 *
 * @return 0 on success, other values on fail.
 */
static inline int usbd_class_control_to_dev(struct usbd_class_data *const c_data,
					    struct usb_setup_packet *const setup,
					    struct net_buf *const buf)
{
	const struct usbd_class_api *api = c_data->api;

	if (api->control_to_dev != NULL) {
		return api->control_to_dev(c_data, setup, buf);
	}

	errno = -ENOTSUP;
	return 0;
}

/**
 * @brief Feature endpoint halt update handler
 *
 * Called when an endpoint of the interface belonging
 * to the instance has been halted or cleared by either
 * a Set Feature Endpoint Halt or Clear Feature Endpoint Halt request.
 *
 * The execution of the handler must not block.
 *
 * @param[in] c_data Pointer to USB device class data
 * @param[in] ep Endpoint
 * @param[in] halted True if the endpoint has been halted and false if
 *                   the endpoint halt has been cleared by a Feature request.
 */
static inline void usbd_class_feature_halt(struct usbd_class_data *const c_data,
					   const uint8_t ep,
					   const bool halted)
{
	const struct usbd_class_api *api = c_data->api;

	if (api->feature_halt != NULL) {
		api->feature_halt(c_data, ep, halted);
	}
}

/**
 * @brief Configuration update handler
 *
 * Called when the configuration of the interface belonging
 * to the instance has been changed, either because of
 * Set Configuration or Set Interface request.
 *
 * The execution of the handler must not block.
 *
 * @param[in] c_data Pointer to USB device class data
 * @param[in] iface Interface
 * @param[in] alternate Alternate setting
 */
static inline void usbd_class_update(struct usbd_class_data *const c_data,
				     const uint8_t iface,
				     const uint8_t alternate)
{
	const struct usbd_class_api *api = c_data->api;

	if (api->update != NULL) {
		api->update(c_data, iface, alternate);
	}
}


/**
 * @brief USB suspended handler
 *
 * @param[in] c_data Pointer to USB device class data
 */
static inline void usbd_class_suspended(struct usbd_class_data *const c_data)
{
	const struct usbd_class_api *api = c_data->api;

	if (api->suspended != NULL) {
		api->suspended(c_data);
	}
}


/**
 * @brief USB resumed handler
 *
 * @param[in] c_data Pointer to USB device class data
 */
static inline void usbd_class_resumed(struct usbd_class_data *const c_data)
{
	const struct usbd_class_api *api = c_data->api;

	if (api->resumed != NULL) {
		api->resumed(c_data);
	}
}

/**
 * @brief USB Start of Frame handler
 *
 * @note The execution of the handler must not block.
 *
 * @param[in] c_data Pointer to USB device class data
 */
static inline void usbd_class_sof(struct usbd_class_data *const c_data)
{
	const struct usbd_class_api *api = c_data->api;

	if (api->sof != NULL) {
		api->sof(c_data);
	}
}

/**
 * @brief Class associated configuration active handler
 *
 * @note The execution of the handler must not block.
 *
 * @param[in] c_data Pointer to USB device class data
 */
static inline void usbd_class_enable(struct usbd_class_data *const c_data)
{
	const struct usbd_class_api *api = c_data->api;

	if (api->enable != NULL) {
		api->enable(c_data);
	}
}

/**
 * @brief Class associated configuration disable handler
 *
 * @note The execution of the handler must not block.
 *
 * @param[in] c_data Pointer to USB device class data
 */
static inline void usbd_class_disable(struct usbd_class_data *const c_data)
{
	const struct usbd_class_api *api = c_data->api;

	if (api->disable != NULL) {
		api->disable(c_data);
	}
}

/**
 * @brief Initialization of the class implementation
 *
 * This is called for each instance during the initialization phase
 * after the interface number and endpoint addresses are assigned
 * to the corresponding instance.
 * It can be used to initialize class specific descriptors or
 * underlying systems.
 *
 * @note If this call fails the core will terminate stack initialization.
 *
 * @param[in] c_data Pointer to USB device class data
 *
 * @return 0 on success, other values on fail.
 */
static inline int usbd_class_init(struct usbd_class_data *const c_data)
{
	const struct usbd_class_api *api = c_data->api;

	if (api->init != NULL) {
		return api->init(c_data);
	}

	return -ENOTSUP;
}

/**
 * @brief Shutdown of the class implementation
 *
 * This is called for each instance during the shutdown phase.
 *
 * @note The execution of the handler must not block.
 *
 * @param[in] c_data Pointer to USB device class data
 */
static inline void usbd_class_shutdown(struct usbd_class_data *const c_data)
{
	const struct usbd_class_api *api = c_data->api;

	if (api->shutdown != NULL) {
		api->shutdown(c_data);
	}
}

/**
 * @brief Get function descriptor
 *
 * @param[in] c_data Pointer to USB device class data
 * @param[in] speed For which speed descriptor is requested.
 *
 * @return Array of struct usb_desc_header pointers with a last element
 *         pointing to a nil descriptor on success, NULL if not available.
 */
static inline void *usbd_class_get_desc(struct usbd_class_data *const c_data,
					const enum usbd_speed speed)
{
	const struct usbd_class_api *api = c_data->api;

	if (api->get_desc != NULL) {
		return api->get_desc(c_data, speed);
	}

	return NULL;
}

#endif /* ZEPHYR_INCLUDE_USBD_CLASS_API_H */
