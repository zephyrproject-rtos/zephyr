/*
 * Arduino UNO Q LED Matrix HAL Interface
 */

#ifndef UNO_Q_MATRIX_HAL_H
#define UNO_Q_MATRIX_HAL_H

#include <stdint.h>

/**
 * Initialize the LED matrix hardware
 * @return 0 on success, negative errno on failure
 */
int matrixBegin(void);

/**
 * Write packed framebuffer to hardware
 * @param packed Array of uint32_t containing packed LED states
 */
void matrixWriteBuffer(const uint8_t *buf, size_t len);

/**
 * Set individual pixel (alternative direct access)
 */
void matrixSetPixel(uint8_t row, uint8_t col, uint8_t value);

/**
 * Get individual pixel state
 */
uint8_t matrixGetPixel(uint8_t row, uint8_t col);

/**
 * Clear all LEDs
 */
void matrixClear(void);

/**
 * Turn on all LEDs
 */
void matrixFill(void);

/**
 * Blank the matrix
 */
void matrixBlanking(bool on);

#endif /* UNO_Q_MATRIX_HAL_H */
