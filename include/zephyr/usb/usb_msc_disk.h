/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief USB MSC LUN volume (transport bind, SCSI probe, optional disk attach)
 *
 * One @ref usb_msc_lun object owns the MSC transport binding,
 * @ref scsi_device, and optional @ref scsi_disk registration.
 *
 * @since 4.3
 */

#ifndef ZEPHYR_INCLUDE_USB_USB_MSC_DISK_H_
#define ZEPHYR_INCLUDE_USB_USB_MSC_DISK_H_

#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/drivers/disk/scsi_disk.h>
#include <zephyr/scsi/scsi.h>
#include <zephyr/usb/usbh_msc.h>

#ifdef __cplusplus
extern "C" {
#endif

/** LUN object lifecycle (single registry slot) */
enum usb_msc_lun_state {
	/** Slot unused */
	USB_MSC_LUN_FREE = 0,
	/** Transport bound; probe or pass-through I/O only */
	USB_MSC_LUN_BOUND,
	/** scsi_disk registered and exposed via disk_access */
	USB_MSC_LUN_DISK,
};

/**
 * @brief One USB MSC LUN (transport + scsi_device [+ scsi_disk])
 *
 * Slots are managed internally; use @ref usb_msc_disk_attach_lun or the
 * pass-through helpers in @ref usb_msc_scsi.h.
 */
struct usb_msc_lun {
	enum usb_msc_lun_state state;
	struct scsi_device sdev;
	struct scsi_disk disk;
	struct usb_device *udev;
	const struct device *uhc;
	struct usbh_msc_iface msc;
	uint8_t lun;
	char disk_name[16];
};

/** @deprecated Use @ref usb_msc_lun — same structure, kept for compatibility */
typedef struct usb_msc_lun usb_msc_disk;

/**
 * @brief Attach one USB MSC LUN as a disk_access volume
 *
 * @param lun Caller-owned LUN context from the internal pool (must remain valid
 *            until detach)
 * @param uhc USB host controller device
 * @param udev USB device
 * @param msc Resolved MSC interface
 * @param lun_id Target LUN
 * @param disk_name disk_access / FatFs volume name
 *
 * @return 0 on success, negative errno on failure
 */
int usb_msc_disk_attach(struct usb_msc_lun *lun, const struct device *uhc, struct usb_device *udev,
			const struct usbh_msc_iface *msc, uint8_t lun_id, const char *disk_name);

/** @brief Detach a volume previously attached with @ref usb_msc_disk_attach */
void usb_msc_disk_detach(struct usb_msc_lun *lun);

/**
 * @brief Detach the volume registered for @a udev and @a lun
 *
 * When @a udev is NULL, detaches the first attached volume matching @a lun.
 *
 * @return 0 on success, -ENOENT when no matching volume is attached
 */
int usb_msc_disk_detach_udev(struct usb_device *udev, uint8_t lun);

/**
 * @brief Detach all scsi_disk volumes for a removed USB device
 *
 * Called from the USB host stack when the device disconnects.
 */
void usb_msc_disk_device_removed(struct usb_device *udev);

/**
 * @brief Attach one LUN as a disk_access volume (auto-generated name)
 *
 * LUN0 uses @kconfig:option:`CONFIG_USBH_MSC_DISK_NAME`; LUN>0 uses
 * ``<name><lun>`` (e.g. ``USB1``).
 */
int usb_msc_disk_attach_lun(const struct device *uhc, struct usb_device *udev,
			    const struct usbh_msc_iface *msc, uint8_t lun);

/**
 * @brief Attach LUN0 using Kconfig defaults (@c CONFIG_USBH_MSC_DISK_NAME)
 *
 * Used from @ref usbh_msc_storage_bringup when auto-attach is enabled.
 */
int usb_msc_disk_attach_lun0(const struct device *uhc, struct usb_device *udev,
			     const struct usbh_msc_iface *msc);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_USB_USB_MSC_DISK_H_ */
