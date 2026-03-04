/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * PMU abstraction layer — routes PMU operations through TFM IOCTL
 * in TrustZone builds, or calls POWER_* directly otherwise.
 */

#include "pmu.h"

#ifdef CONFIG_TRUSTED_EXECUTION_NONSECURE

/*
 * Minimal TFM type stubs - same approach as power.c.
 * The actual tfm_platform_ioctl() symbol comes from the TFM interface library.
 */
typedef int32_t tfm_platform_ioctl_req_t;

typedef struct psa_invec {
	const void *base;
	size_t len;
} psa_invec;

typedef struct psa_outvec {
	void *base;
	size_t len;
} psa_outvec;

enum tfm_platform_err_t {
	TFM_PLATFORM_ERR_PMU_SUCCESS = 0,
};

/* Weak fallback for builds where libtfm_api.a is not linked
 * (e.g. when CONFIG_TFM_USE_NS_APP=y) The strong symbol
 * from libtfm_api.a overrides this in normal builds
 */
__attribute__((weak))
enum tfm_platform_err_t tfm_platform_ioctl(tfm_platform_ioctl_req_t request,
					    psa_invec *input,
					    psa_outvec *output)
{
	(void)request;
	(void)input;
	(void)output;
	return TFM_PLATFORM_ERR_PMU_SUCCESS;

}

#define TFM_PLATFORM_IOCTL_RW61X_PMU 3

static uint32_t pmu_ioctl(uint32_t cmd, uint32_t arg,
			  const power_init_config_t *pcfg)
{
	struct nxp_pmu_ioctl_in pmu_in = {
		.cmd = cmd,
		.arg = arg,
	};
	struct nxp_pmu_ioctl_out pmu_out = {0};

	if (pcfg) {
		pmu_in.power_cfg = *pcfg;
	}

	psa_invec in_vec = { .base = &pmu_in, .len = sizeof(pmu_in) };
	psa_outvec out_vec = { .base = &pmu_out, .len = sizeof(pmu_out) };

	tfm_platform_ioctl((tfm_platform_ioctl_req_t)TFM_PLATFORM_IOCTL_RW61X_PMU,
			   &in_vec, &out_vec);

	return pmu_out.result;
}

void nxp_pmu_enable_wakeup(IRQn_Type irqn)
{
	pmu_ioctl(NXP_PMU_CMD_ENABLE_WAKEUP, (uint32_t)irqn, NULL);
}

void nxp_pmu_disable_wakeup(IRQn_Type irqn)
{
	pmu_ioctl(NXP_PMU_CMD_DISABLE_WAKEUP, (uint32_t)irqn, NULL);
}

uint32_t nxp_pmu_get_wakeup_status(IRQn_Type irqn)
{
	return pmu_ioctl(NXP_PMU_CMD_GET_WAKEUP_STATUS, (uint32_t)irqn, NULL);
}

void nxp_pmu_clear_wakeup_status(IRQn_Type irqn)
{
	pmu_ioctl(NXP_PMU_CMD_CLEAR_WAKEUP_STATUS, (uint32_t)irqn, NULL);
}

uint32_t nxp_pmu_get_power_mode_status(void)
{
	return pmu_ioctl(NXP_PMU_CMD_GET_POWER_MODE_STATUS, 0, NULL);
}

void nxp_pmu_init_power_config(const power_init_config_t *cfg)
{
	pmu_ioctl(NXP_PMU_CMD_INIT_POWER_CONFIG, 0, cfg);
}

void nxp_pmu_clear_reset_cause(uint32_t mask)
{
	pmu_ioctl(NXP_PMU_CMD_CLEAR_RESET_CAUSE, mask, NULL);
}

uint32_t nxp_pmu_get_wakeup_pins(void)
{
	return pmu_ioctl(NXP_PMU_CMD_GET_WAKEUP_PINS, 0, NULL);
}

void nxp_pmu_config_wakeup_pin(uint32_t pin, uint8_t level)
{
	/* Pack pin and level into arg: pin in low 16, level in high 16 */
	pmu_ioctl(NXP_PMU_CMD_CONFIG_WAKEUP_PIN,
		  pin | ((uint32_t)level << 16), NULL);
}

void nxp_pmu_enable_xtal32k(bool enable)
{
	pmu_ioctl(NXP_PMU_CMD_ENABLE_XTAL32K, (uint32_t)enable, NULL);
}

#else /* Non-TZ build: direct PMU access */

void nxp_pmu_enable_wakeup(IRQn_Type irqn)
{
	POWER_EnableWakeup(irqn);
}

void nxp_pmu_disable_wakeup(IRQn_Type irqn)
{
	POWER_DisableWakeup(irqn);
}

uint32_t nxp_pmu_get_wakeup_status(IRQn_Type irqn)
{
	return POWER_GetWakeupStatus(irqn);
}

void nxp_pmu_clear_wakeup_status(IRQn_Type irqn)
{
	POWER_ClearWakeupStatus(irqn);
}

uint32_t nxp_pmu_get_power_mode_status(void)
{
	return PMU->PWR_MODE_STATUS;
}

void nxp_pmu_init_power_config(const power_init_config_t *cfg)
{
	POWER_InitPowerConfig((power_init_config_t *)cfg);
}

void nxp_pmu_clear_reset_cause(uint32_t mask)
{
	POWER_ClearResetCause(mask);
}

uint32_t nxp_pmu_get_wakeup_pins(void)
{
	return PMU->WAKEUP_STATUS &
	       (PMU_WAKEUP_STATUS_PIN0_MASK | PMU_WAKEUP_STATUS_PIN1_MASK);
}

void nxp_pmu_config_wakeup_pin(uint32_t pin, uint8_t level)
{
	POWER_ConfigWakeupPin((power_wakeup_pin_t)pin, level);
}

void nxp_pmu_enable_xtal32k(bool enable)
{
	CLOCK_EnableXtal32K(enable);
	if (enable) {
		CLOCK_AttachClk(kXTAL32K_to_CLK32K);
	}
}

#endif /* CONFIG_TRUSTED_EXECUTION_NONSECURE */
