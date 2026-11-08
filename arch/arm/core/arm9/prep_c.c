/*
 * Copyright (c) 2026 Takayuki Goto
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Full C support initialization for ARMv5 cores
 */

#include <zephyr/kernel.h>
#include <kernel_internal.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/platform/hooks.h>
#include <zephyr/arch/common/xip.h>
#include <zephyr/arch/common/init.h>
#include <arm9/stack.h>

extern void z_armv5_vector_table_init(void);

extern FUNC_NORETURN void z_cstart(void);

/**
 * @brief Prepare to and run C code
 */
FUNC_NORETURN void z_prep_c(void)
{
	soc_prep_hook();

	arch_bss_zero();
	arch_data_copy();

#if defined(CONFIG_INIT_STACKS)
	z_arm_init_stacks();
#endif

	z_armv5_vector_table_init();

	z_arm_interrupt_init();

	z_cstart();
	CODE_UNREACHABLE;
}
