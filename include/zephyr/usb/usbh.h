/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
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

/**
 * @brief USB Class Code triple
 */
struct usbh_code_triple {
	/** Device Class Code */
	uint8_t dclass;
	/** Class Subclass Code */
	uint8_t sub;
	/** Class Protocol Code */
	uint8_t proto;
};

struct usbh_class_data;

/**
 * @brief USB host class instance API
 */
struct usbh_class_api {
	/** Host init handler, before any device is connected */
	int (*init)(struct usbh_class_data *const c_data,
		    struct usbh_context *const uhs_ctx);
	/** Request completion event handler */
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
	/** Pointer to USB host stack context structure */
	struct usbh_context *uhs_ctx;
	/** Pointer to USB device this class is used for */
	struct usb_device *udev;
	/** Interface number for which this class matched */
	uint8_t iface;
	/** Pointer to host support class API */
	struct usbh_class_api *api;
	/** Pointer to private data */
	void *priv;
};

/**
 */
#define USBH_DEFINE_CLASS(name) \
	static STRUCT_SECTION_ITERABLE(usbh_class_data, name)


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
