/*
 * Copyright 2024 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_INPUT_KEYMAP_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_INPUT_KEYMAP_H_

#define MATRIX_KEY(row, col, code) \
	((((row) & 0xff) << 24) | (((col) & 0xff) << 16) | ((code) & 0xffff))

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_INPUT_KEYMAP_H_ */
