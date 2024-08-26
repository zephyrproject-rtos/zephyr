/*
 * Copyright (c) 2024 Semtech Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/lora_lbm/lora_lbm_transceiver.h>

#include <ral_lr11xx_bsp.h>
#include "lr11xx_hal_context.h"

typedef enum lr11xx_pa_type_s {
	LR11XX_WITH_LF_LP_PA,
	LR11XX_WITH_LF_HP_PA,
	LR11XX_WITH_LF_LP_HP_PA,
	LR11XX_WITH_HF_PA,
} lr11xx_pa_type_t;

#define LR11XX_PWR_VREG_VBAT_SWITCH 8

#define LR11XX_MIN_PWR_LP_LF -17
#define LR11XX_MAX_PWR_LP_LF 15

#define LR11XX_MIN_PWR_HP_LF -9
#define LR11XX_MAX_PWR_HP_LF 22

#define LR11XX_MIN_PWR_PA_HF -18
#define LR11XX_MAX_PWR_PA_HF 13

#define LR11XX_RSSI_CALIBRATION_TUNE_LENGTH 17

static void lr11xx_get_tx_cfg(const void *context, lr11xx_pa_type_t pa_type,
			      int8_t expected_output_pwr_in_dbm,
			      ral_lr11xx_bsp_tx_cfg_output_params_t *output_params)
{
	const struct device *dev = (const struct device *)context;
	const struct lr11xx_hal_context_cfg_t *config = dev->config;

	int8_t power = expected_output_pwr_in_dbm;

	/* Ramp time is the same for any config */
	output_params->pa_ramp_time = LR11XX_RADIO_RAMP_48_US;

	switch (pa_type) {
	case LR11XX_WITH_LF_LP_PA: {
		/* Check power boundaries for LP LF PA:
		 * The output power must be in range [ -17 , +15 ] dBm
		 */
		power = CLAMP(power, LR11XX_MIN_PWR_LP_LF, LR11XX_MAX_PWR_LP_LF);
		lr11xx_pa_pwr_cfg_t *pwr_cfg =
			&config->pa_lf_lp_cfg_table[power - LR11XX_MIN_PWR_LP_LF];

		output_params->pa_cfg.pa_sel = LR11XX_RADIO_PA_SEL_LP;
		output_params->pa_cfg.pa_reg_supply = LR11XX_RADIO_PA_REG_SUPPLY_VREG;
		output_params->pa_cfg.pa_duty_cycle = pwr_cfg->pa_duty_cycle;
		output_params->pa_cfg.pa_hp_sel = pwr_cfg->pa_hp_sel;
		output_params->chip_output_pwr_in_dbm_configured = pwr_cfg->power;
		output_params->chip_output_pwr_in_dbm_expected = power;
		break;
	}
	case LR11XX_WITH_LF_HP_PA: {
		/* Check power boundaries for HP LF PA:
		 * The output power must be in range [ -9 , +22 ] dBm
		 */
		power = CLAMP(power, LR11XX_MIN_PWR_HP_LF, LR11XX_MAX_PWR_HP_LF);
		lr11xx_pa_pwr_cfg_t *pwr_cfg =
			&config->pa_lf_hp_cfg_table[power - LR11XX_MIN_PWR_HP_LF];

		output_params->pa_cfg.pa_sel = LR11XX_RADIO_PA_SEL_HP;

		/* For powers below 8dBm use regulated supply for
		 * HP PA for a better efficiency.
		 */
		output_params->pa_cfg.pa_reg_supply = (power <= LR11XX_PWR_VREG_VBAT_SWITCH)
							      ? LR11XX_RADIO_PA_REG_SUPPLY_VREG
							      : LR11XX_RADIO_PA_REG_SUPPLY_VBAT;
		output_params->pa_cfg.pa_duty_cycle = pwr_cfg->pa_duty_cycle;
		output_params->pa_cfg.pa_hp_sel = pwr_cfg->pa_hp_sel;
		output_params->chip_output_pwr_in_dbm_configured = pwr_cfg->power;
		output_params->chip_output_pwr_in_dbm_expected = power;
		break;
	}
	case LR11XX_WITH_LF_LP_HP_PA: {
		/* Check power boundaries for LP/HP LF PA:
		 * The output power must be in range [ -17 , +22 ] dBm
		 */
		power = CLAMP(power, LR11XX_MIN_PWR_LP_LF, LR11XX_MAX_PWR_HP_LF);

		if (power <= LR11XX_MAX_PWR_LP_LF) {
			lr11xx_pa_pwr_cfg_t *pwr_cfg =
				&config->pa_lf_lp_cfg_table[power - LR11XX_MIN_PWR_LP_LF];

			output_params->chip_output_pwr_in_dbm_expected = power;
			output_params->pa_cfg.pa_sel = LR11XX_RADIO_PA_SEL_LP;
			output_params->pa_cfg.pa_reg_supply = LR11XX_RADIO_PA_REG_SUPPLY_VREG;
			output_params->pa_cfg.pa_duty_cycle = pwr_cfg->pa_duty_cycle;
			output_params->pa_cfg.pa_hp_sel = pwr_cfg->pa_hp_sel;
			output_params->chip_output_pwr_in_dbm_configured = pwr_cfg->power;
		} else {
			lr11xx_pa_pwr_cfg_t *pwr_cfg =
				&config->pa_lf_hp_cfg_table[power - LR11XX_MIN_PWR_HP_LF];

			output_params->chip_output_pwr_in_dbm_expected = power;
			output_params->pa_cfg.pa_sel = LR11XX_RADIO_PA_SEL_HP;
			output_params->pa_cfg.pa_reg_supply = LR11XX_RADIO_PA_REG_SUPPLY_VBAT;
			output_params->pa_cfg.pa_duty_cycle = pwr_cfg->pa_duty_cycle;
			output_params->pa_cfg.pa_hp_sel = pwr_cfg->pa_hp_sel;
			output_params->chip_output_pwr_in_dbm_configured = pwr_cfg->power;
		}
		break;
	}
	case LR11XX_WITH_HF_PA: {
		/* Check power boundaries for HF PA:
		 * The output power must be in range [ -18 , +13 ] dBm
		 */
		power = CLAMP(power, LR11XX_MIN_PWR_PA_HF, LR11XX_MAX_PWR_PA_HF);
		lr11xx_pa_pwr_cfg_t *pwr_cfg =
			&config->pa_hf_cfg_table[power - LR11XX_MIN_PWR_PA_HF];

		output_params->pa_cfg.pa_sel = LR11XX_RADIO_PA_SEL_HF;
		output_params->pa_cfg.pa_reg_supply = LR11XX_RADIO_PA_REG_SUPPLY_VREG;
		output_params->pa_cfg.pa_duty_cycle = pwr_cfg->pa_duty_cycle;
		output_params->pa_cfg.pa_hp_sel = pwr_cfg->pa_hp_sel;
		output_params->chip_output_pwr_in_dbm_configured = pwr_cfg->power;
		output_params->chip_output_pwr_in_dbm_expected = power;
		break;
	}
	}
}

void ral_lr11xx_bsp_get_tx_cfg(const void *context,
			       const ral_lr11xx_bsp_tx_cfg_input_params_t *input_params,
			       ral_lr11xx_bsp_tx_cfg_output_params_t *output_params)
{
	/* get board tx power offset */
	int8_t board_tx_pwr_offset_db = radio_utilities_get_tx_power_offset(context);

	int16_t power = input_params->system_output_pwr_in_dbm + board_tx_pwr_offset_db;

	lr11xx_pa_type_t pa_type;

	/* check frequency band first to choose LF of HF PA */
	if (input_params->freq_in_hz >= 2400000000) {
		pa_type = LR11XX_WITH_HF_PA;
	} else {
		/* Modem is acting in subgig band: use LP/HP PA
		 * (both LP and HP are connected on lr11xx evk board)
		 */
		pa_type = LR11XX_WITH_LF_LP_HP_PA;
	}

	/* call the configuration function */
	lr11xx_get_tx_cfg(context, pa_type, power, output_params);
}

void ral_lr11xx_bsp_get_rssi_calibration_table(
	const void *context, const uint32_t freq_in_hz,
	lr11xx_radio_rssi_calibration_table_t *rssi_calibration_table)
{
	const struct device *dev = (const struct device *)context;
	const struct lr11xx_hal_context_cfg_t *config = dev->config;

	if (freq_in_hz <= 600000000) {
		*rssi_calibration_table = config->rssi_calibration_table_below_600mhz;
	} else if ((600000000 <= freq_in_hz) && (freq_in_hz <= 2000000000)) {
		*rssi_calibration_table = config->rssi_calibration_table_from_600mhz_to_2ghz;
	} else {
		/* freq_in_hz > 2000000000 */
		*rssi_calibration_table = config->rssi_calibration_table_above_2ghz;
	}
}
