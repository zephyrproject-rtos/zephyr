/*
 * Copyright (c) 2014-2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Private kernel definitions
 *
 * This file contains private kernel structures definitions and various
 * other definitions for the ARCv2 processor architecture.
 *
 * This file is also included by assembly language files which must #define
 * _ASMLANGUAGE before including this header file.  Note that kernel
 * assembly source files obtains structure offset values via "absolute
 * symbols" in the offsets.o module.
 */

#ifndef ZEPHYR_ARCH_ARC_INCLUDE_KERNEL_ARCH_FUNC_H_
#define ZEPHYR_ARCH_ARC_INCLUDE_KERNEL_ARCH_FUNC_H_

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(_ASMLANGUAGE)

#ifdef CONFIG_CPU_ARCV2
#include <v2/cache.h>
#include <v2/irq.h>
#endif

static ALWAYS_INLINE void kernel_arch_init(void)
{
	_irq_setup();
}

static ALWAYS_INLINE void
_set_thread_return_value(struct k_thread *thread, unsigned int value)
{
	thread->arch.return_value = value;
}

/**
 *
 * @brief Indicates the interrupt number of the highest priority
 * active interrupt
 *
 * @return IRQ number
 */
static ALWAYS_INLINE int _INTERRUPT_CAUSE(void)
{
	u32_t irq_num = _arc_v2_aux_reg_read(_ARC_V2_ICAUSE);

	return irq_num;
}

#define _is_in_isr	_arc_v2_irq_unit_is_in_isr

extern void _thread_entry_wrapper(void);
extern void _user_thread_entry_wrapper(void);

static inline void _IntLibInit(void)
{
	/* nothing needed, here because the kernel requires it */
}

extern void _arc_userspace_enter(k_thread_entry_t user_entry, void *p1,
		 void *p2, void *p3, u32_t stack, u32_t size);

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_ARCH_ARC_INCLUDE_KERNEL_ARCH_FUNC_H_ */
