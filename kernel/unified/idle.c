/*
 * Copyright (c) 2016 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <nanokernel.h>
#include <kernel_structs.h>
#include <toolchain.h>
#include <sections.h>
#include <drivers/system_timer.h>
#include <wait_q.h>
#include <power.h>

#if defined(CONFIG_TICKLESS_IDLE)
/*
 * Idle time must be this value or higher for timer to go into tickless idle
 * state.
 */
int32_t _sys_idle_threshold_ticks = CONFIG_TICKLESS_IDLE_THRESH;
#endif /* CONFIG_TICKLESS_IDLE */

#ifdef CONFIG_SYS_POWER_MANAGEMENT
/*
 * Used to allow _sys_soc_suspend() implementation to control notification
 * of the event that caused exit from kernel idling after pm operations.
 */
unsigned char _sys_pm_idle_exit_notify;

void __attribute__((weak)) _sys_soc_resume(void)
{
}

void __attribute__((weak)) _sys_soc_resume_from_deep_sleep(void)
{
}

/**
 *
 * @brief Indicate that kernel is idling in tickless mode
 *
 * Sets the nanokernel data structure idle field to either a positive value or
 * K_FOREVER.
 *
 * @param ticks the number of ticks to idle
 *
 * @return N/A
 */
static void set_kernel_idle_time_in_ticks(int32_t ticks)
{
	_kernel.idle = ticks;
}
#else
#define set_kernel_idle_time_in_ticks(x) do { } while (0)
#endif

static void _sys_power_save_idle(int32_t ticks __unused)
{
#if defined(CONFIG_TICKLESS_IDLE)
	if ((ticks == K_FOREVER) || ticks >= _sys_idle_threshold_ticks) {
		/*
		 * Stop generating system timer interrupts until it's time for
		 * the next scheduled kernel timer to expire.
		 */

		_timer_idle_enter(ticks);
	}
#endif /* CONFIG_TICKLESS_IDLE */

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
	 * processing in those cases i.e. skips nano_cpu_idle(). The kernel's
	 * idle processing re-enables interrupts which is essential for
	 * the kernel's scheduling logic.
	 */
	if (_sys_soc_suspend(ticks) == SYS_PM_NOT_HANDLED) {
		_sys_pm_idle_exit_notify = 0;
		nano_cpu_idle();
	}
#else
	nano_cpu_idle();
#endif
}

void _sys_power_save_idle_exit(int32_t ticks)
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
#ifdef CONFIG_TICKLESS_IDLE
	if ((ticks == K_FOREVER) || ticks >= _sys_idle_threshold_ticks) {
		/* Resume normal periodic system timer interrupts */

		_timer_idle_exit();
	}
#else
	ARG_UNUSED(ticks);
#endif /* CONFIG_TICKLESS_IDLE */
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

	extern uint64_t __idle_tsc;

	__idle_tsc = _tsc_read();
#endif

	for (;;) {
		(void)irq_lock();
		_sys_power_save_idle(_get_next_timeout_expiry());

		IDLE_YIELD_IF_COOP();
	}
}
