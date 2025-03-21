/*
 * Copyright (c) 2022, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/common/semihost.h>

long semihost_exec(enum semihost_instr instr, void *args)
{
	register unsigned int r0 __asm__("r0") = instr;
	register void *r1 __asm__("r1") = args;
	register int ret __asm__("r0");

	__asm__ volatile("bkpt 0xab" : "=r"(ret) : "r"(r0), "r"(r1) : "memory");
	return ret;
}
