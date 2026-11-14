/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <kernel_internal.h>
#include <kernel_arch_func.h>

void arch_new_thread(struct k_thread *thread, k_thread_stack_t *stack, char *stack_ptr,
		     k_thread_entry_t entry, void *p1, void *p2, void *p3)
{
	struct arch_esf *frame;

	ARG_UNUSED(stack);

	frame = (struct arch_esf *)Z_STACK_PTR_ALIGN(stack_ptr - sizeof(*frame));
	(void)memset(frame, 0, sizeof(*frame));

	frame->r6 = (uint32_t)entry;
	frame->r7 = (uint32_t)p1;
	frame->r8 = (uint32_t)p2;
	frame->r9 = (uint32_t)p3;
	frame->eipc = (uint32_t)z_thread_entry;
	frame->eipsw = RH850_PSW_THREAD;
	frame->fpsr = RH850_FPSR_FS;

#ifdef CONFIG_FXU
	thread->arch.fxu_user = 0U;
#endif

	thread->switch_handle = frame;
}

int arch_coprocessors_disable(struct k_thread *thread)
{
#ifdef CONFIG_FXU
	z_rh850_fxu_disable(thread);

	return 0;
#else
	ARG_UNUSED(thread);

	return -ENOTSUP;
#endif
}

#if defined(CONFIG_FPU_SHARING)
/* The FPU works on the general registers, which every frame saves */
int arch_float_disable(struct k_thread *thread)
{
	ARG_UNUSED(thread);

	return -ENOTSUP;
}

int arch_float_enable(struct k_thread *thread, unsigned int options)
{
	ARG_UNUSED(thread);
	ARG_UNUSED(options);

	return -ENOTSUP;
}
#endif /* CONFIG_FPU_SHARING */
