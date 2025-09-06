/*
 * Copyright (c) 2019-2020 Cobham Gaisler AB
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Full C support initialization
 */
#include <zephyr/kernel.h>
#include <zephyr/platform/hooks.h>
#include <zephyr/arch/cache.h>
#include <zephyr/arch/common/xip.h>
#include <zephyr/arch/common/init.h>

/**
 * @brief Prepare to and run C code
 *
 * This routine prepares for the execution of and runs C code.
 */

FUNC_NORETURN void z_prep_c(void)
{
#if defined(CONFIG_SOC_PREP_HOOK)
	soc_prep_hook();
#endif
	arch_data_copy();
#if CONFIG_ARCH_CACHE
	arch_cache_init();
#endif
	z_cstart();
	CODE_UNREACHABLE;
}
