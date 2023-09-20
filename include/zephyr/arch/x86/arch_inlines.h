/*
 * Copyright (c) 2019 Intel Corporation
 * Copyright (c) 2019 Stephanos Ioannidis <root@stephanos.io>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_X86_ARCH_INLINES_H_
#define ZEPHYR_INCLUDE_ARCH_X86_ARCH_INLINES_H_

#ifndef _ASMLANGUAGE

#if defined(CONFIG_X86_64)

#include <zephyr/arch/x86/intel64/thread.h>
#include <zephyr/kernel_structs.h>
#include <zephyr/arch/x86/arch.h>

static inline struct _cpu *arch_curr_cpu(void)
{
	struct _cpu *cpu;

	__asm__ volatile("movq %%gs:(%c1), %0"
			 : "=r" (cpu)
			 : "i" (offsetof(x86_tss64_t, cpu)));

	return cpu;
}

static ALWAYS_INLINE uint32_t arch_proc_id(void)
{
	/*
	 * Placeholder implementation to be replaced with an architecture
	 * specific call to get processor ID
	 */
	return arch_curr_cpu()->id;
}

static ALWAYS_INLINE struct x86_cpu_info *arch_cpu_info(void)
{
	struct _cpu_arch *arch = &arch_curr_cpu()->arch;

	return &arch->info;
}

static ALWAYS_INLINE  struct x86_cpu_info *arch_cpu_info_get(uint8_t cpu_id)
{
	return z_x86_cpu_info_get(cpu_id);
}

#endif /* CONFIG_X86_64 */

static ALWAYS_INLINE unsigned int arch_num_cpus(void)
{
	return CONFIG_MP_MAX_NUM_CPUS;
}

#endif /* !_ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_X86_ARCH_INLINES_H_ */
