/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief USB host controller helpers for class drivers
 *
 * Class and transport code should use these helpers instead of calling UHC
 * optional hooks directly.
 *
 * @since 4.3
 */

#ifndef ZEPHYR_INCLUDE_USBH_HCD_H_
#define ZEPHYR_INCLUDE_USBH_HCD_H_

#include <stdbool.h>
#include <zephyr/device.h>
#include <zephyr/usb/usbh.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Return the UHC device for a USB device.
 */
static inline const struct device *usbh_hcd_dev(const struct usb_device *udev)
{
	return ((const struct usbh_context *)udev->ctx)->dev;
}

/**
 * @brief Resync HCD endpoint programming after CLEAR_FEATURE(ENDPOINT_HALT).
 */
int usbh_ep_sync_after_clear_feature(struct usb_device *udev);

/**
 * @brief Ask the HCD whether configured endpoints appear steady.
 */
int usbh_eps_verify_steady(struct usb_device *udev);

/**
 * @brief Query HCD post-configure steady state.
 */
bool usbh_post_configure_steady(const struct usb_device *udev);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_USBH_HCD_H_ */
