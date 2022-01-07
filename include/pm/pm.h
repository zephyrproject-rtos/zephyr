/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_PM_PM_H_
#define ZEPHYR_INCLUDE_PM_PM_H_

#include <zephyr/types.h>
#include <sys/slist.h>
#include <pm/state.h>
#include <toolchain.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief System and device power management
 * @defgroup subsys_pm Power Management (PM)
 * @ingroup subsys
 * @{
 * @}
 */

/**
 * @brief System Power Management API
 * @defgroup subsys_pm_sys System
 * @ingroup subsys_pm
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
 *
 * @note It is not allowed to call @ref pm_notifier_unregister or
 *       @ref pm_notifier_register from these callbacks because they are called
 *       with the spin locked in those functions.
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

#if defined(CONFIG_PM) || defined(__DOXYGEN__)
/**
 * @brief Force usage of given power state.
 *
 * This function overrides decision made by PM policy forcing
 * usage of given power state upon next entry of the idle thread.
 *
 * @note This function can only run in thread context
 *
 * @param cpu CPU index.
 * @param info Power state which should be used in the ongoing
 *	suspend operation.
 */
bool pm_power_state_force(uint8_t cpu, struct pm_state_info info);

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
 * @brief Gets the next power state that will be used.
 *
 * This function returns the next power state that will be used by the
 * SoC.
 *
 * @param cpu CPU index.
 * @return next pm_state_info that will be used
 */
struct pm_state_info pm_power_state_next_get(uint8_t cpu);

/**
 * @}
 */

/**
 * @brief System Power Management Constraints API
 * @defgroup subsys_pm_sys_constraint Constraints
 * @ingroup subsys_pm_sys
 * @{
 */

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
 * @brief System Power Management Hooks
 * @defgroup subsys_pm_sys_hooks Hooks
 * @ingroup subsys_pm_sys
 * @{
 */

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

/**
 * @brief Do any SoC or architecture specific post ops after sleep state exits.
 *
 * This function is a place holder to do any operations that may
 * be needed to be done after sleep state exits. Currently it enables
 * interrupts after resuming from sleep state. In future, the enabling
 * of interrupts may be moved into the kernel.
 *
 * @param info Power state that the given cpu is leaving.
 */
void pm_power_state_exit_post_ops(struct pm_state_info info);

/**
 * @}
 */

#else  /* CONFIG_PM */

#define pm_notifier_register(notifier)
#define pm_notifier_unregister(notifier) (-ENOSYS)

#define pm_constraint_set(pm_state)
#define pm_constraint_release(pm_state)
#define pm_constraint_get(pm_state) (true)

#define pm_power_state_set(info)
#define pm_power_state_exit_post_ops(info)
#define pm_power_state_next_get(cpu) \
	((struct pm_state_info){PM_STATE_ACTIVE, 0, 0})

#endif /* CONFIG_PM */

void z_pm_save_idle_exit(void);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_PM_PM_H_ */
