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
 * Assigns the lowest unused volume name across all attached MSC devices:
 * LUN0 on the first stick is @kconfig:option:`CONFIG_USBH_MSC_DISK_NAME` (e.g.
 * ``USB``), the next MSC LUN on any port becomes ``USB1``, then ``USB2``, and
 * so on (up to @kconfig:option:`CONFIG_USBH_MSC_LUN_SLOTS`).
 */
int usb_msc_disk_attach_lun(const struct device *uhc, struct usb_device *udev,
			    const struct usbh_msc_iface *msc, uint8_t lun);

/**
 * @brief Format a global MSC volume name by index (0 → ``USB``, 1 → ``USB1``, …)
 *
 * @return Length written, or negative errno
 */
int usb_msc_disk_format_volume_name(unsigned int vol_index, char *buf, size_t buflen);

/**
 * @brief Return the disk_access name for an attached @a udev + @a lun
 *
 * @return Length written, -ENOENT when not attached, other negative errno on error
 */
int usb_msc_disk_get_volume_name(struct usb_device *udev, uint8_t lun, char *buf, size_t buflen);

/**
 * @brief Format disk_access names for every LUN attached on @a udev
 *
 * Multiple names are comma-separated (e.g. ``USB,USB1``).
 *
 * @return Length written, -ENOENT when no volumes are attached, other negative errno on error
 */
int usb_msc_disk_format_device_volumes(struct usb_device *udev, char *buf, size_t buflen);

/**
 * @brief Attach LUN0 using the next free global volume name
 *
 * Same naming as @ref usb_msc_disk_attach_lun (``USB``, ``USB1``, …).
 */
int usb_msc_disk_attach_lun0(const struct device *uhc, struct usb_device *udev,
			     const struct usbh_msc_iface *msc);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_USB_USB_MSC_DISK_H_ */
