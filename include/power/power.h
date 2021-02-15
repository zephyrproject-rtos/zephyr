/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_POWER_POWER_H_
#define ZEPHYR_INCLUDE_POWER_POWER_H_

#include <zephyr/types.h>
#include <sys/slist.h>
#include <power/power_state.h>
#include <toolchain.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup power_management_api Power Management
 * @{
 * @}
 */

#ifdef CONFIG_PM

extern unsigned char pm_idle_exit_notify;

/**
 * @brief System Power Management API
 *
 * @defgroup system_power_management_api System Power Management API
 * @ingroup power_management_api
 * @{
 */

/**
 * Power management notifier struct
 *
 * This struct contains callbacks that are called when the target enters and
 * exits power states.
 *
 * As currently implemented the entry callback is invoked when
 * transitioning from PM_STATE_ACTIVE to another state, and the exit
 * callback is invoked when transitioning from a non-active state to
 * PM_STATE_ACTIVE. This behavior may change in the future.
 *
 * @note These callbacks can be called from the ISR of the event
 *       that caused the kernel exit from idling.
 */
struct pm_notifier {
	sys_snode_t _node;
	/**
	 * Application defined function for doing any target specific operations
	 * for power state entry.
	 */
	void (*state_entry)(enum pm_state state);
	/**
	 * Application defined function for doing any target specific operations
	 * for power state exit.
	 */
	void (*state_exit)(enum pm_state state);
};

/**
 * @brief Check if particular power state is a sleep state.
 *
 * This function returns true if given power state is a sleep state.
 */
static inline bool pm_is_sleep_state(enum pm_state state)
{
	bool ret = true;

	switch (state) {
	case PM_STATE_RUNTIME_IDLE:
		__fallthrough;
	case PM_STATE_SUSPEND_TO_IDLE:
		__fallthrough;
	case PM_STATE_STANDBY:
		break;
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
static inline bool pm_is_deep_sleep_state(enum pm_state state)
{
	bool ret = true;

	switch (state) {
	case PM_STATE_SUSPEND_TO_RAM:
		__fallthrough;
	case PM_STATE_SUSPEND_TO_DISK:
		break;
	default:
		ret = false;
		break;
	}

	return ret;
}

/**
 * @brief Function to disable power management idle exit notification
 *
 * The pm_system_resume() would be called from the ISR of the event that caused
 * exit from kernel idling after PM operations. For some power operations,
 * this notification may not be necessary. This function can be called in
 * pm_system_suspend to disable the corresponding pm_system_resume notification.
 *
 */
static inline void pm_idle_exit_notification_disable(void)
{
	pm_idle_exit_notify = 0U;
}

/**
 * @brief Force usage of given power state.
 *
 * This function overrides decision made by PM policy forcing
 * usage of given power state in the ongoing suspend operation.
 * And before the end of suspend, the state of forced_pm_state
 * is cleared with interrupt disabled.
 *
 * If enabled PM_DIRECT_FORCE_MODE, this function can only
 * run in thread context.
 *
 * @param info Power state which should be used in the ongoing
 *	suspend operation.
 */
void pm_power_state_force(struct pm_state_info info);

/**
 * @brief Put processor into a power state.
 *
 * This function implements the SoC specific details necessary
 * to put the processor into available power states.
 *
 * @param info Power state which should be used in the ongoing
 *	suspend operation.
 */
void pm_power_state_set(struct pm_state_info info);

#ifdef CONFIG_PM_DEBUG
/**
 * @brief Dump Low Power states related debug info
 *
 * Dump Low Power states debug info like LPS entry count and residencies.
 */
void pm_dump_debug_info(void);

#endif /* CONFIG_PM_DEBUG */

/**
 * @brief Set a constraint for a power state
 *
 * @details Disabled state cannot be selected by the Zephyr power
 *	    management policies. Application defined policy should
 *	    use the @ref pm_constraint_get function to
 *	    check if given state is enabled and could be used.
 *
 * @note This API is refcount
 *
 * @param [in] state Power state to be disabled.
 */
void pm_constraint_set(enum pm_state state);

/**
 * @brief Release a constraint for a power state
 *
 * @details Enabled state can be selected by the Zephyr power
 *	    management policies. Application defined policy should
 *	    use the @ref pm_constraint_get function to
 *	    check if given state is enabled and could be used.
 *	    By default all power states are enabled.
 *
 * @note This API is refcount
 *
 * @param [in] state Power state to be enabled.
 */
void pm_constraint_release(enum pm_state state);

/**
 * @brief Check if particular power state is enabled
 *
 * This function returns true if given power state is enabled.
 *
 * @param [in] state Power state.
 */
bool pm_constraint_get(enum pm_state state);


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
void pm_system_resume_from_deep_sleep(void);

/**
 * @brief Notify exit from kernel idling after PM operations
 *
 * This function would notify exit from kernel idling if a corresponding
 * pm_system_suspend() notification was handled and did not return
 * POWER_STATE_ACTIVE.
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
 * notification. Alternatively pm_idle_exit_notification_disable() can
 * be called in pm_system_suspend to disable this notification.
 */
void pm_system_resume(void);

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
 * @return Power state which was entered or POWER_STATE_ACTIVE if SoC was
 *         kept in the active state.
 */
enum pm_state pm_system_suspend(int32_t ticks);

/**
 * @brief Do any SoC or architecture specific post ops after sleep state exits.
 *
 * This function is a place holder to do any operations that may
 * be needed to be done after sleep state exits. Currently it enables
 * interrupts after resuming from sleep state. In future, the enabling
 * of interrupts may be moved into the kernel.
 */
void pm_power_state_exit_post_ops(struct pm_state_info info);

/**
 * @brief Register a power management notifier
 *
 * Register the given notifier from the power management notification
 * list.
 *
 * @param notifier pm_notifier object to be registered.
 */
void pm_notifier_register(struct pm_notifier *notifier);

/**
 * @brief Unregister a power management notifier
 *
 * Remove the given notifier from the power management notification
 * list. After that this object callbacks will not be called.
 *
 * @param notifier pm_notifier object to be unregistered.
 *
 * @return 0 if the notifier was successfully removed, a negative value
 * otherwise.
 */
int pm_notifier_unregister(struct pm_notifier *notifier);

/**
 * @}
 */


void z_pm_save_idle_exit(int32_t ticks);
#endif /* CONFIG_PM */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_POWER_POWER_H_ */
