/*
 * Copyright (c) 2022, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/util_macro.h>
#include <zephyr/arch/common/semihost.h>

#if !(defined(CONFIG_ISA_ARM) || defined(CONFIG_ISA_THUMB2))
#error Unsupported ISA
#endif

long semihost_exec(enum semihost_instr instr, void *args)
{
	register unsigned long r0 __asm__ ("r0") = instr;
	register void *r1 __asm__ ("r1") = args;
	register long ret __asm__ ("r0");

	if (IS_ENABLED(CONFIG_ISA_THUMB2)) {
		__asm__ __volatile__ ("svc 0xab"
				      : "=r" (ret) : "r" (r0), "r" (r1) : "memory");
	} else {
		__asm__ __volatile__ ("svc 0x123456"
				      : "=r" (ret) : "r" (r0), "r" (r1) : "memory");
	}
	return ret;
}
