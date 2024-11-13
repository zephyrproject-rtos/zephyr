/*
 * Copyright (c) 2019 Intel Corporation
 * Copyright (c) 2019 Stephanos Ioannidis <root@stephanos.io>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_X86_ARCH_INLINES_H_
#define ZEPHYR_INCLUDE_ARCH_X86_ARCH_INLINES_H_

#ifndef _ASMLANGUAGE

#include <zephyr/arch/x86/x86_acpi.h>

#if defined(CONFIG_X86_64)

#include <zephyr/arch/x86/intel64/thread.h>
#include <zephyr/kernel_structs.h>

static inline struct _cpu *arch_curr_cpu(void)
{
#ifdef CONFIG_VALIDATE_ARCH_CURR_CPU
	__ASSERT_NO_MSG(!z_smp_cpu_mobile());
#endif /* CONFIG_VALIDATE_ARCH_CURR_CPU */

#ifdef CONFIG_SMP
	struct _cpu *cpu;

	__asm__ volatile("movq %%gs:(%c1), %0"
			 : "=r" (cpu)
			 : "i" (offsetof(x86_tss64_t, cpu)));

	return cpu;
#else
	return &_kernel.cpus[0];
#endif /* CONFIG_SMP */
}

static ALWAYS_INLINE uint32_t arch_proc_id(void)
{
	/*
	 * Placeholder implementation to be replaced with an architecture
	 * specific call to get processor ID
	 */
	return arch_curr_cpu()->id;
}

#endif /* CONFIG_X86_64 */

static ALWAYS_INLINE unsigned int arch_num_cpus(void)
{
	return CONFIG_MP_MAX_NUM_CPUS;
}

#endif /* !_ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_X86_ARCH_INLINES_H_ */
