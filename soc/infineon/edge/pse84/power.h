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
 * @brief Lazily rebuild a device after a DeepSleep-RAM warm boot.
 *
 * DS-RAM power-cycles the peripheral domain, so after a warm boot every
 * peripheral has lost its hardware state and must be rebuilt.  Rather than
 * rebuild everything up front from a worker - which races the already-runnable
 * application thread the moment a re-init relay blocks on the secure core - each
 * driver rebuilds itself lazily, on its first use after the warm boot, from the
 * thread that is about to use it.  Because the consuming thread performs (and
 * blocks on) its own re-init, there is no cross-thread race and no console
 * corruption.
 *
 * The SoC bumps an internal generation counter once per genuine warm boot.  The
 * driver stores the generation it last rebuilt at in its (retained) driver data
 * and passes a pointer to it here; the counters differ only on the first call
 * after a warm boot, which is when the rebuild runs.  On that call this
 * re-establishes the SRF client (once per generation) and then invokes the
 * device's PM_DEVICE_ACTION_TURN_ON handler.
 *
 * Must be called in thread context - the rebuild relays over blocking IPC.  When
 * called from an ISR it is a no-op (returns false) and the device is left stale
 * until a thread-context caller rebuilds it.  The caller must serialize
 * concurrent calls for the same device (drivers do this with their existing
 * per-device lock; the console is single-threaded during recovery).
 *
 * @param dev      Device to rebuild.
 * @param last_gen Pointer to the driver's stored warm-boot generation.
 * @return true if a rebuild was performed, false otherwise.
 */
bool ifx_pm_warm_boot_reinit(const struct device *dev, uint32_t *last_gen);

/**
 * @brief Eagerly rebuild every device after a DeepSleep-RAM warm boot.
 *
 * Walks every device in link (initialization) order and invokes its
 * PM_DEVICE_ACTION_TURN_ON handler, restoring all peripherals in a single
 * up-front pass rather than lazily on first use (see ifx_pm_warm_boot_reinit()).
 * Intended for an application that wants to force a full peripheral restore
 * after a warm boot instead of relying on the per-driver lazy path.
 *
 * Must be called from thread context on SRF builds: each TURN_ON handler relays
 * to the secure core over blocking IPC, so this re-establishes the SRF client
 * first and serializes the walk under the PM-policy lock.  A device with no
 * PM_DEVICE_ACTION_TURN_ON handler is skipped.
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

#endif /* ZEPHYR_SOC_INFINEON_EDGE_PSE84_POWER_H_ */
