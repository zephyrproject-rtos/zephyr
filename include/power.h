/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_POWER_H_
#define ZEPHYR_INCLUDE_POWER_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_SYS_POWER_MANAGEMENT

/* Constants identifying power state categories */
#define SYS_PM_ACTIVE_STATE		0 /* SOC and CPU are in active state */
#define SYS_PM_LOW_POWER_STATE		1 /* CPU low power state */
#define SYS_PM_DEEP_SLEEP		2 /* SOC low power state */

#define SYS_PM_NOT_HANDLED		SYS_PM_ACTIVE_STATE

extern unsigned char _sys_pm_idle_exit_notify;

/**
 * @brief Power Management Hook Interface
 *
 * @defgroup power_management_hook_interface Power Management Hook Interface
 * @ingroup power_management_api
 * @{
 */

/**
 * @brief Function to disable power management idle exit notification
 *
 * _sys_soc_resume() would be called from the ISR of the event that caused
 * exit from kernel idling after PM operations. For some power operations,
 * this notification may not be necessary. This function can be called in
 * _sys_soc_suspend to disable the corresponding _sys_soc_resume notification.
 *
 */
static inline void _sys_soc_pm_idle_exit_notification_disable(void)
{
	_sys_pm_idle_exit_notify = 0;
}

/**
 * @brief Hook function to notify exit from deep sleep
 *
 * The purpose of this function is to notify exit from
 * deep sleep. The implementation of this function can vary
 * depending on the soc specific boot flow.
 *
 * This function would switch cpu context to the execution point at the time
 * system entered deep sleep power state. Some implementations may not require
 * use of this function e.g. the BSP or boot loader may do the context switch.
 *
 * In boot flows where this function gets called even at cold boot, the
 * function should return immediately.
 *
 */
void _sys_soc_resume_from_deep_sleep(void);

/**
 * @brief Hook function to notify exit from kernel idling after PM operations
 *
 * This function would notify exit from kernel idling if a corresponding
 * _sys_soc_suspend() notification was handled and did not return
 * SYS_PM_NOT_HANDLED.
 *
 * This function would be called from the ISR context of the event
 * that caused the exit from kernel idling. This will be called immediately
 * after interrupts are enabled. This is called to give a chance to do
 * any operations before the kernel would switch tasks or processes nested
 * interrupts. This is required for cpu low power states that would require
 * interrupts to be enabled while entering low power states. e.g. C1 in x86. In
 * those cases, the ISR would be invoked immediately after the event wakes up
 * the CPU, before code following the CPU wait, gets a chance to execute. This
 * can be ignored if no operation needs to be done at the wake event
 * notification. Alternatively _sys_soc_pm_idle_exit_notification_disable() can
 * be called in _sys_soc_suspend to disable this notification.
 *
 */
void _sys_soc_resume(void);

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
extern int _sys_soc_suspend(s32_t ticks);

/**
 * @}
 */

#endif /* CONFIG_SYS_POWER_MANAGEMENT */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_POWER_H_ */
