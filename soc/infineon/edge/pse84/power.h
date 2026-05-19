/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Infineon PSOC Edge power management interface.
 */

#ifndef ZEPHYR_SOC_INFINEON_EDGE_PSE84_POWER_H_
#define ZEPHYR_SOC_INFINEON_EDGE_PSE84_POWER_H_

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
 * Holds a Zephyr PM-policy lock on both deep states this SoC can enter from
 * idle - DeepSleep (PM_STATE_SUSPEND_TO_IDLE) and DeepSleep-RAM
 * (PM_STATE_SUSPEND_TO_RAM) - so that while the calling thread blocks (on a
 * transfer-completion semaphore, a vendor deepsleep-lock section, a blocking
 * IPC round-trip, etc.) the idle thread cannot let the policy gate the core
 * mid-operation and drop the pending completion.  Only shallow runtime-idle
 * (WFI, interrupts live) stays available for the duration.
 *
 * The locks are reference-counted, so nested and concurrent sections balance
 * correctly; every ifx_pm_deepsleep_lock() must be paired with a matching
 * ifx_pm_deepsleep_unlock().
 */
static inline void ifx_pm_deepsleep_lock(void)
{
	pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_RAM, PM_ALL_SUBSTATES);
}

/**
 * @brief Release the lock taken by ifx_pm_deepsleep_lock().
 *
 * Releases in reverse order of acquisition.
 */
static inline void ifx_pm_deepsleep_unlock(void)
{
	pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_RAM, PM_ALL_SUBSTATES);
	pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
}

#if defined(CONFIG_PM_S2RAM) && defined(CONFIG_PSOC_EDGE_M55_SRF_SUPPORT)
/**
 * @brief Re-establish the CM55 SRF/IPC client after a DeepSleep-RAM warm boot.
 *
 * DeepSleep-RAM power-cycles this core: it resumes from retained RAM without
 * re-running soc_early_init_hook(), so its SRF client still references the
 * IPC/SRF endpoint published before sleep.  Meanwhile the CM33 secure image
 * cold-boots and republishes that endpoint.  The stale client would fault on
 * the first secure request (a PPC-secured clock/PDL call relayed over IPC), so
 * the SoC power resume path calls this to re-run the SRF portion of the SoC
 * bring-up and re-synchronize the client with the freshly published endpoint.
 *
 * Must be called in thread context (interrupts enabled, scheduler running): the
 * IPC handle handshake blocks on a secure-core semaphore.
 */
void ifx_soc_srf_client_reinit(void);
#endif /* CONFIG_PM_S2RAM && CONFIG_PSOC_EDGE_M55_SRF_SUPPORT */

#if defined(CONFIG_PM_S2RAM) && defined(CONFIG_PM_DEVICE)
/**
 * @brief Eagerly rebuild every device after a DeepSleep-RAM warm boot.
 *
 * Walks every device in link (initialization) order and invokes its
 * PM_DEVICE_ACTION_TURN_ON handler, restoring all peripherals in a single
 * up-front pass.  DeepSleep-RAM power-cycles the peripheral domain, so after a
 * warm boot every peripheral has lost its hardware state; the application calls
 * this from thread context, once it is running again, to restore them.
 *
 * Must be called from thread context on SRF builds: each TURN_ON handler relays
 * to the secure core over blocking IPC, so this re-establishes the SRF client
 * first and serializes the walk under the PM-policy lock.  A device with no
 * PM_DEVICE_ACTION_TURN_ON handler is skipped.
 */
void ifx_pm_warm_boot_reinit_all(void);
#else
static inline void ifx_pm_warm_boot_reinit_all(void)
{
}
#endif /* CONFIG_PM_S2RAM && CONFIG_PM_DEVICE */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SOC_INFINEON_EDGE_PSE84_POWER_H_ */
