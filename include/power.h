/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_POWER_H_
#define ZEPHYR_INCLUDE_POWER_H_

#include <zephyr/types.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup power_management_api Power Management
 * @{
 * @}
 */

/**
 * @brief System power states.
 */
enum power_states {
	SYS_POWER_STATE_AUTO	= (-2),
	SYS_POWER_STATE_ACTIVE	= (-1),
#ifdef CONFIG_SYS_POWER_LOW_POWER_STATE
# ifdef CONFIG_SYS_POWER_STATE_CPU_LPS_SUPPORTED
	SYS_POWER_STATE_CPU_LPS,
# endif
# ifdef CONFIG_SYS_POWER_STATE_CPU_LPS_1_SUPPORTED
	SYS_POWER_STATE_CPU_LPS_1,
# endif
# ifdef CONFIG_SYS_POWER_STATE_CPU_LPS_2_SUPPORTED
	SYS_POWER_STATE_CPU_LPS_2,
# endif
#endif /* CONFIG_SYS_POWER_LOW_POWER_STATE */

#ifdef CONFIG_SYS_POWER_DEEP_SLEEP
# ifdef CONFIG_SYS_POWER_STATE_DEEP_SLEEP_SUPPORTED
	SYS_POWER_STATE_DEEP_SLEEP,
# endif
# ifdef CONFIG_SYS_POWER_STATE_DEEP_SLEEP_1_SUPPORTED
	SYS_POWER_STATE_DEEP_SLEEP_1,
# endif
# ifdef CONFIG_SYS_POWER_STATE_DEEP_SLEEP_2_SUPPORTED
	SYS_POWER_STATE_DEEP_SLEEP_2,
# endif
#endif /* CONFIG_SYS_POWER_DEEP_SLEEP */
	SYS_POWER_STATE_MAX
};

#ifdef CONFIG_SYS_POWER_MANAGEMENT

extern unsigned char sys_pm_idle_exit_notify;

/**
 * @brief System Power Management API
 *
 * @defgroup system_power_management_api System Power Management API
 * @ingroup power_management_api
 * @{
 */

/**
 * @brief Check if particular power state is a low power state.
 *
 * This function returns true if given power state is a low power state.
 */
static inline bool sys_pm_is_low_power_state(enum power_states state)
{
	switch (state) {
#ifdef CONFIG_SYS_POWER_LOW_POWER_STATE
# ifdef CONFIG_SYS_POWER_STATE_CPU_LPS_SUPPORTED
	case SYS_POWER_STATE_CPU_LPS:
		/* FALLTHROUGH */
# endif
# ifdef CONFIG_SYS_POWER_STATE_CPU_LPS_1_SUPPORTED
	case SYS_POWER_STATE_CPU_LPS_1:
		/* FALLTHROUGH */
# endif
# ifdef CONFIG_SYS_POWER_STATE_CPU_LPS_2_SUPPORTED
	case SYS_POWER_STATE_CPU_LPS_2:
		/* FALLTHROUGH */
# endif
		return true;
#endif /* CONFIG_SYS_POWER_LOW_POWER_STATE */

	default:
		return false;
	}
}

/**
 * @brief Check if particular power state is a deep sleep state.
 *
 * This function returns true if given power state is a deep sleep state.
 */
static inline bool sys_pm_is_deep_sleep_state(enum power_states state)
{
	switch (state) {
#ifdef CONFIG_SYS_POWER_DEEP_SLEEP
# ifdef CONFIG_SYS_POWER_STATE_DEEP_SLEEP_SUPPORTED
	case SYS_POWER_STATE_DEEP_SLEEP:
		/* FALLTHROUGH */
# endif
# ifdef CONFIG_SYS_POWER_STATE_DEEP_SLEEP_1_SUPPORTED
	case SYS_POWER_STATE_DEEP_SLEEP_1:
		/* FALLTHROUGH */
# endif
# ifdef CONFIG_SYS_POWER_STATE_DEEP_SLEEP_2_SUPPORTED
	case SYS_POWER_STATE_DEEP_SLEEP_2:
		/* FALLTHROUGH */
# endif
		return true;
#endif /* CONFIG_SYS_POWER_DEEP_SLEEP */

	default:
		return false;
	}
}

/**
 * @brief Function to disable power management idle exit notification
 *
 * The sys_resume() would be called from the ISR of the event that caused
 * exit from kernel idling after PM operations. For some power operations,
 * this notification may not be necessary. This function can be called in
 * sys_suspend to disable the corresponding sys_resume notification.
 *
 */
static inline void sys_pm_idle_exit_notification_disable(void)
{
	sys_pm_idle_exit_notify = 0;
}

/**
 * @brief Force usage of given power state.
 *
 * This function overrides decision made by PM policy
 * forcing usage of given power state in all subseqent
 * suspend operations. Forcing the SYS_POWER_STATE_AUTO
 * state restores normal operation.
 *
 * @param state Power state which should be used in all
 *		subsequent suspend operations or
 *		SYS_POWER_STATE_AUTO.
 */
extern void sys_pm_force_power_state(enum power_states state);

#ifdef CONFIG_PM_CONTROL_OS_DEBUG
/**
 * @brief Dump Low Power states related debug info
 *
 * Dump Low Power states debug info like LPS entry count and residencies.
 */
extern void sys_pm_dump_debug_info(void);

#endif /* CONFIG_PM_CONTROL_OS_DEBUG */

#ifdef CONFIG_PM_CONTROL_STATE_LOCK
/**
 * @brief Disable particular power state
 *
 * @details Disabled state cannot be selected by the Zephyr power
 *	    management policies. Application defined policy should
 *	    use the @ref sys_pm_ctrl_is_state_enabled function to
 *	    check if given state could is enabled and could be used.
 *
 * @param [in] state Power state to be disabled.
 */
extern void sys_pm_ctrl_disable_state(enum power_states state);

/**
 * @brief Enable particular power state
 *
 * @details Enabled state can be selected by the Zephyr power
 *	    management policies. Application defined policy should
 *	    use the @ref sys_pm_ctrl_is_state_enabled function to
 *	    check if given state could is enabled and could be used.
 *	    By default all power states are enabled.
 *
 * @param [in] state Power state to be enabled.
 */
extern void sys_pm_ctrl_enable_state(enum power_states state);

/**
 * @brief Check if particular power state is enabled
 *
 * This function returns true if given power state is enabled.
 *
 * @param [in] state Power state.
 */
extern bool sys_pm_ctrl_is_state_enabled(enum power_states state);

#endif /* CONFIG_PM_CONTROL_STATE_LOCK */

/**
 * @}
 */

/**
 * @brief Power Management Hooks
 *
 * @defgroup power_management_hook_interface Power Management Hooks
 * @ingroup power_management_api
 * @{
 */

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
void sys_resume_from_deep_sleep(void);

/**
 * @brief Hook function to notify exit from kernel idling after PM operations
 *
 * This function would notify exit from kernel idling if a corresponding
 * sys_suspend() notification was handled and did not return
 * SYS_POWER_STATE_ACTIVE.
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
 * notification. Alternatively sys_pm_idle_exit_notification_disable() can
 * be called in sys_suspend to disable this notification.
 *
 */
void sys_resume(void);

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
 * @return Power state which was selected and entered.
 */
extern enum power_states sys_suspend(s32_t ticks);

/**
 * @}
 */

#endif /* CONFIG_SYS_POWER_MANAGEMENT */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_POWER_H_ */
