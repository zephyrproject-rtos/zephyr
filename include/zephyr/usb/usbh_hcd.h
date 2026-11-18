/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
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
#include <zephyr/drivers/usb/uhc.h>
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
 *
 * @param udev USB device whose endpoints were cleared.
 *
 * @retval 0 Endpoint programming resynced.
 * @retval -EINVAL Invalid argument.
 * @retval -ENOTSUP HCD does not implement the hook.
 */
int usbh_ep_sync_after_clear_feature(struct usb_device *udev);

/**
 * @brief Recover an endpoint after a USB-level recovery event.
 *
 * @param udev USB device
 * @param ep Endpoint address (0 where the HCD applies device-wide recovery)
 * @param reason Recovery reason
 *
 * @retval 0 Endpoint ready for a new transfer
 * @retval -ENODEV Device disconnected
 * @retval negative errno Recovery failed
 */
int usbh_endpoint_recover(struct usb_device *udev, uint8_t ep, enum uhc_ep_recovery_reason reason);

/**
 * @brief Ask the HCD whether configured endpoints appear steady.
 *
 * @param udev USB device to verify.
 *
 * @retval 0 Endpoints appear steady.
 * @retval -EINVAL Invalid argument.
 * @retval -EIO Endpoints not steady or HCD reported failure.
 * @retval -ENOTSUP HCD does not implement the hook.
 */
int usbh_eps_verify_steady(struct usb_device *udev);

/**
 * @brief Query whether endpoint programming is steady for @a udev.
 *
 * @param udev USB device that completed configuration.
 *
 * @retval true Steady, or the HCD has no hook (assumed steady).
 * @retval false Not steady, disconnected, or invalid argument.
 */
bool usbh_post_configure_steady(const struct usb_device *udev);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_USBH_HCD_H_ */
