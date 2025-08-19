/*
 * Copyright 2024 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup input_keymap
 * @brief Header file for keymap utilities.
 */

#ifndef ZEPHYR_INCLUDE_INPUT_INPUT_KEYMAP_H_
#define ZEPHYR_INCLUDE_INPUT_INPUT_KEYMAP_H_

/**
 * @defgroup input_keymap Keymap utilities
 * @ingroup input_interface
 * @{
 */

/**
 * @brief Extract the row index from a keymap entry.
 *
 * This macro extracts the row index from a 32-bit keymap entry. The row index
 * is stored in bits 24-31 of the keymap entry.
 *
 * @param keymap_entry The 32-bit keymap entry value.
 * @return The row index (0-255) extracted from the keymap entry.
 */
#define MATRIX_ROW(keymap_entry) (((keymap_entry) >> 24) & 0xff)

/**
 * @brief Extract the column index from a keymap entry.
 *
 * This macro extracts the column index from a 32-bit keymap entry. The column
 * index is stored in bits 16-23 of the keymap entry.
 *
 * @param keymap_entry The 32-bit keymap entry value.
 * @return The column index (0-255) extracted from the keymap entry.
 */
#define MATRIX_COL(keymap_entry) (((keymap_entry) >> 16) & 0xff)

/** @} */

#endif /* ZEPHYR_INCLUDE_INPUT_INPUT_KEYMAP_H_ */
