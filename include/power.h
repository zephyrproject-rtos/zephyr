/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
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

#ifndef __INCpower
#define __INCpower

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_SYS_POWER_MANAGEMENT

/* Constants identifying power state categories */
#define SYS_PM_ACTIVE_STATE		0 /* SOC and CPU are in active state */
#define SYS_PM_LOW_POWER_STATE		1 /* CPU low power state */
#define SYS_PM_DEEP_SLEEP		2 /* SOC low power state */

#define SYS_PM_NOT_HANDLED		SYS_PM_ACTIVE_STATE

extern unsigned char _sys_soc_notify_wake_event;

/**
 * @brief Power Management Hook Interface
 *
 * @defgroup power_management_hook_interface Power Management Hook Interface
 * @ingroup power_management_api
 * @{
 */

/**
 * @brief Function to disable wake event notification
 *
 * _sys_soc_resume() would be called from the ISR that caused exit from
 * low power state. This function can be called at _sys_soc_suspend to disable
 * this notification.
 */
static inline void _sys_soc_disable_wake_event_notification(void)
{
	_sys_soc_notify_wake_event = 0;
}

/**
 * @brief Hook function to notify exit from low power state
 *
 * The purpose of this function is to notify exit from
 * low power states. The implementation of this function can vary
 * depending on the soc specific boot flow.
 *
 * In the case of recovery from soc low power states like deep sleep,
 * this function would switch cpu context to the execution point at the time
 * system entered the soc low power state.
 *
 * In boot flows where this function gets called even at cold boot, the
 * function should return immediately.
 *
 * Wake event notification:
 * This function would also be called from the ISR context of the event
 * that caused exit from the low power state. This will be called immediately
 * after interrupts are enabled. This is called to give a chance to do
 * any operations before the kernel would switch tasks or processes nested
 * interrupts. This is required for cpu low power states that would require
 * interrupts to be enabled while entering low power states. e.g. C1 in x86. In
 * those cases, the ISR would be invoked immediately after the event wakes up
 * the CPU, before code following the CPU wait, gets a chance to execute. This
 * can be ignored if no operation needs to be done at the wake event
 * notification. Alternatively _sys_soc_disable_wake_event_notification() can
 * be called in _sys_soc_suspend to disable this notification.
 *
 * @note A dedicated function may be created in future to notify wake
 * events, instead of overloading this one.
 */
extern void _sys_soc_resume(void);

/**
 * @brief Hook function to allow entry to low power state
 *
 * This function is called by the kernel when it is about to idle.
 * It is passed the number of clock ticks that the kernel calculated
 * as available time to idle.
 *
 * The implementation of this function is dependent on the soc specific
 * components and the various schemes they support. Some implementations
 * may choose to do device PM operations in this function, while others
 * would not need to, because they would have done it at other places.
 *
 * Typically a wake event is set and the soc or cpu is put to any of the
 * supported low power states. The wake event should be set to wake up
 * the soc or cpu before the available idle time expires to avoid disrupting
 * the kernel's scheduling.
 *
 * This function is entered with interrupts disabled. It should
 * re-enable interrupts if it had entered a low power state.
 *
 * @param ticks the upcoming kernel idle time
 *
 * @retval SYS_PM_NOT_HANDLED If low power state was not entered.
 * @retval SYS_PM_LOW_POWER_STATE If CPU low power state was entered.
 * @retval SYS_PM_DEEP_SLEEP If SOC low power state was entered.
 */
extern int _sys_soc_suspend(int32_t ticks);

/**
 * @}
 */

#endif /* CONFIG_SYS_POWER_MANAGEMENT */

#ifdef __cplusplus
}
#endif

#endif /* __INCpower */
