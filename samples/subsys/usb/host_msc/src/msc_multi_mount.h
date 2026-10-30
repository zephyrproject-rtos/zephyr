/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef USB_HOST_MSC_MULTI_MOUNT_H_
#define USB_HOST_MSC_MULTI_MOUNT_H_

#include <zephyr/drivers/usb/uhc.h>

/**
 * @brief Mount one MSC disk_access volume at /\<disk\>:
 *
 * @param disk_name disk_access name (e.g. ``USB``, ``USB1``)
 * @return 0 on success, negative errno on failure
 */
int msc_fs_mount_disk(const char *disk_name);

/**
 * @brief Unmount /\<disk\>: if mounted by this helper
 *
 * @param disk_name disk_access name without colon
 * @return 0 on success, -ENOENT when not mounted, other negative errno on failure
 */
int msc_fs_unmount_disk(const char *disk_name);

/** @brief Unmount every MSC volume attached to @a udev */
void msc_fs_unmount_udev(struct usb_device *udev);

#endif /* USB_HOST_MSC_MULTI_MOUNT_H_ */
