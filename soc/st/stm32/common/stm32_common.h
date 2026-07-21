/*
 * Copyright (c) 2025 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/toolchain.h>

/**
 * @brief Performs the entry-in-poweroff sequence.
 *
 * This function is reserved for internal usage by
 * the SoC-specific z_sys_poweroff() implementations.
 *
 * It merely performs the architectural low-power entry
 * sequence: the caller must configure the PWR registers
 * with the proper low-power mode, clear all wake-up flags
 * and possibly other operations (disable RAM retention,
 * turn off DBGMCU, ...) before calling this function.
 */
FUNC_NORETURN void stm32_enter_poweroff(void);
