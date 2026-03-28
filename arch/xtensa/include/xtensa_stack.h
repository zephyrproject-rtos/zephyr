/*
 * Copyright (c) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_XTENSA_XTENSA_STACK_H_
#define ZEPHYR_ARCH_XTENSA_XTENSA_STACK_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <xtensa_asm2_context.h>

/**
 * @defgroup xtensa_stack_internal_apis Xtensa Stack related Internal APIs
 * @ingroup xtensa_stack_apis
 * @{
 */

/**
 * @brief Check if memory region is within correct stack boundaries.
 *
 * Check if the memory region [@a addr, (@a addr + @a sz)) is within
 * correct stack boundaries:
 * - Interrupt stack if servicing interrupts.
 * - Privileged stack if in kernel mode doing syscalls.
 * - Thread stack otherwise.
 *
 * @note When @ps == UINT32_MAX, it checks the whole range of stack
 *       object because we cannot get PS via frame pointer yet.
 *
 * @param addr Beginning address of memory region to check.
 * @param sz Size of memory region to check. Can be zero.
 * @param ps PS register value of interrupted context. Use UINT32_MAX if
 *           PS cannot be determined at time of call.
 *
 * @return True if memory region is outside stack bounds, false otherwise.
 */
bool xtensa_is_outside_stack_bounds(uintptr_t addr, size_t sz, uint32_t ps);

/**
 * @brief Check if frame pointer is within correct stack boundaries.
 *
 * Check if the frame pointer and its associated BSA (base save area) are
 * within correct stack boundaries. Use @ref xtensa_is_outside_stack_bounds
 * to determine validity.
 *
 * @param frame Frame Pointer. Cannot be NULL.
 */
bool xtensa_is_frame_pointer_valid(_xtensa_irq_stack_frame_raw_t *frame);

/**
 * @}
 */

#endif /* ZEPHYR_ARCH_XTENSA_XTENSA_STACK_H_ */
