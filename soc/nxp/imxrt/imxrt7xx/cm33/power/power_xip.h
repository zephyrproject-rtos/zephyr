/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SOC_NXP_IMXRT_IMXRT7XX_CM33_POWER_POWER_XIP_H_
#define SOC_NXP_IMXRT_IMXRT7XX_CM33_POWER_POWER_XIP_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Prepare the XIP flash and code/system caches for deep sleep.
 *
 * Flushes and deinitialises the XSPI AHB window used to fetch code,
 * then pushes and disables the selected XCACHE instances. The
 * previously enabled state is saved so power_xip_resume() can restore
 * the exact set of caches that was active before entry.
 *
 * All state and code paths touched after XSPI deinit live in the
 * QUICKACCESS SRAM section, so callers can invoke this function even
 * when the flash is about to become unreachable.
 *
 * @param flush_xcache0  Push and disable XCACHE0 (system cache) when true.
 * @param flush_xcache1  Push and disable XCACHE1 (code cache) when true.
 */
void power_xip_suspend(bool flush_xcache0, bool flush_xcache1);

/**
 * @brief Restore the XIP flash and caches after deep sleep exit.
 *
 * Reinitialises the XSPI AHB window and re-enables the XCACHE
 * instances that power_xip_suspend() had disabled (and only those).
 */
void power_xip_resume(void);

#ifdef __cplusplus
}
#endif

#endif /* SOC_NXP_IMXRT_IMXRT7XX_CM33_POWER_POWER_XIP_H_ */
