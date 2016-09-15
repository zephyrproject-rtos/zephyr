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
#include <nano_private.h>
#include <toolchain.h>
#include <sections.h>
#include <drivers/system_timer.h>
#include <wait_q.h>

#if defined(CONFIG_TICKLESS_IDLE)
/*
 * Idle time must be this value or higher for timer to go into tickless idle
 * state.
 */
int32_t _sys_idle_threshold_ticks = CONFIG_TICKLESS_IDLE_THRESH;
#endif /* CONFIG_TICKLESS_IDLE */

#ifdef CONFIG_SYS_POWER_MANAGEMENT
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
	_nanokernel.idle = ticks;
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
defined(CONFIG_SYS_POWER_DEEP_SLEEP) || \
defined(CONFIG_DEVICE_POWER_MANAGEMENT))
	/*
	 * Call the suspend hook function, which checks if the system should
	 * enter deep sleep, low power state or only suspend devices.
	 * If the time available is too short for any PM operation then
	 * the function returns SYS_PM_NOT_HANDLED immediately and kernel
	 * does normal idle processing. Otherwise it will return the code
	 * corresponding to the action taken.
	 *
	 * This function can just suspend devices without entering
	 * deep sleep or cpu low power state.  In this case it should return
	 * SYS_PM_DEVICE_SUSPEND_ONLY and kernel would do normal idle
	 * processing.
	 *
	 * This function is entered with interrupts disabled. If the function
	 * returns either SYS_PM_LOW_POWER_STATE or SYS_PM_DEEP_SLEEP then
	 * it should ensure interrupts are re-enabled before returning.
	 * This is because the kernel does not do its own idle processing in
	 * these cases i.e. skips nano_cpu_idle(). The kernel's idle
	 * processing re-enables interrupts which is essential for kernel's
	 * scheduling logic.
	 */
	if (!(_sys_soc_suspend(ticks) &
	    (SYS_PM_DEEP_SLEEP | SYS_PM_LOW_POWER_STATE))) {
		nano_cpu_idle();
	}
#else
	nano_cpu_idle();
#endif
}

void _sys_power_save_idle_exit(int32_t ticks)
{
#if (defined(CONFIG_SYS_POWER_LOW_POWER_STATE) || \
	defined(CONFIG_SYS_POWER_DEEP_SLEEP) || \
	defined(CONFIG_DEVICE_POWER_MANAGEMENT))
	/*
	 * Any idle wait based on CPU low power state will be exited by
	 * interrupt. This function is called within that interrupt's
	 * ISR context.  _sys_soc_resume() needs to be called here
	 * to handle exit from CPU low power states. This gives an
	 * opportunity for device states altered in _sys_soc_suspend()
	 * to be restored before the kernel schedules another thread.
	 * _sys_soc_resume() is not called from here for deep sleep
	 * exit. Deep sleep recovery happens at cold boot path.
	 */
	_sys_soc_resume();
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


void idle(void *unused1, void *unused2, void *unused3)
{
	ARG_UNUSED(unused1);
	ARG_UNUSED(unused2);
	ARG_UNUSED(unused3);

	for (;;) {
		_sys_power_save_idle(_timeout_get_next_expiry());

		k_yield();
	}
}
