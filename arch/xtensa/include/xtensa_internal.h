/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 * Copyright (c) 2016 Cadence Design Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_XTENSA_INCLUDE_XTENSA_INTERNAL_H_
#define ZEPHYR_ARCH_XTENSA_INCLUDE_XTENSA_INTERNAL_H_

#include <stdint.h>

#include <zephyr/arch/xtensa/exception.h>
#include <zephyr/arch/arch_interface.h>

/**
 * @ingroup xtensa_internal_apis
 * @{
 */

/**
 * @brief Dump and print out the stack frame content.
 *
 * This mainly prints out the registers stashed in the stack frame.
 *
 * @param stack Pointer to stack frame.
 */
void xtensa_dump_stack(const z_arch_esf_t *stack);

/**
 * @brief Get string description from an exception code.
 *
 * @param cause_code Exception code.
 *
 * @return String description.
 */
char *xtensa_exccause(unsigned int cause_code);

/**
 * @brief Called upon a fatal error.
 *
 * @param reason The reason for the fatal error
 * @param esf Exception context, with details and partial or full register
 *            state when the error occurred. May in some cases be NULL.
 */
void xtensa_fatal_error(unsigned int reason, const z_arch_esf_t *esf);

/**
 * @brief Perform a one-way transition from supervisor to user mode.
 *
 * @see arch_user_mode_enter
 */
void xtensa_userspace_enter(k_thread_entry_t user_entry,
			    void *p1, void *p2, void *p3,
			    uintptr_t stack_end,
			    uintptr_t stack_start);

/**
 * @}
 */

#endif /* ZEPHYR_ARCH_XTENSA_INCLUDE_XTENSA_INTERNAL_H_ */
