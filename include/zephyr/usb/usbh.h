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

/**
 * @brief USB host class data and class instance API
 */
struct usbh_class_data {
	/** Class code supported by this instance */
	struct usbh_code_triple code;

	/** Initialization of the class implementation */
	/* int (*init)(struct usbh_context *const uhs_ctx); */
	/** Request completion event handler */
	int (*request)(struct usbh_context *const uhs_ctx,
			struct uhc_transfer *const xfer, int err);
	/** Device connected handler  */
	int (*connected)(struct usbh_context *const uhs_ctx);
	/** Device removed handler  */
	int (*removed)(struct usbh_context *const uhs_ctx);
	/** Bus remote wakeup handler  */
	int (*rwup)(struct usbh_context *const uhs_ctx);
	/** Bus suspended handler  */
	int (*suspended)(struct usbh_context *const uhs_ctx);
	/** Bus resumed handler  */
	int (*resumed)(struct usbh_context *const uhs_ctx);
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
