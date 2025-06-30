/*
 * Copyright (c) 2025, Microchip Technology Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel_internal.h>
#include <zephyr/irq.h>
#include <zephyr/platform/hooks.h>
#include <zephyr/arch/cache.h>

void z_prep_c(void);
/**
 * @brief Prepare to and run C code
 *
 * This routine prepares for the execution of and runs C code.
 */

void z_prep_c(void)
{
#if defined(CONFIG_SOC_PREP_HOOK)
	soc_prep_hook();
#endif
	z_cstart();
	CODE_UNREACHABLE;
}
