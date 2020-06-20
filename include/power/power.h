/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_POWER_POWER_H_
#define ZEPHYR_INCLUDE_POWER_POWER_H_

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
#ifdef CONFIG_SYS_POWER_SLEEP_STATES
# ifdef CONFIG_HAS_SYS_POWER_STATE_SLEEP_1
	SYS_POWER_STATE_SLEEP_1,
# endif
# ifdef CONFIG_HAS_SYS_POWER_STATE_SLEEP_2
	SYS_POWER_STATE_SLEEP_2,
# endif
# ifdef CONFIG_HAS_SYS_POWER_STATE_SLEEP_3
	SYS_POWER_STATE_SLEEP_3,
# endif
#endif /* CONFIG_SYS_POWER_SLEEP_STATES */

#ifdef CONFIG_SYS_POWER_DEEP_SLEEP_STATES
# ifdef CONFIG_HAS_SYS_POWER_STATE_DEEP_SLEEP_1
	SYS_POWER_STATE_DEEP_SLEEP_1,
# endif
# ifdef CONFIG_HAS_SYS_POWER_STATE_DEEP_SLEEP_2
	SYS_POWER_STATE_DEEP_SLEEP_2,
# endif
# ifdef CONFIG_HAS_SYS_POWER_STATE_DEEP_SLEEP_3
	SYS_POWER_STATE_DEEP_SLEEP_3,
# endif
#endif /* CONFIG_SYS_POWER_DEEP_SLEEP_STATES */
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
 * @brief Check if particular power state is a sleep state.
 *
 * This function returns true if given power state is a sleep state.
 */
static inline bool sys_pm_is_sleep_state(enum power_states state)
{
	bool ret = true;

	switch (state) {
#ifdef CONFIG_SYS_POWER_SLEEP_STATES
# ifdef CONFIG_HAS_SYS_POWER_STATE_SLEEP_1
	case SYS_POWER_STATE_SLEEP_1:
		break;
# endif
# ifdef CONFIG_HAS_SYS_POWER_STATE_SLEEP_2
	case SYS_POWER_STATE_SLEEP_2:
		break;
# endif
# ifdef CONFIG_HAS_SYS_POWER_STATE_SLEEP_3
	case SYS_POWER_STATE_SLEEP_3:
		break;
# endif
#endif /* CONFIG_SYS_POWER_SLEEP_STATES */
	default:
		ret = false;
		break;
	}

	return ret;
}

/**
 * @brief Check if particular power state is a deep sleep state.
 *
 * This function returns true if given power state is a deep sleep state.
 */
static inline bool sys_pm_is_deep_sleep_state(enum power_states state)
{
	bool ret = true;

	switch (state) {
#ifdef CONFIG_SYS_POWER_DEEP_SLEEP_STATES
# ifdef CONFIG_HAS_SYS_POWER_STATE_DEEP_SLEEP_1
	case SYS_POWER_STATE_DEEP_SLEEP_1:
		break;
# endif
# ifdef CONFIG_HAS_SYS_POWER_STATE_DEEP_SLEEP_2
	case SYS_POWER_STATE_DEEP_SLEEP_2:
		break;
# endif
# ifdef CONFIG_HAS_SYS_POWER_STATE_DEEP_SLEEP_3
	case SYS_POWER_STATE_DEEP_SLEEP_3:
		break;
# endif
#endif /* CONFIG_SYS_POWER_DEEP_SLEEP_STATES */

	default:
		ret = false;
		break;
	}

	return ret;
}

/**
 * @brief Function to disable power management idle exit notification
 *
 * The _sys_resume() would be called from the ISR of the event that caused
 * exit from kernel idling after PM operations. For some power operations,
 * this notification may not be necessary. This function can be called in
 * _sys_suspend to disable the corresponding _sys_resume notification.
 *
 */
static inline void _sys_pm_idle_exit_notification_disable(void)
{
	sys_pm_idle_exit_notify = 0U;
}

/**
 * @brief Force usage of given power state.
 *
 * This function overrides decision made by PM policy forcing
 * usage of given power state in the ongoing suspend operation.
 * And before the end of suspend, the state of forced_pm_state
 * is cleared with interrupt disabled.
 *
 * If enabled SYS_PM_DIRECT_FORCE_MODE, this function can only
 * run in thread context.
 *
 * @param state Power state which should be used in the ongoing
 *		suspend operation or SYS_POWER_STATE_AUTO.
 */
void sys_pm_force_power_state(enum power_states state);

/**
 * @brief Put processor into a power state.
 *
 * This function implements the SoC specific details necessary
 * to put the processor into available power states.
 */
void sys_set_power_state(enum power_states state);

#ifdef CONFIG_SYS_PM_DEBUG
/**
 * @brief Dump Low Power states related debug info
 *
 * Dump Low Power states debug info like LPS entry count and residencies.
 */
void sys_pm_dump_debug_info(void);

#endif /* CONFIG_SYS_PM_DEBUG */

#ifdef CONFIG_SYS_PM_STATE_LOCK
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
void sys_pm_ctrl_disable_state(enum power_states state);

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
void sys_pm_ctrl_enable_state(enum power_states state);

/**
 * @brief Check if particular power state is enabled
 *
 * This function returns true if given power state is enabled.
 *
 * @param [in] state Power state.
 */
bool sys_pm_ctrl_is_state_enabled(enum power_states state);

#endif /* CONFIG_SYS_PM_STATE_LOCK */

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
 * @brief Restore context to the point where system entered the deep sleep
 * state.
 *
 * This function is optionally called when exiting from deep sleep if the SOC
 * interface does not have bootloader support to handle resume from deep sleep.
 * This function should restore context to the point where system entered
 * the deep sleep state.
 *
 * If the function is called at cold boot it should return immediately.
 *
 * @note This function is not supported on all architectures.
 */
void _sys_resume_from_deep_sleep(void);

/**
 * @brief Notify exit from kernel idling after PM operations
 *
 * This function would notify exit from kernel idling if a corresponding
 * _sys_suspend() notification was handled and did not return
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
 * notification. Alternatively _sys_pm_idle_exit_notification_disable() can
 * be called in _sys_suspend to disable this notification.
 */
void _sys_resume(void);

/**
 * @brief Allow entry to power state
 *
 * When the kernel is about to go idle, it calls this function to notify the
 * power management subsystem, that the kernel is ready to enter the idle state.
 *
 * At this point, the kernel has disabled interrupts and computed the maximum
 * time the system can remain idle. The function passes the time that the system
 * can remain idle. The SOC interface performs power operations that can be done
 * in the available time. The power management operations must halt execution of
 * the CPU.
 *
 * This function assumes that a wake up event has already been set up by the
 * application.
 *
 * This function is entered with interrupts disabled. It should re-enable
 * interrupts if it had entered a power state.
 *
 * @param ticks The upcoming kernel idle time.
 *
 * @return Power state which was entered or SYS_POWER_STATE_ACTIVE if SoC was
 *         kept in the active state.
 */
enum power_states _sys_suspend(int32_t ticks);

/**
 * @brief Do any SoC or architecture specific post ops after sleep state exits.
 *
 * This function is a place holder to do any operations that may
 * be needed to be done after sleep state exits. Currently it enables
 * interrupts after resuming from sleep state. In future, the enabling
 * of interrupts may be moved into the kernel.
 */
void _sys_pm_power_state_exit_post_ops(enum power_states state);

/**
 * @brief Application defined function for power state entry
 *
 * Application defined function for doing any target specific operations
 * for power state entry.
 */
void sys_pm_notify_power_state_entry(enum power_states state);

/**
 * @brief Application defined function for sleep state exit
 *
 * Application defined function for doing any target specific operations
 * for sleep state exit.
 */
void sys_pm_notify_power_state_exit(enum power_states state);

/**
 * @}
 */

#endif /* CONFIG_SYS_POWER_MANAGEMENT */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_POWER_POWER_H_ */
