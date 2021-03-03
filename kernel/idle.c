/*
 * Copyright (c) 2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <toolchain.h>
#include <linker/sections.h>
#include <drivers/timer/system_timer.h>
#include <wait_q.h>
#include <power/power.h>
#include <stdbool.h>
#include <logging/log.h>
#include <ksched.h>

extern uint32_t z_timestamp_idle;

LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

#ifdef CONFIG_PM
/*
 * Used to allow pm_system_suspend() implementation to control notification
 * of the event that caused exit from kernel idling after pm operations.
 */
unsigned char pm_idle_exit_notify;

/* LCOV_EXCL_START
 * These are almost certainly overidden and in any event do nothing
 */
void __attribute__((weak)) pm_system_resume(void)
{
}

void __attribute__((weak)) pm_system_resume_from_deep_sleep(void)
{
}
/* LCOV_EXCL_STOP */

#endif /* CONFIG_PM */

/**
 * @brief Indicate that kernel is idling in tickless mode
 *
 * Sets the kernel data structure idle field to either a positive value or
 * K_FOREVER.
 */
static void pm_save_idle(void)
{
#ifdef CONFIG_PM
	int32_t ticks = z_get_next_timeout_expiry();
	_kernel.idle = ticks;

	pm_idle_exit_notify = 1U;

	/*
	 * Call the suspend hook function of the soc interface to allow
	 * entry into a low power state. The function returns
	 * POWER_STATE_ACTIVE if low power state was not entered, in which
	 * case, kernel does normal idle processing.
	 *
	 * This function is entered with interrupts disabled. If a low power
	 * state was entered, then the hook function should enable inerrupts
	 * before exiting. This is because the kernel does not do its own idle
	 * processing in those cases i.e. skips k_cpu_idle(). The kernel's
	 * idle processing re-enables interrupts which is essential for
	 * the kernel's scheduling logic.
	 */
	if (pm_system_suspend(ticks) == PM_STATE_ACTIVE) {
		pm_idle_exit_notify = 0U;
		k_cpu_idle();
	}
#endif
}

void z_pm_save_idle_exit(int32_t ticks)
{
#ifdef CONFIG_PM
	/* Some CPU low power states require notification at the ISR
	 * to allow any operations that needs to be done before kernel
	 * switches task or processes nested interrupts. This can be
	 * disabled by calling pm_idle_exit_notification_disable().
	 * Alternatively it can be simply ignored if not required.
	 */
	if (pm_idle_exit_notify) {
		pm_system_resume();
	}
#endif
	z_clock_idle_exit();
}

void idle(void *unused1, void *unused2, void *unused3)
{
	ARG_UNUSED(unused1);
	ARG_UNUSED(unused2);
	ARG_UNUSED(unused3);

#ifdef CONFIG_BOOT_TIME_MEASUREMENT
	z_timestamp_idle = k_cycle_get_32();
#endif

	while (true) {
		/* SMP systems without a working IPI can't
		 * actual enter an idle state, because they
		 * can't be notified of scheduler changes
		 * (i.e. threads they should run).  They just
		 * spin in a yield loop.  This is intended as
		 * a fallback configuration for new platform
		 * bringup.
		 */
		if (IS_ENABLED(CONFIG_SMP) &&
		    !IS_ENABLED(CONFIG_SCHED_IPI_SUPPORTED)) {
			k_busy_wait(100);
			k_yield();
			continue;
		}

		/* Note weird API: k_cpu_idle() is called with local
		 * CPU interrupts masked, and returns with them
		 * unmasked.  It does not take a spinlock or other
		 * higher level construct.
		 */
		(void) arch_irq_lock();

		if (IS_ENABLED(CONFIG_PM)) {
			pm_save_idle();
		} else {
			k_cpu_idle();
		}

		/* It is possible to (pathologically) configure the
		 * idle thread to have a non-preemptible priority.
		 * You might think this is an API bug, but we actually
		 * have a test that exercises this.  Handle the edge
		 * case when that happens.
		 */
		if (K_IDLE_PRIO < 0) {
			k_yield();
		}
	}
}
