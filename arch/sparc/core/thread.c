/*
 * Copyright (c) 2019-2020 Cobham Gaisler AB
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <ksched.h>

void z_thread_entry_wrapper(k_thread_entry_t thread,
			    void *arg1,
			    void *arg2,
			    void *arg3);

/*
 * Frame used by _thread_entry_wrapper
 *
 * Allocate a 16 register window save area at bottom of the stack. This is
 * required if we need to taken a trap (interrupt) in the thread entry wrapper.
 */
struct init_stack_frame {
	uint32_t window_save_area[16];
};

#if defined(CONFIG_FPU_SHARING)
  #define USER_FP_MASK K_FP_REGS
#else
  #define USER_FP_MASK 0
#endif

void arch_new_thread(struct k_thread *thread, k_thread_stack_t *stack,
		     char *stack_ptr, k_thread_entry_t entry,
		     void *p1, void *p2, void *p3)
{
	struct init_stack_frame *iframe;

	/* Initial stack frame data, stored at base of the stack */
	iframe = Z_STACK_PTR_TO_FRAME(struct init_stack_frame, stack_ptr);

	thread->callee_saved.i0 = (uint32_t) entry;
	thread->callee_saved.i1 = (uint32_t) p1;
	thread->callee_saved.i2 = (uint32_t) p2;
	thread->callee_saved.i3 = (uint32_t) p3;
	thread->callee_saved.i6 = 0;                    /* frame pointer */
	thread->callee_saved.o6 = (uint32_t) iframe;    /* stack pointer */
	thread->callee_saved.o7 = (uint32_t) z_thread_entry_wrapper - 8;
	thread->callee_saved.psr = PSR_S | PSR_PS | PSR_ET;

	if (IS_ENABLED(CONFIG_FPU_SHARING)) {
		/* Selected threads can use the FPU */
		if (thread->base.user_options & USER_FP_MASK) {
			thread->callee_saved.psr |= PSR_EF;
		}
	} else if (IS_ENABLED(CONFIG_FPU)) {
		/* Any thread can use the FPU */
		thread->callee_saved.psr |= PSR_EF;
	}

	thread->switch_handle = thread;
}

void *z_arch_get_next_switch_handle(struct k_thread **old_thread)
{
	*old_thread =  _current;

	return z_get_next_switch_handle(*old_thread);
}

#if defined(CONFIG_FPU_SHARING)
int arch_float_disable(struct k_thread *thread)
{
	return -ENOTSUP;
}

int arch_float_enable(struct k_thread *thread, unsigned int options)
{
	return -ENOTSUP;
}
#endif /* CONFIG_FPU_SHARING */
