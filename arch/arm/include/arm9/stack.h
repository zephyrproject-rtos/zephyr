/*
 * Copyright (c) 2026 Takayuki Goto
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Stack helpers for ARMv5 (ARM9/XScale) cores
 */

#ifndef ZEPHYR_ARCH_ARM_INCLUDE_ARM9_STACK_H_
#define ZEPHYR_ARCH_ARM_INCLUDE_ARM9_STACK_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _ASMLANGUAGE

#include <zephyr/types.h>

extern uint8_t z_arm_fiq_stack[];
extern uint8_t z_arm_irq_stack[];
extern uint8_t z_arm_abort_stack[];
extern uint8_t z_arm_undef_stack[];
extern uint8_t z_arm_svc_stack[];
extern uint8_t z_arm_sys_stack[];

extern void z_arm_init_stacks(void);

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_ARCH_ARM_INCLUDE_ARM9_STACK_H_ */
