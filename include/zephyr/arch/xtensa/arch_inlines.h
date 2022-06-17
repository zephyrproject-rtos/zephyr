/*
 * Copyright (c) 2016 Cadence Design Systems, Inc.
 * Copyright (c) 2019 Stephanos Ioannidis <root@stephanos.io>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_XTENSA_ARCH_INLINES_H_
#define ZEPHYR_INCLUDE_ARCH_XTENSA_ARCH_INLINES_H_

#ifndef _ASMLANGUAGE

#include <zephyr/kernel_structs.h>
#include <zsr.h>

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

	cpu = (_cpu_t *)RSR(ZSR_CPU_STR);

	return cpu;
}

static ALWAYS_INLINE uint32_t arch_proc_id(void)
{
	uint32_t prid;

	__asm__ volatile("rsr %0, PRID" : "=r"(prid));
	return prid;
}

#endif /* !_ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_XTENSA_ARCH_INLINES_H_ */
