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

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _ASMLANGUAGE

void k_cpu_idle(void);
void k_cpu_atomic_idle(unsigned int key);

static ALWAYS_INLINE void kernel_arch_init(void)
{
	_kernel.irq_stack =
		K_THREAD_STACK_BUFFER(_interrupt_stack) + CONFIG_ISR_STACK_SIZE;
}

static ALWAYS_INLINE void
_set_thread_return_value(struct k_thread *thread, unsigned int value)
{
	thread->callee_saved.retval = value;
}

static inline void _IntLibInit(void)
{
	/* No special initialization of the interrupt subsystem required */
}

#define _is_in_isr() (_kernel.nested != 0)

#ifdef CONFIG_IRQ_OFFLOAD
void _irq_do_offload(void);
#endif

#if ALT_CPU_ICACHE_SIZE > 0
void _nios2_icache_flush_all(void);
#else
#define _nios2_icache_flush_all() do { } while (0)
#endif

#if ALT_CPU_DCACHE_SIZE > 0
void _nios2_dcache_flush_all(void);
void _nios2_dcache_flush_no_writeback(void *start, u32_t len);
#else
#define _nios2_dcache_flush_all() do { } while (0)
#define _nios2_dcache_flush_no_writeback(x, y) do { } while (0)
#endif

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_ARCH_NIOS2_INCLUDE_KERNEL_ARCH_FUNC_H_ */
