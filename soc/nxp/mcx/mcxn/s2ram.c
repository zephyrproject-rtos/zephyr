/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/init.h>
#include <zephyr/devicetree.h>
#include <zephyr/platform/hooks.h>
#include <zephyr/arch/common/pm_s2ram.h>
#include <soc.h>
#include <cmsis_core.h>
#include <zephyr/arch/arm/cortex_m/scb.h>
#include <fsl_cmc.h>
#include <fsl_spc.h>
#include <fsl_vbat.h>

#define MCXN_CMC_ADDR		(CMC_Type *)DT_REG_ADDR(DT_INST(0, nxp_cmc))
#define MCXN_SPC_ADDR		(SPC_Type *)DT_REG_ADDR(DT_INST(0, nxp_spc))
#define MCXN_VBAT_ADDR		(VBAT_Type *)DT_REG_ADDR(DT_INST(0, nxp_vbat))
#define MCXN_WAKEUP_DELAY	DT_PROP_OR(DT_INST(0, nxp_spc), wakeup_delay, 0)

/* All four 8 KB RAMA arrays. */
#define MCXN_RAMA_ALL	(kVBAT_SramArray0 | kVBAT_SramArray1 | \
			 kVBAT_SramArray2 | kVBAT_SramArray3)

/*
 * Deep Power Down (mapped to PM_STATE_SUSPEND_TO_RAM) power gates the whole
 * CORE domain, including the ARM System Control Space (NVIC, SCB). The chip
 * wakes through the reset routine; arch_pm_s2ram_resume() then returns directly
 * to the suspend call site without re-running kernel/CPU initialization, so the
 * SCS state is saved here before entry and restored on resume.
 */
static struct {
	uint32_t iser[ARRAY_SIZE(((NVIC_Type *)NVIC_BASE)->ISER)];
	uint32_t ipr[(sizeof(((NVIC_Type *)NVIC_BASE)->IPR)) / sizeof(uint32_t)];
	struct scb_context scb;
} mcxn_scs_context;

static void mcxn_scs_save(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(mcxn_scs_context.iser); i++) {
		mcxn_scs_context.iser[i] = NVIC->ISER[i];
	}

	for (size_t i = 0; i < ARRAY_SIZE(mcxn_scs_context.ipr); i++) {
		mcxn_scs_context.ipr[i] = ((volatile uint32_t *)NVIC->IPR)[i];
	}

	z_arm_save_scb_context(&mcxn_scs_context.scb);
}

static void mcxn_scs_restore(void)
{
	z_arm_restore_scb_context(&mcxn_scs_context.scb);

	for (size_t i = 0; i < ARRAY_SIZE(mcxn_scs_context.ipr); i++) {
		((volatile uint32_t *)NVIC->IPR)[i] = mcxn_scs_context.ipr[i];
	}

	for (size_t i = 0; i < ARRAY_SIZE(mcxn_scs_context.iser); i++) {
		NVIC->ISER[i] = mcxn_scs_context.iser[i];
	}
}

static int mcxn_s2ram_retain_sram(void)
{
	if (!VBAT_CheckFRO16kEnabled(MCXN_VBAT_ADDR)) {
		VBAT_EnableFRO16k(MCXN_VBAT_ADDR, true);
	}

	VBAT_UngateFRO16k(MCXN_VBAT_ADDR, kVBAT_EnableClockToVddSys);

	if (!VBAT_CheckBandgapEnabled(MCXN_VBAT_ADDR)) {
		if (VBAT_EnableBandgap(MCXN_VBAT_ADDR, true) != kStatus_Success) {
			return -EIO;
		}
	}

	/* Enable refresh mode to save power */
	VBAT_EnableBandgapRefreshMode(MCXN_VBAT_ADDR, true);

	/* The backup SRAM regulator supplies the retained RAMA arrays across
	 * Deep Power Down; if it cannot be enabled the context would be lost.
	 */
	if (VBAT_EnableBackupSRAMRegulator(MCXN_VBAT_ADDR, true) != kStatus_Success) {
		return -EIO;
	}

	VBAT_RetainSRAMsInLowPowerModes(MCXN_VBAT_ADDR, MCXN_RAMA_ALL);

	return 0;
}

static int mcxn_enter_deep_power_down(void)
{
	const cmc_power_domain_config_t cmc_config = {
		.clock_mode = kCMC_GateAllSystemClocksEnterLowPowerMode,
		.main_domain = kCMC_DeepPowerDown,
		.wake_domain = kCMC_DeepPowerDown,
	};

	CMC_EnterLowPowerMode(MCXN_CMC_ADDR, &cmc_config);

	return -EBUSY;
}

void mcxn_pm_suspend_to_ram(void)
{
	SPC_SetLowPowerWakeUpDelay(MCXN_SPC_ADDR, MCXN_WAKEUP_DELAY);

	if (mcxn_s2ram_retain_sram() != 0) {
		/* SRAM retention could not be armed; entering Deep Power Down now
		 * would lose the retained context, so stay running.
		 */
		return;
	}

	mcxn_scs_save();

	/* A non-zero return means the SoC never entered Deep Power Down and is
	 * still fully operational, so skip the resume-path rebuild below.
	 */
	if (arch_pm_s2ram_suspend(mcxn_enter_deep_power_down) != 0) {
		return;
	}

	/* re-enable aGDET/dGDET; re-disable RAM ECC */
	SystemInit();

	/* re-build clock tree */
	board_early_init_hook();

	/* idle-thread stack is nearly full, it is easy to reach the
	 * PSPLIM limit on resume. Drop the limit here, the next context
	 * switch re-establishes the per-thread PSPLIM.
	 */
	__set_PSPLIM(0U);

	mcxn_scs_restore();
}
