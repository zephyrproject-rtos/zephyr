/*
 * Copyright (c) 2022, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/common/semihost.h>

long semihost_exec(enum semihost_instr instr, void *args)
{
	register unsigned long a0 __asm__ ("a0") = instr;
	register void *a1 __asm__ ("a1") = args;
	register long ret __asm__ ("a0");

	__asm__ volatile (
		".option push\n\t"
		".option norvc\n\t"
		"slli zero, zero, 0x1f\n\t"
		"ebreak\n\t"
		"srai zero, zero, 0x7\n\t"
		".option pop"
		: "=r" (ret) : "r" (a0), "r" (a1) : "memory");

	return ret;
}
