/*
 * Copyright (c) 2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <kernel_structs.h>
#include <toolchain.h>
#include <linker/sections.h>
#include <drivers/system_timer.h>
#include <wait_q.h>
#include <power.h>
#include <stdbool.h>

#if defined(CONFIG_TICKLESS_IDLE)
/*
 * Idle time must be this value or higher for timer to go into tickless idle
 * state.
 */
s32_t _sys_idle_threshold_ticks = CONFIG_TICKLESS_IDLE_THRESH;

#if defined(CONFIG_TICKLESS_KERNEL)
#define _must_enter_tickless_idle(ticks) (true)
#else
#define _must_enter_tickless_idle(ticks) \
		((ticks == K_FOREVER) || (ticks >= _sys_idle_threshold_ticks))
#endif
#else
#define _must_enter_tickless_idle(ticks) ((void)ticks, (0))
#endif /* CONFIG_TICKLESS_IDLE */

#ifdef CONFIG_SYS_POWER_MANAGEMENT
/*
 * Used to allow _sys_soc_suspend() implementation to control notification
 * of the event that caused exit from kernel idling after pm operations.
 */
unsigned char _sys_pm_idle_exit_notify;

#if defined(CONFIG_SYS_POWER_LOW_POWER_STATE)
void __attribute__((weak)) _sys_soc_resume(void)
{
}
#endif

#if defined(CONFIG_SYS_POWER_DEEP_SLEEP)
void __attribute__((weak)) _sys_soc_resume_from_deep_sleep(void)
{
}
#endif
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
static void set_kernel_idle_time_in_ticks(s32_t ticks)
{
	_kernel.idle = ticks;
}
#else
#define set_kernel_idle_time_in_ticks(x) do { } while (false)
#endif

#ifndef CONFIG_SMP
static void sys_power_save_idle(s32_t ticks)
{
#ifdef CONFIG_TICKLESS_KERNEL
	if (ticks != K_FOREVER) {
		ticks -= _get_elapsed_program_time();
		if (!ticks) {
			/*
			 * Timer has expired or about to expire
			 * No time for power saving operations
			 *
			 * Note that it will never be zero unless some time
			 * had elapsed since timer was last programmed.
			 */
			k_cpu_idle();
			return;
		}
	}
#endif
	if (_must_enter_tickless_idle(ticks)) {
		/*
		 * Stop generating system timer interrupts until it's time for
		 * the next scheduled kernel timer to expire.
		 */

		/*
		 * In the case of tickless kernel, timer driver should
		 * reprogram timer only if the currently programmed time
		 * duration is smaller than the idle time.
		 */
		_timer_idle_enter(ticks);
	}

	set_kernel_idle_time_in_ticks(ticks);
#if (defined(CONFIG_SYS_POWER_LOW_POWER_STATE) || \
	defined(CONFIG_SYS_POWER_DEEP_SLEEP))

	_sys_pm_idle_exit_notify = 1;

	/*
	 * Call the suspend hook function of the soc interface to allow
	 * entry into a low power state. The function returns
	 * SYS_PM_NOT_HANDLED if low power state was not entered, in which
	 * case, kernel does normal idle processing.
	 *
	 * This function is entered with interrupts disabled. If a low power
	 * state was entered, then the hook function should enable inerrupts
	 * before exiting. This is because the kernel does not do its own idle
	 * processing in those cases i.e. skips k_cpu_idle(). The kernel's
	 * idle processing re-enables interrupts which is essential for
	 * the kernel's scheduling logic.
	 */
	if (_sys_soc_suspend(ticks) == SYS_PM_NOT_HANDLED) {
		_sys_pm_idle_exit_notify = 0;
		k_cpu_idle();
	}
#else
	k_cpu_idle();
#endif
}
#endif

void _sys_power_save_idle_exit(s32_t ticks)
{
#if defined(CONFIG_SYS_POWER_LOW_POWER_STATE)
	/* Some CPU low power states require notification at the ISR
	 * to allow any operations that needs to be done before kernel
	 * switches task or processes nested interrupts. This can be
	 * disabled by calling _sys_soc_pm_idle_exit_notification_disable().
	 * Alternatively it can be simply ignored if not required.
	 */
	if (_sys_pm_idle_exit_notify) {
		_sys_soc_resume();
	}
#endif

	if (_must_enter_tickless_idle(ticks)) {
		/* Resume normal periodic system timer interrupts */
		_timer_idle_exit();
	}
}


#if K_IDLE_PRIO < 0
#define IDLE_YIELD_IF_COOP() k_yield()
#else
#define IDLE_YIELD_IF_COOP() do { } while ((0))
#endif

void idle(void *unused1, void *unused2, void *unused3)
{
	ARG_UNUSED(unused1);
	ARG_UNUSED(unused2);
	ARG_UNUSED(unused3);

#ifdef CONFIG_BOOT_TIME_MEASUREMENT
	/* record timestamp when idling begins */

	extern u64_t __idle_time_stamp;

	__idle_time_stamp = (u64_t)k_cycle_get_32();
#endif

#ifdef CONFIG_SMP
	/* Simplified idle for SMP CPUs pending driver support.  The
	 * busy waiting is needed to prevent lock contention.  Long
	 * term we need to wake up idle CPUs with an IPI.
	 */
	while (true) {
		k_busy_wait(100);
		k_yield();
	}
#else
	for (;;) {
		(void)irq_lock();
		sys_power_save_idle(_get_next_timeout_expiry());

		IDLE_YIELD_IF_COOP();
	}
#endif
}
