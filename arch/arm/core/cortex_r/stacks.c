/*
 * Copyright (c) 2018 Lexmark International, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <cortex_r/stack.h>
#include <string.h>

K_THREAD_STACK_DEFINE(z_arm_fiq_stack,   CONFIG_ARMV7_FIQ_STACK_SIZE);
K_THREAD_STACK_DEFINE(z_arm_abort_stack, CONFIG_ARMV7_EXCEPTION_STACK_SIZE);
K_THREAD_STACK_DEFINE(z_arm_undef_stack, CONFIG_ARMV7_EXCEPTION_STACK_SIZE);
K_THREAD_STACK_DEFINE(z_arm_svc_stack,   CONFIG_ARMV7_SVC_STACK_SIZE);
K_THREAD_STACK_DEFINE(z_arm_sys_stack,   CONFIG_ARMV7_SYS_STACK_SIZE);

#if defined(CONFIG_INIT_STACKS)
void z_arm_init_stacks(void)
{
	memset(z_arm_fiq_stack, 0xAA, CONFIG_ARMV7_FIQ_STACK_SIZE);
	memset(z_arm_svc_stack, 0xAA, CONFIG_ARMV7_SVC_STACK_SIZE);
	memset(z_arm_abort_stack, 0xAA, CONFIG_ARMV7_EXCEPTION_STACK_SIZE);
	memset(z_arm_undef_stack, 0xAA, CONFIG_ARMV7_EXCEPTION_STACK_SIZE);
	memset(&_interrupt_stack, 0xAA, CONFIG_ISR_STACK_SIZE);
}
#endif
