/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

/*
 * Relocated to SRAM2. dup_lib/reloc_dup.c has the same file name and must
 * stay in flash.
 *
 * The section is set explicitly so that the variable is not placed in a
 * small data section (e.g. .sdata on RISC-V).
 */
__in_section(data, sram2, var) uint32_t reloc_dup_app_data = 10U;

void reloc_dup_app_func(void)
{
}
