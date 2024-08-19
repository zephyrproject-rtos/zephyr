/*
 * Copyright (c) 2021 KT-Elektronik, Klaucke und Partner GmbH
 * Copyright (c) 2024 Renesas Electronics Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <kernel_internal.h>
#include <ksched.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

/* variables to store the arguments of z_rx_context_switch_isr() (zephyr\arch\rx\core\switch.S)
 * when performing a cooperative thread switch. In that case, z_rx_context_switch_isr() triggerss
 * unmaskable interrupt 1 to actually perform the switch. The ISR to interrupt 1
 * (switch_isr_wrapper()) reads the arguments from these variables.
 */
void *coop_switch_to;
void **coop_switched_from;

/* the initial content of the stack */
struct init_stack_frame {
	uint32_t r1;
	uint32_t r2;
	uint32_t r3;
	uint32_t r4;
	uint32_t r5;
	uint32_t r6;
	uint32_t r7;
	uint32_t r8;
	uint32_t r9;
	uint32_t r10;
	uint32_t r11;
	uint32_t r12;
	uint32_t r13;
	uint32_t r14;
	uint32_t r15;
	uint32_t entry_point;
	uint32_t psw;
};

void arch_new_thread(struct k_thread *thread, k_thread_stack_t *stack, char *stack_ptr,
		     k_thread_entry_t entry, void *arg1, void *arg2, void *arg3)
{
	struct init_stack_frame *iframe;

	iframe = Z_STACK_PTR_TO_FRAME(struct init_stack_frame, stack_ptr);

	/* initial value for the PSW (bits U and I are set) */
	iframe->psw = 0x30000;
	/* the initial entry point is the function z_thread_entry */
	iframe->entry_point = (uint32_t)z_thread_entry;
	/* arguments for the call of z_thread_entry (to be written to r1-r4) */
	iframe->r1 = (uint32_t)entry;
	iframe->r2 = (uint32_t)arg1;
	iframe->r3 = (uint32_t)arg2;
	iframe->r4 = (uint32_t)arg3;
	/* for debugging: */
	iframe->r5 = 5;
	iframe->r6 = 6;
	iframe->r7 = 7;
	iframe->r8 = 8;
	iframe->r9 = 9;
	iframe->r10 = 10;
	iframe->r11 = 11;
	iframe->r12 = 12;
	iframe->r13 = 13;
	iframe->r14 = 14;
	iframe->r15 = 15;

	thread->switch_handle = (void *)iframe;
}
