/*
 * Copyright (c) 2025 tinyVision.ai Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_VIDEO_EMUL_IMAGER_H_
#define ZEPHYR_DRIVERS_VIDEO_EMUL_IMAGER_H_

/**
 * @brief Acces the 1-line buffer written to by the image sensor driver
 *
 * This 1-line buffer simulates the MIPI link going between an image sensor and a video receiver
 * peripheral.
 *
 * @param dev Image sensor device to query the buffer from.
 * @param pitch Pointer filled with the size of this buffer in bytes.
 * @retval The 1-line buffer from this device.
 */
const uint8_t *video_emul_imager_get_linebuffer(const struct device *dev, size_t *pitch);

#endif /* ZEPHYR_DRIVERS_VIDEO_EMUL_IMAGER_H_ */
