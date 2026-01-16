/*
 * Copyright (c) 2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/llext/symbol.h>
#include <zephyr/pm/pm.h>
#include <stdbool.h>
#include <zephyr/logging/log.h>
/* private kernel APIs */
#include <ksched.h>
#include <kswap.h>
#include <wait_q.h>

LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

#ifdef CONFIG_DEFERRABLE_TIMEOUT
int32_t timeout_type = K_TIMEOUT_NON_DEFERRABLE;
#endif /* CONFIG_DEFERRABLE_TIMEOUT */

void idle(void *unused1, void *unused2, void *unused3)
{
	ARG_UNUSED(unused1);
	ARG_UNUSED(unused2);
	ARG_UNUSED(unused3);

	__ASSERT_NO_MSG(_current->base.prio >= 0);

	while (true) {
		/* SMP systems without a working IPI can't actual
		 * enter an idle state, because they can't be notified
		 * of scheduler changes (i.e. threads they should
		 * run).  They just spin instead, with a minimal
		 * relaxation loop to prevent hammering the scheduler
		 * lock and/or timer driver.  This is intended as a
		 * fallback configuration for new platform bringup.
		 */
		if (IS_ENABLED(CONFIG_SMP) && !IS_ENABLED(CONFIG_SCHED_IPI_SUPPORTED)) {
			for (volatile int i = 0; i < 100000; i++) {
				/* Empty loop */
			}
			z_swap_unlocked();
		}

		/* Note weird API: k_cpu_idle() is called with local
		 * CPU interrupts masked, and returns with them
		 * unmasked.  It does not take a spinlock or other
		 * higher level construct.
		 */
		(void) arch_irq_lock();

#ifdef CONFIG_PM
#ifdef CONFIG_DEFERRABLE_TIMEOUT
		/* Use only the next non-deferrable timeout when deciding
		 * how long the system may sleep. Deferrable timeouts can be
		 * postponed in sleep, reducing unnecessary wakeups and
		 * improving residency.
		 */
		_kernel.idle = k_get_next_non_deferrable_timeout_expiry(timeout_type);
#else
		_kernel.idle = z_get_next_timeout_expiry();
#endif /* CONFIG_DEFERRABLE_TIMEOUT */

		/*
		 * Call the suspend hook function of the soc interface
		 * to allow entry into a low power state. The function
		 * returns false if low power state was not entered, in
		 * which case, kernel does normal idle processing.
		 *
		 * This function is entered with interrupts disabled.
		 * If a low power state was entered, then the hook
		 * function should enable interrupts before exiting.
		 * This is because the kernel does not do its own idle
		 * processing in those cases i.e. skips k_cpu_idle().
		 * The kernel's idle processing re-enables interrupts
		 * which is essential for the kernel's scheduling
		 * logic.
		 */
		if (k_is_pre_kernel() || !pm_system_suspend(_kernel.idle)) {
			k_cpu_idle();
		}
#else
		k_cpu_idle();
#endif /* CONFIG_PM */

#if !defined(CONFIG_PREEMPT_ENABLED)
# if !defined(CONFIG_USE_SWITCH) || defined(CONFIG_SPARC)
		/* A legacy mess: the idle thread is by definition
		 * preemptible as far as the modern scheduler is
		 * concerned, but older platforms use
		 * CONFIG_PREEMPT_ENABLED=n as an optimization hint
		 * that interrupt exit always returns to the
		 * interrupted context.  So in that setup we need to
		 * explicitly yield in the idle thread otherwise
		 * nothing else will run once it starts.
		 */
		if (_kernel.ready_q.cache != _current) {
			z_swap_unlocked();
		}
# endif /* !defined(CONFIG_USE_SWITCH) || defined(CONFIG_SPARC) */
#endif /* !defined(CONFIG_PREEMPT_ENABLED) */
	}
}

void __weak arch_spin_relax(void)
{
	__ASSERT(!arch_irq_unlocked(arch_irq_lock()),
		 "this is meant to be called with IRQs disabled");

	arch_nop();
}
EXPORT_SYMBOL(arch_spin_relax);
