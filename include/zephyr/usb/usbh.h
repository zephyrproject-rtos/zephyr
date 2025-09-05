/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 * Copyright 2025 NXP
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
 * @brief Match flags for USB device identification
 */
#define USBH_MATCH_DEVICE   (1U << 0)	/* Match device class code */
#define USBH_MATCH_INTFACE  (1U << 1)	/* Match interface code */

/* device signal value definitions */
#define USBH_DEVICE_CONNECTED			1
#define USBH_DEVICE_DISCONNECTED		2

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
	/** List of registered classes (functions) */
	sys_slist_t class_list;
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
	/** Initialization of the class implementation */
	int (*init)(struct usbh_class_data *cdata);
	/** Request completion event handler */
	int (*request)(struct usbh_class_data *cdata,
		       struct uhc_transfer *const xfer, int err);
	/** Device connected handler  */
	int (*connected)(struct usb_device *udev, struct usbh_class_data *cdata,
			 void *desc_start_addr,
			 void *desc_end_addr);
	/** Device removed handler  */
	int (*removed)(struct usb_device *udev, struct usbh_class_data *cdata);
	/** Bus remote wakeup handler  */
	int (*rwup)(struct usbh_class_data *cdata);
	/** Bus suspended handler  */
	int (*suspended)(struct usbh_class_data *cdata);
	/** Bus resumed handler  */
	int (*resumed)(struct usbh_class_data *cdata);
};

/** Match a class code triple */
#define USBH_CLASS_FILTER_CODE_TRIPLE BIT(0)

/** Match a device's vendor ID */
#define USBH_CLASS_FILTER_VID BIT(1)

/** Match a device's product ID */
#define USBH_CLASS_FILTER_PID BIT(2)

/**
 * @brief Filter rule for matching a host class instance to a device class
 */
struct usbh_class_filter {
	/** Mask of match types for selecting which rules to apply */
	uint32_t flags;
	/** The device's class code, subclass code, protocol code. */
	struct usbh_code_triple code_triple;
	/** Vendor ID */
	uint16_t vid;
	/** Product ID */
	uint16_t pid;
};

/**
 * @brief USB host class instance data
 */
struct usbh_class_data {
	/** Name of the USB host class instance */
	const char *name;
	/** Pointer to USB host stack context structure */
	struct usbh_context *uhs_ctx;
	/** System linked list node for registered classes */
	sys_snode_t node;
	/** Table of filter rules used to match device classes */
	const struct usbh_class_filter *filters;
	/** Pointer to host support class API */
	struct usbh_class_api *api;
	/** Pointer to device code table for class matching */
	const struct usbh_device_code_table *device_code_table;
	/** Number of items in device code table */
	uint8_t table_items_count;
	/** Flag indicating if class has been matched to a device */
	uint8_t class_matched;
	/** Pointer to private data */
	void *priv;
};

/**
 * @brief USB device code table for device matching
 */
struct usbh_device_code_table {
	/** Match type for device identification */
	uint32_t match_type;
	/** Vendor ID */
	uint16_t vid;
	/** Product ID */
	uint16_t pid;
	/** device's class code, subclass code,  protocol code. */
	struct usbh_code_triple device_code;
	/** USB interface class code */
	uint8_t interface_class_code;
	/** USB interface subclass code */
	uint8_t interface_subclass_code;
	/** USB interface protocol code */
	uint8_t interface_protocol_code;
};

/**
 * @brief USB host speed
 */
enum usbh_speed {
	/** Host supports or is connected to a full speed bus */
	USBH_SPEED_FS,
	/** Host supports or is connected to a high speed bus  */
	USBH_SPEED_HS,
	/** Host supports or is connected to a super speed bus */
	USBH_SPEED_SS,
};

/**
 * @brief Define USB host support class data
 *
 * Macro defines class (function) data, as well as corresponding node
 * structures used internally by the stack.
 *
 * @param class_name   Class name
 * @param class_api    Pointer to struct usbd_class_api
 * @param class_priv   Class private data
 */
#define USBH_DEFINE_CLASS(class_name, class_api, class_priv, code_table, items_count) \
	static STRUCT_SECTION_ITERABLE(usbh_class_data, class_name) = {		\
		.name = STRINGIFY(class_name),					\
		.api = class_api,						\
		.priv = class_priv,						\
		.device_code_table = code_table,				\
		.table_items_count = items_count,				\
		.class_matched = 0,						\
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
