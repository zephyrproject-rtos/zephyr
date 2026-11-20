/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Cortex-A/R exception stack sizing (AArch32)
 *
 * Z_ARMV7_EXCEPTION_ENTRY_STACK_BYTES matches CFI_EXC_FRAME_SIZE in exc.S:
 * the stack consumed on the abort/undef stack before calling the C fault
 * handlers when CONFIG_USE_SWITCH is disabled.
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARM_CORTEX_A_R_EXCEPTION_STACK_H_
#define ZEPHYR_INCLUDE_ARCH_ARM_CORTEX_A_R_EXCEPTION_STACK_H_

#include <stddef.h>

#include <zephyr/sys/util.h>

#if (defined(CONFIG_CPU_AARCH32_CORTEX_A) || defined(CONFIG_CPU_AARCH32_CORTEX_R)) && \
	!defined(CONFIG_USE_SWITCH)

#include <zephyr/arch/arm/thread.h>
#include <zephyr/arch/arm/cortex_a_r/exception.h>

#if defined(CONFIG_EXTRA_EXCEPTION_INFO)
/*
 * Matches CFI_EXC_EXTRA_SIZE. extra_info is already in offsetof(arch_esf, sp).
 * _callee_saved is rounded up to 8 bytes to keep SP aligned, as in
 * EXC_CALLEE_SAVED_SPACE in exc.S.
 */
#define Z_ARMV7_EXCEPTION_EXTRA_STACK_BYTES ROUND_UP(sizeof(struct _callee_saved), 8U)
#define Z_ARMV7_EXCEPTION_CALLEE_GPR_BYTES 0U
#else
#define Z_ARMV7_EXCEPTION_EXTRA_STACK_BYTES 0U
/* r4-r11; matches EXC_CALLEE_GPR_SIZE in exc.S. */
#define Z_ARMV7_EXCEPTION_CALLEE_GPR_BYTES 32U
#endif

/*
 * Matches CFI_EXC_FRAME_SIZE in exc.S. Do not use sizeof(struct arch_esf):
 * with FPU_SHARING the struct may include tail padding that is not pushed.
 */
#define Z_ARMV7_EXCEPTION_ENTRY_STACK_BYTES                                          \
	(offsetof(struct arch_esf, sp) + Z_ARM_ESF_TRAILER_SIZE +                    \
	 Z_ARMV7_EXCEPTION_EXTRA_STACK_BYTES + Z_ARMV7_EXCEPTION_CALLEE_GPR_BYTES)

#endif /* Cortex-A/R && !USE_SWITCH */

#endif /* ZEPHYR_INCLUDE_ARCH_ARM_CORTEX_A_R_EXCEPTION_STACK_H_ */
