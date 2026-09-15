/*
 * Copyright (c) 2025 Mohamed ElShahawi (extremegtx@hotmail.com)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief USB Media Transfer Protocol (MTP) public header
 *
 * Header exposes API for registering MTP storages.
 */

#ifndef ZEPHYR_INCLUDE_USB_CLASS_USBD_MTP_H_
#define ZEPHYR_INCLUDE_USB_CLASS_USBD_MTP_H_

#include <stdbool.h>
#include <stddef.h>

#include <zephyr/sys/iterable_sections.h>
#include <zephyr/sys/util.h>

/**
 * @brief MTP storage registration entry.
 */
struct usbd_mtp_storage {
	/** Filesystem mount point exposed through MTP. */
	const char *mountpoint;
	/** Whether the storage is exposed as read-only. */
	bool read_only;
};

/**
 * @brief MTP instance registration entry.
 */
struct usbd_mtp_instance {
	/** MTP Manufacturer string. Optional, pass NULL when unused. */
	const char *manufacturer;
	/** MTP Model string. Optional, pass NULL when unused. */
	const char *product;
	/** MTP DeviceVersion string. Optional, pass NULL when unused. */
	const char *device_version;
	/** MTP SerialNumber string. Must be a 32 character hexadecimal string. */
	const char *serial_number;
	/** Storages exposed by this MTP instance. */
	const struct usbd_mtp_storage *storages;
	/** Number of storages exposed by this MTP instance. */
	size_t storage_count;
};

/**
 * @brief USB Media Transfer Protocol device API
 * @defgroup usbd_mtp_device USB Media Transfer Protocol device API
 * @ingroup usb
 * @{
 */

/**
 * @brief Initialize a USB MTP storage entry
 *
 * Use this macro inside USBD_MTP_DEFINE_INSTANCE().
 *
 * @param storage_mountpoint Filesystem mount point
 * @param storage_read_only Whether the storage is exposed read-only
 */
#define USBD_MTP_STORAGE_ENTRY(storage_mountpoint, storage_read_only)                              \
	{                                                                                          \
		.mountpoint = storage_mountpoint,                                                  \
		.read_only = storage_read_only,                                                    \
	}

/**
 * @brief Define USB MTP instance
 *
 * Use this macro to expose one or more mounted filesystems through one USB MTP
 * class instance. Up to `CONFIG_USBD_MTP_STORAGES_PER_INSTANCE` storages can be
 * registered per MTP instance.
 *
 * @param id Identifier by which the linker sorts registered MTP instances
 * @param mtp_manufacturer MTP Manufacturer string, or NULL
 * @param mtp_product MTP Model/Product string, or NULL
 * @param mtp_device_version MTP DeviceVersion string, or NULL
 * @param mtp_serial_number MTP SerialNumber string, 32 hexadecimal characters
 * @param ... USBD_MTP_STORAGE_ENTRY() items exposed by this MTP instance
 */
#define USBD_MTP_DEFINE_INSTANCE(id, mtp_manufacturer, mtp_product, mtp_device_version,            \
				 mtp_serial_number, ...)                                           \
	static const struct usbd_mtp_storage usbd_mtp_storages_##id[] = {__VA_ARGS__};             \
	BUILD_ASSERT(ARRAY_SIZE(usbd_mtp_storages_##id) <= CONFIG_USBD_MTP_STORAGES_PER_INSTANCE,  \
		     "Too many MTP storages");                                                     \
	static const STRUCT_SECTION_ITERABLE(usbd_mtp_instance, usbd_mtp_instance_##id) = {        \
		.manufacturer = mtp_manufacturer,                                                  \
		.product = mtp_product,                                                            \
		.device_version = mtp_device_version,                                              \
		.serial_number = mtp_serial_number,                                                \
		.storages = usbd_mtp_storages_##id,                                                \
		.storage_count = ARRAY_SIZE(usbd_mtp_storages_##id),                               \
	}

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_USB_CLASS_USBD_MTP_H_ */
