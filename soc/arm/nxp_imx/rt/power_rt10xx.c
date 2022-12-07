/*
 * Copyright (c) 2021 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Note: this file is linked to RAM. Any functions called while preparing for
 * sleep mode must be defined within this file, or linked to RAM.
 */
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/pm/pm.h>
#include <fsl_dcdc.h>
#include <fsl_pmu.h>
#include <fsl_gpc.h>
#include <fsl_clock.h>
#include <zephyr/logging/log.h>

#include "power_rt10xx.h"

LOG_MODULE_REGISTER(soc_power, CONFIG_SOC_LOG_LEVEL);


static struct clock_callbacks lpm_clock_hooks;

/*
 * Boards with RT10XX SOCs can register callbacks to set their clocks into
 * normal/full speed mode, low speed mode, and low power mode.
 * If callbacks are present, the low power subsystem will disable
 * PLLs for power savings when entering low power states.
 */
void imxrt_clock_pm_callbacks_register(struct clock_callbacks *callbacks)
{
	/* If run callback is set, low power must be as well. */
	__ASSERT_NO_MSG(callbacks && callbacks->clock_set_run && callbacks->clock_set_low_power);
	lpm_clock_hooks.clock_set_run = callbacks->clock_set_run;
	lpm_clock_hooks.clock_set_low_power = callbacks->clock_set_low_power;
	if (callbacks->clock_lpm_init) {
		lpm_clock_hooks.clock_lpm_init = callbacks->clock_lpm_init;
	}
}

static void lpm_set_sleep_mode_config(clock_mode_t mode)
{
	uint32_t clpcr;

	/* Set GPC wakeup config to GPT timer interrupt */
	GPC_EnableIRQ(GPC, DT_IRQN(DT_INST(0, nxp_gpt_hw_timer)));
	/*
	 * ERR050143: CCM: When improper low-power sequence is used,
	 * the SoC enters low power mode before the ARM core executes WFI.
	 *
	 * Software workaround:
	 * 1) Software should trigger IRQ #41 (GPR_IRQ) to be always pending
	 *      by setting IOMUXC_GPR_GPR1_GINT.
	 * 2) Software should then unmask IRQ #41 in GPC before setting CCM
	 *      Low-Power mode.
	 * 3) Software should mask IRQ #41 right after CCM Low-Power mode
	 *      is set (set bits 0-1 of CCM_CLPCR).
	 */
	GPC_EnableIRQ(GPC, GPR_IRQ_IRQn);
	clpcr = CCM->CLPCR & (~(CCM_CLPCR_LPM_MASK | CCM_CLPCR_ARM_CLK_DIS_ON_LPM_MASK));
	/* Note: if CCM_CLPCR_ARM_CLK_DIS_ON_LPM_MASK is set,
	 * debugger will not connect in sleep mode
	 */
	/* Set clock control module to transfer system to idle mode */
	clpcr |= CCM_CLPCR_LPM(mode) | CCM_CLPCR_MASK_SCU_IDLE_MASK |
		     CCM_CLPCR_MASK_L2CC_IDLE_MASK |
		     CCM_CLPCR_STBY_COUNT_MASK |
		     CCM_CLPCR_ARM_CLK_DIS_ON_LPM_MASK;
#ifndef CONFIG_SOC_MIMXRT1011
	/* RT1011 does not include handshake bits */
	clpcr |= CCM_CLPCR_BYPASS_LPM_HS0_MASK | CCM_CLPCR_BYPASS_LPM_HS1_MASK;
#endif
	CCM->CLPCR = clpcr;
	GPC_DisableIRQ(GPC, GPR_IRQ_IRQn);
}

static void lpm_enter_soft_off_mode(void)
{
	/* Enable the SNVS RTC as a wakeup source from soft-off mode, in case an RTC alarm
	 * was set.
	 */
	GPC_EnableIRQ(GPC, DT_IRQN(DT_INST(0, nxp_imx_snvs_rtc)));
	SNVS->LPCR |= SNVS_LPCR_TOP_MASK;
}

static void lpm_enter_sleep_mode(clock_mode_t mode)
{
	/* FIXME: When this function is entered the Kernel has disabled
	 * interrupts using BASEPRI register. This is incorrect as it prevents
	 * waking up from any interrupt which priority is not 0. Work around the
	 * issue and disable interrupts using PRIMASK register as recommended
	 * by ARM.
	 */

	/* Set PRIMASK */
	__disable_irq();
	/* Set BASEPRI to 0 */
	irq_unlock(0);
	__DSB();
	__ISB();

	if (mode == kCLOCK_ModeWait) {
		/* Clear the SLEEPDEEP bit to go into sleep mode (WAIT) */
		SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;
	} else {
		/* Set the SLEEPDEEP bit to enable deep sleep mode (STOP) */
		SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
	}
	/* WFI instruction will start entry into WAIT/STOP mode */
	__WFI();

}

static void lpm_set_run_mode_config(void)
{
	/* Clear GPC wakeup source */
	GPC_DisableIRQ(GPC, DT_IRQN(DT_INST(0, nxp_gpt_hw_timer)));
	CCM->CLPCR &= ~(CCM_CLPCR_LPM_MASK | CCM_CLPCR_ARM_CLK_DIS_ON_LPM_MASK);
}

/* Toggle the analog bandgap reference circuitry on and off */
static void bandgap_set(bool on)
{
	if (on) {
		/* Enable bandgap in PMU */
		PMU->MISC0_CLR = PMU_MISC0_REFTOP_PWD_MASK;
		/* Wait for it to stabilize */
		while ((PMU->MISC0 & PMU_MISC0_REFTOP_VBGUP_MASK) == 0) {

		}
		/* Disable low power bandgap */
		XTALOSC24M->LOWPWR_CTRL_CLR = XTALOSC24M_LOWPWR_CTRL_LPBG_SEL_MASK;
	} else {
		/* Disable bandgap in PMU and switch to low power one */
		XTALOSC24M->LOWPWR_CTRL_SET = XTALOSC24M_LOWPWR_CTRL_LPBG_SEL_MASK;
		PMU->MISC0_SET = PMU_MISC0_REFTOP_PWD_MASK;
	}
}

/* Should only be used if core clocks have been reduced- drops SOC voltage */
static void lpm_drop_voltage(void)
{
	/* Move to the internal RC oscillator, since we are using low power clocks */
	CLOCK_InitRcOsc24M();
	/* Switch to internal RC oscillator */
	CLOCK_SwitchOsc(kCLOCK_RcOsc);
	CLOCK_DeinitExternalClk();
	/*
	 * Change to 1.075V SOC voltage. If you are experiencing issues with
	 * low power mode stability, try raising this voltage value.
	 */
	DCDC_AdjustRunTargetVoltage(DCDC, 0xB);
	/* Enable 2.5 and 1.1V weak regulators */
	PMU_2P5EnableWeakRegulator(PMU, true);
	PMU_1P1EnableWeakRegulator(PMU, true);
	/* Disable normal regulators */
	PMU_2P5EnableOutput(PMU, false);
	PMU_1P1EnableOutput(PMU, false);
	/* Disable analog bandgap */
	bandgap_set(false);
}

/* Undo the changes made by lpm_drop_voltage so clocks can be raised */
static void lpm_raise_voltage(void)
{
	/* Enable analog bandgap */
	bandgap_set(true);
	/* Enable regulator LDOs */
	PMU_2P5EnableOutput(PMU, true);
	PMU_1P1EnableOutput(PMU, true);
	/* Disable weak LDOs */
	PMU_2P5EnableWeakRegulator(PMU, false);
	PMU_1P1EnableWeakRegulator(PMU, false);
	/* Change to 1.275V SOC voltage */
	DCDC_AdjustRunTargetVoltage(DCDC, 0x13);
	/* Move to the external RC oscillator */
	CLOCK_InitExternalClk(0);
	/* Switch clock source to external OSC. */
	CLOCK_SwitchOsc(kCLOCK_XtalOsc);
}


/* Sets device into low power mode */
__weak void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(substate_id);

	switch (state) {
	case PM_STATE_RUNTIME_IDLE:
		LOG_DBG("entering PM state runtime idle");
		lpm_set_sleep_mode_config(kCLOCK_ModeWait);
		lpm_enter_sleep_mode(kCLOCK_ModeWait);
		break;
	case PM_STATE_SUSPEND_TO_IDLE:
		LOG_DBG("entering PM state suspend to idle");
		if (lpm_clock_hooks.clock_set_low_power) {
			/* Drop the SOC clocks to low power mode, and decrease core voltage */
			lpm_clock_hooks.clock_set_low_power();
			lpm_drop_voltage();
		}
		lpm_set_sleep_mode_config(kCLOCK_ModeWait);
		lpm_enter_sleep_mode(kCLOCK_ModeWait);
		break;
	case PM_STATE_SOFT_OFF:
		LOG_DBG("Entering PM state soft off");
		lpm_enter_soft_off_mode();
		break;
	default:
		return;
	}
}

/* Handle SOC specific activity after Low Power Mode Exit */
__weak void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(substate_id);

	/* Set run mode config after wakeup */
	switch (state) {
	case PM_STATE_RUNTIME_IDLE:
		lpm_set_run_mode_config();
		LOG_DBG("exited PM state runtime idle");
		break;
	case PM_STATE_SUSPEND_TO_IDLE:
		lpm_set_run_mode_config();
		if (lpm_clock_hooks.clock_set_run) {
			/* Raise core voltage and restore SOC clocks */
			lpm_raise_voltage();
			lpm_clock_hooks.clock_set_run();
		}
		LOG_DBG("exited PM state suspend to idle");
		break;
	default:
		break;
	}
	/* Clear PRIMASK after wakeup */
	__enable_irq();
}

/* Initialize power system */
static int rt10xx_power_init(const struct device *dev)
{
	dcdc_internal_regulator_config_t reg_config;

	ARG_UNUSED(dev);

	/* Ensure clocks to ARM core memory will not be gated in low power mode
	 * if interrupt is pending
	 */
	CCM->CGPR |= CCM_CGPR_INT_MEM_CLK_LPM_MASK;

	if (lpm_clock_hooks.clock_lpm_init) {
		lpm_clock_hooks.clock_lpm_init();
	}

	/* Errata ERR050143 */
	IOMUXC_GPR->GPR1 |= IOMUXC_GPR_GPR1_GINT_MASK;

	/* Configure DCDC */
	DCDC_BootIntoDCM(DCDC);
	/* Set target voltage for low power mode to 0.925V*/
	DCDC_AdjustLowPowerTargetVoltage(DCDC, 0x1);
	/* Reconfigure DCDC to disable internal load resistor */
	reg_config.enableLoadResistor = false;
	reg_config.feedbackPoint = 0x1; /* 1.0V with 1.3V reference voltage */
	DCDC_SetInternalRegulatorConfig(DCDC, &reg_config);

	/* Enable high gate drive on power FETs to reduce leakage current */
	PMU_CoreEnableIncreaseGateDrive(PMU, true);


	return 0;
}

SYS_INIT(rt10xx_power_init, PRE_KERNEL_2, 0);
