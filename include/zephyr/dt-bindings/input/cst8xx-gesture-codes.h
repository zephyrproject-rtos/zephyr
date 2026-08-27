/*
 * Copyright (c) 2024 Felipe Neves.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Gesture code definitions for Hynitron CST8xx touch controllers
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_INPUT_CST8XX_GESTURE_CODES_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_INPUT_CST8XX_GESTURE_CODES_H_

/**
 * @brief No gesture detected
 */
#define CST8XX_GESTURE_CODE_NONE         0x00

/**
 * @brief Swipe up gesture
 */
#define CST8XX_GESTURE_CODE_SWIPE_UP     0x01

/**
 * @brief Swipe down gesture
 */
#define CST8XX_GESTURE_CODE_SWIPE_DOWN   0x02

/**
 * @brief Swipe left gesture
 */
#define CST8XX_GESTURE_CODE_SWIPE_LEFT   0x03

/**
 * @brief Swipe right gesture
 */
#define CST8XX_GESTURE_CODE_SWIPE_RIGHT  0x04

/**
 * @brief Single tap / click gesture
 */
#define CST8XX_GESTURE_CODE_SINGLE_CLICK 0x05

/**
 * @brief Double tap / click gesture
 */
#define CST8XX_GESTURE_CODE_DOUBLE_CLICK 0x0B

/**
 * @brief Long press gesture
 */
#define CST8XX_GESTURE_CODE_LONG_PRESS   0x0C

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_INPUT_CST8XX_GESTURE_CODES_H_ */
