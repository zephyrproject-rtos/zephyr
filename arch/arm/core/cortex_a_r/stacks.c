/*
 * Copyright (c) 2018 Lexmark International, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zephyr/kernel/thread_stack.h"
#include <zephyr/kernel.h>
#include <cortex_a_r/stack.h>
#include <string.h>
#include <kernel_internal.h>

K_KERNEL_STACK_ARRAY_DEFINE(z_arm_fiq_stack, CONFIG_MP_MAX_NUM_CPUS,
		CONFIG_ARMV7_FIQ_STACK_SIZE);
K_KERNEL_STACK_ARRAY_DEFINE(z_arm_abort_stack, CONFIG_MP_MAX_NUM_CPUS,
		CONFIG_ARMV7_EXCEPTION_STACK_SIZE);
K_KERNEL_STACK_ARRAY_DEFINE(z_arm_undef_stack, CONFIG_MP_MAX_NUM_CPUS,
		CONFIG_ARMV7_EXCEPTION_STACK_SIZE);
K_KERNEL_STACK_ARRAY_DEFINE(z_arm_svc_stack, CONFIG_MP_MAX_NUM_CPUS,
		CONFIG_ARMV7_SVC_STACK_SIZE);
K_KERNEL_STACK_ARRAY_DEFINE(z_arm_sys_stack, CONFIG_MP_MAX_NUM_CPUS,
		CONFIG_ARMV7_SYS_STACK_SIZE);

#if defined(CONFIG_INIT_STACKS)
void z_arm_init_stacks(void)
{
	memset(z_arm_fiq_stack, 0xAA, CONFIG_ARMV7_FIQ_STACK_SIZE);
	memset(z_arm_svc_stack, 0xAA, CONFIG_ARMV7_SVC_STACK_SIZE);
	memset(z_arm_abort_stack, 0xAA, CONFIG_ARMV7_EXCEPTION_STACK_SIZE);
	memset(z_arm_undef_stack, 0xAA, CONFIG_ARMV7_EXCEPTION_STACK_SIZE);
	memset(Z_KERNEL_STACK_BUFFER(z_interrupt_stacks[0]), 0xAA,
	       K_KERNEL_STACK_SIZEOF(z_interrupt_stacks[0]));
}
#endif
