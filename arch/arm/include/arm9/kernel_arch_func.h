/*
 * Copyright (c) 2025-2026, Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Private kernel definitions (ARM)
 *
 * This file contains private kernel function definitions and various other
 * definitions for the 32-bit ARM9 processor architecture family.
 *
 * This file is also included by assembly language files which must #define
 * _ASMLANGUAGE before including this header file.  Note that kernel assembly
 * source files obtains structure offset values via "absolute symbols" in the
 * offsets.o module.
 */

#ifndef ZEPHYR_ARCH_ARM_INCLUDE_ARM9_KERNEL_ARCH_FUNC_H_
#define ZEPHYR_ARCH_ARM_INCLUDE_ARM9_KERNEL_ARCH_FUNC_H_

#include <zephyr/platform/hooks.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _ASMLANGUAGE

extern void z_arm_context_switch(struct k_thread *new, struct k_thread *old);

static ALWAYS_INLINE void arch_kernel_init(void)
{
	soc_per_core_init_hook();
}

static ALWAYS_INLINE void arch_switch(void *switch_to, void **switched_from)
{
	struct k_thread *new = switch_to;
	struct k_thread *old = CONTAINER_OF(switched_from, struct k_thread,
					    switch_handle);

	z_arm_context_switch(new, old);
}

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_ARCH_ARM_INCLUDE_ARM9_KERNEL_ARCH_FUNC_H_ */
