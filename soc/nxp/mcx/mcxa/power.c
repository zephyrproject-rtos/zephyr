/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/pm/pm.h>
#include <zephyr/arch/arch_interface.h>
#include <fsl_cmc.h>
#include <fsl_spc.h>
#include <zephyr/platform/hooks.h>
#include <soc.h>

#if defined(CONFIG_PM_S2RAM)
#include <errno.h>
#include <cmsis_core.h>
#include <zephyr/arch/arm/cortex_m/scb.h>
#include <zephyr/arch/common/pm_s2ram.h>
#if defined(CONFIG_SOC_SERIES_MCXAXX7)
#include <fsl_vbat.h>
#endif
#endif

#define MCXA_WAKEUP_DELAY	DT_PROP_OR(DT_INST(0, nxp_spc), wakeup_delay, 0)
#define MCXA_CMC_ADDR		(CMC_Type *)DT_REG_ADDR(DT_INST(0, nxp_cmc))
#define MCXA_SPC_ADDR		(SPC_Type *)DT_REG_ADDR(DT_INST(0, nxp_spc))

static void pm_enter_hook(void)
{
	CMC_SetPowerModeProtection(MCXA_CMC_ADDR, kCMC_AllowAllLowPowerModes);
	CMC_EnableDebugOperation(MCXA_CMC_ADDR, false);
	CMC_ConfigFlashMode(MCXA_CMC_ADDR, true, true, false);
}

static void enter_low_power(void)
{
	unsigned int key;

	key = arch_pm_state_set_prepare();
	__DSB();
	__ISB();
	__WFI();
	arch_pm_state_set_finish(key);
}

#if defined(CONFIG_PM_S2RAM)
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
} mcxa_scs_context;

static void mcxa_scs_save(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(mcxa_scs_context.iser); i++) {
		mcxa_scs_context.iser[i] = NVIC->ISER[i];
	}

	for (size_t i = 0; i < ARRAY_SIZE(mcxa_scs_context.ipr); i++) {
		mcxa_scs_context.ipr[i] = ((volatile uint32_t *)NVIC->IPR)[i];
	}

	z_arm_save_scb_context(&mcxa_scs_context.scb);
}

static void mcxa_scs_restore(void)
{
	z_arm_restore_scb_context(&mcxa_scs_context.scb);

	for (size_t i = 0; i < ARRAY_SIZE(mcxa_scs_context.ipr); i++) {
		((volatile uint32_t *)NVIC->IPR)[i] = mcxa_scs_context.ipr[i];
	}

	for (size_t i = 0; i < ARRAY_SIZE(mcxa_scs_context.iser); i++) {
		NVIC->ISER[i] = mcxa_scs_context.iser[i];
	}
}

#if !defined(CONFIG_SOC_SERIES_MCXAXX7)
/* 0xF keeps every RAM array powered by the SPC SRAM retention LDO. */
#define MCXA_SRAM_RETAIN_ALL	0xFU

static int mcxa_s2ram_retain_sram(void)
{
	SPC_EnableSRAMLdo(MCXA_SPC_ADDR, true);
	SPC_RetainSRAMArray(MCXA_SPC_ADDR, MCXA_SRAM_RETAIN_ALL);

	return 0;
}
#else
/* Parts without an SPC SRAM retention LDO (e.g. MCXAxx7) keep the RAMA arrays
 * alive through the VBAT backup SRAM regulator instead, so the retained working
 * set must fit in the VBAT-retained RAMA region.
 */
#define MCXA_VBAT_ADDR	(VBAT_Type *)DT_REG_ADDR(DT_INST(0, nxp_vbat))
#define MCXA_RAMA_ALL	(kVBAT_SramArray0 | kVBAT_SramArray1 | \
			 kVBAT_SramArray2 | kVBAT_SramArray3)

static int mcxa_s2ram_retain_sram(void)
{
	if (!VBAT_CheckFRO16kEnabled(MCXA_VBAT_ADDR)) {
		VBAT_EnableFRO16k(MCXA_VBAT_ADDR, true);
	}

	VBAT_UngateFRO16k(MCXA_VBAT_ADDR, kVBAT_EnableClockToVddSys);

	if (!VBAT_CheckBandgapEnabled(MCXA_VBAT_ADDR)) {
		if (VBAT_EnableBandgap(MCXA_VBAT_ADDR, true) != kStatus_Success) {
			return -EIO;
		}
	}

	/* Enable refresh mode to save power. */
	VBAT_EnableBandgapRefreshMode(MCXA_VBAT_ADDR, true);

	/* The backup SRAM regulator supplies the retained RAMA arrays across
	 * Deep Power Down; if it cannot be enabled the context would be lost.
	 */
	if (VBAT_EnableBackupSRAMRegulator(MCXA_VBAT_ADDR, true) != kStatus_Success) {
		return -EIO;
	}

	VBAT_RetainSRAMsInLowPowerModes(MCXA_VBAT_ADDR, MCXA_RAMA_ALL);

	return 0;
}
#endif

static int mcxa_enter_deep_power_down(void)
{
	const cmc_power_domain_config_t cmc_config = {
		.clock_mode = kCMC_GateAllSystemClocksEnterLowPowerMode,
		.main_domain = kCMC_DeepPowerDown,
	};

	CMC_EnterLowPowerMode(MCXA_CMC_ADDR, &cmc_config);

	return -EBUSY;
}
#endif /* CONFIG_PM_S2RAM */

__weak void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	pm_enter_hook();

	SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;

	switch (state) {
	case PM_STATE_RUNTIME_IDLE:
		CMC_SetClockMode(MCXA_CMC_ADDR, kCMC_GateCoreClock);
		CMC_SetMAINPowerMode(MCXA_CMC_ADDR, kCMC_ActiveOrSleepMode);
		enter_low_power();
		break;

	case PM_STATE_SUSPEND_TO_IDLE:
		CMC_SetClockMode(MCXA_CMC_ADDR, kCMC_GateAllSystemClocksEnterLowPowerMode);
		CMC_SetMAINPowerMode(MCXA_CMC_ADDR, kCMC_DeepSleepMode);
		enter_low_power();
		break;

	case PM_STATE_STANDBY:
		SPC_SetLowPowerWakeUpDelay(MCXA_SPC_ADDR, MCXA_WAKEUP_DELAY);
		CMC_SetClockMode(MCXA_CMC_ADDR, kCMC_GateAllSystemClocksEnterLowPowerMode);
		CMC_SetMAINPowerMode(MCXA_CMC_ADDR, kCMC_PowerDownMode);
		enter_low_power();
		break;

#if defined(CONFIG_PM_S2RAM)
	case PM_STATE_SUSPEND_TO_RAM:
		SPC_SetLowPowerWakeUpDelay(MCXA_SPC_ADDR, MCXA_WAKEUP_DELAY);

		if (mcxa_s2ram_retain_sram() != 0) {
			/* SRAM retention could not be armed; entering Deep Power
			 * Down now would lose the retained context, so stay running.
			 */
			break;
		}

		mcxa_scs_save();

		/* A non-zero return means the SoC never entered Deep Power Down
		 * and is still fully operational, so skip the resume-path
		 * re-initialization below.
		 */
		if (arch_pm_s2ram_suspend(mcxa_enter_deep_power_down) != 0) {
			break;
		}

		SystemInit();

		/* re-build clock tree */
		board_early_init_hook();

		/* idle-thread stack is nearly full, it is easy to reach the
		 * PSPLIM limit on resume. Drop the limit here, the next context
		 * switch re-establishes the per-thread PSPLIM.
		 */
		__set_PSPLIM(0U);

		mcxa_scs_restore();
		break;
#endif

	default:
		break;
	}
}

__weak void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(substate_id);
#if !defined(CONFIG_PM_S2RAM)
	ARG_UNUSED(state);
#endif

	if ((SCB->SCR & SCB_SCR_SLEEPDEEP_Msk) == SCB_SCR_SLEEPDEEP_Msk) {
		SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;
	}

#if defined(CONFIG_PM_S2RAM)
	if (state == PM_STATE_SUSPEND_TO_RAM) {
		SPC_ClearPeriphIOIsolationFlag(MCXA_SPC_ADDR);
	}
#endif

	SPC_ClearPowerDomainLowPowerRequestFlag(MCXA_SPC_ADDR, kSPC_PowerDomain0);
	SPC_ClearLowPowerRequest(MCXA_SPC_ADDR);
}
