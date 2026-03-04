/*
 * Copyright (c) 2026 Shankar Sridhar
 * SPDX-License-Identifier: Apache-2.0
 *
 * Arduino UNO Q LED Matrix Driver API
 */

#ifndef ZEPHYR_DRIVERS_LED_MATRIX_UNO_Q_H_
#define ZEPHYR_DRIVERS_LED_MATRIX_UNO_Q_H_

#include <zephyr/device.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Arduino UNO Q LED Matrix dimensions */
#define UNO_Q_MATRIX_ROWS    8
#define UNO_Q_MATRIX_COLS    13
#define UNO_Q_MATRIX_PIXELS  (UNO_Q_MATRIX_ROWS * UNO_Q_MATRIX_COLS)

/**
 * @brief Initialize the LED matrix
 *
 * @param dev Pointer to the device structure
 * @return 0 on success, negative errno otherwise
 */
int uno_q_ledmatrix_init(const struct device *dev);

/**
 * @brief Write frame buffer to LED matrix
 *
 * The frame buffer uses packed binary format:
 * - 104 LEDs total (8 rows × 13 columns)
 * - Data is packed into uint32_t words
 * - Each LED can be 0 (off) or 1 (on) for binary mode
 * - For grayscale mode, each LED uses 8 bits (0-255)
 *
 * @param dev Pointer to the device structure
 * @param buffer Frame buffer data (binary: 13 uint32_t words, grayscale: 104 uint8_t)
 * @param len Length of buffer in bytes
 * @param grayscale true for 8-bit grayscale, false for binary
 * @return 0 on success, negative errno otherwise
 */
int uno_q_ledmatrix_write(const struct device *dev, const void *buffer, 
							size_t len, bool grayscale);

/**
 * @brief Set a single pixel
 *
 * @param dev Pointer to the device structure
 * @param row Row index (0-7)
 * @param col Column index (0-12)
 * @param value Pixel value (0-255 for grayscale, 0-1 for binary)
 * @return 0 on success, negative errno otherwise
 */
int uno_q_ledmatrix_set_pixel(const struct device *dev, uint8_t row, 
							   uint8_t col, uint8_t value);

/**
 * @brief Clear the entire matrix
 *
 * @param dev Pointer to the device structure
 * @return 0 on success, negative errno otherwise
 */
int uno_q_ledmatrix_clear(const struct device *dev);

/**
 * @brief Set brightness level
 *
 * @param dev Pointer to the device structure
 * @param brightness Brightness level (0-255)
 * @return 0 on success, negative errno otherwise
 */
int uno_q_ledmatrix_set_brightness(const struct device *dev, uint8_t brightness);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_LED_MATRIX_UNO_Q_H_ */

/**
 * @brief Display a bitmap pattern
 * 
 * @param dev LED matrix device
 * @param bitmap 8x13 array of pixel values (0=off, non-zero=on)
 * @return 0 on success, negative errno on failure
 */
int uno_q_ledmatrix_display_bitmap(const struct device *dev,
									const uint8_t bitmap[8][13]);
