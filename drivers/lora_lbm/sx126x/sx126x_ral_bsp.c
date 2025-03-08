/*
 * Copyright (c) 2024 Semtech Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include <zephyr/lora_lbm/lora_lbm_transceiver.h>

#include <sx126x.h>
#include <ral_sx126x_bsp.h>
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

void ral_sx126x_bsp_get_lora_cad_det_peak(const void *context, ral_lora_sf_t sf, ral_lora_bw_t bw,
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

#define SX126X_LP_MIN_OUTPUT_POWER -17
#define SX126X_LP_MAX_OUTPUT_POWER 15

#define SX126X_HP_MIN_OUTPUT_POWER -9
#define SX126X_HP_MAX_OUTPUT_POWER 22

#define SX126X_LP_CONVERT_TABLE_INDEX_OFFSET 17
#define SX126X_HP_CONVERT_TABLE_INDEX_OFFSET 9

/* TODO: check values */
#define SX126X_GFSK_RX_CONSUMPTION_DCDC         4200
#define SX126X_GFSK_RX_BOOSTED_CONSUMPTION_DCDC 4800

#define SX126X_GFSK_RX_CONSUMPTION_LDO         8000
#define SX126X_GFSK_RX_BOOSTED_CONSUMPTION_LDO 9300

#define SX126X_LORA_RX_CONSUMPTION_DCDC         4600
#define SX126X_LORA_RX_BOOSTED_CONSUMPTION_DCDC 5300

#define SX126X_LORA_RX_CONSUMPTION_LDO         8880
#define SX126X_LORA_RX_BOOSTED_CONSUMPTION_LDO 10100

static const uint32_t ral_sx126x_convert_tx_dbm_to_ua_reg_mode_dcdc_lp[] = {
	5200,  /* -17 dBm */
	5400,  /* -16 dBm */
	5600,  /* -15 dBm */
	5700,  /* -14 dBm */
	5800,  /* -13 dBm */
	6000,  /* -12 dBm */
	6100,  /* -11 dBm */
	6200,  /* -10 dBm */
	6500,  /*  -9 dBm */
	6800,  /*  -8 dBm */
	7000,  /*  -7 dBm */
	7300,  /*  -6 dBm */
	7500,  /*  -5 dBm */
	7900,  /*  -4 dBm */
	8300,  /*  -3 dBm */
	8800,  /*  -2 dBm */
	9300,  /*  -1 dBm */
	9800,  /*   0 dBm */
	10600, /*   1 dBm */
	11400, /*   2 dBm */
	12200, /*   3 dBm */
	12900, /*   4 dBm */
	13800, /*   5 dBm */
	14700, /*   6 dBm */
	15700, /*   7 dBm */
	16600, /*   8 dBm */
	17900, /*   9 dBm */
	18500, /*  10 dBm */
	20500, /*  11 dBm */
	21900, /*  12 dBm */
	23500, /*  13 dBm */
	25500, /*  14 dBm */
	32500, /*  15 dBm */
};

static const uint32_t ral_sx126x_convert_tx_dbm_to_ua_reg_mode_ldo_lp[] = {
	9800,  /* -17 dBm */
	10300, /* -16 dBm */
	10500, /* -15 dBm */
	10800, /* -14 dBm */
	11100, /* -13 dBm */
	11300, /* -12 dBm */
	11600, /* -11 dBm */
	11900, /* -10 dBm */
	12400, /*  -9 dBm */
	12900, /*  -8 dBm */
	13400, /*  -7 dBm */
	13900, /*  -6 dBm */
	14500, /*  -5 dBm */
	15300, /*  -4 dBm */
	16000, /*  -3 dBm */
	17000, /*  -2 dBm */
	18000, /*  -1 dBm */
	19000, /*   0 dBm */
	20600, /*   1 dBm */
	22000, /*   2 dBm */
	23500, /*   3 dBm */
	24900, /*   4 dBm */
	26600, /*   5 dBm */
	28400, /*   6 dBm */
	30200, /*   7 dBm */
	32000, /*   8 dBm */
	34300, /*   9 dBm */
	36600, /*  10 dBm */
	39200, /*  11 dBm */
	41700, /*  12 dBm */
	44700, /*  13 dBm */
	48200, /*  14 dBm */
	52200, /*  15 dBm */
};

static const uint32_t ral_sx126x_convert_tx_dbm_to_ua_reg_mode_dcdc_hp[] = {
	24000,  /*  -9 dBm */
	25400,  /*  -8 dBm */
	26700,  /*  -7 dBm */
	28000,  /*  -6 dBm */
	30600,  /*  -5 dBm */
	31900,  /*  -4 dBm */
	33200,  /*  -3 dBm */
	35700,  /*  -2 dBm */
	38200,  /*  -1 dBm */
	40600,  /*   0 dBm */
	42900,  /*   1 dBm */
	46200,  /*   2 dBm */
	48200,  /*   3 dBm */
	51800,  /*   4 dBm */
	54100,  /*   5 dBm */
	57000,  /*   6 dBm */
	60300,  /*   7 dBm */
	63500,  /*   8 dBm */
	67100,  /*   9 dBm */
	70500,  /*  10 dBm */
	74200,  /*  11 dBm */
	78400,  /*  12 dBm */
	83500,  /*  13 dBm */
	89300,  /*  14 dBm */
	92400,  /*  15 dBm */
	94500,  /*  16 dBm */
	95400,  /*  17 dBm */
	97500,  /*  18 dBm */
	100100, /*  19 dBm */
	103800, /*  20 dBm */
	109100, /*  21 dBm */
	117900, /*  22 dBm */
};

static const uint32_t ral_sx126x_convert_tx_dbm_to_ua_reg_mode_ldo_hp[] = {
	25900,  /*  -9 dBm */
	27400,  /*  -8 dBm */
	28700,  /*  -7 dBm */
	30000,  /*  -6 dBm */
	32600,  /*  -5 dBm */
	33900,  /*  -4 dBm */
	35200,  /*  -3 dBm */
	37700,  /*  -2 dBm */
	40100,  /*  -1 dBm */
	42600,  /*   0 dBm */
	44900,  /*   1 dBm */
	48200,  /*   2 dBm */
	50200,  /*   3 dBm */
	53800,  /*   4 dBm */
	56100,  /*   5 dBm */
	59000,  /*   6 dBm */
	62300,  /*   7 dBm */
	65500,  /*   8 dBm */
	69000,  /*   9 dBm */
	72500,  /*  10 dBm */
	76200,  /*  11 dBm */
	80400,  /*  12 dBm */
	85400,  /*  13 dBm */
	90200,  /*  14 dBm */
	94400,  /*  15 dBm */
	96500,  /*  16 dBm */
	97700,  /*  17 dBm */
	99500,  /*  18 dBm */
	102100, /*  19 dBm */
	105800, /*  20 dBm */
	111000, /*  21 dBm */
	119800, /*  22 dBm */
};

__weak ral_status_t ral_sx126x_bsp_get_instantaneous_tx_power_consumption(
	const void *context, const ral_sx126x_bsp_tx_cfg_output_params_t *tx_cfg_output_params,
	sx126x_reg_mod_t radio_reg_mode, uint32_t *pwr_consumption_in_ua)
{
	/* SX1261 */
	if (tx_cfg_output_params->pa_cfg.device_sel == 0x01) {
		uint8_t index = 0;

		if (tx_cfg_output_params->chip_output_pwr_in_dbm_expected >
		    SX126X_LP_MAX_OUTPUT_POWER) {
			index = SX126X_LP_MAX_OUTPUT_POWER + SX126X_LP_CONVERT_TABLE_INDEX_OFFSET;
		} else if (tx_cfg_output_params->chip_output_pwr_in_dbm_expected <
			   SX126X_LP_MIN_OUTPUT_POWER) {
			index = SX126X_LP_MIN_OUTPUT_POWER + SX126X_LP_CONVERT_TABLE_INDEX_OFFSET;
		} else {
			index = tx_cfg_output_params->chip_output_pwr_in_dbm_expected +
				SX126X_LP_CONVERT_TABLE_INDEX_OFFSET;
		}

		if (radio_reg_mode == SX126X_REG_MODE_DCDC) {
			*pwr_consumption_in_ua =
				ral_sx126x_convert_tx_dbm_to_ua_reg_mode_dcdc_lp[index];
		} else {
			*pwr_consumption_in_ua =
				ral_sx126x_convert_tx_dbm_to_ua_reg_mode_ldo_lp[index];
		}
	}
	/* SX1262 */
	else if (tx_cfg_output_params->pa_cfg.device_sel == 0x00) {
		uint8_t index = 0;

		if (tx_cfg_output_params->chip_output_pwr_in_dbm_expected >
		    SX126X_HP_MAX_OUTPUT_POWER) {
			index = SX126X_HP_MAX_OUTPUT_POWER + SX126X_HP_CONVERT_TABLE_INDEX_OFFSET;
		} else if (tx_cfg_output_params->chip_output_pwr_in_dbm_expected <
			   SX126X_HP_MIN_OUTPUT_POWER) {
			index = SX126X_HP_MIN_OUTPUT_POWER + SX126X_HP_CONVERT_TABLE_INDEX_OFFSET;
		} else {
			index = tx_cfg_output_params->chip_output_pwr_in_dbm_expected +
				SX126X_HP_CONVERT_TABLE_INDEX_OFFSET;
		}

		if (radio_reg_mode == SX126X_REG_MODE_DCDC) {
			*pwr_consumption_in_ua =
				ral_sx126x_convert_tx_dbm_to_ua_reg_mode_dcdc_hp[index];
		} else {
			*pwr_consumption_in_ua =
				ral_sx126x_convert_tx_dbm_to_ua_reg_mode_ldo_hp[index];
		}
	} else {
		return RAL_STATUS_UNKNOWN_VALUE;
	}

	return RAL_STATUS_OK;
}

__weak ral_status_t ral_sx126x_bsp_get_instantaneous_gfsk_rx_power_consumption(
	const void *context, sx126x_reg_mod_t radio_reg_mode, bool rx_boosted,
	uint32_t *pwr_consumption_in_ua)
{
	/* TODO: find the br/bw dependent values */

	if (radio_reg_mode == SX126X_REG_MODE_DCDC) {
		*pwr_consumption_in_ua = rx_boosted ? SX126X_GFSK_RX_BOOSTED_CONSUMPTION_DCDC
						    : SX126X_GFSK_RX_CONSUMPTION_DCDC;
	} else {
		*pwr_consumption_in_ua = rx_boosted ? SX126X_GFSK_RX_BOOSTED_CONSUMPTION_LDO
						    : SX126X_GFSK_RX_CONSUMPTION_LDO;
	}

	return RAL_STATUS_OK;
}

__weak ral_status_t ral_sx126x_bsp_get_instantaneous_lora_rx_power_consumption(
	const void *context, sx126x_reg_mod_t radio_reg_mode, bool rx_boosted,
	uint32_t *pwr_consumption_in_ua)
{
	/* TODO: find the bw dependent values */

	if (radio_reg_mode == SX126X_REG_MODE_DCDC) {
		*pwr_consumption_in_ua = rx_boosted ? SX126X_LORA_RX_BOOSTED_CONSUMPTION_DCDC
						    : SX126X_LORA_RX_CONSUMPTION_DCDC;
	} else {
		*pwr_consumption_in_ua = rx_boosted ? SX126X_LORA_RX_BOOSTED_CONSUMPTION_LDO
						    : SX126X_LORA_RX_CONSUMPTION_LDO;
	}

	return RAL_STATUS_OK;
}
