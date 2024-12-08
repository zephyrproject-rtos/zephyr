/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief USBD Mass Storage Class public header
 *
 * Header exposes API for registering LUNs.
 */

#include <zephyr/sys/iterable_sections.h>

#ifndef ZEPHYR_INCLUDE_USB_CLASS_USBD_MSC_H_
#define ZEPHYR_INCLUDE_USB_CLASS_USBD_MSC_H_

struct usbd_msc_lun {
	const char *disk;
	const char *vendor;
	const char *product;
	const char *revision;
};

/**
 * @brief USB Mass Storage Class device API
 * @defgroup usbd_msc_device USB Mass Storage Class device API
 * @ingroup usb
 * @since 3.4
 * @version 0.1.0
 * @{
 */

/**
 * @brief Define USB Mass Storage Class logical unit
 *
 * Use this macro to create Logical Unit mapping in USB MSC for selected disk.
 * Up to `CONFIG_USBD_MSC_LUNS_PER_INSTANCE` disks can be registered on single
 * USB MSC instance. Currently only one USB MSC instance is supported.
 *
 * @param id Identifier by which the linker sorts registered LUNs
 * @param disk_name Disk name as used in @ref disk_access_interface
 * @param t10_vendor T10 Vendor Indetification
 * @param t10_product T10 Product Identification
 * @param t10_revision T10 Product Revision Level
 */
#define USBD_DEFINE_MSC_LUN(id, disk_name, t10_vendor, t10_product, t10_revision)	\
	static const STRUCT_SECTION_ITERABLE(usbd_msc_lun, usbd_msc_lun_##id) = {	\
		.disk = disk_name,							\
		.vendor = t10_vendor,							\
		.product = t10_product,							\
		.revision = t10_revision,						\
	}

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_USB_CLASS_USBD_MSC_H_ */
