/*
 * Copyright (c) 2020 Andes Technology Corporation
 * Copyright (c) 2020 Jim Shu <cwshu@andestech.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_RISCV_ARCH_INLINES_H_
#define ZEPHYR_INCLUDE_ARCH_RISCV_ARCH_INLINES_H_

#ifndef _ASMLANGUAGE

static ALWAYS_INLINE struct _cpu *arch_curr_cpu(void)
{
	struct _cpu *cpu;

	__asm__ volatile("csrr %0, mscratch" : "=r" (cpu));

	return cpu;
}

#endif /* !_ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_RISCV_ARCH_INLINES_H_ */
