/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/pm/pm.h>
#include <fsl_cmc.h>
#include <fsl_spc.h>
#include <fsl_vbat.h>
#include <fsl_wuu.h>

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(soc, CONFIG_SOC_LOG_LEVEL);

#define WUU_WAKEUP_LPTMR_IDX 0
#define MCXW7_WUU_ADDR (WUU_Type *)DT_REG_ADDR(DT_INST(0, nxp_wuu))
#define MCXW7_CMC_ADDR (CMC_Type *)DT_REG_ADDR(DT_INST(0, nxp_cmc))
#define MCXW7_VBAT_ADDR (VBAT_Type *)DT_REG_ADDR(DT_INST(0, nxp_vbat))
#define MCXW7_SPC_ADDR (SPC_Type *)DT_REG_ADDR(DT_INST(0, nxp_spc))

void mcxw7xx_set_wakeup(int32_t sig)
{
	WUU_SetInternalWakeUpModulesConfig(MCXW7_WUU_ADDR, sig, kWUU_InternalModuleInterrupt);
}

/*
 * 1. Set power mode protection
 * 2. Disable low power mode debug
 * 3. Enable Flash Doze mode.
 */
static void set_cmc_configuration(void)
{
	CMC_SetPowerModeProtection(MCXW7_CMC_ADDR, kCMC_AllowAllLowPowerModes);
	CMC_LockPowerModeProtectionSetting(MCXW7_CMC_ADDR);
	CMC_EnableDebugOperation(MCXW7_CMC_ADDR, IS_ENABLED(CONFIG_DEBUG));
	CMC_ConfigFlashMode(MCXW7_CMC_ADDR, false, false, false);
}

/*
 * Disable Backup SRAM regulator, FRO16K and Bandgap which
 * locates in VBAT power domain for most of power modes.
 *
 */
static void deinit_vbat(void)
{
	VBAT_EnableBackupSRAMRegulator(MCXW7_VBAT_ADDR, false);
	VBAT_EnableFRO16k(MCXW7_VBAT_ADDR, false);
	while (VBAT_CheckFRO16kEnabled(MCXW7_VBAT_ADDR)) {
	};
	VBAT_EnableBandgap(MCXW7_VBAT_ADDR, false);
	while (VBAT_CheckBandgapEnabled(MCXW7_VBAT_ADDR)) {
	};
}

/* Invoke Low Power/System Off specific Tasks */
__weak void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	/* Set PRIMASK */
	__disable_irq();
	/* Set BASEPRI to 0 */
	irq_unlock(0);

	set_cmc_configuration();
	deinit_vbat();

	switch (state) {
	case PM_STATE_SUSPEND_TO_IDLE:
		cmc_power_domain_config_t config;

		/* Set NBU into Sleep Mode */
		RFMC->RF2P4GHZ_CTRL = (RFMC->RF2P4GHZ_CTRL &
				       (~RFMC_RF2P4GHZ_CTRL_LP_MODE_MASK)) |
				       RFMC_RF2P4GHZ_CTRL_LP_MODE(0x1);
		RFMC->RF2P4GHZ_CTRL |= RFMC_RF2P4GHZ_CTRL_LP_ENTER_MASK;

		/* Set MAIN_CORE and MAIN_WAKE power domain into sleep mode. */
		config.clock_mode  = kCMC_GateAllSystemClocksEnterLowPowerMode;
		config.main_domain = kCMC_SleepMode;
		config.wake_domain = kCMC_SleepMode;
		CMC_EnterLowPowerMode(MCXW7_CMC_ADDR, &config);

		break;
	case PM_STATE_STANDBY:
		/* Enable CORE VDD Voltage scaling. */
		SPC_EnableLowPowerModeCoreVDDInternalVoltageScaling(MCXW7_SPC_ADDR, true);

		/* Set NBU into Deep Sleep Mode */
		RFMC->RF2P4GHZ_CTRL = (RFMC->RF2P4GHZ_CTRL & (~RFMC_RF2P4GHZ_CTRL_LP_MODE_MASK)) |
				       RFMC_RF2P4GHZ_CTRL_LP_MODE(0x3);
		RFMC->RF2P4GHZ_CTRL |= RFMC_RF2P4GHZ_CTRL_LP_ENTER_MASK;

		/* Set MAIN_CORE and MAIN_WAKE power domain into Deep Sleep Mode. */
		config.clock_mode  = kCMC_GateAllSystemClocksEnterLowPowerMode;
		config.main_domain = kCMC_DeepSleepMode;
		config.wake_domain = kCMC_DeepSleepMode;

		CMC_EnterLowPowerMode(MCXW7_CMC_ADDR, &config);

		break;
	default:
		LOG_DBG("Unsupported power state %u", state);
		break;
	}
}

/* Handle SOC specific activity after Low Power Mode Exit */
__weak void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(state);
	ARG_UNUSED(substate_id);

	/* Clear PRIMASK */
	__enable_irq();

	if (SPC_CheckPowerDomainLowPowerRequest(MCXW7_SPC_ADDR, kSPC_PowerDomain0)) {
		SPC_ClearPowerDomainLowPowerRequestFlag(MCXW7_SPC_ADDR, kSPC_PowerDomain0);
	}
	if (SPC_CheckPowerDomainLowPowerRequest(MCXW7_SPC_ADDR, kSPC_PowerDomain1)) {
		SPC_ClearPowerDomainLowPowerRequestFlag(MCXW7_SPC_ADDR, kSPC_PowerDomain1);
	}
	if (SPC_CheckPowerDomainLowPowerRequest(MCXW7_SPC_ADDR, kSPC_PowerDomain2)) {
		RFMC->RF2P4GHZ_CTRL = (RFMC->RF2P4GHZ_CTRL & (~RFMC_RF2P4GHZ_CTRL_LP_MODE_MASK));
		RFMC->RF2P4GHZ_CTRL &= ~RFMC_RF2P4GHZ_CTRL_LP_ENTER_MASK;
		SPC_ClearPowerDomainLowPowerRequestFlag(MCXW7_SPC_ADDR, kSPC_PowerDomain2);
	}
	SPC_ClearLowPowerRequest(MCXW7_SPC_ADDR);
}

/*
 * In active mode, all HVDs/LVDs are disabled.
 * DCDC regulated to 1.8V, Core LDO regulated to 1.1V;
 * In low power modes, all HVDs/LVDs are disabled.
 * Bandgap is disabled, DCDC regulated to 1.25V, Core LDO regulated to 1.05V.
 */
__weak void set_spc_configuration(void)
{
	/* Disable LVDs and HVDs in Active mode. */
	SPC_EnableActiveModeCoreHighVoltageDetect(MCXW7_SPC_ADDR, false);
	SPC_EnableActiveModeCoreLowVoltageDetect(MCXW7_SPC_ADDR, false);
	SPC_EnableActiveModeSystemHighVoltageDetect(MCXW7_SPC_ADDR, false);
	SPC_EnableActiveModeSystemLowVoltageDetect(MCXW7_SPC_ADDR, false);
	SPC_EnableActiveModeIOHighVoltageDetect(MCXW7_SPC_ADDR, false);
	SPC_EnableActiveModeIOLowVoltageDetect(MCXW7_SPC_ADDR, false);
	while (SPC_GetBusyStatusFlag(MCXW7_SPC_ADDR)) {
	}

	spc_active_mode_regulators_config_t active_mode_regulator;

	active_mode_regulator.bandgapMode = kSPC_BandgapEnabledBufferDisabled;
	active_mode_regulator.lpBuff = false;
	/* DCDC regulate to 1.8V. */
	active_mode_regulator.DCDCOption.DCDCVoltage = kSPC_DCDC_SafeModeVoltage;
	active_mode_regulator.DCDCOption.DCDCDriveStrength = kSPC_DCDC_NormalDriveStrength;
	active_mode_regulator.SysLDOOption.SysLDOVoltage = kSPC_SysLDO_NormalVoltage;
	active_mode_regulator.SysLDOOption.SysLDODriveStrength = kSPC_SysLDO_NormalDriveStrength;
	/* Core LDO regulate to 1.1V. */
	active_mode_regulator.CoreLDOOption.CoreLDOVoltage = kSPC_CoreLDO_MidDriveVoltage;
#if defined(FSL_FEATURE_SPC_HAS_CORELDO_VDD_DS) && FSL_FEATURE_SPC_HAS_CORELDO_VDD_DS
	active_mode_regulator.CoreLDOOption.CoreLDODriveStrength = kSPC_CoreLDO_NormalDriveStrength;
#endif /* FSL_FEATURE_SPC_HAS_CORELDO_VDD_DS */

	SPC_SetActiveModeDCDCRegulatorConfig(MCXW7_SPC_ADDR, &active_mode_regulator.DCDCOption);

	while (SPC_GetBusyStatusFlag(MCXW7_SPC_ADDR)) {
	}

	SPC_SetActiveModeSystemLDORegulatorConfig(MCXW7_SPC_ADDR,
						  &active_mode_regulator.SysLDOOption);

	SPC_SetActiveModeBandgapModeConfig(MCXW7_SPC_ADDR, active_mode_regulator.bandgapMode);

	SPC_SetActiveModeCoreLDORegulatorConfig(MCXW7_SPC_ADDR,
						&active_mode_regulator.CoreLDOOption);

	SPC_EnableActiveModeCMPBandgapBuffer(MCXW7_SPC_ADDR, active_mode_regulator.lpBuff);

	spc_lowpower_mode_regulators_config_t low_power_regulator;

	low_power_regulator.lpIREF = false;
	low_power_regulator.bandgapMode = kSPC_BandgapDisabled;
	low_power_regulator.lpBuff = false;
	low_power_regulator.CoreIVS = false;
	low_power_regulator.DCDCOption.DCDCVoltage = kSPC_DCDC_LowUnderVoltage;
	low_power_regulator.DCDCOption.DCDCDriveStrength = kSPC_DCDC_LowDriveStrength;
	low_power_regulator.SysLDOOption.SysLDODriveStrength = kSPC_SysLDO_LowDriveStrength;
	low_power_regulator.CoreLDOOption.CoreLDOVoltage = kSPC_CoreLDO_MidDriveVoltage;
	low_power_regulator.CoreLDOOption.CoreLDODriveStrength = kSPC_CoreLDO_LowDriveStrength;

	SPC_SetLowPowerModeRegulatorsConfig(MCXW7_SPC_ADDR, &low_power_regulator);

	SPC_SetLowPowerWakeUpDelay(MCXW7_SPC_ADDR, 0xFFFFU);
}

void nxp_mcxw7x_power_init(void)
{
	set_spc_configuration();
	/* Enable LPTMR0 as wakeup source */
	NXP_ENABLE_WAKEUP_SIGNAL(WUU_WAKEUP_LPTMR_IDX);
}
