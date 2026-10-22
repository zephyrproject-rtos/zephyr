/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SOC_NXP_IMXRT_IMXRT7XX_CM33_POWER_POWER_MODES_H_
#define SOC_NXP_IMXRT_IMXRT7XX_CM33_POWER_POWER_MODES_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Enter deep sleep mode.
 */
void power_enter_deep_sleep(void);

#if defined(CONFIG_SOC_IMXRT7XX_POWER_DOMAIN_COMPUTE)
/**
 * @brief Enter deep sleep retention (DSR) mode.
 */
void power_enter_dsr(void);
#endif

#if defined(CONFIG_POWEROFF)
/**
 * @brief Enter Deep Power Down (DPD) or Full Deep Power Down (FDPD).
 *
 * Backs sys_poweroff(): both modes power the calling core's domain (and
 * its SRAM) off and cold boot on wake, so this never returns. Implemented
 * for both the compute and sense domains; the chip only actually powers off
 * once the aggregation resolves, i.e. the other domain is also powered off.
 *
 * Nothing is retained across DPD/FDPD, so no keep-alive mask applies.
 *
 * @param full  true for FDPD (also turns off VDD1V8_PMC), false for DPD.
 */
void power_enter_deep_power_down(bool full);
#endif /* CONFIG_POWEROFF */

#ifdef __cplusplus
}
#endif

#endif /* SOC_NXP_IMXRT_IMXRT7XX_CM33_POWER_POWER_MODES_H_ */
