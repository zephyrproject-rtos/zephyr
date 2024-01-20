/*
 * Copyright 2024 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_INPUT_HID_H_
#define ZEPHYR_INCLUDE_INPUT_HID_H_

/**
 * @addtogroup input_interface
 * @{
 */

/**
 * @brief Convert an input code to HID code.
 *
 * Takes an input code as input and returns the corresponding HID code as
 * output. The return value is -1 if the code is not found, if found it can
 * safely be casted to a uint8_t type.
 *
 * @param input_code Event code (see @ref INPUT_KEY_CODES).
 * @retval the HID code corresponding to the input code.
 * @retval -1 if there's no HID code for the specified input code.
 */
int16_t input_to_hid_code(uint16_t input_code);

/**
 * @brief Convert an input code to HID modifier.
 *
 * Takes an input code as input and returns the corresponding HID modifier as
 * output or 0.
 *
 * @param input_code Event code (see @ref INPUT_KEY_CODES).
 * @retval the HID modifier corresponding to the input code.
 * @retval 0 if there's no HID modifier for the specified input code.
 */
uint8_t input_to_hid_modifier(uint16_t input_code);

/** @} */

#endif /* ZEPHYR_INCLUDE_INPUT_HID_H_ */
