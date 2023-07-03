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

#define USBD_DEFINE_MSC_LUN(disk_name, t10_vendor, t10_product, t10_revision)	\
	STRUCT_SECTION_ITERABLE(usbd_msc_lun, usbd_msc_lun_##disk_name) = {	\
		.disk = STRINGIFY(disk_name),					\
		.vendor = t10_vendor,						\
		.product = t10_product,						\
		.revision = t10_revision,					\
	}

#endif /* ZEPHYR_INCLUDE_USB_CLASS_USBD_MSC_H_ */
