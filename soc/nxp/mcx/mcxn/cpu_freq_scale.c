/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <fsl_spc.h>
#include <soc.h>
#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <errno.h>
#include <zephyr/cpu_freq/cpu_freq.h>
#include <zephyr/cpu_freq/pstate.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/timer/system_timer.h>

LOG_MODULE_REGISTER(mcxn_cpu_freq, CONFIG_CPU_FREQ_LOG_LEVEL);

static int mcxn_set_cpu_frequency_to_150mhz(void);
static int mcxn_set_cpu_frequency_to_48mhz(void);
static int mcxn_set_cpu_frequency_to_12mhz(void);

static ALWAYS_INLINE void mcxn_update_system_timer_hz(uint32_t new_hz)
{
	SystemCoreClock = new_hz;

#if defined(CONFIG_CORTEX_M_SYSTICK)
#if !defined(CONFIG_SYSTEM_CLOCK_HW_CYCLES_PER_SEC_RUNTIME_UPDATE)
#error "MCXN CPU_FREQ with SysTick requires CONFIG_SYSTEM_CLOCK_HW_CYCLES_PER_SEC_RUNTIME_UPDATE"
#endif
	z_sys_clock_hw_cycles_per_sec_update((uint32_t)SystemCoreClock);
#endif
}

/* MCXN frequency levels. */
typedef enum {
	MCXN_FREQ_LOW  = 0U,
	MCXN_FREQ_MED  = 1U,
	MCXN_FREQ_HIGH = 2U,
} mcxn_freq_level_t;

/* Map level index to CPU frequency and apply function. */
struct mcxn_level_entry {
	uint32_t hz;
	int (*apply)(void);
};

static const struct mcxn_level_entry mcxn_levels[] = {
	[MCXN_FREQ_LOW]  = { .hz = 12000000U,  .apply = mcxn_set_cpu_frequency_to_12mhz },
	[MCXN_FREQ_MED]  = { .hz = 48000000U,  .apply = mcxn_set_cpu_frequency_to_48mhz },
	[MCXN_FREQ_HIGH] = { .hz = 150000000U, .apply = mcxn_set_cpu_frequency_to_150mhz },
};

/* MCXN SoC specific P-state configuration. Each P-state node in
 * the devicetree provides a CPU frequency mode (low/medium/high),
 * which we map to a concrete CPU frequency at runtime.
 */
struct mcxn_pstate_cfg {
	const struct mcxn_level_entry *level;
};

/* Map devicetree performance-states into pstate instances visible to the
 * CPUFreq policy. The generic pstate struct carries a vendor-specific
 * configuration pointer which we populate with mcxn_pstate_cfg.
 */
#define DEFINE_MCXN_PSTATE(node_id)								\
	BUILD_ASSERT(DT_ENUM_IDX(node_id, cpu_frequency_level) < ARRAY_SIZE(mcxn_levels),	\
			 "cpu_frequency_level index out of range for mcxn_levels array");	\
	static const struct mcxn_pstate_cfg _CONCAT(mcxn_pstate_cfg_, node_id) = {		\
			.level = &mcxn_levels[DT_ENUM_IDX(node_id, cpu_frequency_level)],	\
	};											\
	PSTATE_DT_DEFINE(node_id, &_CONCAT(mcxn_pstate_cfg_, node_id))

DT_FOREACH_CHILD_STATUS_OKAY(DT_PATH(performance_states), DEFINE_MCXN_PSTATE)

int cpu_freq_pstate_set(const struct pstate *state)
{
	if (state == NULL) {
		LOG_ERR("P-state is NULL");
		return -EINVAL;
	}

	const struct mcxn_pstate_cfg *cfg = (const struct mcxn_pstate_cfg *)state->config;

	if (cfg == NULL) {
		LOG_ERR("P-state vendor config is NULL");
		return -EINVAL;
	}

	if ((cfg->level == NULL) || (cfg->level->apply == NULL)) {
		LOG_ERR("Invalid mcxn level entry in P-state config");
		return -ENOTSUP;
	}

	const struct mcxn_level_entry *entry = cfg->level;

	LOG_DBG("Requesting CPU frequency change: level_idx=%u -> %u Hz (threshold=%u%%)",
			(unsigned int)(entry - mcxn_levels), entry->hz, state->load_threshold);

	return entry->apply();
}

static int mcxn_set_cpu_frequency_to_150mhz(void)
{
	/* Switch to FRO 12M first to ensure we can change the clock setting */
	CLOCK_AttachClk(kFRO12M_to_MAIN_CLK);

	/* Set the DCDC VDD regulator to 1.2 V voltage level */
	spc_active_mode_dcdc_option_t dcdc_cfg = {
		.DCDCVoltage	 = kSPC_DCDC_OverdriveVoltage,
		.DCDCDriveStrength = kSPC_DCDC_NormalDriveStrength,
	};
	SPC_SetActiveModeDCDCRegulatorConfig(SPC0, &dcdc_cfg);

	/* Set the LDO_CORE VDD regulator to 1.2 V voltage level */
	spc_active_mode_core_ldo_option_t ldo_cfg = {
		.CoreLDOVoltage	 = kSPC_CoreLDO_OverDriveVoltage,
		.CoreLDODriveStrength = kSPC_CoreLDO_NormalDriveStrength,
	};
	SPC_SetActiveModeCoreLDORegulatorConfig(SPC0, &ldo_cfg);

	/* Configure Flash wait-states to support 1.2V voltage level and 15MHz frequency */;
	FMU0->FCTRL = (FMU0->FCTRL & ~((uint32_t)FMU_FCTRL_RWSC_MASK)) | (FMU_FCTRL_RWSC(0x3U));

	/* Specifies the 1.2V operating voltage for the SRAM's read/write timing margin */
	spc_sram_voltage_config_t sram_cfg = {
		.operateVoltage	 = kSPC_sramOperateAt1P2V,
		.requestVoltageUpdate = true,
	};
	SPC_SetSRAMOperateVoltage(SPC0, &sram_cfg);

	/*!< Set up PLL0 to 150MHz */
	const pll_setup_t pll0_cfg = {
		.pllctrl = SCG_APLLCTRL_SOURCE(1U) | SCG_APLLCTRL_SELI(27U) |
			SCG_APLLCTRL_SELP(13U),
		.pllndiv = SCG_APLLNDIV_NDIV(8U),
		.pllpdiv = SCG_APLLPDIV_PDIV(1U),
		.pllmdiv = SCG_APLLMDIV_MDIV(50U),
		.pllRate = 150000000U
	};
	CLOCK_SetPLL0Freq(&pll0_cfg);
	CLOCK_SetPll0MonitorMode(kSCG_Pll0MonitorDisable);

	/* Attach PLL0 (150MHz) to MainClock, set clock divider to 1 */
	CLOCK_AttachClk(kPLL0_to_MAIN_CLK);
	CLOCK_SetClkDiv(kCLOCK_DivAhbClk, 1U);
	mcxn_update_system_timer_hz(150000000U);

	return 0;
}

static int mcxn_set_cpu_frequency_to_48mhz(void)
{
	/* Switch to FRO 12M first to ensure we can change the clock setting */
	CLOCK_AttachClk(kFRO12M_to_MAIN_CLK);

	/* Set the LDO_CORE VDD regulator to 1.0 V voltage level */
	spc_active_mode_core_ldo_option_t ldo_cfg = {
		.CoreLDOVoltage	 = kSPC_CoreLDO_MidDriveVoltage,
		.CoreLDODriveStrength = kSPC_CoreLDO_NormalDriveStrength,
	};
	SPC_SetActiveModeCoreLDORegulatorConfig(SPC0, &ldo_cfg);

	/* Set the DCDC VDD regulator to 1.0 V voltage level */
	spc_active_mode_dcdc_option_t dcdc_cfg = {
		.DCDCVoltage	 = kSPC_DCDC_MidVoltage,
		.DCDCDriveStrength = kSPC_DCDC_NormalDriveStrength,
	};
	SPC_SetActiveModeDCDCRegulatorConfig(SPC0, &dcdc_cfg);

	/* Configure Flash wait-states to support 1V voltage level and 48MHz frequency */;
	FMU0->FCTRL = (FMU0->FCTRL & ~((uint32_t)FMU_FCTRL_RWSC_MASK)) | (FMU_FCTRL_RWSC(0x1U));

	/* Specifies the 1V operating voltage for the SRAM's read/write timing margin */
	spc_sram_voltage_config_t sram_cfg = {
		.operateVoltage	 = kSPC_sramOperateAt1P0V,
		.requestVoltageUpdate = true,
	};
	SPC_SetSRAMOperateVoltage(SPC0, &sram_cfg);

	/* Attach FROHF (48MHz) to MainClock, set clock divider to 1 */
	CLOCK_SetupFROHFClocking(48000000U);
	CLOCK_AttachClk(kFRO_HF_to_MAIN_CLK);
	CLOCK_SetClkDiv(kCLOCK_DivAhbClk, 1U);
	mcxn_update_system_timer_hz(48000000U);

	return 0;
}

static int mcxn_set_cpu_frequency_to_12mhz(void)
{
	/* Attach FRO12M to MainClock, set clock divider to 1 */
	CLOCK_AttachClk(kFRO12M_to_MAIN_CLK);
	CLOCK_SetClkDiv(kCLOCK_DivAhbClk, 1U);
	mcxn_update_system_timer_hz(12000000U);

	/* Set the LDO_CORE VDD regulator to 1.0 V voltage level */
	spc_active_mode_core_ldo_option_t ldo_cfg = {
		.CoreLDOVoltage = kSPC_CoreLDO_MidDriveVoltage,
		.CoreLDODriveStrength = kSPC_CoreLDO_NormalDriveStrength,
	};
	SPC_SetActiveModeCoreLDORegulatorConfig(SPC0, &ldo_cfg);

	/* Set the DCDC VDD regulator to 1.0 V voltage level */
	spc_active_mode_dcdc_option_t dcdc_cfg = {
		.DCDCVoltage = kSPC_DCDC_MidVoltage,
		.DCDCDriveStrength = kSPC_DCDC_NormalDriveStrength,
	};
	SPC_SetActiveModeDCDCRegulatorConfig(SPC0, &dcdc_cfg);

	/* Configure Flash wait-states to support 1V voltage level and 12MHz frequency */;
	FMU0->FCTRL = (FMU0->FCTRL & ~((uint32_t)FMU_FCTRL_RWSC_MASK)) | (FMU_FCTRL_RWSC(0x0U));

	/* Specifies the 1V operating voltage for the SRAM's read/write timing margin */
	spc_sram_voltage_config_t sram_cfg = {
		.operateVoltage	 = kSPC_sramOperateAt1P0V,
		.requestVoltageUpdate = true,
	};
	SPC_SetSRAMOperateVoltage(SPC0, &sram_cfg);

	return 0;
}
