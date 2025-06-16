/*
 * Copyright (c) 2025 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _NUVOTON_NCT_SOC_DBG_H_
#define _NUVOTON_NCT_SOC_DBG_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Configure the Automatic Freeze mode. If this mode is enabled, whenever
 * the Core is halted, various modules’ clocks, counters are stopped and
 * destructive reads are disabled, pending the respective module enable bit for
 * debugging.
 */
void nct_dbg_freeze_enable(bool enable);

#ifdef __cplusplus
}
#endif

#endif /* _NUVOTON_NCT_SOC_DBG_H_ */
