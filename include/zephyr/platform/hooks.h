/*
 * Copyright (c) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_PLATFORM_PLATFORM_H_
#define ZEPHYR_INCLUDE_PLATFORM_PLATFORM_H_

/**
 * @file
 * @brief Platform hooks
 *
 * This header file contains function prototypes for the interfaces between
 * zephyr architecture and initialization code and the platform pecific logic
 * that resides under boards/ and soc/
 *
 * @note These are all standard platform interfaces that are exported from
 * platform specific logic to OS internal logic.  These should never be accessed
 * directly from application code but may be freely used within the OS.
 */


/**
 * @brief Platform hook executed at the beginning of the reset vector.
 *
 * This hook is implemented by the SoC and can be used to perform any
 * SoC-specific
 */
void platform_reset(void);

/**
 * @brief Platform hook executed after the reset vector.
 *
 *
 * This hook is implemented by the SoC and can be used to perform any
 * SoC-specific
 */
void platform_prep(void);

/*
 * @brief Platform hook executed before the kernel and devices are initialized.
 *
 * This hook is implemented by the SoC and can be used to perform any
 * SoC-specific
 */
void platform_early_init(void);

/*
 * @brief Platform hook executed after the kernel and devices are initialized.
 *
 * This hook is implemented by the SoC and can be used to perform any
 * SoC-specific
 */
void platform_late_init(void);

/*
 * @brief Additional platform hook executed before the kernel starts.
 *
 * This is called before the kernel has started. This hook
 * is implemented by the board and can be used to perform any board-specific
 * initialization.
 */
void board_early_init(void);

/*
 * @brief Additional platform hook executed after the kernel starts.

 * This is called after the kernel has started, but before the main. This hook
 * is implemented by the board and can be used to perform any board-specific
 * initialization.
 */
void board_late_init(void);

#endif
