/*
 * Copyright (c) 2018 Lexmark International, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Stack helpers for Cortex-A and Cortex-R CPUs
 *
 * Stack helper functions.
 */

#ifndef ZEPHYR_ARCH_ARM_INCLUDE_AARCH32_CORTEX_A_R_STACK_H_
#define ZEPHYR_ARCH_ARM_INCLUDE_AARCH32_CORTEX_A_R_STACK_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _ASMLANGUAGE

/* nothing */

#else

extern void z_arm_init_stacks(void);

/**
 *
 * @brief Setup interrupt stack
 *
 * On Cortex-A and Cortex-R, the interrupt stack is set up by reset.S
 *
 */
static ALWAYS_INLINE void z_arm_interrupt_stack_setup(void)
{
}

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_ARCH_ARM_INCLUDE_AARCH32_CORTEX_A_R_STACK_H_ */
