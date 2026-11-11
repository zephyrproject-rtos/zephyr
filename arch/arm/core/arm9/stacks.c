/*
 * Copyright (c) 2026 Takayuki Goto
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Per-mode stacks for ARMv5 cores
 *
 * Threads and exception handlers both run in system mode. The five exception
 * modes only ever touch the three word scratch area at the very top of their
 * stack (see z_armv5_enter_exc()), but a full stack is still reserved for each
 * so that the mode can be given real work later without changing the boot
 * code.
 *
 * The system mode stack is the stack z_prep_c()/z_cstart() run on, up to the
 * point where the kernel switches into the main thread.
 */

#include <zephyr/kernel.h>
#include <zephyr/toolchain.h>
#include <string.h>
#include <kernel_internal.h>
#include <arm9/stack.h>

/*
 * These live in .noinit on purpose: z_prep_c() runs on the system mode stack
 * while it zeroes .bss, so a stack placed there would erase its own frame.
 */
#define STACK_DEFINE(name, size) uint8_t __noinit __aligned(8) name[size]

STACK_DEFINE(z_arm_fiq_stack, CONFIG_ARMV5_FIQ_STACK_SIZE);
STACK_DEFINE(z_arm_irq_stack, CONFIG_ARMV5_EXCEPTION_STACK_SIZE);
STACK_DEFINE(z_arm_abort_stack, CONFIG_ARMV5_EXCEPTION_STACK_SIZE);
STACK_DEFINE(z_arm_undef_stack, CONFIG_ARMV5_EXCEPTION_STACK_SIZE);
STACK_DEFINE(z_arm_svc_stack, CONFIG_ARMV5_SVC_STACK_SIZE);
STACK_DEFINE(z_arm_sys_stack, CONFIG_ARMV5_SYS_STACK_SIZE);

#if defined(CONFIG_INIT_STACKS)
void z_arm_init_stacks(void)
{
	memset(z_arm_fiq_stack, 0xAA, sizeof(z_arm_fiq_stack));
	memset(z_arm_irq_stack, 0xAA, sizeof(z_arm_irq_stack));
	memset(z_arm_abort_stack, 0xAA, sizeof(z_arm_abort_stack));
	memset(z_arm_undef_stack, 0xAA, sizeof(z_arm_undef_stack));
	memset(z_arm_svc_stack, 0xAA, sizeof(z_arm_svc_stack));
	memset(K_KERNEL_STACK_BUFFER(z_interrupt_stacks[0]), 0xAA,
	       K_KERNEL_STACK_SIZEOF(z_interrupt_stacks[0]));
}
#endif
