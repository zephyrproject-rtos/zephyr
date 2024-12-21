/*
 * Copyright (c) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_PLATFORM_PLATFORM_H_
#define ZEPHYR_INCLUDE_PLATFORM_PLATFORM_H_

/**
 * @file
 * @brief Soc and Board hooks
 *
 * This header file contains function prototypes for the interfaces between
 * zephyr architecture and initialization code and the SoC and board specific logic
 * that resides under boards/ and soc/
 *
 * @note These are all standard soc and board interfaces that are exported from
 * soc and board specific logic to OS internal logic.  These should never be accessed
 * directly from application code but may be freely used within the OS.
 */


/**
 * @brief SoC  hook executed at the beginning of the reset vector.
 *
 * This hook is implemented by the SoC and can be used to perform any
 * SoC-specific initialization.
 */
void soc_reset_hook(void);

/**
 * @brief SoC hook executed after the reset vector.
 *
 *
 * This hook is implemented by the SoC and can be used to perform any
 * SoC-specific initialization.
 */
void soc_prep_hook(void);

/*
 * @brief SoC hook executed before the kernel and devices are initialized.
 *
 * This hook is implemented by the SoC and can be used to perform any
 * SoC-specific initialization.
 */
void soc_early_init_hook(void);

/*
 * @brief SoC hook executed after the kernel and devices are initialized.
 *
 * This hook is implemented by the SoC and can be used to perform any
 * SoC-specific initialization.
 */
void soc_late_init_hook(void);

/*
 * @brief Board hook executed before the kernel starts.
 *
 * This is called before the kernel has started. This hook
 * is implemented by the board and can be used to perform any board-specific
 * initialization.
 */
void board_early_init_hook(void);

/*
 * @brief Board hook executed after the kernel starts.

 * This is called after the kernel has started, but before the main function is
 * called. This hook is implemented by the board and can be used to perform
 * any board-specific initialization.
 */
void board_late_init_hook(void);

#endif
