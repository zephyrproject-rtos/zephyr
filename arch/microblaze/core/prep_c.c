/*
 * Copyright (c) 2023 Advanced Micro Devices, Inc. (AMD)
 * Copyright (c) 2023 Alp Sayin <alpsayin@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */



#include <zephyr/arch/cpu.h>
#include <zephyr/cache.h>
#include <zephyr/irq.h>
#include <zephyr/init.h>

#include <kernel_internal.h>

/**
 *
 * @brief Prepare to and run C code
 *
 * This routine prepares for the execution of and runs C code.
 *
 * @return N/A
 */

void _PrepC(void)
{
	microblaze_disable_interrupts();

#if defined(CONFIG_CACHE_MANAGEMENT)
#if defined(CONFIG_ICACHE)
	cache_instr_enable();
#endif
#if defined(CONFIG_DCACHE)
	cache_data_enable();
#endif
#endif

	z_bss_zero();

	z_cstart();
	CODE_UNREACHABLE;
}

/**
 *
 * @brief Re-enable interrupts after kernel is initialised
 *
 * @return 0
 */
static int interrupt_init_post_kernel(void)
{
	microblaze_enable_interrupts();
	return 0;
}

SYS_INIT(interrupt_init_post_kernel, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
