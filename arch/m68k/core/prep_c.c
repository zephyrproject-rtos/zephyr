/*
 * Copyright (c) 2026 Dimitri Varpusvuori
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>

#include <zephyr/toolchain.h>
#include <zephyr/arch/common/init.h>
#include <zephyr/platform/hooks.h>

FUNC_NORETURN void z_prep_c(void)
{
	soc_prep_hook();
	arch_bss_zero();
	z_cstart();
	CODE_UNREACHABLE;
}
