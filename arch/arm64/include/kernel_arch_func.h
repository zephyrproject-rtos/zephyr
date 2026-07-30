/*
 * Copyright (c) 2019 Carlo Caione <ccaione@baylibre.com<
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Private kernel definitions (ARM64)
 *
 * This file contains private kernel function definitions and various
 * other definitions for the ARM Cortex-A processor architecture family.
 *
 * This file is also included by assembly language files which must #define
 * _ASMLANGUAGE before including this header file.  Note that kernel
 * assembly source files obtains structure offset values via "absolute symbols"
 * in the offsets.o module.
 */

#ifndef ZEPHYR_ARCH_ARM64_INCLUDE_KERNEL_ARCH_FUNC_H_
#define ZEPHYR_ARCH_ARM64_INCLUDE_KERNEL_ARCH_FUNC_H_

#include <kernel_arch_data.h>

#include <zephyr/platform/hooks.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _ASMLANGUAGE

static ALWAYS_INLINE void arch_kernel_init(void)
{
	soc_per_core_init_hook();
}

static inline void arch_switch(void *switch_to, void **switched_from)
{
	extern void z_arm64_context_switch(struct k_thread *new_thread,
					   struct k_thread *old);
	struct k_thread *new_thread = switch_to;
	struct k_thread *old = CONTAINER_OF(switched_from, struct k_thread,
					    switch_handle);

	z_arm64_context_switch(new_thread, old);
}

extern void z_arm64_fatal_error(unsigned int reason, struct arch_esf *esf);
extern void z_arm64_set_ttbr0(uint64_t ttbr0);
extern void z_arm64_mem_cfg_ipi(void);

#ifdef CONFIG_FPU_SHARING
void arch_flush_local_fpu(void);
void arch_flush_fpu_ipi(unsigned int cpu);
#endif

#ifdef CONFIG_ARM64_SAFE_EXCEPTION_STACK
void z_arm64_safe_exception_stack_init(void);
#endif

#ifdef CONFIG_DEBUG_COREDUMP_SMP_FREEZE_CPUS
/*
 * arch_coredump_freeze_other_cpus()/arch_coredump_thaw_other_cpus() are the
 * generic hooks (declared in kernel_arch_interface.h) implemented in smp.c.
 *
 * Retrieve the raw live snapshot captured for the given CPU index.
 * Returns false if that CPU was never successfully frozen (self, never
 * booted, or timed out) -- in which case *esf, *x19_x29, *sp and *thread
 * are left untouched. *sp is the pre-interrupt stack pointer computed at
 * capture time; it is NOT simply (*esf + sizeof(**esf)), since *esf points
 * into this snapshot's own storage, not the original stack.
 */
bool arm64_coredump_freeze_get_snapshot(unsigned int cpu,
					 const struct arch_esf **esf,
					 const uint64_t **x19_x29,
					 uintptr_t *sp,
					 struct k_thread **thread);
#endif /* CONFIG_DEBUG_COREDUMP_SMP_FREEZE_CPUS */

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_ARCH_ARM64_INCLUDE_KERNEL_ARCH_FUNC_H_ */
