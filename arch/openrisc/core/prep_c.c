/*
 * Copyright (c) 2025 NVIDIA Corporation <jholdsworth@nvidia.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Full C support initialization
 */

#include <kernel_internal.h>
#include <zephyr/platform/hooks.h>
#include <zephyr/arch/cache.h>
#include <zephyr/arch/common/init.h>
#include <zephyr/arch/common/xip.h>

/**
 *
 * @brief Prepare to and run C code
 *
 * This routine prepares for the execution of and runs C code.
 *
 * @return N/A
 */

void z_prep_c(void)
{
#if CONFIG_ARCH_CACHE
	arch_cache_init();
#endif

	soc_prep_hook();

	arch_bss_zero();
	arch_data_copy();

	z_cstart();
	CODE_UNREACHABLE;
}
