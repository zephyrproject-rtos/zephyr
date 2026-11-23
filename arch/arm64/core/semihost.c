/*
 * Copyright (c) 2022, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/common/semihost.h>

long semihost_exec(enum semihost_instr instr, void *args)
{
	register unsigned long w0 __asm__ ("w0") = instr;
	register void *x1 __asm__ ("x1") = args;
	register long ret __asm__ ("x0");

	__asm__ volatile ("hlt 0xf000"
			  : "=r" (ret) : "r" (w0), "r" (x1) : "memory");
	return ret;
}
