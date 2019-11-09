/*
 * Copyright (c) 2018 Lexmark International, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Stack helpers for Cortex-R CPUs
 *
 * Stack helper functions.
 */

#ifndef _ARM_CORTEXR_STACK__H_
#define _ARM_CORTEXR_STACK__H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _ASMLANGUAGE

/* nothing */

#else

extern K_THREAD_STACK_DEFINE(_interrupt_stack, CONFIG_ISR_STACK_SIZE);

extern void z_arm_init_stacks(void);

/**
 *
 * @brief Setup interrupt stack
 *
 * On Cortex-R, the interrupt stack is set up by reset.S
 *
 * @return N/A
 */
static ALWAYS_INLINE void z_arm_interrupt_stack_setup(void)
{
}

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* _ARM_CORTEXR_STACK__H_ */
