/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/pm/pm.h>
#include <zephyr/arch/arch_interface.h>
#include <zephyr/device.h>
#include <fsl_cmc.h>
#include <fsl_spc.h>

#if defined(CONFIG_PM_S2RAM)
void mcxn_pm_suspend_to_ram(void);
#endif

#define MCXN_WAKEUP_DELAY	DT_PROP_OR(DT_INST(0, nxp_spc), wakeup_delay, 0)
#define MCXN_CMC_ADDR		(CMC_Type *)DT_REG_ADDR(DT_INST(0, nxp_cmc))
#define MCXN_SPC_ADDR		(SPC_Type *)DT_REG_ADDR(DT_INST(0, nxp_spc))

static void pm_enter_hook(void)
{
	CMC_SetPowerModeProtection(MCXN_CMC_ADDR, kCMC_AllowAllLowPowerModes);
	CMC_EnableDebugOperation(MCXN_CMC_ADDR, false);
	CMC_ConfigFlashMode(MCXN_CMC_ADDR, true, false);
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

__weak void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	pm_enter_hook();

	SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;

	switch (state) {
	case PM_STATE_RUNTIME_IDLE:
		CMC_SetClockMode(MCXN_CMC_ADDR, kCMC_GateCoreClock);
		CMC_SetMAINPowerMode(MCXN_CMC_ADDR, kCMC_ActiveOrSleepMode);
		CMC_SetWAKEPowerMode(MCXN_CMC_ADDR, kCMC_ActiveOrSleepMode);
		enter_low_power();
		break;

	case PM_STATE_SUSPEND_TO_IDLE:
		CMC_SetClockMode(MCXN_CMC_ADDR, kCMC_GateAllSystemClocksEnterLowPowerMode);
		CMC_SetMAINPowerMode(MCXN_CMC_ADDR, kCMC_DeepSleepMode);
		CMC_SetWAKEPowerMode(MCXN_CMC_ADDR, kCMC_DeepSleepMode);
		enter_low_power();
		break;

	case PM_STATE_STANDBY:
		SPC_SetLowPowerWakeUpDelay(MCXN_SPC_ADDR, MCXN_WAKEUP_DELAY);
		CMC_SetClockMode(MCXN_CMC_ADDR, kCMC_GateAllSystemClocksEnterLowPowerMode);
		CMC_SetMAINPowerMode(MCXN_CMC_ADDR, kCMC_PowerDownMode);
		CMC_SetWAKEPowerMode(MCXN_CMC_ADDR, kCMC_PowerDownMode);
		enter_low_power();
		break;

#if defined(CONFIG_PM_S2RAM)
	case PM_STATE_SUSPEND_TO_RAM:
		mcxn_pm_suspend_to_ram();
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

	CMC_SetClockMode(MCXN_CMC_ADDR, kCMC_GateNoneClock);

#if defined(CONFIG_PM_S2RAM)
	if (state == PM_STATE_SUSPEND_TO_RAM) {
		SPC_ClearPeriphIOIsolationFlag(MCXN_SPC_ADDR);
	}
#endif

	SPC_ClearPowerDomainLowPowerRequestFlag(MCXN_SPC_ADDR, kSPC_PowerDomain0);
	SPC_ClearPowerDomainLowPowerRequestFlag(MCXN_SPC_ADDR, kSPC_PowerDomain1);
	SPC_ClearLowPowerRequest(MCXN_SPC_ADDR);
}
