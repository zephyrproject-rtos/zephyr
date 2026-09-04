/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include <fsl_common.h>

#include <sram_banks.h>

#include "power_domain.h"
#include "power_regmap.h"
#include "power_resources.h"
#include "power_modes.h"

/*
 * Sense domain resource map. Only includes resources that the Sense domain
 * can actually vote on. Sense domain does not have exclusive resources.
 */
static const struct power_res_map sense_res_map[PWR_RES_COUNT] = {
	PWR_RES_MAP_PMC_SHARED
	PWR_RES_MAP_MEM_SHARED
	PWR_RES_MAP_SLEEPCON_SHARED
};

#if defined(CONFIG_PM)
/*
 * CPU1 executes from RAM, so its main SRAM partitions must be retained or the image
 * is gone on wake. POWER_SRAM_KEEPALIVE covers that: it unions this image's text
 * and RAM regions, and on this core both are on-chip SRAM.
 *
 * Nothing is named in .res. CPU1 uses none of the peripheral memories -- the caches
 * and TCMs belong to CPU0, the DSP and the NPU -- so it keeps none of them, which is
 * exactly the vote that hands control of them to whoever does use them.
 */
static const struct power_mode_desc sense_deep_sleep = {
	.low_power_mode = LP_DEEP_SLEEP,
	.keep = {
		.sram = POWER_SRAM_KEEPALIVE,
	},
	.pmic_mode = 1U,
};
#endif /* CONFIG_PM */

#if defined(CONFIG_POWEROFF)
static const struct power_mode_desc sense_dpd = {
	.low_power_mode = LP_DPD,
	.keep = {0},
	.pmic_mode = 2U,
};

static const struct power_mode_desc sense_fdpd = {
	.low_power_mode = LP_FDPD,
	.keep = {0},
	.pmic_mode = 3U,
};
#endif /* CONFIG_POWEROFF */

#if defined(CONFIG_PM) || defined(CONFIG_POWEROFF)
static void sense_arm_shared_clock_pdr_ignores(const struct power_request *req)
{
	ARG_UNUSED(req);

	SOC_SLEEPCON->PWRDOWN_WAIT |= SLEEPCON1_PWRDOWN_WAIT_IGN_FRO2PDR_MASK |
				      SLEEPCON1_PWRDOWN_WAIT_IGN_LPOSCPDR_MASK;
}

static const struct power_domain sense_domain = {
	.res_map = sense_res_map,
	.res_sleepcon_rail = PWR_RES_NONE,
	.ldo_vsel_clear = PMC_PDSLEEPCFG0_LDO2_VSEL_MASK,
	.arm_shared_clock_pdr_ignores = sense_arm_shared_clock_pdr_ignores,
	.regulator = &power_ldo_regulator_ops,
	.xip_suspend = NULL,
	.xip_resume = NULL,
};

#if defined(CONFIG_PM)
void power_enter_deep_sleep(void)
{
	power_enter_common(&sense_domain, &sense_deep_sleep);
}
#endif /* CONFIG_PM */

#if defined(CONFIG_POWEROFF)
void power_enter_deep_power_down(bool full)
{
	power_enter_common(&sense_domain, full ? &sense_fdpd : &sense_dpd);
}
#endif /* CONFIG_POWEROFF */
#endif /* CONFIG_PM || CONFIG_POWEROFF */
