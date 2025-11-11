/*
 * Copyright (c) 2019 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_X86_INCLUDE_INTEL64_KERNEL_ARCH_FUNC_H_
#define ZEPHYR_ARCH_X86_INCLUDE_INTEL64_KERNEL_ARCH_FUNC_H_

#include <zephyr/kernel_structs.h>

#include <zephyr/platform/hooks.h>

#ifndef _ASMLANGUAGE

extern void z_x86_switch(void *switch_to, void **switched_from);

static inline void arch_switch(void *switch_to, void **switched_from)
{
	z_x86_switch(switch_to, switched_from);
}

/**
 * @brief Initialize scheduler IPI vector.
 *
 * Called in early BSP boot to set up scheduler IPI handling.
 */

extern void z_x86_ipi_setup(void);

static inline void arch_kernel_init(void)
{
#ifdef CONFIG_SOC_PER_CORE_INIT_HOOK
	soc_per_core_init_hook();
#endif /* CONFIG_SOC_PER_CORE_INIT_HOOK */
}

FUNC_NORETURN void z_x86_cpu_init(struct x86_cpuboot *cpuboot);

void x86_sse_init(struct k_thread *thread);

void z_x86_syscall_entry_stub(void);

bool z_x86_do_kernel_nmi(const struct arch_esf *esf);

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_ARCH_X86_INCLUDE_INTEL64_KERNEL_ARCH_FUNC_H_ */
