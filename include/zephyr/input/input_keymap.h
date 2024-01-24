/*
 * Copyright 2024 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_INPUT_INPUT_KEYMAP_H_
#define ZEPHYR_INCLUDE_INPUT_INPUT_KEYMAP_H_

#define MATRIX_ROW(keymap_entry) (((keymap_entry) >> 24) & 0xff)
#define MATRIX_COL(keymap_entry) (((keymap_entry) >> 16) & 0xff)

#endif /* ZEPHYR_INCLUDE_INPUT_INPUT_KEYMAP_H_ */
