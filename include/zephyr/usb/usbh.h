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
#include <zephyr/net/buf.h>
#include <zephyr/sys/dlist.h>
#include <zephyr/drivers/usb/uhc.h>
#include <zephyr/linker/linker-defs.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief USB HOST Core Layer API
 * @defgroup _usb_host_core_api USB Host Core API
 * @{
 */

/**
 * USB host support runtime context
 */
struct usbh_contex {
	/** Name of the USB device */
	const char *name;
	/** Access mutex */
	struct k_mutex mutex;
	/** Pointer to UHC device struct */
	const struct device *dev;
	/** peripheral list */
	sys_dlist_t peripherals;
};

#define USBH_CONTROLLER_DEFINE(device_name, uhc_dev)			\
	static STRUCT_SECTION_ITERABLE(usbh_contex, device_name) = {	\
		.name = STRINGIFY(device_name),				\
		.mutex = Z_MUTEX_INITIALIZER(device_name.mutex),	\
		.dev = uhc_dev,						\
	}

/**
 * @brief USB host peripheral structure
 */
struct usbh_peripheral {
	/** Peripheral dlist node */
	sys_dnode_t node;
	/** Peripheral address */
	uint8_t addr;
	/** Detected speed (TBD) */
	uint8_t speed;
};

/**
 * @brief Class Code
 */
struct usbh_class_code {
	/** Device Class Code */
	uint8_t dclass;
	/** Class Subclass Code */
	uint8_t sub;
	/** Class Protocol Code */
	uint8_t proto;
	/** Reserved */
	uint8_t reserved;
};

/**
 * @brief USB host class data and class instance API
 */
struct usbh_class_data {
	/** Class code supported by this instance */
	struct usbh_class_code code;

	/** Initialization of the class implementation */
	/* int (*init)(struct usbh_contex *const uhs_ctx); */
	/** Request completion event handler */
	int (*request)(struct usbh_contex *const uhs_ctx,
			struct uhc_transfer *const xfer, int err);
	/** Device connected handler  */
	int (*connected)(struct usbh_contex *const uhs_ctx);
	/** Device removed handler  */
	int (*removed)(struct usbh_contex *const uhs_ctx);
	/** Bus remote wakeup handler  */
	int (*rwup)(struct usbh_contex *const uhs_ctx);
	/** Bus suspended handler  */
	int (*suspended)(struct usbh_contex *const uhs_ctx);
	/** Bus resumed handler  */
	int (*resumed)(struct usbh_contex *const uhs_ctx);
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
int usbh_init(struct usbh_contex *uhs_ctx);

/**
 * @brief Enable the USB host support and class instances
 *
 * This function enables the USB host support.
 *
 * @param[in] uhs_ctx Pointer to USB host support context
 *
 * @return 0 on success, other values on fail.
 */
int usbh_enable(struct usbh_contex *uhs_ctx);

/**
 * @brief Disable the USB host support
 *
 * This function disables the USB host support.
 *
 * @param[in] uhs_ctx Pointer to USB host support context
 *
 * @return 0 on success, other values on fail.
 */
int usbh_disable(struct usbh_contex *uhs_ctx);

/**
 * @brief Shutdown the USB host support
 *
 * This function completely disables the USB host support.
 *
 * @param[in] uhs_ctx Pointer to USB host support context
 *
 * @return 0 on success, other values on fail.
 */
int usbh_shutdown(struct usbh_contex *const uhs_ctx);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_USBH_H_ */
