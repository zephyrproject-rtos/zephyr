/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Private kernel definitions
 *
 * This file contains private kernel function/macro definitions and various
 * other definitions for the Nios II processor architecture.
 *
 * This file is also included by assembly language files which must #define
 * _ASMLANGUAGE before including this header file.  Note that kernel
 * assembly source files obtains structure offset values via "absolute
 * symbols" in the offsets.o module.
 */

#ifndef ZEPHYR_ARCH_NIOS2_INCLUDE_KERNEL_ARCH_FUNC_H_
#define ZEPHYR_ARCH_NIOS2_INCLUDE_KERNEL_ARCH_FUNC_H_

#include <kernel_arch_data.h>

#include <zephyr/platform/hooks.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _ASMLANGUAGE

static ALWAYS_INLINE void arch_kernel_init(void)
{
#ifdef CONFIG_SOC_PER_CORE_INIT_HOOK
	soc_per_core_init_hook();
#endif /* CONFIG_SOC_PER_CORE_INIT_HOOK */
}

static ALWAYS_INLINE void
arch_thread_return_value_set(struct k_thread *thread, unsigned int value)
{
	thread->callee_saved.retval = value;
}

FUNC_NORETURN void z_nios2_fatal_error(unsigned int reason,
				       const struct arch_esf *esf);

static inline bool arch_is_in_isr(void)
{
	return _kernel.cpus[0].nested != 0U;
}

int arch_swap(unsigned int key);

#ifdef CONFIG_IRQ_OFFLOAD
void z_irq_do_offload(void);
#endif

#if ALT_CPU_ICACHE_SIZE > 0
void z_nios2_icache_flush_all(void);
#else
#define z_nios2_icache_flush_all() do { } while (false)
#endif

#if ALT_CPU_DCACHE_SIZE > 0
void z_nios2_dcache_flush_all(void);
void z_nios2_dcache_flush_no_writeback(void *start, uint32_t len);
#else
#define z_nios2_dcache_flush_all() do { } while (false)
#define z_nios2_dcache_flush_no_writeback(x, y) do { } while (false)
#endif

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_ARCH_NIOS2_INCLUDE_KERNEL_ARCH_FUNC_H_ */
