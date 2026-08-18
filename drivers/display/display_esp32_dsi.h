/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Internal interface between the ESP32 MIPI DSI host and its scanout
 *        controller driver.
 */

#ifndef ZEPHYR_DRIVERS_DISPLAY_DISPLAY_ESP32_DSI_H_
#define ZEPHYR_DRIVERS_DISPLAY_DISPLAY_ESP32_DSI_H_

#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Start streaming the framebuffer to the DSI bridge.
 *
 * Called by the MIPI DSI host once a panel has attached, since the
 * framebuffer size depends on the pixel format the panel reports.
 *
 * @param dev DSI scanout controller device.
 * @param bits_per_pixel Bits per pixel the panel attached with.
 *
 * @retval 0 on success.
 * @retval -EINVAL if the panel format does not match the controller.
 * @retval -ENOMEM if a framebuffer could not be allocated.
 * @retval -EIO if the scanout DMA could not be set up.
 */
int display_esp32_dsi_start(const struct device *dev, uint32_t bits_per_pixel);

/**
 * @brief Stop the scanout and release the framebuffers.
 *
 * @param dev DSI scanout controller device.
 *
 * @retval 0 on success.
 */
int display_esp32_dsi_stop(const struct device *dev);

/**
 * @brief Address a fixed framebuffer by index.
 *
 * Unlike display_get_framebuffer(), which follows the buffer being drawn
 * into, this addresses one specific buffer. A renderer that draws one frame
 * while another is on screen needs the pair to stay put.
 *
 * @param dev DSI scanout controller device.
 * @param index Framebuffer index.
 *
 * @retval Pointer to the framebuffer, or NULL if the index is out of range.
 */
void *display_esp32_dsi_get_framebuffer_by_index(const struct device *dev, uint32_t index);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_DISPLAY_DISPLAY_ESP32_DSI_H_ */
