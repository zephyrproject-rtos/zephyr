/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel_internal.h>
#include <zephyr/arch/common/init.h>
#include <zephyr/arch/common/xip.h>

void z_prep_c(void)
{
	arch_bss_zero();
	arch_data_copy();

	z_cstart();
	CODE_UNREACHABLE;
}
