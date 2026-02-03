/*
 * Copyright (C) 2025 Microchip Technology Inc. and its subsidiaries
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel_internal.h>
#include <zephyr/arch/cache.h>
#include <zephyr/arch/common/init.h>
#include <zephyr/arch/common/xip.h>
#include <zephyr/platform/hooks.h>

#if defined(CONFIG_INIT_STACKS)
#include <arm9/stack.h>
#endif

#if defined(CONFIG_ARM_AARCH32_MMU)
extern int z_arm_mmu_init(void);
#endif

extern FUNC_NORETURN void z_cstart(void);

/**
 *
 * @brief Prepare to and run C code
 *
 * This routine prepares for the execution of and runs C code.
 *
 */
void z_prep_c(void)
{
#if defined(CONFIG_SOC_PREP_HOOK)
	soc_prep_hook();
#endif

	arch_bss_zero();
	arch_data_copy();

#if defined(CONFIG_INIT_STACKS)
	z_arm_init_stacks();
#endif

#if CONFIG_ARCH_CACHE
	arch_cache_init();
#endif

#if defined(CONFIG_ARM_AARCH32_MMU)
	z_arm_mmu_init();
#endif

	z_cstart();
	CODE_UNREACHABLE;
}
