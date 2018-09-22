/* Copyright (c) 2017 dXplore
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DRIVERS_DISPLAY_ILI9340_H_
#define ZEPHYR_INCLUDE_DRIVERS_DISPLAY_ILI9340_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <device.h>

/**
 * @brief Write pixel to given color
 *
 * @param dev Pointer to device structure
 * @param x x coordinate
 * @param y y coordinate
 * @param r Value for red
 * @param g Value for green
 * @param b Value for blue
 */
void ili9340_write_pixel(const struct device *dev, const u16_t x, const u16_t y,
			 const u8_t r, const u8_t g, const u8_t b);

/**
 * @brief Write bitmap
 *
 * @param dev Pointer to device structure
 * @param x x coordinate of the upper left corner
 * @param y y coordinate of the upper left corner
 * @param w width of the bitmap
 * @param h height of the bitmap
 * @param rgb_data pointer to the RGB data array, the data array should be at
 * minimum w * h * 3 bytes
 */
void ili9340_write_bitmap(const struct device *dev, const u16_t x,
			  const u16_t y, const u16_t w, const u16_t h,
			  const u8_t *rgb_data);

/**
 * @brief Turn display on
 *
 * @param dev Pointer to device structure
 */
void ili9340_display_on(struct  device *dev);

/**
 * @brief Turn display off
 *
 * @param dev Pointer to device structure
 */
void ili9340_display_off(struct device *dev);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_DISPLAY_ILI9340_H_ */
