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
#include <misc/stack.h>

#ifdef CONFIG_TICKLESS_IDLE_THRESH
#define IDLE_THRESH CONFIG_TICKLESS_IDLE_THRESH
#else
#define IDLE_THRESH 1
#endif

#ifdef CONFIG_SYS_POWER_MANAGEMENT
/*
 * Used to allow _sys_suspend() implementation to control notification
 * of the event that caused exit from kernel idling after pm operations.
 */
unsigned char sys_pm_idle_exit_notify;

#if defined(CONFIG_SYS_POWER_SLEEP_STATES)
void __attribute__((weak)) _sys_resume(void)
{
}
#endif

#if defined(CONFIG_SYS_POWER_DEEP_SLEEP_STATES)
void __attribute__((weak)) _sys_resume_from_deep_sleep(void)
{
}
#endif

#endif /* CONFIG_SYS_POWER_MANAGEMENT */

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
#ifndef CONFIG_SMP
static void set_kernel_idle_time_in_ticks(s32_t ticks)
{
#ifdef CONFIG_SYS_POWER_MANAGEMENT
	_kernel.idle = ticks;
#endif
}

static void sys_power_save_idle(void)
{
	s32_t ticks = z_get_next_timeout_expiry();

	/* The documented behavior of CONFIG_TICKLESS_IDLE_THRESH is
	 * that the system should not enter a tickless idle for
	 * periods less than that.  This seems... silly, given that it
	 * saves no power and does not improve latency.  But it's an
	 * API we need to honor...
	 */
#ifdef CONFIG_SYS_CLOCK_EXISTS
	z_set_timeout_expiry((ticks < IDLE_THRESH) ? 1 : ticks, true);
#endif

	set_kernel_idle_time_in_ticks(ticks);
#if (defined(CONFIG_SYS_POWER_SLEEP_STATES) || \
	defined(CONFIG_SYS_POWER_DEEP_SLEEP_STATES))

	sys_pm_idle_exit_notify = 1U;

	/*
	 * Call the suspend hook function of the soc interface to allow
	 * entry into a low power state. The function returns
	 * SYS_POWER_STATE_ACTIVE if low power state was not entered, in which
	 * case, kernel does normal idle processing.
	 *
	 * This function is entered with interrupts disabled. If a low power
	 * state was entered, then the hook function should enable inerrupts
	 * before exiting. This is because the kernel does not do its own idle
	 * processing in those cases i.e. skips k_cpu_idle(). The kernel's
	 * idle processing re-enables interrupts which is essential for
	 * the kernel's scheduling logic.
	 */
	if (_sys_suspend(ticks) == SYS_POWER_STATE_ACTIVE) {
		sys_pm_idle_exit_notify = 0U;
		k_cpu_idle();
	}
#else
	k_cpu_idle();
#endif
}
#endif

void z_sys_power_save_idle_exit(s32_t ticks)
{
#if defined(CONFIG_SYS_POWER_SLEEP_STATES)
	/* Some CPU low power states require notification at the ISR
	 * to allow any operations that needs to be done before kernel
	 * switches task or processes nested interrupts. This can be
	 * disabled by calling _sys_pm_idle_exit_notification_disable().
	 * Alternatively it can be simply ignored if not required.
	 */
	if (sys_pm_idle_exit_notify) {
		_sys_resume();
	}
#endif

	z_clock_idle_exit();
}


#if defined(CONFIG_INIT_STACKS)
extern K_THREAD_STACK_DEFINE(_interrupt_stack, CONFIG_ISR_STACK_SIZE);
extern K_THREAD_STACK_DEFINE(sys_work_q_stack,
			     CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE);
extern K_THREAD_STACK_DEFINE(_main_stack, CONFIG_MAIN_STACK_SIZE);
extern K_THREAD_STACK_DEFINE(_idle_stack, CONFIG_IDLE_STACK_SIZE);
static u32_t idle_ts;

static void stack_dump(const struct k_thread *thread, void *user_data)
{
	LOG_MODULE_DECLARE(kernel, CONFIG_KERNEL_LOG_LEVEL);

	const char *name = k_thread_name_get((struct k_thread *)thread);
	unsigned int size = thread->stack_info.size;
	unsigned int pcnt, unused;

	unused = stack_unused_space_get((char *)thread->stack_info.start,
					size);
	pcnt = ((size - unused) * 100U) / size;

	LOG_INF("%s :\tunused %u\tusage %u / %u (%u %%)",
		name, unused, size - unused, size, pcnt);
}
#endif

#if K_IDLE_PRIO < 0
#define IDLE_YIELD_IF_COOP() k_yield()
#else
#define IDLE_YIELD_IF_COOP() do { } while (false)
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
#if defined(CONFIG_INIT_STACKS)
		if (k_uptime_get_32() - idle_ts > K_SECONDS(5)) {
			STACK_ANALYZE("interrupt stack", _interrupt_stack);
			STACK_ANALYZE("sys work q stack", sys_work_q_stack);
			STACK_ANALYZE("main stack", _main_stack);
			STACK_ANALYZE("idle stack", _idle_stack);

			k_thread_foreach(stack_dump, NULL);

			idle_ts = k_uptime_get_32();
		}
#endif
		(void)irq_lock();
		sys_power_save_idle();

		IDLE_YIELD_IF_COOP();
	}
#endif
}
