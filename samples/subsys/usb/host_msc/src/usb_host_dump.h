/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief USB host MSC sample enumeration dump and MSC bring-up helpers
 */

#ifndef USB_HOST_DUMP_H_
#define USB_HOST_DUMP_H_

#include <stddef.h>
#include <stdint.h>

#include <zephyr/usb/usbh_msc.h>

/**
 * @brief Log configuration descriptor interfaces and endpoints after enumeration.
 *
 * @param udev USB device with valid @c cfg_desc
 */
void usb_host_dump_cfg_interfaces(struct usb_device *udev);

/**
 * @brief Attempt MSC capacity logging when a configured device is present.
 *
 * No-op unless the device is configured and MSC descriptors are available.
 *
 * @param udev USB device to inspect
 */
void usb_host_try_print_msc_capacity(struct usb_device *udev);

/**
 * @brief MSC storage bring-up via host stack (SCSI probe + optional LUN0 scsi_disk).
 *
 * Wrapper around @ref usbh_msc_storage_bringup() with default LUN0 attach when
 * @kconfig:option:`CONFIG_USBH_MSC_DISK_AUTO_ATTACH` is enabled.
 *
 * @param udev USB device
 * @param uhc_dev Host controller device for BOT recovery
 *
 * @retval 0 Bring-up succeeded
 * @retval negative errno from host stack bring-up
 */
int usb_host_msc_storage_bringup(struct usb_device *udev, const struct device *uhc_dev);

#if IS_ENABLED(CONFIG_USB_HOST_MSC_SAMPLE_FAT_FILE_DEMO)
/**
 * @brief Mount FAT on USB MSC, list root directory, and write a demo file.
 *
 * Requires prior disk access registration during storage bring-up.
 *
 * @retval 0 Demo completed
 * @retval negative errno from mount, directory listing, or file I/O
 */
int usb_host_msc_sample_fat_file_demo(void);

/**
 * @brief Mount the USB MSC FAT volume registered via disk_access.
 *
 * @retval 0 Volume mounted
 * @retval negative errno from disk init or fs_mount
 */
int usb_host_msc_sample_fat_mount_volume(void);

/**
 * @brief Unmount the USB MSC FAT volume.
 *
 * @retval 0 Volume unmounted
 * @retval negative errno from fs_unmount
 */
int usb_host_msc_sample_fat_unmount_volume(void);

/**
 * @brief Mount point string (e.g. "/USB:").
 */
const char *usb_host_msc_sample_fat_mount_point(void);
#endif

#if IS_ENABLED(CONFIG_USB_HOST_MSC_SAMPLE_LARGE_FILE_WRITE)
/**
 * @brief Create and write a large text file on the USB MSC FAT volume.
 *
 * Mounts FAT, writes CONFIG_USB_HOST_MSC_SAMPLE_LARGE_FILE_SIZE_MB megabytes,
 * verifies size with fs_stat, syncs, and unmounts.
 *
 * @retval 0 Write completed
 * @retval negative errno from mount or file I/O
 */
int usb_host_msc_sample_write_large_text_file(void);
#endif

#endif /* USB_HOST_DUMP_H_ */
