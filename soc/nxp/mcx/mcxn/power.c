/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/pm/pm.h>
#include <zephyr/device.h>
#include <fsl_cmc.h>
#include <fsl_spc.h>
#include <fsl_wuu.h>

#define WUU_WAKEUP_LPTMR0_IDX	6U
#define MCXN_WAKEUP_DELAY	DT_PROP_OR(DT_NODELABEL(spc), wakeup_delay, 0)
#define MCXN_WUU_ADDR		(WUU_Type *)DT_REG_ADDR(DT_INST(0, nxp_wuu))
#define MCXN_CMC_ADDR		(CMC_Type *)DT_REG_ADDR(DT_INST(0, nxp_cmc))
#define MCXN_SPC_ADDR		(SPC_Type *)DT_REG_ADDR(DT_INST(0, nxp_spc))

static void pm_enter_hook(void)
{
	CMC_SetPowerModeProtection(MCXN_CMC_ADDR, kCMC_AllowAllLowPowerModes);
	CMC_EnableDebugOperation(MCXN_CMC_ADDR, false);
	CMC_ConfigFlashMode(MCXN_CMC_ADDR, true, false);
	WUU_SetInternalWakeUpModulesConfig(MCXN_WUU_ADDR, WUU_WAKEUP_LPTMR0_IDX,
					kWUU_InternalModuleInterrupt);
}

__weak void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	pm_enter_hook();

	__enable_irq();
	__set_BASEPRI(0);

	SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;

	switch (state) {
	case PM_STATE_RUNTIME_IDLE:
		CMC_SetClockMode(MCXN_CMC_ADDR, kCMC_GateCoreClock);
		CMC_SetMAINPowerMode(MCXN_CMC_ADDR, kCMC_ActiveOrSleepMode);
		CMC_SetWAKEPowerMode(MCXN_CMC_ADDR, kCMC_ActiveOrSleepMode);
		__WFI();
		break;

	case PM_STATE_SUSPEND_TO_IDLE:
		CMC_SetClockMode(MCXN_CMC_ADDR, kCMC_GateAllSystemClocksEnterLowPowerMode);
		CMC_SetMAINPowerMode(MCXN_CMC_ADDR, kCMC_DeepSleepMode);
		CMC_SetWAKEPowerMode(MCXN_CMC_ADDR, kCMC_DeepSleepMode);
		__WFI();
		break;

	case PM_STATE_STANDBY:
		SPC_SetLowPowerWakeUpDelay(SPC0, MCXN_WAKEUP_DELAY);
		CMC_SetClockMode(MCXN_CMC_ADDR, kCMC_GateAllSystemClocksEnterLowPowerMode);
		CMC_SetMAINPowerMode(MCXN_CMC_ADDR, kCMC_PowerDownMode);
		CMC_SetWAKEPowerMode(MCXN_CMC_ADDR, kCMC_PowerDownMode);
		__WFI();
		break;

	default:
		break;
	}
}

__weak void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(state);
	ARG_UNUSED(substate_id);

	if ((SCB->SCR & SCB_SCR_SLEEPDEEP_Msk) == SCB_SCR_SLEEPDEEP_Msk) {
		SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;
	}

	__enable_irq();
	__ISB();

	SPC_ClearPowerDomainLowPowerRequestFlag(MCXN_SPC_ADDR, kSPC_PowerDomain0);
	SPC_ClearPowerDomainLowPowerRequestFlag(MCXN_SPC_ADDR, kSPC_PowerDomain1);
	SPC_ClearLowPowerRequest(MCXN_SPC_ADDR);
}
