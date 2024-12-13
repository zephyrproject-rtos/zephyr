/*
 * Copyright (c) 2025 tinyVision.ai Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief USB Video Class (UVC) public header
 */

#ifndef ZEPHYR_INCLUDE_USB_CLASS_USBD_UVC_H
#define ZEPHYR_INCLUDE_USB_CLASS_USBD_UVC_H

#include <zephyr/device.h>

/**
 * @brief USB Video Class (UVC) device API
 * @defgroup usbd_uvc USB Video Class (UVC) device API
 * @ingroup usb
 * @since 4.2
 * @version 0.1.0
 * @see uvc: "Universal Serial Bus Device Class Definition for Video Devices"
 *      Document Release 1.5 (August 9, 2012)
 * @{
 */

/**
 * @brief Set the video device that a UVC instance will use.
 *
 * It will query its supported controls, formats and frame rates, and use this information to
 * generate USB descriptors sent to the host.
 *
 * At runtime, it will forward all USB controls from the host to this device.
 *
 * @note This function must be called before @ref usbd_enable.
 *
 * @param uvc_dev The UVC device
 * @param video_dev The video device that this UVC instance controls
 */
void uvc_set_video_dev(const struct device *uvc_dev, const struct device *video_dev);

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_USB_CLASS_USBD_UVC_H */
