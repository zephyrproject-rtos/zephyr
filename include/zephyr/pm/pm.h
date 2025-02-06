/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_PM_PM_H_
#define ZEPHYR_INCLUDE_PM_PM_H_

#include <zephyr/types.h>
#include <zephyr/sys/slist.h>
#include <zephyr/pm/state.h>
#include <zephyr/toolchain.h>
#include <errno.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief System and device power management
 * @defgroup subsys_pm Power Management (PM)
 * @since 1.2
 * @ingroup os_services
 * @{
 * @}
 */

/**
 * @brief System Power Management API
 * @defgroup subsys_pm_sys System
 * @since 1.2
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
bool pm_state_force(uint8_t cpu, const struct pm_state_info *info);

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
const struct pm_state_info *pm_state_next_get(uint8_t cpu);

/**
 * @brief Notify exit from kernel sleep.
 *
 * This function would notify exit from kernel idling if a corresponding
 * pm_system_suspend() notification was handled and did not return
 * PM_STATE_ACTIVE.
 *
 * This function should be called from the ISR context of the event
 * that caused the exit from kernel idling.
 *
 * This is required for cpu power states that would require
 * interrupts to be enabled while entering low power states. e.g. C1 in x86. In
 * those cases, the ISR would be invoked immediately after the event wakes up
 * the CPU, before code following the CPU wait, gets a chance to execute. This
 * can be ignored if no operation needs to be done at the wake event
 * notification.
 */
void pm_system_resume(void);

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
 * @param state Power state.
 * @param substate_id Power substate id.
 */
void pm_state_set(enum pm_state state, uint8_t substate_id);

/**
 * @brief Do any SoC or architecture specific post ops after sleep state exits.
 *
 * This function is a place holder to do any operations that may
 * be needed to be done after sleep state exits. Currently it enables
 * interrupts after resuming from sleep state. In future, the enabling
 * of interrupts may be moved into the kernel.
 *
 * @param state Power state.
 * @param substate_id Power substate id.
 */
void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id);

/**
 * @}
 */

#else  /* CONFIG_PM */

static inline void pm_notifier_register(struct pm_notifier *notifier)
{
	ARG_UNUSED(notifier);
}

static inline int pm_notifier_unregister(struct pm_notifier *notifier)
{
	ARG_UNUSED(notifier);

	return -ENOSYS;
}

static inline const struct pm_state_info *pm_state_next_get(uint8_t cpu)
{
	ARG_UNUSED(cpu);

	return NULL;
}

static inline void pm_system_resume(void)
{
}

#endif /* CONFIG_PM */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_PM_PM_H_ */
