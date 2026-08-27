/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Active-mode DCDC regulator output voltage configuration for MCXW7x.
 *
 * The target voltage is taken from the SPC device tree node
 * (active-dcdc-output-microvolt). This is compiled unconditionally so the
 * configuration is applied regardless of whether CONFIG_PM is enabled. When
 * the property is absent the routine is a no-op and the DCDC output voltage is
 * left untouched.
 *
 * This logic is intentionally kept self-contained (save/disable LVDs and HVDs,
 * change the voltage, restore the detectors) so it can later be lifted into a
 * dedicated SPC driver without device tree or binding churn.
 */

#include <zephyr/devicetree.h>
#include <zephyr/sys/util.h>
#include <fsl_spc.h>

#define SPC_NODE DT_INST(0, nxp_spc)

#if DT_NODE_HAS_PROP(SPC_NODE, active_dcdc_output_microvolt)

#define SPC_BASE ((SPC_Type *)DT_REG_ADDR(SPC_NODE))
#define DCDC_OUTPUT_UV DT_PROP(SPC_NODE, active_dcdc_output_microvolt)

/*
 * MCXW7x active-mode DCDC output voltage levels, in microvolts.
 *
 * The DCDC voltage level register selects one of four nominal levels. The
 * 2.5V output is not a register level on its own: it is a custom mode that
 * selects the NormalVoltage (1.5V) level and additionally sets the
 * DCDC_CFG VOUT2P5_SEL bit. It is only useful to reach the 10 dBm BLE TX
 * power and is therefore enabled only for that case.
 */
#define DCDC_UV_LOW_UNDER 1250000 /* kSPC_DCDC_LowUnderVoltage */
#define DCDC_UV_MID       1350000 /* kSPC_DCDC_MidVoltage */
#define DCDC_UV_NORMAL    1500000 /* kSPC_DCDC_NormalVoltage */
#define DCDC_UV_SAFE_MODE 1800000 /* kSPC_DCDC_SafeModeVoltage */
#define DCDC_UV_2P5       2500000 /* kSPC_DCDC_NormalVoltage + VOUT2P5_SEL */

BUILD_ASSERT(DCDC_OUTPUT_UV == DCDC_UV_LOW_UNDER || DCDC_OUTPUT_UV == DCDC_UV_MID ||
		     DCDC_OUTPUT_UV == DCDC_UV_NORMAL || DCDC_OUTPUT_UV == DCDC_UV_SAFE_MODE ||
		     DCDC_OUTPUT_UV == DCDC_UV_2P5,
	     "active-dcdc-output-microvolt is not a level the MCXW7x DCDC can produce "
	     "(legal values: 1250000, 1350000, 1500000, 1800000, 2500000)");

/*
 * Cross-check the configured DCDC output voltage against the BLE radio max TX
 * power. Achieving 10 dBm requires the DCDC in its custom 2.5V mode; up to
 * 7 dBm requires at least 1.8V. The check only fires when both properties are
 * present.
 */
#define NBU_NODE DT_CHOSEN(zephyr_nbu)

#if DT_NODE_EXISTS(NBU_NODE) && DT_NODE_HAS_PROP(NBU_NODE, max_tx_power_dbm)
#define NBU_MAX_TX_POWER_DBM DT_PROP(NBU_NODE, max_tx_power_dbm)

BUILD_ASSERT(NBU_MAX_TX_POWER_DBM <= 7 || DCDC_OUTPUT_UV == DCDC_UV_2P5,
	     "max-tx-power-dbm above 7 dBm requires active-dcdc-output-microvolt = 2500000");
BUILD_ASSERT(NBU_MAX_TX_POWER_DBM <= 0 || DCDC_OUTPUT_UV >= DCDC_UV_SAFE_MODE,
	     "max-tx-power-dbm above 0 dBm requires active-dcdc-output-microvolt >= 1800000");
#endif /* nbu max-tx-power-dbm present */

static spc_dcdc_voltage_level_t mcxw7x_dcdc_level(void)
{
	switch (DCDC_OUTPUT_UV) {
	case DCDC_UV_LOW_UNDER:
		return kSPC_DCDC_LowUnderVoltage;
	case DCDC_UV_MID:
		return kSPC_DCDC_MidVoltage;
	case DCDC_UV_SAFE_MODE:
		return kSPC_DCDC_SafeModeVoltage;
	case DCDC_UV_NORMAL:
	case DCDC_UV_2P5:
	default:
		/* The 2.5V custom mode also uses the NormalVoltage level. */
		return kSPC_DCDC_NormalVoltage;
	}
}

void nxp_mcxw7x_dcdc_init(void)
{
	SPC_Type *base = SPC_BASE;
	spc_active_mode_dcdc_option_t dcdc_option;

	/*
	 * Save the active-mode voltage detector state, then disable all LVDs
	 * and HVDs so the DCDC output voltage can be changed safely. The HAL
	 * exposes the enable bits as a single masked word rather than per
	 * detector getters, so decode each bit from that status word.
	 */
	uint32_t vd_status = SPC_GetActiveModeVoltageDetectStatus(base);
	bool core_hvd = (vd_status & SPC_ACTIVE_CFG_CORE_HVDE_MASK) != 0U;
	bool core_lvd = (vd_status & SPC_ACTIVE_CFG_CORE_LVDE_MASK) != 0U;
	bool sys_hvd = (vd_status & SPC_ACTIVE_CFG_SYS_HVDE_MASK) != 0U;
	bool sys_lvd = (vd_status & SPC_ACTIVE_CFG_SYS_LVDE_MASK) != 0U;
	bool io_hvd = (vd_status & SPC_ACTIVE_CFG_IO_HVDE_MASK) != 0U;
	bool io_lvd = (vd_status & SPC_ACTIVE_CFG_IO_LVDE_MASK) != 0U;

	SPC_EnableActiveModeCoreHighVoltageDetect(base, false);
	SPC_EnableActiveModeCoreLowVoltageDetect(base, false);
	SPC_EnableActiveModeSystemHighVoltageDetect(base, false);
	SPC_EnableActiveModeSystemLowVoltageDetect(base, false);
	SPC_EnableActiveModeIOHighVoltageDetect(base, false);
	SPC_EnableActiveModeIOLowVoltageDetect(base, false);
	while (SPC_GetBusyStatusFlag(base)) {
	}

	/*
	 * VOUT2P5_SEL turns the NormalVoltage level into the custom 2.5V
	 * output. It is set only for the 2.5V mode and cleared otherwise.
	 */
	if (DCDC_OUTPUT_UV == DCDC_UV_2P5) {
		base->DCDC_CFG |= SPC_DCDC_CFG_VOUT2P5_SEL_MASK;
	} else {
		base->DCDC_CFG &= ~SPC_DCDC_CFG_VOUT2P5_SEL_MASK;
	}

	dcdc_option.DCDCVoltage = mcxw7x_dcdc_level();
	dcdc_option.DCDCDriveStrength = kSPC_DCDC_NormalDriveStrength;
	(void)SPC_SetActiveModeDCDCRegulatorConfig(base, &dcdc_option);
	while (SPC_GetBusyStatusFlag(base)) {
	}

	/* Restore the voltage detectors to their previous state. */
	SPC_EnableActiveModeCoreHighVoltageDetect(base, core_hvd);
	SPC_EnableActiveModeCoreLowVoltageDetect(base, core_lvd);
	SPC_EnableActiveModeSystemHighVoltageDetect(base, sys_hvd);
	SPC_EnableActiveModeSystemLowVoltageDetect(base, sys_lvd);
	SPC_EnableActiveModeIOHighVoltageDetect(base, io_hvd);
	SPC_EnableActiveModeIOLowVoltageDetect(base, io_lvd);
	while (SPC_GetBusyStatusFlag(base)) {
	}
}

#else /* active-dcdc-output-microvolt not present */

void nxp_mcxw7x_dcdc_init(void)
{
	/* No DCDC output voltage configured in device tree; leave it untouched. */
}

#endif /* DT_NODE_HAS_PROP(SPC_NODE, active_dcdc_output_microvolt) */
