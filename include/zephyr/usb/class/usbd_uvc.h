/*
 * Copyright (c) 2025 tinyVision.ai Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief USB Device Firmware Upgrade (DFU) public header
 *
 * Header exposes API for registering DFU images.
 */

#ifndef ZEPHYR_INCLUDE_USB_CLASS_USBD_UVC_H
#define ZEPHYR_INCLUDE_USB_CLASS_USBD_UVC_H

#include <zephyr/kernel.h>

void uvc_set_video_dev(const struct device *const dev, const struct device *const video_dev);

#endif /* ZEPHYR_INCLUDE_USB_CLASS_USBD_UVC_H */
