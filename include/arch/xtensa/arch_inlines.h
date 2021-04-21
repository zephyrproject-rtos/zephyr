/*
 * Copyright (c) 2016 Cadence Design Systems, Inc.
 * Copyright (c) 2019 Stephanos Ioannidis <root@stephanos.io>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_XTENSA_ARCH_INLINES_H_
#define ZEPHYR_INCLUDE_ARCH_XTENSA_ARCH_INLINES_H_

#ifndef _ASMLANGUAGE

#include <kernel_structs.h>

#define RSR(sr) \
	({uint32_t v; \
	 __asm__ volatile ("rsr." sr " %0" : "=a"(v)); \
	 v; })

#define WSR(sr, v) \
	do { \
		__asm__ volatile ("wsr." sr " %0" : : "r"(v)); \
	} while (false)

static ALWAYS_INLINE _cpu_t *arch_curr_cpu(void)
{
	_cpu_t *cpu;

	cpu = (_cpu_t *)RSR(CONFIG_XTENSA_KERNEL_CPU_PTR_SR);

	return cpu;
}

#endif /* !_ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_XTENSA_ARCH_INLINES_H_ */
