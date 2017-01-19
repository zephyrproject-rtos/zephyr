/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Stack helpers for Cortex-M CPUs
 *
 * Stack helper functions.
 */

#ifndef _ARM_CORTEXM_STACK__H_
#define _ARM_CORTEXM_STACK__H_

#include <kernel_structs.h>
#include <asm_inline.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_STACK_ALIGN_DOUBLE_WORD
#define STACK_ALIGN_SIZE 8
#else
#define STACK_ALIGN_SIZE 4
#endif

#ifdef _ASMLANGUAGE

/* nothing */

#else

extern char _interrupt_stack[CONFIG_ISR_STACK_SIZE];

/**
 *
 * @brief Setup interrupt stack
 *
 * On Cortex-M, the interrupt stack is registered in the MSP (main stack
 * pointer) register, and switched to automatically when taking an exception.
 *
 * @return N/A
 */
static ALWAYS_INLINE void _InterruptStackSetup(void)
{
	uint32_t msp = (uint32_t)(_interrupt_stack + CONFIG_ISR_STACK_SIZE);

	_MspSet(msp);
}

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* _ARM_CORTEXM_STACK__H_ */
