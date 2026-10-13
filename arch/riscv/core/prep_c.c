/*
 * Copyright (c) 2016 Jean-Paul Etienne <fractalclone@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Full C support initialization
 *
 *
 * Initialization of full C support: zero the .bss and call z_cstart().
 *
 * Stack is available in this module, but not the global data/bss until their
 * initialization is performed.
 */

#include <kernel_arch_func.h>
#include <stddef.h>
#include <zephyr/toolchain.h>
#include <zephyr/kernel_structs.h>
#include <zephyr/platform/hooks.h>
#include <zephyr/arch/cache.h>
#include <zephyr/arch/common/xip.h>
#include <zephyr/arch/common/init.h>

#if defined(CONFIG_RISCV_SOC_INTERRUPT_INIT)
void soc_interrupt_init(void);
#endif

/**
 *
 * @brief Prepare to and run C code
 *
 * This routine prepares for the execution of and runs C code.
 */

FUNC_NORETURN void z_prep_c(void)
{
	soc_prep_hook();

	arch_bss_zero();
	arch_data_copy();
#if defined(CONFIG_RISCV_SOC_INTERRUPT_INIT)
	soc_interrupt_init();
#endif
#if CONFIG_ARCH_CACHE
	arch_cache_init();
#endif
#ifdef CONFIG_RISCV_MMU
	if (z_riscv_mmu_init() != 0) {
		k_panic();
	}
#endif
	z_cstart();
	CODE_UNREACHABLE;
}
