/*
 * Copyright (c) 2024 Semtech Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include "ral_sx126x_bsp.h"
#include "radio_utilities.h"
#include "sx126x.h"
#include "sx126x_hal_context.h"

void ral_sx126x_bsp_get_reg_mode(const void *context, sx126x_reg_mod_t *reg_mode)
{
	const struct device *dev = context;
	const struct sx126x_hal_context_cfg_t *config = dev->config;
	*reg_mode = config->reg_mode;
}

void ral_sx126x_bsp_get_rf_switch_cfg(const void *context, bool *dio2_is_set_as_rf_switch)
{
	const struct device *dev = context;
	const struct sx126x_hal_context_cfg_t *config = dev->config;
	*dio2_is_set_as_rf_switch = config->dio2_as_rf_switch;
}

void ral_sx126x_bsp_get_tx_cfg(const void *context,
			       const ral_sx126x_bsp_tx_cfg_input_params_t *input_params,
			       ral_sx126x_bsp_tx_cfg_output_params_t *output_params)
{
	/* get board tx power offset */
	int8_t board_tx_pwr_offset_db = radio_utilities_get_tx_power_offset(context);

	int16_t power = input_params->system_output_pwr_in_dbm + board_tx_pwr_offset_db;

	output_params->pa_ramp_time = SX126X_RAMP_40_US;
	/* reserved value, same for sx1261 sx1262 and sx1268 */
	output_params->pa_cfg.pa_lut = 0x01;

#if defined(SX1262) || defined(SX1268)
	/* Clamp power if needed */
	power = CLAMP(power, -9, 22);

	output_params->pa_cfg.device_sel = 0x00; /* select SX1262/SX1268 device */
	output_params->pa_cfg.hp_max = 0x07;     /* to achieve 22dBm */
	output_params->pa_cfg.pa_duty_cycle = 0x04;
	output_params->chip_output_pwr_in_dbm_configured = (int8_t)power;
	output_params->chip_output_pwr_in_dbm_expected = (int8_t)power;
#else
	/* Clamp power if needed */
	power = CLAMP(power, -17, 15);

	/* config pa according to power */
	if (power == 15) {
		output_params->pa_cfg.device_sel = 0x01; /* select SX1261 device */
		output_params->pa_cfg.hp_max = 0x00;     /* not used on sx1261 */
		output_params->pa_cfg.pa_duty_cycle = 0x06;
		output_params->chip_output_pwr_in_dbm_configured = 14;
		output_params->chip_output_pwr_in_dbm_expected = 15;
	} else if (power == 14) {
		output_params->pa_cfg.device_sel = 0x01; /* select SX1261 device */
		output_params->pa_cfg.hp_max = 0x00;     /* not used on sx1261 */
		output_params->pa_cfg.pa_duty_cycle = 0x04;
		output_params->chip_output_pwr_in_dbm_configured = 14;
		output_params->chip_output_pwr_in_dbm_expected = 14;
	} else {
		output_params->pa_cfg.device_sel = 0x01; /* select SX1261 device */
		output_params->pa_cfg.hp_max = 0x00;     /* not used on sx1261 */
		output_params->pa_cfg.pa_duty_cycle = 0x04;
		output_params->chip_output_pwr_in_dbm_configured = (int8_t)power;
		output_params->chip_output_pwr_in_dbm_expected = (int8_t)power;
	}

#endif
}

void ral_sx126x_bsp_get_xosc_cfg(const void *context, ral_xosc_cfg_t *xosc_cfg,
				 sx126x_tcxo_ctrl_voltages_t *supply_voltage,
				 uint32_t *startup_time_in_tick)
{
	const struct device *dev = context;
	const struct sx126x_hal_context_cfg_t *config = dev->config;
	const struct sx126x_hal_context_tcxo_cfg_t tcxo_cfg = config->tcxo_cfg;

	*xosc_cfg = tcxo_cfg.xosc_cfg;
	*supply_voltage = tcxo_cfg.voltage;
	*startup_time_in_tick = sx126x_convert_timeout_in_ms_to_rtc_step(tcxo_cfg.wakeup_time_ms);
}

void ral_sx126x_bsp_get_trim_cap(const void *context, uint8_t *trimming_cap_xta,
				 uint8_t *trimming_cap_xtb)
{
	const struct device *dev = context;
	const struct sx126x_hal_context_cfg_t *config = dev->config;

	if (config->capa_xta != 0xFF) {
		*trimming_cap_xta = config->capa_xta;
	}
	if (config->capa_xtb != 0xFF) {
		*trimming_cap_xtb = config->capa_xtb;
	}
}

void ral_sx126x_bsp_get_rx_boost_cfg(const void *context, bool *rx_boost_is_activated)
{
	const struct device *dev = (const struct device *)context;
	const struct sx126x_hal_context_cfg_t *config = dev->config;
	*rx_boost_is_activated = config->rx_boosted;
}

void ral_sx126x_bsp_get_ocp_value(const void *context, uint8_t *ocp_in_step_of_2_5_ma)
{
	/* Do nothing, let the driver choose the default values */
}

void ral_sx126x_bsp_get_lora_cad_det_peak(ral_lora_sf_t sf, ral_lora_bw_t bw,
					  ral_lora_cad_symbs_t nb_symbol,
					  uint8_t *in_out_cad_det_peak)
{
	/* Function used to fine tune the cad detection peak, update if needed */
}

void radio_utilities_set_tx_power_offset(const void *context, uint8_t tx_pwr_offset_db)
{
	const struct device *dev = (const struct device *)context;
	struct sx126x_hal_context_data_t *data = dev->data;

	data->tx_offset = tx_pwr_offset_db;
}

uint8_t radio_utilities_get_tx_power_offset(const void *context)
{
	const struct device *dev = (const struct device *)context;
	struct sx126x_hal_context_data_t *data = dev->data;

	return data->tx_offset;
}

/* --- EOF ------------------------------------------------------------------ */
