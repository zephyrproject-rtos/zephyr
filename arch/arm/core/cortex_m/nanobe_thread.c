/*
 * Copyright (c) 2016-2026 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Zephyr threads as nanobes on ARM Cortex-M
 *
 * With CONFIG_USE_NANOBE_SWITCH, a Zephyr thread context switch is a nanobe
 * switch: arch_swap() saves only the callee-saved registers on the outgoing
 * thread's stack and swaps the process stack pointer, in thread mode. The
 * saved stack pointer of a switched out thread is kept in
 * thread->callee_saved.psp.
 *
 * All scheduling decisions are taken in thread mode, PendSV is not used. An
 * interrupt service routine that readies or halts threads only defers the
 * decision (see update_cache()); on interrupt exit, z_sched_deferred_reschedule()
 * is injected into the interrupted thread, which then decides and switches in
 * thread mode.
 */

#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/kernel/nanobe.h>
#include <kernel_internal.h>
#include <ksched.h>
#include <kswap.h>
#include <cmsis_core.h>

/* Thread entry and arguments, kept at the top of a new thread's stack */
struct nanobe_thread_entry {
	void (*wrapper)(k_thread_entry_t entry, void *p1, void *p2, void *p3);
	k_thread_entry_t entry;
	void *p1;
	void *p2;
	void *p3;
};

#if defined(CONFIG_THREAD_LOCAL_STORAGE)
extern uintptr_t z_arm_tls_ptr;
#endif

static void nanobe_thread_start(void *arg)
{
	struct nanobe_thread_entry *start = arg;

	/* First switched in by arch_swap(), with interrupts locked */
	arch_irq_unlock(0U);

	start->wrapper(start->entry, start->p1, start->p2, start->p3);
}

void *z_arm_nanobe_new_thread(char *stack_ptr, void *wrapper, k_thread_entry_t entry, void *p1,
			      void *p2, void *p3)
{
	struct nanobe_thread_entry *start;

	start = (struct nanobe_thread_entry *)ROUND_DOWN(
		(uintptr_t)stack_ptr - sizeof(*start), NANOBE_STACK_ALIGN);
	start->wrapper = wrapper;
	start->entry = entry;
	start->p1 = p1;
	start->p2 = p2;
	start->p3 = p3;

	return z_nanobe_frame_init(nanobe_thread_start, start, start);
}

int z_arm_nanobe_swap(unsigned int key)
{
	struct k_thread *old_thread = _current;
	struct k_thread *new_thread = _kernel.ready_q.cache;

	old_thread->arch.swap_return_value = -EAGAIN;

	if (new_thread != old_thread) {
		/* Nanobe scheduler lock of the outgoing thread, released by
		 * nanobe_switch() for the incoming one.
		 */
		uint8_t sgrd = z_nanobe_sgrd;

#if defined(CONFIG_INSTRUMENT_THREAD_SWITCHING)
		z_thread_mark_switched_out();
#endif

		z_current_thread_set(new_thread);

#if defined(CONFIG_THREAD_LOCAL_STORAGE)
		z_arm_tls_ptr = new_thread->tls;
#endif

#if defined(CONFIG_MPU_STACK_GUARD)
		z_arm_configure_dynamic_mpu_regions(new_thread);
#endif

#if defined(CONFIG_INSTRUMENT_THREAD_SWITCHING)
		z_thread_mark_switched_in();
#endif

		nanobe_switch((void *)new_thread->callee_saved.psp,
			      (void **)&old_thread->callee_saved.psp);

		/* Switched back in */
		z_nanobe_sgrd = sgrd;
	}

	arch_irq_unlock(key);

	return _current->arch.swap_return_value;
}

FUNC_ALIAS(z_arm_exc_exit, z_arm_int_exit, void);

/* Interrupt and exception exit, no scheduling here */
Z_GENERIC_SECTION(.text._HandlerModeExit) void z_arm_exc_exit(void)
{
	/* Not injected if nested, or if an IT block or an interrupted LDM/STM
	 * would have to be resumed (-EBUSY); the deferred decision is then
	 * taken on a later interrupt exit or thread mode reschedule.
	 */
	if (z_sched_deferred != 0U) {
		(void)z_nanobe_isr_inject(z_sched_deferred_reschedule);
	}

#ifdef CONFIG_STACK_SENTINEL
	z_check_stack_sentinel();
#endif /* CONFIG_STACK_SENTINEL */
}

void z_arm_nanobe_abort_exit(void)
{
	/* The aborted thread is never resumed, discard its IT block or
	 * interrupted LDM/STM state so that the reschedule is injected.
	 */
	z_nanobe_isr_iciit_discard();

	z_arm_exc_exit();
}
