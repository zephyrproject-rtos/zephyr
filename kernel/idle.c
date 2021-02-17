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

LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

#ifdef CONFIG_TICKLESS_IDLE_THRESH
#define IDLE_THRESH CONFIG_TICKLESS_IDLE_THRESH
#else
#define IDLE_THRESH 1
#endif

/* Fallback idle spin loop for SMP platforms without a working IPI */
#if (defined(CONFIG_SMP) && !defined(CONFIG_SCHED_IPI_SUPPORTED))
#define SMP_FALLBACK 1
#else
#define SMP_FALLBACK 0
#endif

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
 *
 * @brief Indicate that kernel is idling in tickless mode
 *
 * Sets the kernel data structure idle field to either a positive value or
 * K_FOREVER.
 *
 * @param ticks the number of ticks to idle
 *
 * @return N/A
 */
#if !SMP_FALLBACK && CONFIG_PM
static enum pm_state pm_save_idle(int32_t ticks)
{
	static enum pm_state idle_state = PM_STATE_ACTIVE;

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
	idle_state = pm_system_suspend(ticks);
	if (idle_state == PM_STATE_ACTIVE) {
		pm_idle_exit_notify = 0U;
	}

	return idle_state;

}
#endif /* !SMP_FALLBACK */


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
#endif	/* CONFIG_PM */
	z_clock_idle_exit();
}


#if K_IDLE_PRIO < 0
#define IDLE_YIELD_IF_COOP() k_yield()
#else
#define IDLE_YIELD_IF_COOP() do { } while (false)
#endif

void idle(void *p1, void *unused2, void *unused3)
{
	ARG_UNUSED(unused2);
	ARG_UNUSED(unused3);

#ifdef CONFIG_BOOT_TIME_MEASUREMENT
	/* record timestamp when idling begins */

	extern uint32_t z_timestamp_idle;

	z_timestamp_idle = k_cycle_get_32();
#endif /* CONFIG_BOOT_TIME_MEASUREMENT */

	while (true) {
#if SMP_FALLBACK
		k_busy_wait(100);
		k_yield();
#else

#ifdef CONFIG_SYS_CLOCK_EXISTS
		int32_t ticks = z_get_next_timeout_expiry();

		/* The documented behavior of CONFIG_TICKLESS_IDLE_THRESH is
		 * that the system should not enter a tickless idle for
		 * periods less than that.  This seems... silly, given that it
		 * saves no power and does not improve latency.  But it's an
		 * API we need to honor...
		 */
		z_set_timeout_expiry((ticks < IDLE_THRESH) ? 1 : ticks, true);
#ifdef CONFIG_PM
		_kernel.idle = ticks;
		/* Check power policy and decide if we are going to sleep or
		 * just idle.
		 */
		if (pm_save_idle(ticks) == PM_STATE_ACTIVE) {
			k_cpu_idle();
		}
#else /* CONFIG_PM */
		k_cpu_idle();
#endif /* CONFIG_PM */
#else /* CONFIG_SYS_CLOCK_EXISTS */
		k_cpu_idle();
#endif /* CONFIG_SYS_CLOCK_EXISTS */

		IDLE_YIELD_IF_COOP();
#endif /* SMP_FALLBACK */
	}
}
