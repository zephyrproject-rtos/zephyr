/*
 * Copyright (c) 2021, 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/pm/pm.h>
#include <fsl_dcdc.h>
#include <fsl_gpc.h>
#include <zephyr/dt-bindings/power/imx_spc.h>
#include "power.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(soc, CONFIG_SOC_LOG_LEVEL);

/*
 * NOTE: When multicore support in RT1170/1160 is properly implemented,
 * power saving will improve when both cores request a transition to a low
 * power mode
 */

#ifdef CONFIG_CPU_CORTEX_M7
#define GPC_CPU_MODE_CTRL GPC_CPU_MODE_CTRL_0
#elif CONFIG_CPU_CORTEX_M4
#define GPC_CPU_MODE_CTRL GPC_CPU_MODE_CTRL_1
#else
#error "RT11xx power code supports M4 and M7 cores only"
#endif

/* Configure the set point mappings for Cortex M4 and M7 cores */
static void gpc_set_core_mappings(void)
{
	uint8_t i, j;
	uint32_t tmp;

#ifdef CONFIG_CPU_CORTEX_M7
	uint8_t mapping[SET_POINT_COUNT][SET_POINT_COUNT] = CPU0_COMPATIBLE_SP_TABLE;
#elif CONFIG_CPU_CORTEX_M4
	uint8_t mapping[SET_POINT_COUNT][SET_POINT_COUNT] = CPU1_COMPATIBLE_SP_TABLE;
#else
#error "RT11xx power code supports M4 and M7 cores only"
#endif
	/* Cortex Set point mappings */
	for (i = 0; i < SET_POINT_COUNT; i++) {
		tmp = 0x0;
		for (j = 0; j < SET_POINT_COUNT; j++) {
			tmp |= mapping[i][j] << mapping[0][j];
		}
		GPC_CM_SetSetPointMapping(GPC_CPU_MODE_CTRL, mapping[i][0], tmp);
	}
}

/* Configure GPC transition steps to enabled */
static void gpc_set_transition_flow(void)
{
	gpc_tran_step_config_t step_cfg;

	step_cfg.enableStep = true;

	/* Cortex M7 */
	GPC_CM_ConfigCpuModeTransitionStep(GPC_CPU_MODE_CTRL,
		kGPC_CM_SleepSsar, &step_cfg);
	GPC_CM_ConfigCpuModeTransitionStep(GPC_CPU_MODE_CTRL,
		kGPC_CM_SleepLpcg, &step_cfg);
	GPC_CM_ConfigCpuModeTransitionStep(GPC_CPU_MODE_CTRL,
		kGPC_CM_SleepPll, &step_cfg);
	GPC_CM_ConfigCpuModeTransitionStep(GPC_CPU_MODE_CTRL,
		kGPC_CM_SleepIso, &step_cfg);
	GPC_CM_ConfigCpuModeTransitionStep(GPC_CPU_MODE_CTRL,
		kGPC_CM_SleepReset, &step_cfg);
	GPC_CM_ConfigCpuModeTransitionStep(GPC_CPU_MODE_CTRL,
		kGPC_CM_SleepPower, &step_cfg);
	GPC_CM_ConfigCpuModeTransitionStep(GPC_CPU_MODE_CTRL,
		kGPC_CM_WakeupPower, &step_cfg);
	GPC_CM_ConfigCpuModeTransitionStep(GPC_CPU_MODE_CTRL,
		kGPC_CM_WakeupReset, &step_cfg);
	GPC_CM_ConfigCpuModeTransitionStep(GPC_CPU_MODE_CTRL,
		kGPC_CM_WakeupIso, &step_cfg);
	GPC_CM_ConfigCpuModeTransitionStep(GPC_CPU_MODE_CTRL,
		kGPC_CM_WakeupPll, &step_cfg);
	GPC_CM_ConfigCpuModeTransitionStep(GPC_CPU_MODE_CTRL,
		kGPC_CM_WakeupLpcg, &step_cfg);
	GPC_CM_ConfigCpuModeTransitionStep(GPC_CPU_MODE_CTRL,
		kGPC_CM_WakeupSsar, &step_cfg);

	/* Enable all steps in flow of set point transition */
	GPC_SP_ConfigSetPointTransitionStep(GPC_SET_POINT_CTRL,
		kGPC_SP_SsarSave, &step_cfg);
	GPC_SP_ConfigSetPointTransitionStep(GPC_SET_POINT_CTRL,
		kGPC_SP_LpcgOff, &step_cfg);
	GPC_SP_ConfigSetPointTransitionStep(GPC_SET_POINT_CTRL,
		kGPC_SP_GroupDown, &step_cfg);
	GPC_SP_ConfigSetPointTransitionStep(GPC_SET_POINT_CTRL,
		kGPC_SP_RootDown, &step_cfg);
	GPC_SP_ConfigSetPointTransitionStep(GPC_SET_POINT_CTRL,
		kGPC_SP_PllOff, &step_cfg);
	GPC_SP_ConfigSetPointTransitionStep(GPC_SET_POINT_CTRL,
		kGPC_SP_IsoOn, &step_cfg);
	GPC_SP_ConfigSetPointTransitionStep(GPC_SET_POINT_CTRL,
		kGPC_SP_ResetEarly, &step_cfg);
	GPC_SP_ConfigSetPointTransitionStep(GPC_SET_POINT_CTRL,
		kGPC_SP_PowerOff, &step_cfg);
	GPC_SP_ConfigSetPointTransitionStep(GPC_SET_POINT_CTRL,
		kGPC_SP_BiasOff, &step_cfg);
	GPC_SP_ConfigSetPointTransitionStep(GPC_SET_POINT_CTRL,
		kGPC_SP_BandgapPllLdoOff, &step_cfg);
	GPC_SP_ConfigSetPointTransitionStep(GPC_SET_POINT_CTRL,
		kGPC_SP_LdoPre, &step_cfg);
	GPC_SP_ConfigSetPointTransitionStep(GPC_SET_POINT_CTRL,
		kGPC_SP_DcdcDown, &step_cfg);
	GPC_SP_ConfigSetPointTransitionStep(GPC_SET_POINT_CTRL,
		kGPC_SP_DcdcUp, &step_cfg);
	GPC_SP_ConfigSetPointTransitionStep(GPC_SET_POINT_CTRL,
		kGPC_SP_LdoPost, &step_cfg);
	GPC_SP_ConfigSetPointTransitionStep(GPC_SET_POINT_CTRL,
		kGPC_SP_BandgapPllLdoOn, &step_cfg);
	GPC_SP_ConfigSetPointTransitionStep(GPC_SET_POINT_CTRL,
		kGPC_SP_BiasOn, &step_cfg);
	GPC_SP_ConfigSetPointTransitionStep(GPC_SET_POINT_CTRL,
		kGPC_SP_PowerOn, &step_cfg);
	GPC_SP_ConfigSetPointTransitionStep(GPC_SET_POINT_CTRL,
		kGPC_SP_ResetLate, &step_cfg);
	GPC_SP_ConfigSetPointTransitionStep(GPC_SET_POINT_CTRL,
		kGPC_SP_IsoOff, &step_cfg);
	GPC_SP_ConfigSetPointTransitionStep(GPC_SET_POINT_CTRL,
		kGPC_SP_PllOn, &step_cfg);
	GPC_SP_ConfigSetPointTransitionStep(GPC_SET_POINT_CTRL,
		kGPC_SP_RootUp, &step_cfg);
	GPC_SP_ConfigSetPointTransitionStep(GPC_SET_POINT_CTRL,
		kGPC_SP_GroupUp, &step_cfg);
	GPC_SP_ConfigSetPointTransitionStep(GPC_SET_POINT_CTRL,
		kGPC_SP_LpcgOn, &step_cfg);
	GPC_SP_ConfigSetPointTransitionStep(GPC_SET_POINT_CTRL,
		kGPC_SP_SsarRestore, &step_cfg);

	/* Enable all steps in standby transition */
	GPC_STBY_ConfigStandbyTransitionStep(GPC_STBY_CTRL,
		kGPC_STBY_LpcgIn, &step_cfg);
	GPC_STBY_ConfigStandbyTransitionStep(GPC_STBY_CTRL,
		kGPC_STBY_PllIn, &step_cfg);
	GPC_STBY_ConfigStandbyTransitionStep(GPC_STBY_CTRL,
		kGPC_STBY_BiasIn, &step_cfg);
	GPC_STBY_ConfigStandbyTransitionStep(GPC_STBY_CTRL,
		kGPC_STBY_PldoIn, &step_cfg);
	GPC_STBY_ConfigStandbyTransitionStep(GPC_STBY_CTRL,
		kGPC_STBY_BandgapIn, &step_cfg);
	GPC_STBY_ConfigStandbyTransitionStep(GPC_STBY_CTRL,
		kGPC_STBY_LdoIn, &step_cfg);
	GPC_STBY_ConfigStandbyTransitionStep(GPC_STBY_CTRL,
		kGPC_STBY_DcdcIn, &step_cfg);
	GPC_STBY_ConfigStandbyTransitionStep(GPC_STBY_CTRL,
		kGPC_STBY_PmicIn, &step_cfg);
	GPC_STBY_ConfigStandbyTransitionStep(GPC_STBY_CTRL,
		kGPC_STBY_PmicOut, &step_cfg);
	GPC_STBY_ConfigStandbyTransitionStep(GPC_STBY_CTRL,
		kGPC_STBY_DcdcOut, &step_cfg);
	GPC_STBY_ConfigStandbyTransitionStep(GPC_STBY_CTRL,
		kGPC_STBY_LdoOut, &step_cfg);
	GPC_STBY_ConfigStandbyTransitionStep(GPC_STBY_CTRL,
		kGPC_STBY_BandgapOut, &step_cfg);
	GPC_STBY_ConfigStandbyTransitionStep(GPC_STBY_CTRL,
		kGPC_STBY_PldoOut, &step_cfg);
	GPC_STBY_ConfigStandbyTransitionStep(GPC_STBY_CTRL,
		kGPC_STBY_BiasOut, &step_cfg);
	GPC_STBY_ConfigStandbyTransitionStep(GPC_STBY_CTRL,
		kGPC_STBY_PllOut, &step_cfg);
	GPC_STBY_ConfigStandbyTransitionStep(GPC_STBY_CTRL,
		kGPC_STBY_LpcgOut, &step_cfg);
}

static void gpc_configure_interrupts(void)
{
	uint8_t i;
	uint32_t irq = DT_IRQN(DT_INST(0, nxp_gpt_hw_timer));

	/* Disable all GPC interrupt sources */
	for (i = 0; i < GPC_CPU_MODE_CTRL_CM_IRQ_WAKEUP_MASK_COUNT; i++) {
		GPC_CPU_MODE_CTRL->CM_IRQ_WAKEUP_MASK[i] |= 0xFFFFFFFF;
	}

	/* Enable GPT interrupt source for GPC- this is system timer */
	GPC_CM_EnableIrqWakeup(GPC_CPU_MODE_CTRL, irq, true);
}

/* Initializes configuration for the GPC */
static void gpc_init(void)
{
	/* Setup GPC set point mappings */
	gpc_set_core_mappings();
	/* Setup GPC set point transition flow */
	gpc_set_transition_flow();
	/* Allow GPC to disable ROSC */
	GPC_SET_POINT_CTRL->SP_ROSC_CTRL = ~OSC_RC_16M_STBY_VAL;
	/* Setup GPC interrupts */
	gpc_configure_interrupts();
}

/* Initializes DCDC converter with power saving settings */
static void dcdc_init(void)
{
	dcdc_config_t dcdc_config;
	dcdc_setpoint_config_t dcdc_setpoint_config;

	dcdc_buck_mode_1P8_target_vol_t buck1_8_voltage[16] =
		DCDC_1P8_BUCK_MODE_CONFIGURATION_TABLE;
	dcdc_buck_mode_1P0_target_vol_t buck1_0_voltage[16] =
		DCDC_1P0_BUCK_MODE_CONFIGURATION_TABLE;
	dcdc_standby_mode_1P8_target_vol_t standby1_8_voltage[16] =
		DCDC_1P8_STANDBY_MODE_CONFIGURATION_TABLE;
	dcdc_standby_mode_1P0_target_vol_t standby1_0_voltage[16] =
		DCDC_1P0_STANDBY_MODE_CONFIGURATION_TABLE;


	DCDC_BootIntoDCM(DCDC);

	dcdc_setpoint_config.enableDCDCMap = DCDC_ONOFF_SP_VAL;
	dcdc_setpoint_config.enableDigLogicMap = DCDC_DIG_ONOFF_SP_VAL;
	dcdc_setpoint_config.lowpowerMap = DCDC_LP_MODE_SP_VAL;
	dcdc_setpoint_config.standbyMap = DCDC_ONOFF_STBY_VAL;
	dcdc_setpoint_config.standbyLowpowerMap = DCDC_LP_MODE_STBY_VAL;
	dcdc_setpoint_config.buckVDD1P8TargetVoltage = buck1_8_voltage;
	dcdc_setpoint_config.buckVDD1P0TargetVoltage = buck1_0_voltage;
	dcdc_setpoint_config.standbyVDD1P8TargetVoltage = standby1_8_voltage;
	dcdc_setpoint_config.standbyVDD1P0TargetVoltage = standby1_0_voltage;
	DCDC_SetPointInit(DCDC, &dcdc_setpoint_config);

	DCDC_GetDefaultConfig(&dcdc_config);
	dcdc_config.controlMode = kDCDC_SetPointControl;
	DCDC_Init(DCDC, &dcdc_config);
}

static void system_enter_sleep(gpc_cpu_mode_t gpc_mode)
{
	__ASSERT_NO_MSG(gpc_mode != kGPC_RunMode);

	if (gpc_mode == kGPC_WaitMode) {
		/* Clear SLEEPDEEP bit to enter WAIT mode*/
		SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;
	} else {
		/* Set SLEEPDEEP bit to enter STOP mode */
		SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
	}
	/* When this function is entered the Kernel has disabled
	 * interrupts using BASEPRI register. We will clear BASEPRI, and use PRIMASK
	 * to disable interrupts, so that the WFI instruction works correctly.
	 */

	/* Set PRIMASK */
	__disable_irq();
	/* Set BASEPRI to 0 */
	irq_unlock(0);

	/* WFI instruction will start entry into WAIT/STOP mode */
	LOG_DBG("Entering LPM via WFI");
	__WFI();
}

void cpu_mode_transition(gpc_cpu_mode_t mode, bool enable_standby)
{
	GPC_CM_SetNextCpuMode(GPC_CPU_MODE_CTRL, mode);
	GPC_CM_EnableCpuSleepHold(GPC_CPU_MODE_CTRL, true);

	/* Mask debugger wakeup */
	GPC_CPU_MODE_CTRL->CM_NON_IRQ_WAKEUP_MASK |=
		GPC_CPU_MODE_CTRL_CM_NON_IRQ_WAKEUP_MASK_EVENT_WAKEUP_MASK_MASK |
		GPC_CPU_MODE_CTRL_CM_NON_IRQ_WAKEUP_MASK_DEBUG_WAKEUP_MASK_MASK;

	if (enable_standby) {
		/* Set standby request */
		GPC_CM_RequestStandbyMode(GPC_CPU_MODE_CTRL, mode);
	} else {
		/* Clear standby request */
		GPC_CM_ClearStandbyModeRequest(GPC_CPU_MODE_CTRL, mode);
	}

	/* Execute WFI- GPC will receive sleep request from CPU */
	system_enter_sleep(mode);
}

/**
 * SOC specific low power mode implementation
 * Drop to lowest power state possible given system's request
 */
void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(state);

	/* Extract set point and GPC mode from the substate ID */
	uint8_t set_point = IMX_SPC(substate_id);
	gpc_cpu_mode_t gpc_mode = IMX_GPC_MODE(substate_id);
	uint8_t current_set_point = GPC_SP_GetCurrentSetPoint(GPC_SET_POINT_CTRL);

	LOG_DBG("Switch to Set Point %d, GPC Mode %d requested", set_point, gpc_mode);
	if (gpc_mode != kGPC_RunMode && (current_set_point != set_point)) {
		/* Request set point transition at sleep */
		GPC_CM_RequestSleepModeSetPointTransition(GPC_CPU_MODE_CTRL,
			set_point, set_point, kGPC_CM_RequestPreviousSetpoint);
		cpu_mode_transition(gpc_mode, true);
	} else if (gpc_mode != kGPC_RunMode) {
		/* Request CPU mode transition without set mode transition */
		GPC_CM_RequestRunModeSetPointTransition(GPC_CPU_MODE_CTRL,
							current_set_point);
		cpu_mode_transition(gpc_mode, true);
	}
}

void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(state);
	ARG_UNUSED(substate_id);

	/* Clear PRIMASK */
	__enable_irq();
	LOG_DBG("Exiting LPM");
	LOG_DBG("CM7 mode was %d", GPC_CM_GetPreviousCpuMode(GPC_CPU_MODE_CTRL_0));
	LOG_DBG("CM4 mode was %d", GPC_CM_GetPreviousCpuMode(GPC_CPU_MODE_CTRL_1));
	LOG_DBG("Previous set point was %d", GPC_SP_GetPreviousSetPoint(GPC_SET_POINT_CTRL));
}

/* Initialize RT11xx Power */
static int rt11xx_power_init(void)
{
	/* Drop SOC target voltage to 1.0 V */
	DCDC_SetVDD1P0BuckModeTargetVoltage(DCDC, kDCDC_1P0BuckTarget1P0V);
	/* Initialize general power controller */
	gpc_init();
	/* Initialize dcdc */
	dcdc_init();
	return 0;
}

SYS_INIT(rt11xx_power_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
