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

#ifdef __cplusplus
}
#endif

#endif /* SOC_NXP_IMXRT_IMXRT7XX_CM33_POWER_POWER_MODES_H_ */
