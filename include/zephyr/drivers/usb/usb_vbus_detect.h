/*
 * Copyright (c) 2026 Leica Geosystems AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief USB VBUS detection API
 *
 * This API provides functions for VBUS detection drivers to notify
 * the USB device controller of VBUS state changes.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_USB_USB_VBUS_DETECT_H_
#define ZEPHYR_INCLUDE_DRIVERS_USB_USB_VBUS_DETECT_H_

#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Notify USB stack that VBUS is ready.
 *
 * Called by VBUS detection drivers when VBUS voltage reaches
 * a stable valid level.
 *
 * @param udc_dev Pointer to USB device controller device.
 * @return 0 on success, negative errno on failure.
 * @retval -EPERM if controller is not initialized.
 */
int usb_vbus_ready(const struct device *udc_dev);

/**
 * @brief Notify USB stack that VBUS was removed.
 *
 * Called by VBUS detection drivers when VBUS voltage drops
 * below the valid threshold.
 *
 * @param udc_dev Pointer to USB device controller device.
 * @return 0 on success, negative errno on failure.
 * @retval -EPERM if controller is not initialized.
 */
int usb_vbus_removed(const struct device *udc_dev);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_USB_USB_VBUS_DETECT_H_ */
