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

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_USBH_HCD_H_ */
