/*
 * Copyright (c) 2020 Intel corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_POWER_POWER_STATE_H_
#define ZEPHYR_INCLUDE_POWER_POWER_STATE_H_

#include <sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Runtime active state
 *
 * During runtime active state, the system is awake and running.
 * In simple terms, the system is in a full running state.
 */
#define PM_STATE_RUNTIME_ACTIVE        BIT(0)

/**
 * @brief Runtime idle state
 *
 * Runtime idle is a system sleep state in which all of the cores
 * enter deepest possible idle state and wait for interrupts, no
 * requirements for the devices, leaving them at the states where
 * they are.
 */
#define PM_STATE_RUNTIME_IDLE          BIT(1)

/**
 * @brief Suspend to idle state
 *
 * The system goes through a normal platform suspend where it puts
 * all of the cores in idle state and may put peripherals into low
 * power states. No operating state is lost (ie. the cpu core does
 * not lose execution context), so the system can go back to where
 * it left off easily enough.
 */
#define PM_STATE_SUSPEND_TO_IDLE       BIT(2)

/**
 * @brief Standby state
 *
 * In addition to putting peripherals into low power state, which is
 * done for suspend to idle too, all non-boot CPUs are powered off.
 * It should allow more energy to be saved relative to suspend to
 * idle, but the resume latency will generally be greater than suspned
 * to idle state. But it should be the same state with suspend to idle
 * state on uniprocesser system.
 */
#define PM_STATE_STANDBY               BIT(3)

/**
 * @brief Suspend to ram state
 *
 * This state offers significant energy savings by powering off as
 * much of the system as possible, where memory should be placed into
 * the self-refresh mode to retain its contents. The state of devices
 * and CPUs is saved and held in memory, and it may require some boot
 * strapping code in ROM to resume the system from it.
 */
#define PM_STATE_SUSPEND_TO_RAM        BIT(4)

/**
 * @brief Suspend to disk state
 *
 * This state offers significant energy savings by powering off as
 * much of the system as possible, including the memory. The contents
 * of memory are written to disk or other non-volatile storage, and
 * on resume it's read back into memory with the help of boot strapping
 * code, restores the system to the same point of execution where it
 * went to suspend to disk.
 */
#define PM_STATE_SUSPEND_TO_DISK       BIT(5)

/**
 * @brief Soft off state
 *
 * This state consumes a minimal amount of power and requires a large
 * latency in order to return to runtime active state. The contents of
 * system(CPU and memory) will not be preserved, so the system will be
 * restarted as if from initial power up and kernel boot.
 */
#define PM_STATE_SOFT_OFF              BIT(6)

/**
 * @brief PM state max
 */
#define PM_STATE_MAX                   (7)

/**
 * @brief PM state bit mask
 */
#define PM_STATE_BIT_MASK              BIT_MASK(PM_STATE_MAX)

/**
 * @brief Device active state
 *
 * Device is fully on and operates in normal state.
 */
#define DEVICE_PM_ACTIVE_STATE         BIT(0)

/**
 * @brief Device suspend state
 *
 * Device is in low-power state whose definition varies by device, but
 * device context can be restored to normal active state.
 */
#define DEVICE_PM_SUSPEND_STATE        BIT(1)

/**
 * @brief Device off state
 *
 * Device is fully powered off, no response at all. Need reinitialize
 * to go back normal active state.
 */
#define DEVICE_PM_OFF_STATE            BIT(2)

/**
 * @brief Device pm state bit mask
 */
#define DEVICE_PM_STATE_MASK           BIT_MASK(3)

/**
 * @brief System power state type definition
 */
typedef uint8_t pm_state_t;

/**
 * @brief Device power state type definition
 */
typedef uint8_t dev_pm_state_t;

/**
 * @brief Mapping from system power state to device power state
 *
 * Power state where devices should locate given system power state.
 *
 * @param pm_state System power state.
 *
 * @return Device power state given system power state.
 */
static inline dev_pm_state_t pm_state_sys2dev(pm_state_t pm_state)
{
	switch (pm_state & PM_STATE_BIT_MASK) {
		case PM_STATE_RUNTIME_IDLE:
			return DEVICE_PM_ACTIVE_STATE;
		case PM_STATE_SUSPEND_TO_IDLE:
			return DEVICE_PM_SUSPEND_STATE;
		case PM_STATE_STANDBY:
			return DEVICE_PM_SUSPEND_STATE;
		case PM_STATE_SUSPEND_TO_RAM:
			return DEVICE_PM_OFF_STATE;
		case PM_STATE_SUSPEND_TO_DISK:
			return DEVICE_PM_OFF_STATE;
		case PM_STATE_SOFT_OFF:
			return DEVICE_PM_OFF_STATE;
		default:
			return DEVICE_PM_ACTIVE_STATE;
	}
}

#ifdef __cplusplus
}
#endif

#endif
