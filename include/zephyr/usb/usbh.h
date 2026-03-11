/*
 * SPDX-FileCopyrightText: Copyright Nordic Semiconductor ASA
 * SPDX-FileCopyrightText: Copyright 2025 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief New experimental USB device stack APIs and structures
 *
 * This file contains the USB device stack APIs and structures.
 */

#ifndef ZEPHYR_INCLUDE_USBH_H_
#define ZEPHYR_INCLUDE_USBH_H_

#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/dlist.h>
#include <zephyr/sys/bitarray.h>
#include <zephyr/drivers/usb/uhc.h>
#include <zephyr/sys/iterable_sections.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief USB HOST Core Layer API
 * @defgroup usb_host_core_api USB Host Core API
 * @ingroup usb
 * @{
 */

/**
 * USB host support status
 */
struct usbh_status {
	/** USB host support is initialized */
	unsigned int initialized : 1;
	/** USB host support is enabled */
	unsigned int enabled : 1;
};

/**
 * USB host support runtime context
 */
struct usbh_context {
	/** Name of the USB device */
	const char *name;
	/** Access mutex */
	struct k_mutex mutex;
	/** Pointer to UHC device struct */
	const struct device *dev;
	/** Status of the USB host support */
	struct usbh_status status;
	/** USB device list */
	sys_dlist_t udevs;
	/** USB root device */
	struct usb_device *root;
	/** Allocated device addresses bit array */
	struct sys_bitarray *addr_ba;
};

#define USBH_CONTROLLER_DEFINE(device_name, uhc_dev)			\
	SYS_BITARRAY_DEFINE_STATIC(ba_##device_name, 128);		\
	static STRUCT_SECTION_ITERABLE(usbh_context, device_name) = {	\
		.name = STRINGIFY(device_name),				\
		.mutex = Z_MUTEX_INITIALIZER(device_name.mutex),	\
		.dev = uhc_dev,						\
		.addr_ba = &ba_##device_name,				\
	}

struct usbh_class_data;

/**
 * @brief Information about a device, which is relevant for matching a particular class.
 */
struct usbh_class_filter {
	/** Vendor ID */
	uint16_t vid;
	/** Product ID */
	uint16_t pid;
	/** Class Code */
	uint8_t class;
	/** Subclass Code */
	uint8_t sub;
	/** Protocol Code */
	uint8_t proto;
	/** Flags that tell which field to match */
	uint8_t flags;
};

/**
 * @brief USB host class instance API
 */
struct usbh_class_api {
	/** Host initialization handler, before any device is connected */
	int (*init)(struct usbh_class_data *const c_data);
	/** Request completion handler */
	int (*completion_cb)(struct usbh_class_data *const c_data,
			     struct uhc_transfer *const xfer);
	/** Device connection handler */
	int (*probe)(struct usbh_class_data *const c_data,
		     struct usb_device *const udev,
		     const uint8_t iface);
	/** Device removal handler */
	int (*removed)(struct usbh_class_data *const c_data);
	/** Bus suspended handler (optional) */
	int (*suspended)(struct usbh_class_data *const c_data);
	/** Bus resumed handler (optional) */
	int (*resumed)(struct usbh_class_data *const c_data);
};

/**
 * @brief USB host class instance data
 */
struct usbh_class_data {
	/** Name of the USB host class instance */
	const char *name;
	/** Pointer to USB device this class is used for */
	struct usb_device *udev;
	/** First interface number or claimed function */
	uint8_t iface;
	/** Pointer to host support class API */
	struct usbh_class_api *api;
	/** Pointer to private data */
	void *priv;
};

/**
 * @cond INTERNAL_HIDDEN
 *
 * Internal state of an USB class. Not corresponding to an USB protocol state,
 * but instead software life cycle.
 */
enum usbh_class_state {
	/** The class is available to be associated to an USB device function. */
	USBH_CLASS_STATE_IDLE,
	/** The class got bound to an USB function of a particular device on the bus. */
	USBH_CLASS_STATE_BOUND,
	/** The class failed to initialize and cannot be used. */
	USBH_CLASS_STATE_ERROR,
};
/* @endcond */

/**
 * @cond INTERNAL_HIDDEN
 *
 * Variables used by the USB host stack but not exposed to the class
 * through the class API.
 */
struct usbh_class_node {
	/** Class information exposed to host class implementations (drivers). */
	struct usbh_class_data *const c_data;
	/** Filter rules to match this USB host class instance against a device class **/
	const struct usbh_class_filter *filters;
	/** State of the USB class instance */
	enum usbh_class_state state;
};
/* @endcond */

/**
 * @brief Define USB host support class data
 *
 * Macro defines class (function) data, as well as corresponding node
 * structures used internally by the stack.
 *
 * @param[in] class_name Class name
 * @param[in] class_api  Pointer to struct usbh_class_api
 * @param[in] class_priv Class private data
 * @param[in] filt Array of @ref usbh_class_filter to match this class or NULL to match everything.
 *                 When non-NULL, then it has to be terminated by an entry with @c flags set to 0.
 */
#define USBH_DEFINE_CLASS(class_name, class_api, class_priv, filt)		\
	static struct usbh_class_data UTIL_CAT(class_data_, class_name) = {	\
		.name = STRINGIFY(class_name),					\
		.api = class_api,						\
		.priv = class_priv,						\
	};									\
	static STRUCT_SECTION_ITERABLE(usbh_class_node, class_name) = {		\
		.c_data = &UTIL_CAT(class_data_, class_name),			\
		.filters = filt,						\
	};

/**
 * @brief Initialize the USB host support;
 *
 * @param[in] uhs_ctx Pointer to USB host support context
 *
 * @return 0 on success, other values on fail.
 */
int usbh_init(struct usbh_context *uhs_ctx);

/**
 * @brief Enable the USB host support and class instances
 *
 * This function enables the USB host support.
 *
 * @param[in] uhs_ctx Pointer to USB host support context
 *
 * @return 0 on success, other values on fail.
 */
int usbh_enable(struct usbh_context *uhs_ctx);

/**
 * @brief Disable the USB host support
 *
 * This function disables the USB host support.
 *
 * @param[in] uhs_ctx Pointer to USB host support context
 *
 * @return 0 on success, other values on fail.
 */
int usbh_disable(struct usbh_context *uhs_ctx);

/**
 * @brief Shutdown the USB host support
 *
 * This function completely disables the USB host support.
 *
 * @param[in] uhs_ctx Pointer to USB host support context
 *
 * @return 0 on success, other values on fail.
 */
int usbh_shutdown(struct usbh_context *const uhs_ctx);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_USBH_H_ */
