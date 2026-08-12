/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief USB Mass Storage Class SCSI transport adapter
 *
 * Transport @ref scsi_driver_api for USB BOT. LUN objects live in the unified
 * @ref usb_msc_lun registry (see @ref usb_msc_disk.h).
 *
 * @since 4.3
 */

#ifndef ZEPHYR_INCLUDE_USB_USB_MSC_SCSI_H_
#define ZEPHYR_INCLUDE_USB_USB_MSC_SCSI_H_

#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/scsi/scsi.h>
#include <zephyr/usb/usbh_msc.h>

#ifdef __cplusplus
extern "C" {
#endif

/** USB MSC SCSI transport operations for @ref scsi_driver_api */
extern const struct scsi_driver_api usb_msc_scsi_api;

/**
 * @brief Associate a SCSI device handle with a USB MSC LUN object
 *
 * @a sdev must be the embedded @c sdev member of a @ref usb_msc_lun slot.
 */
int usb_msc_scsi_bind(struct scsi_device *sdev, const struct device *uhc, struct usb_device *udev,
		      const struct usbh_msc_iface *msc, uint8_t lun);

/**
 * @brief Remove a SCSI device binding from its LUN slot
 *
 * Does not unregister scsi_disk; use @ref usb_msc_disk_detach for that.
 */
void usb_msc_scsi_unbind(struct scsi_device *sdev);

/** Callback for @ref usb_msc_scsi_with_lun */
typedef int (*usb_msc_scsi_lun_fn)(struct scsi_device *sdev, void *ctx);

/**
 * @brief Run a mid-layer operation on a USB MSC LUN
 *
 * Uses an existing LUN when present; otherwise allocates a temporary slot.
 */
int usb_msc_scsi_with_lun(const struct device *uhc, struct usb_device *udev,
			  const struct usbh_msc_iface *msc, uint8_t lun, usb_msc_scsi_lun_fn fn,
			  void *ctx);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_USB_USB_MSC_SCSI_H_ */
