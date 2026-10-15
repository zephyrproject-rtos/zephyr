/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/pm/pm.h>
#include <zephyr/arch/arch_interface.h>
#include <fsl_cmc.h>
#include <fsl_spc.h>
#include <fsl_vbat.h>

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(soc, CONFIG_SOC_LOG_LEVEL);

#define MCXW7_CMC_ADDR (CMC_Type *)DT_REG_ADDR(DT_INST(0, nxp_cmc))
#define MCXW7_VBAT_ADDR (VBAT_Type *)DT_REG_ADDR(DT_INST(0, nxp_vbat))
#define MCXW7_SPC_ADDR (SPC_Type *)DT_REG_ADDR(DT_INST(0, nxp_spc))

#define MCXW7_LPWKUP_DELAY_10MHz (0xAAU)

#ifdef CONFIG_SOC_SERIES_MCXW7XX_SHUTDOWN_UNUSED_RAM

#define CMC_BANKS_NODE DT_NODELABEL(cmc)

BUILD_ASSERT(DT_NODE_HAS_PROP(CMC_BANKS_NODE, sram_banks),
	     "cmc node must define the sram-banks property");
BUILD_ASSERT((DT_PROP_LEN(CMC_BANKS_NODE, sram_banks) % 3) == 0,
	     "sram-banks must contain three cells (<start size disable-bit>) per bank");

/* Three cells per bank: <start-address size cmc-disable-bit>. */
#define MCXW7_BANK_CELLS 3U
#define MCXW7_BANK_NUM   (DT_PROP_LEN(CMC_BANKS_NODE, sram_banks) / MCXW7_BANK_CELLS)

/*
 * A memory region the linked image relies on: keep the SRAM banks it overlaps
 * powered across all modes. Built from devicetree, so it does not depend on any
 * region-scoped linker symbol (_image_ram_end only marks the end of use inside
 * zephyr,sram, not of every distinct linker region).
 */
struct mcxw7x_keep_region {
	uintptr_t start;
	size_t size;
};

/* Emit a {start, size} entry for a devicetree node's reg. */
#define MCXW7_KEEP_ENTRY(node_id) {DT_REG_ADDR(node_id), DT_REG_SIZE(node_id)},

/*
 * Regions to keep powered: the chosen RAM node (whole node, not up to
 * _image_ram_end), the chosen flash node when it is linked on-chip, and every
 * status-okay zephyr,memory-region (this is how an application keeps a distinct
 * region alive, e.g. an ECC region holding critical data). Banks overlapping
 * none of these are powered off.
 */
static const struct mcxw7x_keep_region mcxw7x_keep_regions[] = {
	MCXW7_KEEP_ENTRY(DT_CHOSEN(zephyr_sram))
#if DT_HAS_CHOSEN(zephyr_flash)
	MCXW7_KEEP_ENTRY(DT_CHOSEN(zephyr_flash))
#endif
	DT_FOREACH_STATUS_OKAY(zephyr_memory_region, MCXW7_KEEP_ENTRY)
};

/*
 * Compute the CMC bank mask of SRAM power partitions that no kept region uses.
 * Pure function of the devicetree memory map (bank geometry from the cmc
 * sram-banks list, kept regions from chosen/memory-region nodes); evaluated
 * once at init. A bank is kept when it overlaps any kept region and powered off
 * otherwise, so distinct regions such as a non-ECC/ECC split are respected.
 */
static uint32_t mcxw7x_compute_unused_ram_mask(void)
{
	/*
	 * Materialize the flat device tree bank list into a C array so it can be
	 * indexed at run time; DT_PROP_BY_IDX() only accepts compile-time indices.
	 */
	static const uint32_t sram_banks[] = DT_PROP(CMC_BANKS_NODE, sram_banks);
	uint32_t mask = 0U;

	for (size_t i = 0U; i < MCXW7_BANK_NUM; i++) {
		uintptr_t bank_start = (uintptr_t)sram_banks[(i * MCXW7_BANK_CELLS) + 0U];
		size_t bank_size = (size_t)sram_banks[(i * MCXW7_BANK_CELLS) + 1U];
		uint32_t bit = sram_banks[(i * MCXW7_BANK_CELLS) + 2U];
		uintptr_t bank_end = bank_start + bank_size;
		bool keep = false;

		for (size_t r = 0U; r < ARRAY_SIZE(mcxw7x_keep_regions); r++) {
			uintptr_t reg_start = mcxw7x_keep_regions[r].start;
			uintptr_t reg_end = reg_start + mcxw7x_keep_regions[r].size;

			/* Half-open overlap: bank_start < reg_end && reg_start < bank_end. */
			if ((bank_start < reg_end) && (reg_start < bank_end)) {
				keep = true;
				break;
			}
		}

		if (!keep) {
			mask |= BIT(bit);
		}
	}

	return mask;
}

#endif /* CONFIG_SOC_SERIES_MCXW7XX_SHUTDOWN_UNUSED_RAM */

/*
 * 1. Set power mode protection
 * 2. Disable low power mode debug
 * 3. Enable Flash Doze mode.
 */
static void set_cmc_configuration(void)
{
	CMC_SetPowerModeProtection(MCXW7_CMC_ADDR, kCMC_AllowAllLowPowerModes);
	CMC_LockPowerModeProtectionSetting(MCXW7_CMC_ADDR);
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
	cmc_power_domain_config_t config;
	unsigned int key;

	set_cmc_configuration();
	deinit_vbat();

	switch (state) {
	case PM_STATE_SUSPEND_TO_IDLE:
		/* Set MAIN_CORE and MAIN_WAKE power domain into sleep mode. */
		config.clock_mode  = kCMC_GateAllSystemClocksEnterLowPowerMode;
		config.main_domain = kCMC_SleepMode;
		config.wake_domain = kCMC_SleepMode;
		key = arch_pm_state_set_prepare();
		CMC_EnterLowPowerMode(MCXW7_CMC_ADDR, &config);
		arch_pm_state_set_finish(key);

		break;
	case PM_STATE_STANDBY:
		/* Enable CORE VDD Voltage scaling. */
		SPC_EnableLowPowerModeCoreVDDInternalVoltageScaling(MCXW7_SPC_ADDR, true);

		/* Set MAIN_CORE and MAIN_WAKE power domain into Deep Sleep Mode. */
		config.clock_mode  = kCMC_GateAllSystemClocksEnterLowPowerMode;
		config.main_domain = kCMC_DeepSleepMode;
		config.wake_domain = kCMC_DeepSleepMode;

		key = arch_pm_state_set_prepare();
		CMC_EnterLowPowerMode(MCXW7_CMC_ADDR, &config);
		arch_pm_state_set_finish(key);

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

	if (SPC_CheckPowerDomainLowPowerRequest(MCXW7_SPC_ADDR, kSPC_PowerDomain0)) {
		SPC_ClearPowerDomainLowPowerRequestFlag(MCXW7_SPC_ADDR, kSPC_PowerDomain0);
	}
	if (SPC_CheckPowerDomainLowPowerRequest(MCXW7_SPC_ADDR, kSPC_PowerDomain1)) {
		SPC_ClearPowerDomainLowPowerRequestFlag(MCXW7_SPC_ADDR, kSPC_PowerDomain1);
	}
	if (SPC_CheckPowerDomainLowPowerRequest(MCXW7_SPC_ADDR, kSPC_PowerDomain2)) {
		SPC_ClearPowerDomainLowPowerRequestFlag(MCXW7_SPC_ADDR, kSPC_PowerDomain2);
	}
	SPC_ClearLowPowerRequest(MCXW7_SPC_ADDR);
}

/*
 * In active mode, all HVDs/LVDs are disabled, Core LDO regulated to 1.1V.
 * The active-mode DCDC output voltage is configured separately and earlier by
 * nxp_mcxw7x_dcdc_init() (see soc_dcdc.c), driven by the SPC device tree node,
 * so it is intentionally not touched here.
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
	active_mode_regulator.SysLDOOption.SysLDOVoltage = kSPC_SysLDO_NormalVoltage;
	active_mode_regulator.SysLDOOption.SysLDODriveStrength = kSPC_SysLDO_NormalDriveStrength;
	/* Core LDO regulate to 1.1V. */
	active_mode_regulator.CoreLDOOption.CoreLDOVoltage = kSPC_CoreLDO_MidDriveVoltage;
#if defined(FSL_FEATURE_SPC_HAS_CORELDO_VDD_DS) && FSL_FEATURE_SPC_HAS_CORELDO_VDD_DS
	active_mode_regulator.CoreLDOOption.CoreLDODriveStrength = kSPC_CoreLDO_NormalDriveStrength;
#endif /* FSL_FEATURE_SPC_HAS_CORELDO_VDD_DS */

	/*
	 * Active-mode DCDC voltage is owned by nxp_mcxw7x_dcdc_init(); do not
	 * reconfigure it here so the device tree configured value is preserved.
	 */

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

	SPC_SetLowPowerWakeUpDelay(MCXW7_SPC_ADDR, MCXW7_LPWKUP_DELAY_10MHz);
}

void nxp_mcxw7x_power_init(void)
{
	set_spc_configuration();

#ifdef CONFIG_SOC_SERIES_MCXW7XX_SHUTDOWN_UNUSED_RAM
	uint32_t unused_ram_mask = mcxw7x_compute_unused_ram_mask();

	if (unused_ram_mask != 0U) {
		/* Power off unused SRAM banks in all modes to reduce leakage. */
		CMC_PowerOffSRAMAllMode(MCXW7_CMC_ADDR, unused_ram_mask);
	}
#endif /* CONFIG_SOC_SERIES_MCXW7XX_SHUTDOWN_UNUSED_RAM */
}
