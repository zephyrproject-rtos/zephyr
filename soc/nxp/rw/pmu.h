/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * PMU abstraction layer for RW612 TrustZone builds.
 *
 * In TZ builds the PMU peripheral is secure (PRIV_S).  NS drivers
 * cannot touch it directly, so all PMU operations are routed through
 * a TFM platform IOCTL.  In non-TZ builds the wrappers call the
 * NXP SDK POWER_* functions directly with zero overhead.
 */

#ifndef ZEPHYR_SOC_NXP_RW_PMU_H_
#define ZEPHYR_SOC_NXP_RW_PMU_H_

#include <fsl_power.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Sub-commands for TFM_PLATFORM_IOCTL_RW61X_PMU */
enum nxp_pmu_cmd {
	NXP_PMU_CMD_ENABLE_WAKEUP,
	NXP_PMU_CMD_DISABLE_WAKEUP,
	NXP_PMU_CMD_GET_WAKEUP_STATUS,
	NXP_PMU_CMD_CLEAR_WAKEUP_STATUS,
	NXP_PMU_CMD_GET_POWER_MODE_STATUS,
	NXP_PMU_CMD_INIT_POWER_CONFIG,
	NXP_PMU_CMD_CLEAR_RESET_CAUSE,
	NXP_PMU_CMD_GET_WAKEUP_PINS,
	NXP_PMU_CMD_CONFIG_WAKEUP_PIN,
	NXP_PMU_CMD_ENABLE_XTAL32K,
};

struct nxp_pmu_ioctl_in {
	uint32_t cmd;                  /* enum nxp_pmu_cmd */
	uint32_t arg;                  /* IRQn, bitmask, or packed pin|level<<16 */
	power_init_config_t power_cfg; /* used by NXP_PMU_CMD_INIT_POWER_CONFIG */
};

struct nxp_pmu_ioctl_out {
	uint32_t result;               /* status/register value for GET operations */
};

void nxp_pmu_enable_wakeup(IRQn_Type irqn);
void nxp_pmu_disable_wakeup(IRQn_Type irqn);
uint32_t nxp_pmu_get_wakeup_status(IRQn_Type irqn);
void nxp_pmu_clear_wakeup_status(IRQn_Type irqn);
uint32_t nxp_pmu_get_power_mode_status(void);
void nxp_pmu_init_power_config(const power_init_config_t *cfg);
void nxp_pmu_clear_reset_cause(uint32_t mask);
uint32_t nxp_pmu_get_wakeup_pins(void);
void nxp_pmu_config_wakeup_pin(uint32_t pin, uint8_t level);
void nxp_pmu_enable_xtal32k(bool enable);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SOC_NXP_RW_PMU_H_ */
