/*
 * Copyright (c) 2025 Yoan Dumas
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_MBOX_VIDEOCORE_IF_H_
#define ZEPHYR_DRIVERS_MBOX_VIDEOCORE_IF_H_

#include <stdint.h>

/**
 * @brief Get the VideoCore firmware version.
 * @return The firmware version as a 32-bit unsigned integer.
 */
uint32_t get_vc_firmware_version(void);

/**
 * @brief Get the board revision.
 * @return The board revision as a 32-bit unsigned integer.
 */
uint32_t get_board_revision(void);

#endif /* ZEPHYR_DRIVERS_MBOX_VIDEOCORE_IF_H_ */
