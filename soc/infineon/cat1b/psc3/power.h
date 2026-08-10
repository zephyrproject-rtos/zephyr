/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Infineon PSOC Control C3 power management interface.
 */

#ifndef ZEPHYR_SOC_INFINEON_CAT1B_PSC3_POWER_H_
#define ZEPHYR_SOC_INFINEON_CAT1B_PSC3_POWER_H_

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/sys/util.h>
#include <zephyr/pm/policy.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Block the CPU-gating low-power states around a critical section.
 *
 * Holds a Zephyr PM-policy lock on the two deep states this SoC can enter from
 * idle - DeepSleep (PM_STATE_SUSPEND_TO_IDLE) and DeepSleep-RAM
 * (PM_STATE_SUSPEND_TO_RAM) - so the idle thread cannot gate the core while the
 * calling thread blocks.  Only shallow runtime-idle (WFI) stays available.  The
 * locks are reference-counted, so every lock must be paired with an unlock.
 */
static inline void ifx_pm_deepsleep_lock(void)
{
	pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_RAM, PM_ALL_SUBSTATES);
}

/**
 * @brief Release the lock taken by ifx_pm_deepsleep_lock().
 */
static inline void ifx_pm_deepsleep_unlock(void)
{
	pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_RAM, PM_ALL_SUBSTATES);
	pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
}

/**
 * @brief Per-driver warm-boot rebuild hook (no-op on this SoC).
 *
 * DeepSleep-RAM power-gates the peripheral domain, so after a warm boot every
 * peripheral has lost its hardware state.  This SoC rebuilds them all eagerly
 * from the resume path (ifx_pm_warm_boot_reinit_all() in ifx_pm_s2ram_enter(),
 * before any application thread runs), so by the time a driver reaches its first
 * post-wake API entry its block is already restored and this hook has nothing to
 * do.  It is kept so a driver can call it unconditionally at its API boundary.
 *
 * @param dev      Device to rebuild (unused).
 * @param last_gen Pointer to the driver's stored warm-boot generation (unused).
 * @return Always false (no rebuild performed; the eager resume-path pass already
 *         restored the device).
 */
#if defined(CONFIG_PM_S2RAM) && defined(CONFIG_PM_DEVICE)
bool ifx_pm_warm_boot_reinit(const struct device *dev, uint32_t *last_gen);

/**
 * @brief Eagerly rebuild every device after a DeepSleep-RAM warm boot.
 *
 * Walks every device in link (initialization) order and invokes its
 * PM_DEVICE_ACTION_TURN_ON handler, restoring all peripherals in a single
 * up-front pass rather than lazily on first use (see ifx_pm_warm_boot_reinit()).
 * Intended for an application that wants to force a full peripheral restore
 * after a warm boot - notably to bring the console UART back before the first
 * post-wake output.  A device with no PM action handler is skipped.  Must be
 * called from thread context.
 */
void ifx_pm_warm_boot_reinit_all(void);
#else
static inline bool ifx_pm_warm_boot_reinit(const struct device *dev, uint32_t *last_gen)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(last_gen);
	return false;
}

static inline void ifx_pm_warm_boot_reinit_all(void)
{
}
#endif /* CONFIG_PM_S2RAM && CONFIG_PM_DEVICE */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SOC_INFINEON_CAT1B_PSC3_POWER_H_ */
