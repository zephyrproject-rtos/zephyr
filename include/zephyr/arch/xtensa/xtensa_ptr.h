/*
 * Copyright (c) 2026 Google LLC
 * Copyright (c) 2026 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

 #include <stdint.h>

 #ifndef ZEPHYR_INCLUDE_ARCH_XTENSA_XTENSA_PTR_H
 #define ZEPHYR_INCLUDE_ARCH_XTENSA_XTENSA_PTR_H

/**
 * @defgroup xtensa_ptr_apis Xtensa Pointer Validating APIs
 * @ingroup xtensa_apis
 * @{
 */

/**
 * @brief Checks whether the given sp value is a valid stack address
 *
 * @param stack address value
 *
 * @return
 *      - True if the address parameter is valid for stack
 *      - False otherwise
 */
bool xtensa_soc_stack_ptr_is_sane(uint32_t sp);

/**
 * @brief Checks whether the given pointer is executable
 *
 * @param pointer
 *
 * @return
 *      - True if the pointer is not executable
 *      - False otherwise
 */
bool xtensa_soc_ptr_is_executable(const void *p);

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_ARCH_XTENSA_XTENSA_PTR_H */
