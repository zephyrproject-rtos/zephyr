/*
 * Copyright (c) 2026 Google LLC
 * Copyright (c) 2026 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_XTENSA_XTENSA_PTR_H
#define ZEPHYR_INCLUDE_ARCH_XTENSA_XTENSA_PTR_H

#include <stdbool.h>
#include <stdint.h>

/**
 * @defgroup xtensa_ptr_apis Xtensa Pointer Validating APIs
 * @ingroup xtensa_apis
 * @{
 */

/**
 * @brief Checks whether the given sp value is a valid stack address
 *
 * @param sp Stack address value
 *
 * @return
 *      - True if the address parameter is valid for stack
 *      - False otherwise
 */
bool xtensa_soc_stack_ptr_is_sane(uint32_t sp);

/**
 * @brief Checks whether the given pointer is executable
 *
 * @param p Pointer to check
 *
 * @return
 *      - True if the pointer is executable
 *      - False otherwise
 */
bool xtensa_soc_ptr_executable(const void *p);

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_ARCH_XTENSA_XTENSA_PTR_H */
