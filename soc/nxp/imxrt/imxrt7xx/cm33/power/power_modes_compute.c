/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include "power_domain.h"
#include "power_regmap.h"
#include "power_resources.h"
#include <sram_banks.h>
#include "power_modes.h"

#include <fsl_common.h>

#if IS_ENABLED(CONFIG_SOC_MIMXRT7XX_PM_XIP_HANDOVER)
#include "power_xip.h"
#endif

/*
 * Compute domain resource map. Only includes resources that the Compute domain
 * can actually vote on.
 */
static const struct power_res_map compute_res_map[PWR_RES_COUNT] = {
	PWR_RES_MAP_PMC_SHARED
	PWR_RES_MAP_MEM_SHARED
	PWR_RES_MAP_PMC_COMPUTE_ONLY
	PWR_RES_MAP_SLEEPCON_SHARED
	PWR_RES_MAP_SLEEPCON0_ONLY
};

#if defined(CONFIG_PM)
#if IS_ENABLED(CONFIG_SOC_MIMXRT7XX_PM_XIP_HANDOVER)
#define COMPUTE_KEEP_XIP_RES	PWR_RES(PWR_RES_VDDN_COM)
#else
#define COMPUTE_KEEP_XIP_RES	0ULL
#endif

/* Kept by both windows: CPU0's two caches and the OCOTP shadow. */
#define COMPUTE_KEEP_MEM_RES	(PWR_RES(PWR_RES_MEM_CPU0_CCACHE) |			\
				 PWR_RES(PWR_RES_MEM_CPU0_SCACHE) |			\
				 PWR_RES(PWR_RES_MEM_OCOTP))

/*
 * Why this keep set, resource by resource.
 *
 * The compute main SRAM partitions (POWER_SRAM_KEEPALIVE) hold this image's code,
 * stack and data, so keeping them is what lets either window resume in place. The
 * macro covers what *this* image links and nothing more: CPU1's partitions are held
 * up by CPU1's own image.
 *
 * CPU0's code and system caches are kept so they need not be refilled on resume.
 * The OCOTP shadow RAM is kept because the OTP driver reads the shadow registers
 * directly and has no resume hook to reload them; once it grows one this entry can
 * go away.
 *
 * Both PLLs are kept because the low-power entry/exit path has no PLL re-lock
 * step, so a powered-down PLL would come back unlocked and leave the compute
 * domain without its main clock. Keeping them also pulls PMC_REF out of low-power
 * mode through res_deps[], so PMC_REF need not be listed. What feeds the PLLs is
 * added at entry by compute_res_clock_tree(), since the board chooses that.
 *
 * Deep sleep keeps VDD2_COMP, the rail holding CPU0's own compute logic; keeping
 * it is what separates deep sleep from DSR, which collapses it. VDD2_COM is not
 * listed next to it because VDD2_COMP cannot stay up while VDD2_COM is off (RM
 * Table 329) and res_deps[] carries that edge.
 *
 * COMPUTE_KEEP_XIP_RES keeps VDDN_COM for an XIP image, which fetches from XSPI
 * flash in that domain. Letting it reach retention across a window that returns
 * brings CPU0 back to a flash controller and an external device that have both
 * lost their configuration, and the fetch that discovers it -- along with the
 * fault handler it would vector to -- is itself in flash. power_xip_resume() puts
 * the XSPI instances back the way it found them, but only the domain staying
 * powered makes that restore mean anything. comn_main_clk, which clocks the same
 * block, is not kept with it: it comes back from the run bank early enough that
 * the first fetch is already clocked.
 */
static const struct power_mode_desc compute_deep_sleep = {
	.low_power_mode = LP_DEEP_SLEEP,
	.keep = {
		.res = PWR_RES(PWR_RES_MAIN_PLL) | PWR_RES(PWR_RES_AUDIO_PLL) |
		       PWR_RES(PWR_RES_VDD2_COMP) | COMPUTE_KEEP_MEM_RES |
		       COMPUTE_KEEP_XIP_RES,
		.sram = POWER_SRAM_KEEPALIVE,
	},
	.pmic_mode = 1U,
};

static const struct power_mode_desc compute_dsr = {
	.low_power_mode = LP_DSR,
	.keep = {
		.res = PWR_RES(PWR_RES_MAIN_PLL) | PWR_RES(PWR_RES_AUDIO_PLL) |
		       COMPUTE_KEEP_MEM_RES | COMPUTE_KEEP_XIP_RES,
		.sram = POWER_SRAM_KEEPALIVE,
	},
	.pmic_mode = 1U,
};
#endif /* CONFIG_PM */

#if defined(CONFIG_POWEROFF)
static const struct power_mode_desc compute_dpd = {
	.low_power_mode = LP_DPD,
	.keep = {0},
	.pmic_mode = 2U,
};

static const struct power_mode_desc compute_fdpd = {
	.low_power_mode = LP_FDPD,
	.keep = {0},
	.pmic_mode = 3U,
};
#endif /* CONFIG_POWEROFF */

#if defined(CONFIG_PM) || defined(CONFIG_POWEROFF)
AT_QUICKACCESS_SECTION_CODE(static void
compute_arm_shared_clock_pdr_ignores(const struct power_request *req))
{
	uint32_t ignores = 0U;

	if ((SOC_PMC->STATUS & PMC_STATUS_DSSENS_MASK) == 0U) {
		ignores = SLEEPCON0_PWRDOWN_WAIT_IGN_FRO2PDR_MASK |
			  SLEEPCON0_PWRDOWN_WAIT_IGN_LPOSCPDR_MASK;
	} else {
		if (power_requested(req, PWR_RES_FRO2)) {
			ignores |= SLEEPCON0_PWRDOWN_WAIT_IGN_FRO2PDR_MASK;
		}
		if (power_requested(req, PWR_RES_LPOSC)) {
			ignores |= SLEEPCON0_PWRDOWN_WAIT_IGN_LPOSCPDR_MASK;
		}
	}

	SOC_SLEEPCON->PWRDOWN_WAIT |= ignores;
}

static void compute_prep_owned_clock_pdr(const struct power_request *req)
{
	if (!power_requested(req, PWR_RES_FRO0)) {
		SOC_SLEEPCON->PWRDOWN_WAIT &= ~SLEEPCON0_PWRDOWN_WAIT_IGN_FRO0PDR_MASK;
	}
	if (!power_requested(req, PWR_RES_FRO1)) {
		SOC_SLEEPCON->PWRDOWN_WAIT &= ~SLEEPCON0_PWRDOWN_WAIT_IGN_FRO1PDR_MASK;
	}
}

/*
 * What the kept PLLs need on their inputs. CLKCTL2 selects each PLL's reference
 * between FRO1_DIV8 (0) and the system oscillator (1); the board picks, so read the
 * select instead of assuming one. A PLL kept powered with its reference gone comes
 * back with no input and leaves CPU0 without a main clock.
 *
 * XTAL is shared, so this is the Compute side claiming what its own PLL consumes and
 * CPU1 still votes the oscillator down (RM 12.5.2 Table 167) -- which is exactly why
 * the gap only shows up once the Sense side stops holding the oscillator up for its
 * own reasons. FRO1 has a bit in SLEEPCON0 alone, so that branch defeats no peer vote.
 */
static power_res_mask_t compute_res_clock_tree(const struct power_request *req)
{
	power_res_mask_t ref = 0U;

	if (power_requested(req, PWR_RES_MAIN_PLL)) {
		ref |= ((CLKCTL2->MAINPLL0CLKSEL & CLKCTL2_MAINPLL0CLKSEL_SEL_MASK) != 0U)
			       ? PWR_RES(PWR_RES_XTAL)
			       : PWR_RES(PWR_RES_FRO1);
	}

	if (power_requested(req, PWR_RES_AUDIO_PLL)) {
		ref |= ((CLKCTL2->AUDIOPLL0CLKSEL & CLKCTL2_AUDIOPLL0CLKSEL_SEL_MASK) != 0U)
			       ? PWR_RES(PWR_RES_XTAL)
			       : PWR_RES(PWR_RES_FRO1);
	}

	return ref;
}

static void compute_stall_dsp_if_powered_down(void)
{
	if ((SOC_PMC->PDSLEEPCFG0 & PMC_PDSLEEPCFG0_V2DSP_PD_MASK) != 0U) {
		SYSCON0->DSPSTALL = SYSCON0_DSPSTALL_DSPSTALL_MASK;
	}
}

static const struct power_domain compute_domain = {
	.res_map = compute_res_map,
	.res_sleepcon_rail = PWR_RES_VDD2_COM,
	.res_vote_exclusive = PWR_RES_COMPUTE_ONLY,
	.peer_needs = power_cross_domain_requested,
	.res_clock_tree = compute_res_clock_tree,
	.ldo_vsel_clear = PMC_PDSLEEPCFG0_LDO1_VSEL_MASK,
	.stall_dsp_if_powered_down = compute_stall_dsp_if_powered_down,
	.prep_owned_clock_pdr = compute_prep_owned_clock_pdr,
	.arm_shared_clock_pdr_ignores = compute_arm_shared_clock_pdr_ignores,
	.regulator = &power_ldo_regulator_ops,
#if IS_ENABLED(CONFIG_SOC_MIMXRT7XX_PM_XIP_HANDOVER)
	.xip_suspend = power_xip_suspend,
	.xip_resume = power_xip_resume,
#endif
};

#if defined(CONFIG_PM)
AT_QUICKACCESS_SECTION_CODE(void power_enter_deep_sleep(void))
{
	power_enter_common(&compute_domain, &compute_deep_sleep);
}

AT_QUICKACCESS_SECTION_CODE(void power_enter_dsr(void))
{
	power_enter_common(&compute_domain, &compute_dsr);
}
#endif /* CONFIG_PM */

#if defined(CONFIG_POWEROFF)
AT_QUICKACCESS_SECTION_CODE(void power_enter_deep_power_down(bool full))
{
	power_enter_common(&compute_domain, full ? &compute_fdpd : &compute_dpd);
}
#endif /* CONFIG_POWEROFF */
#endif /* CONFIG_PM || CONFIG_POWEROFF */
