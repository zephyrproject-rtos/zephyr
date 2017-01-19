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

#ifndef _kernel_arch_func__h_
#define _kernel_arch_func__h_

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(_ASMLANGUAGE)

#ifdef CONFIG_CPU_ARCV2
#include <v2/cache.h>
#include <v2/irq.h>
#endif

static ALWAYS_INLINE void nanoArchInit(void)
{
	_irq_setup();
}

static ALWAYS_INLINE void
_set_thread_return_value(struct k_thread *thread, unsigned int value)
{
	thread->arch.return_value = value;
}

static ALWAYS_INLINE int _is_in_isr(void)
{
	uint32_t act = _arc_v2_aux_reg_read(_ARC_V2_AUX_IRQ_ACT);
#if CONFIG_IRQ_OFFLOAD
	/* Check if we're in a TRAP_S exception as well */
	if (_arc_v2_aux_reg_read(_ARC_V2_STATUS32) & _ARC_V2_STATUS32_AE &&
	    _ARC_V2_ECR_VECTOR(_arc_v2_aux_reg_read(_ARC_V2_ECR)) == EXC_EV_TRAP
	   ) {
		return 1;
	}
#endif
	return ((act & 0xffff) != 0);
}

/**
 *
 * @bried Indicates the interrupt number of the highest priority
 * active interrupt
 *
 * @return IRQ number
 */
static ALWAYS_INLINE int _INTERRUPT_CAUSE(void)
{
	uint32_t irq_num = _arc_v2_aux_reg_read(_ARC_V2_ICAUSE);

	return irq_num;
}


extern void _thread_entry_wrapper(void);

static inline void _IntLibInit(void)
{
	/* nothing needed, here because the kernel requires it */
}

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* _kernel_arch_func__h_ */
