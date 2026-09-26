/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * ARM_PLL programming for the i.MX9 Cortex-A cores whose clock Zephyr programs
 * itself, for example, i.MX91 and i.MX93.
 *
 * The Cortex-A55 core clock is driven either by ARM_PLL or by the clock root,
 * arm_a55_clk_root, selected by CA55_CLOCK_SELECT in CCM GPR_SHARED1. ARM_PLL
 * cannot be reprogrammed while it clocks a core, so a rate change runs the
 * cores off the step clock programmed here, reprograms ARM_PLL and selects
 * ARM_PLL again.
 *
 * Only the core frequency is changed. VDD_SOC is left at the level configured
 * by the boot loader, so every frequency requested must be usable at that
 * voltage.
 */

#include <errno.h>

#include <zephyr/devicetree.h>
#include <zephyr/init.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/sys/util.h>
#include <fsl_common.h>
#include <soc.h>

#if defined(CONFIG_CPU_FREQ_PSTATE_SET_SOC) && DT_NODE_HAS_STATUS(DT_NODELABEL(arm_pll), okay)

#define ARM_PLL_NODE DT_NODELABEL(arm_pll)

/*
 * The supported HAL versions name the CCM GPR_SHARED1 field differently.
 */
#if defined(CCM_GPR_SHARED1_CA55_CLOCK_SELECT_MASK)
#define CA55_CLOCK_SELECT_MASK CCM_GPR_SHARED1_CA55_CLOCK_SELECT_MASK
#else
#define CA55_CLOCK_SELECT_MASK CCM_CTRL_GPR_SHARED1_CA55_CLOCK_SELECT_MASK
#endif

/* Step clock used while ARM_PLL is reprogrammed: SYS_PLL1_PFD0 (1 GHz) / 2. */
#define STEP_CLOCK_MUX kCLOCK_A55_ClockRoot_MuxSysPll1Pfd0
#define STEP_CLOCK_DIV 2U

/* ARM_PLL is fed by the 24 MHz oscillator. */
#define ARM_PLL_REF_FREQ (24000000ULL)

/* DIV[RDIV] of 0 and 1 both select divide by 1. */
#define ARM_PLL_RDIV(node) (DT_PROP(node, rdiv) < 2 ? 1 : DT_PROP(node, rdiv))

/* DIV[ODIV] of 0 and 1 both divide by 2. */
#define ARM_PLL_ODIV(node) (DT_PROP(node, odiv) < 2 ? 2 : DT_PROP(node, odiv))

/* Fout = Fref * MFI / (RDIV * ODIV) */
#define ARM_PLL_FOUT(node)                                                                         \
	(ARM_PLL_REF_FREQ * DT_PROP(node, mfi) / (ARM_PLL_RDIV(node) * ARM_PLL_ODIV(node)))

struct arm_pll_rate {
	uint32_t frequency;
	fracn_pll_init_t cfg;
};

/*
 * The ARM PLL runs in integer mode, so the operating points omit mfn and mfd.
 * MFN defaults to 0 and MFD to 1, since writing 0 to DENOMINATOR[MFD] is not
 * allowed. The dividers must produce the frequency the operating point
 * advertises.
 */
#define ARM_PLL_OPP_ASSERT(node)                                                                   \
	BUILD_ASSERT(DT_PROP_OR(node, mfd, 1) != 0, "ARM_PLL mfd must not be zero");               \
	BUILD_ASSERT(ARM_PLL_FOUT(node) == DT_PROP(node, clock_frequency),                         \
		     "ARM_PLL dividers do not produce clock-frequency");

DT_FOREACH_CHILD_STATUS_OKAY(ARM_PLL_NODE, ARM_PLL_OPP_ASSERT)

#define ARM_PLL_OPP(node)                                                                          \
	{                                                                                          \
		.frequency = DT_PROP(node, clock_frequency),                                       \
		.cfg =                                                                             \
			{                                                                          \
				.rdiv = DT_PROP(node, rdiv),                                       \
				.mfi = DT_PROP(node, mfi),                                         \
				.mfn = DT_PROP_OR(node, mfn, 0),                                   \
				.mfd = DT_PROP_OR(node, mfd, 1),                                   \
				.odiv = DT_PROP(node, odiv),                                       \
			},                                                                         \
	},

static const struct arm_pll_rate arm_pll_rates[] = {
	DT_FOREACH_CHILD_STATUS_OKAY(ARM_PLL_NODE, ARM_PLL_OPP)};

/* Serializes the transition against a request from another core. */
static struct k_spinlock arm_pll_lock;

/**
 * Run the Cortex-A55 cluster off the step clock.
 *
 * Programs the A55 clock root to a frequency that is safe at the VDD_SOC level
 * set by the boot loader and selects it, so that ARM_PLL can be reprogrammed
 * while it does not clock any core.
 */
static void imx9_a55_clk_use_step_clk(void)
{
	const clock_root_config_t step_cfg = {
		.clockOff = false,
		.mux = STEP_CLOCK_MUX,
		.div = STEP_CLOCK_DIV,
	};

	/*
	 * The root is programmed before it is selected so that the cores run at
	 * a frequency that is safe at the current VDD_SOC level.
	 */
	CLOCK_SetRootClock(kCLOCK_Root_A55, &step_cfg);
	CLOCK_PowerOnRootClock(kCLOCK_Root_A55);

	CCM_CTRL->GPR_SHARED1.CLR = CA55_CLOCK_SELECT_MASK;
	barrier_dsync_fence_full();
	barrier_isync_fence_full();
}

/**
 * Run the Cortex-A55 cluster off ARM_PLL again, once it has relocked.
 */
static void imx9_a55_clk_use_arm_pll(void)
{
	CCM_CTRL->GPR_SHARED1.SET = CA55_CLOCK_SELECT_MASK;
	barrier_dsync_fence_full();
	barrier_isync_fence_full();
}

/**
 * Look up the ARM_PLL dividers that produce a core frequency.
 */
static const fracn_pll_init_t *imx9_arm_pll_get_cfg(uint32_t frequency)
{
	for (size_t i = 0U; i < ARRAY_SIZE(arm_pll_rates); i++) {
		if (arm_pll_rates[i].frequency == frequency) {
			return &arm_pll_rates[i].cfg;
		}
	}

	return NULL;
}

/**
 * Compute the frequency ARM_PLL is programmed to from the dividers in DIV.
 *
 * ARM_PLL has only the CTRL, DIV and PLL_STATUS registers, so there is no
 * fractional part to account for: the multiplier is the integer DIV[MFI] and
 * Fout = Fref * MFI / (RDIV * ODIV).
 */
static uint32_t imx9_arm_pll_get_rate(void)
{
	uint32_t div_val = ARMPLL->DIV.RW;
	uint32_t rdiv = (div_val & PLL_DIV_RDIV_MASK) >> PLL_DIV_RDIV_SHIFT;
	uint32_t mfi = (div_val & PLL_DIV_MFI_MASK) >> PLL_DIV_MFI_SHIFT;
	uint32_t odiv = (div_val & PLL_DIV_ODIV_MASK) >> PLL_DIV_ODIV_SHIFT;

	/* DIV[RDIV] of 0 and 1 both select divide by 1. */
	if (rdiv < 2U) {
		rdiv = 1U;
	}

	/* DIV[ODIV] of 0 and 1 both divide by 2. */
	if (odiv < 2U) {
		odiv = 2U;
	}

	return (uint32_t)(ARM_PLL_REF_FREQ * mfi / (rdiv * odiv));
}

int imx9_arm_pll_set_rate(uint32_t frequency)
{
	const fracn_pll_init_t *pll_cfg;
	k_spinlock_key_t key;

	pll_cfg = imx9_arm_pll_get_cfg(frequency);
	if (pll_cfg == NULL) {
		return -ENOTSUP;
	}

	key = k_spin_lock(&arm_pll_lock);

	if (frequency == g_clockSourceFreq[kCLOCK_ArmPll]) {
		/* Another core already moved the cores to this rate. */
		k_spin_unlock(&arm_pll_lock, key);
		return 0;
	}

	imx9_a55_clk_use_step_clk();

	/*
	 * CLOCK_PllInit() bypasses and powers down the PLL, writes the dividers
	 * and then polls the lock status, so no core may be clocked by ARM_PLL
	 * while it runs. Every core follows the step clock, so this is safe on
	 * any CPU.
	 */
	CLOCK_PllInit(ARMPLL, pll_cfg);

	imx9_a55_clk_use_arm_pll();

	g_clockSourceFreq[kCLOCK_ArmPll] = frequency;
	g_clockSourceFreq[kCLOCK_ArmPllOut] = frequency;

	k_spin_unlock(&arm_pll_lock, key);

	return 0;
}

/**
 * Publish the frequency ARM_PLL runs at to the HAL clock source table.
 *
 * The HAL initializes g_clockSourceFreq[] from a static table that assumes the
 * reset dividers, so the entries do not describe the frequency the boot loader
 * programmed.
 */
static int imx9_pll_init(void)
{
	uint32_t frequency = imx9_arm_pll_get_rate();

	g_clockSourceFreq[kCLOCK_ArmPll] = frequency;
	g_clockSourceFreq[kCLOCK_ArmPllOut] = frequency;

	return 0;
}

/*
 * Runs before the CCM driver, which is PRE_KERNEL_1 at
 * CONFIG_CLOCK_CONTROL_INIT_PRIORITY, so a clock rate read through the driver
 * already sees the ARM_PLL frequency.
 */
SYS_INIT(imx9_pll_init, PRE_KERNEL_1, 0);
#endif /* CONFIG_CPU_FREQ_PSTATE_SET_SOC && arm_pll okay */
