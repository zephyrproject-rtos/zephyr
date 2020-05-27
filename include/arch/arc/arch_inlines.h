/*
 * Copyright (c) 2014-2016 Wind River Systems, Inc.
 * Copyright (c) 2019 Stephanos Ioannidis <root@stephanos.io>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_XTENSA_ARCH_INLINES_H_
#define ZEPHYR_INCLUDE_ARCH_XTENSA_ARCH_INLINES_H_

#ifndef _ASMLANGUAGE

#include <kernel_structs.h>

#ifdef CONFIG_CPU_ARCV2
#include <arch/arc/v2/aux_regs.h>
#endif

static ALWAYS_INLINE _cpu_t *arch_curr_cpu(void)
{
#ifdef CONFIG_SMP
	uint32_t core;

	core = z_arc_v2_core_id();

	return &_kernel.cpus[core];
#else
	return &_kernel.cpus[0];
#endif /* CONFIG_SMP */
}

#endif /* !_ASMLANGUAGE */
#endif /* ZEPHYR_INCLUDE_ARCH_XTENSA_ARCH_INLINES_H_ */
