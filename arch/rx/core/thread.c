/*
 * Copyright (c) 2021 KT-Elektronik, Klaucke und Partner GmbH
 * Copyright (c) 2024 Renesas Electronics Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <kernel_internal.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

/* variables to store the arguments of z_rx_context_switch_isr() (zephyr\arch\rx\core\switch.S)
 * when performing a cooperative thread switch. In that case, z_rx_context_switch_isr() triggerss
 * unmaskable interrupt 1 to actually perform the switch. The ISR to interrupt 1
 * (switch_isr_wrapper()) reads the arguments from these variables.
 */
void *coop_switch_to;
void **coop_switched_from;

void arch_new_thread(struct k_thread *thread, k_thread_stack_t *stack, char *stack_ptr,
			 k_thread_entry_t entry, void *arg1, void *arg2, void *arg3)
{
	struct arch_esf *iframe;

	iframe = Z_STACK_PTR_TO_FRAME(struct arch_esf, stack_ptr);

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
	iframe->acc_l = 16;
	iframe->acc_h = 17;

	thread->switch_handle = (void *)iframe;
}

int arch_coprocessors_disable(struct k_thread *thread)
{
	return -ENOTSUP;
}
