/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SOC_NXP_IMXRT_IMXRT7XX_CM33_POWER_POWER_MODES_H_
#define SOC_NXP_IMXRT_IMXRT7XX_CM33_POWER_POWER_MODES_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Enter deep sleep mode.
 *
 * @param exclude_from_pd  Bitmask array of SLEEPCON_SLEEPCFG and
 *        PMC_PDSLEEPCFG0..5 bits to keep modules powered during deep sleep mode.
 */
void power_enter_deep_sleep(const uint32_t exclude_from_pd[7]);

#if defined(CONFIG_SOC_IMXRT7XX_POWER_DOMAIN_COMPUTE)
/**
 * @brief Enter deep sleep retention (dsr) mode.
 *
 * Similar to deep sleep but with additional power-down and state retention.
 * Only available in the compute power domain.
 *
 * @param exclude_from_pd  Bitmask array of SLEEPCON_SLEEPCFG and
 *        PMC_PDSLEEPCFG0..5 bits to keep modules powered during dsr mode.
 */
void power_enter_dsr(const uint32_t exclude_from_pd[7]);
#endif

#if defined(CONFIG_POWEROFF)
/**
 * @brief Enter Deep Power Down (DPD) or Full Deep Power Down (FDPD).
 *
 * Backs sys_poweroff(): both modes power the calling core's domain (and its
 * SRAM) off and cold boot on wake, so this never returns. Implemented for
 * both the compute (PMC0/SLEEPCON0) and sense (PMC1/SLEEPCON1) domains;
 * the chip only actually powers off once the aggregation resolves,
 * i.e. the other domain is also reset / powered off / selecting DPD-FDPD.
 *
 * Nothing is retained across DPD/FDPD, so there is no keep-alive/exclude mask:
 * the [DPD]/[FDPD] override bit powers every domain down regardless.
 *
 * @param full  true for FDPD (also turns off VDD1V8_PMC), false for DPD.
 */
void power_enter_deep_power_down(bool full);
#endif /* CONFIG_POWEROFF */

#ifdef __cplusplus
}
#endif

#endif /* SOC_NXP_IMXRT_IMXRT7XX_CM33_POWER_POWER_MODES_H_ */
