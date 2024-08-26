/*
 * Copyright (c) 2024 Semtech Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <zephyr/lora_lbm/lora_lbm_transceiver.h>

#include <ral_lr11xx_bsp.h>
#include <lr11xx_radio.h>
#include "lr11xx_hal_context.h"

LOG_MODULE_DECLARE(lora_lr11xx, CONFIG_LORA_BASICS_MODEM_DRIVERS_LOG_LEVEL);

void ral_lr11xx_bsp_get_rf_switch_cfg(const void *context,
				      lr11xx_system_rfswitch_cfg_t *rf_switch_cfg)
{
	const struct device *dev = (const struct device *)context;
	const struct lr11xx_hal_context_cfg_t *config = dev->config;

	rf_switch_cfg->enable = config->rf_switch_cfg.enable;
	rf_switch_cfg->standby = 0;
	rf_switch_cfg->rx = config->rf_switch_cfg.rx;
	rf_switch_cfg->tx = config->rf_switch_cfg.tx;
	rf_switch_cfg->tx_hp = config->rf_switch_cfg.tx_hp;
	rf_switch_cfg->tx_hf = config->rf_switch_cfg.tx_hf;
	rf_switch_cfg->gnss = config->rf_switch_cfg.gnss;
	rf_switch_cfg->wifi = config->rf_switch_cfg.wifi;
}

void ral_lr11xx_bsp_get_reg_mode(const void *context, lr11xx_system_reg_mode_t *reg_mode)
{
	const struct device *dev = (const struct device *)context;
	const struct lr11xx_hal_context_cfg_t *config = dev->config;
	*reg_mode = config->reg_mode;
}

void ral_lr11xx_bsp_get_xosc_cfg(const void *context, ral_xosc_cfg_t *xosc_cfg,
				 lr11xx_system_tcxo_supply_voltage_t *supply_voltage,
				 uint32_t *startup_time_in_tick)
{
	const struct device *transceiver = context;
	const struct lr11xx_hal_context_cfg_t *config = transceiver->config;
	const struct lr11xx_hal_context_tcxo_cfg_t tcxo_cfg = config->tcxo_cfg;

	*xosc_cfg = tcxo_cfg.xosc_cfg;
	*supply_voltage = tcxo_cfg.voltage;
	*startup_time_in_tick =
		lr11xx_radio_convert_time_in_ms_to_rtc_step(tcxo_cfg.wakeup_time_ms);
}

void ral_lr11xx_bsp_get_crc_state(const void *context, bool *crc_is_activated)
{
#if defined(CONFIG_LR11XX_USE_CRC_OVER_SPI)
	LOG_DBG("LR11XX CRC over spi is activated");
	*crc_is_activated = true;
#else
	*crc_is_activated = false;
#endif
}

void ral_lr11xx_bsp_get_lora_cad_det_peak(const void *context, ral_lora_sf_t sf, ral_lora_bw_t bw,
					  ral_lora_cad_symbs_t nb_symbol,
					  uint8_t *in_out_cad_det_peak)
{
	/* Function used to fine tune the cad detection peak, update if needed */
}

void ral_lr11xx_bsp_get_rx_boost_cfg(const void *context, bool *rx_boost_is_activated)
{
	const struct device *transceiver = context;
	const struct lr11xx_hal_context_cfg_t *config = transceiver->config;
	*rx_boost_is_activated = config->rx_boosted;
}

void ral_lr11xx_bsp_get_lfclk_cfg_in_sleep(const void *context, bool *lfclk_is_running)
{
#if defined(CONFIG_LORA_BASICS_MODEM_GEOLOCATION)
	*lfclk_is_running = true;
#else
	*lfclk_is_running = false;
#endif
}

void radio_utilities_set_tx_power_offset(const void *context, uint8_t tx_pwr_offset_db)
{
	const struct device *dev = (const struct device *)context;
	struct lr11xx_hal_context_data_t *data = dev->data;

	data->tx_offset = tx_pwr_offset_db;
}

uint8_t radio_utilities_get_tx_power_offset(const void *context)
{
	const struct device *dev = (const struct device *)context;
	struct lr11xx_hal_context_data_t *data = dev->data;

	return data->tx_offset;
}

/* TODO: check values */
#define LR11XX_GFSK_RX_CONSUMPTION_DCDC         5400
#define LR11XX_GFSK_RX_BOOSTED_CONSUMPTION_DCDC 7500

#define LR11XX_GFSK_RX_CONSUMPTION_LDO         5400
#define LR11XX_GFSK_RX_BOOSTED_CONSUMPTION_LDO 7500

#define LR11XX_LORA_RX_CONSUMPTION_DCDC         5700
#define LR11XX_LORA_RX_BOOSTED_CONSUMPTION_DCDC 7800

#define LR11XX_LORA_RX_CONSUMPTION_LDO         5700
#define LR11XX_LORA_RX_BOOSTED_CONSUMPTION_LDO 7800

#define LR11XX_LP_MIN_OUTPUT_POWER -17
#define LR11XX_LP_MAX_OUTPUT_POWER 15

#define LR11XX_HP_MIN_OUTPUT_POWER -9
#define LR11XX_HP_MAX_OUTPUT_POWER 22

#define LR11XX_HF_MIN_OUTPUT_POWER -18
#define LR11XX_HF_MAX_OUTPUT_POWER 13

#define LR11XX_LP_CONVERT_TABLE_INDEX_OFFSET 17
#define LR11XX_HP_CONVERT_TABLE_INDEX_OFFSET 9
#define LR11XX_HF_CONVERT_TABLE_INDEX_OFFSET 18

#define LR11XX_PWR_VREG_VBAT_SWITCH 8

static const uint32_t ral_lr11xx_convert_tx_dbm_to_ua_reg_mode_dcdc_lp_vreg[] = {
	10820, /* -17 dBm */
	10980, /* -16 dBm */
	11060, /* -15 dBm */
	11160, /* -14 dBm */
	11300, /* -13 dBm */
	11430, /* -12 dBm */
	11550, /* -11 dBm */
	11680, /* -10 dBm */
	11930, /*  -9 dBm */
	12170, /*  -8 dBm */
	12420, /* -7 dBm */
	12650, /* -6 dBm */
	12900, /* -5 dBm */
	13280, /* -4 dBm */
	13600, /* -3 dBm */
	14120, /* -2 dBm */
	14600, /* -1 dBm */
	15090, /*  0 dBm */
	15780, /*  1 dBm */
	16490, /*  2 dBm */
	17250, /*  3 dBm */
	17850, /*  4 dBm */
	18720, /*  5 dBm */
	19640, /*  6 dBm */
	20560, /*  7 dBm */
	21400, /*  8 dBm */
	22620, /*  9 dBm */
	23720, /* 10 dBm */
	25050, /* 11 dBm */
	26350, /* 12 dBm */
	27870, /* 13 dBm */
	28590, /* 14 dBm */
	37820, /* 15 dBm */
};

static const uint32_t ral_lr11xx_convert_tx_dbm_to_ua_reg_mode_ldo_lp_vreg[] = {
	14950, /* -17 dBm */
	15280, /* -16 dBm */
	15530, /* -15 dBm */
	15770, /* -14 dBm */
	16020, /* -13 dBm */
	16290, /* -12 dBm */
	16550, /* -11 dBm */
	16760, /* -10 dBm */
	17280, /*  -9 dBm */
	17770, /*  -8 dBm */
	18250, /* -7 dBm */
	18750, /* -6 dBm */
	19250, /* -5 dBm */
	19960, /* -4 dBm */
	20710, /* -3 dBm */
	21620, /* -2 dBm */
	22570, /* -1 dBm */
	23570, /*  0 dBm */
	24990, /*  1 dBm */
	26320, /*  2 dBm */
	27830, /*  3 dBm */
	29070, /*  4 dBm */
	30660, /*  5 dBm */
	32490, /*  6 dBm */
	34220, /*  7 dBm */
	35820, /*  8 dBm */
	38180, /*  9 dBm */
	40220, /* 10 dBm */
	42800, /* 11 dBm */
	45030, /* 12 dBm */
	47900, /* 13 dBm */
	51220, /* 14 dBm */
	66060, /* 15 dBm */
};

static const uint32_t ral_lr11xx_convert_tx_dbm_to_ua_reg_mode_dcdc_hp_vbat[] = {
	27750,  /*  -9 dBm */
	29100,  /*  -8 dBm */
	30320,  /*  -7 dBm */
	31650,  /*  -6 dBm */
	34250,  /*  -5 dBm */
	35550,  /*  -4 dBm */
	36770,  /*  -3 dBm */
	39250,  /*  -2 dBm */
	41480,  /*  -1 dBm */
	43820,  /*   0 dBm */
	46000,  /*  1 dBm */
	49020,  /*  2 dBm */
	50900,  /*  3 dBm */
	54200,  /*  4 dBm */
	56330,  /*  5 dBm */
	59050,  /*  6 dBm */
	62210,  /*  7 dBm */
	65270,  /*  8 dBm */
	68600,  /*  9 dBm */
	71920,  /* 10 dBm */
	75500,  /* 11 dBm */
	79500,  /* 12 dBm */
	84130,  /* 13 dBm */
	88470,  /* 14 dBm */
	92200,  /* 15 dBm */
	94340,  /* 16 dBm */
	96360,  /* 17 dBm */
	98970,  /* 18 dBm */
	102220, /* 19 dBm */
	106250, /* 20 dBm */
	111300, /* 21 dBm */
	113040, /* 22 dBm */
};

static const uint32_t ral_lr11xx_convert_tx_dbm_to_ua_reg_mode_ldo_hp_vbat[] = {
	31310,  /*  -9 dBm */
	32700,  /*  -8 dBm */
	33970,  /*  -7 dBm */
	35270,  /*  -6 dBm */
	37900,  /*  -5 dBm */
	39140,  /*  -4 dBm */
	40380,  /*  -3 dBm */
	42860,  /*  -2 dBm */
	45150,  /*  -1 dBm */
	47400,  /*   0 dBm */
	49600,  /*  1 dBm */
	52600,  /*  2 dBm */
	54460,  /*  3 dBm */
	57690,  /*  4 dBm */
	59840,  /*  5 dBm */
	62550,  /*  6 dBm */
	65750,  /*  7 dBm */
	68520,  /*  8 dBm */
	72130,  /*  9 dBm */
	75230,  /* 10 dBm */
	78600,  /* 11 dBm */
	82770,  /* 12 dBm */
	87450,  /* 13 dBm */
	91700,  /* 14 dBm */
	95330,  /* 15 dBm */
	97520,  /* 16 dBm */
	99520,  /* 17 dBm */
	102080, /* 18 dBm */
	105140, /* 19 dBm */
	109300, /* 20 dBm */
	114460, /* 21 dBm */
	116530, /* 22 dBm */
};

static const uint32_t ral_lr11xx_convert_tx_dbm_to_ua_reg_mode_dcdc_hf_vreg[] = {
	11800, /* -18 dBm */
	11800, /* -17 dBm */
	11800, /* -16 dBm */
	11900, /* -15 dBm */
	12020, /* -14 dBm */
	12120, /* -13 dBm */
	12230, /* -12 dBm */
	12390, /* -11 dBm */
	12540, /* -10 dBm */
	12740, /*  -9 dBm */
	12960, /* -8 dBm */
	13150, /* -7 dBm */
	13460, /* -6 dBm */
	13770, /* -5 dBm */
	14070, /* -4 dBm */
	14460, /* -3 dBm */
	15030, /* -2 dBm */
	15440, /* -1 dBm */
	16030, /*  0 dBm */
	16980, /*  1 dBm */
	17590, /*  2 dBm */
	18270, /*  3 dBm */
	19060, /*  4 dBm */
	19900, /*  5 dBm */
	20740, /*  6 dBm */
	21610, /*  7 dBm */
	22400, /*  8 dBm */
	23370, /*  9 dBm */
	24860, /* 10 dBm */
	26410, /* 11 dBm */
	26430, /* 12 dBm */
	27890, /* 13 dBm */
};

__weak ral_status_t ral_lr11xx_bsp_get_instantaneous_tx_power_consumption(
	const void *context, const ral_lr11xx_bsp_tx_cfg_output_params_t *tx_cfg,
	lr11xx_system_reg_mode_t radio_reg_mode, uint32_t *pwr_consumption_in_ua)
{
	if (tx_cfg->pa_cfg.pa_sel == LR11XX_RADIO_PA_SEL_LP) {
		if (tx_cfg->pa_cfg.pa_reg_supply == LR11XX_RADIO_PA_REG_SUPPLY_VREG) {
			uint8_t index = 0;

			if (tx_cfg->chip_output_pwr_in_dbm_expected > LR11XX_LP_MAX_OUTPUT_POWER) {
				index = LR11XX_LP_MAX_OUTPUT_POWER +
					LR11XX_LP_CONVERT_TABLE_INDEX_OFFSET;
			} else if (tx_cfg->chip_output_pwr_in_dbm_expected <
				   LR11XX_LP_MIN_OUTPUT_POWER) {
				index = LR11XX_LP_MIN_OUTPUT_POWER +
					LR11XX_LP_CONVERT_TABLE_INDEX_OFFSET;
			} else {
				index = tx_cfg->chip_output_pwr_in_dbm_expected +
					LR11XX_LP_CONVERT_TABLE_INDEX_OFFSET;
			}

			if (radio_reg_mode == LR11XX_SYSTEM_REG_MODE_DCDC) {
				*pwr_consumption_in_ua =
					ral_lr11xx_convert_tx_dbm_to_ua_reg_mode_dcdc_lp_vreg
						[index];
			} else {
				*pwr_consumption_in_ua =
					ral_lr11xx_convert_tx_dbm_to_ua_reg_mode_ldo_lp_vreg[index];
			}
		} else {
			return RAL_STATUS_UNSUPPORTED_FEATURE;
		}
	} else if (tx_cfg->pa_cfg.pa_sel == LR11XX_RADIO_PA_SEL_HP) {
		if (tx_cfg->pa_cfg.pa_reg_supply == LR11XX_RADIO_PA_REG_SUPPLY_VBAT) {
			uint8_t index = 0;

			if (tx_cfg->chip_output_pwr_in_dbm_expected > LR11XX_HP_MAX_OUTPUT_POWER) {
				index = LR11XX_HP_MAX_OUTPUT_POWER +
					LR11XX_HP_CONVERT_TABLE_INDEX_OFFSET;
			} else if (tx_cfg->chip_output_pwr_in_dbm_expected <
				   LR11XX_HP_MIN_OUTPUT_POWER) {
				index = LR11XX_HP_MIN_OUTPUT_POWER +
					LR11XX_HP_CONVERT_TABLE_INDEX_OFFSET;
			} else {
				index = tx_cfg->chip_output_pwr_in_dbm_expected +
					LR11XX_HP_CONVERT_TABLE_INDEX_OFFSET;
			}

			if (radio_reg_mode == LR11XX_SYSTEM_REG_MODE_DCDC) {
				*pwr_consumption_in_ua =
					ral_lr11xx_convert_tx_dbm_to_ua_reg_mode_dcdc_hp_vbat
						[index];
			} else {
				*pwr_consumption_in_ua =
					ral_lr11xx_convert_tx_dbm_to_ua_reg_mode_ldo_hp_vbat[index];
			}
		} else {
			return RAL_STATUS_UNSUPPORTED_FEATURE;
		}
	} else if (tx_cfg->pa_cfg.pa_sel == LR11XX_RADIO_PA_SEL_HF) {
		if (tx_cfg->pa_cfg.pa_reg_supply == LR11XX_RADIO_PA_REG_SUPPLY_VREG) {
			uint8_t index = 0;

			if (tx_cfg->chip_output_pwr_in_dbm_expected > LR11XX_HF_MAX_OUTPUT_POWER) {
				index = LR11XX_HF_MAX_OUTPUT_POWER +
					LR11XX_HF_CONVERT_TABLE_INDEX_OFFSET;
			} else if (tx_cfg->chip_output_pwr_in_dbm_expected <
				   LR11XX_HF_MIN_OUTPUT_POWER) {
				index = LR11XX_HF_MIN_OUTPUT_POWER +
					LR11XX_HF_CONVERT_TABLE_INDEX_OFFSET;
			} else {
				index = tx_cfg->chip_output_pwr_in_dbm_expected +
					LR11XX_HF_CONVERT_TABLE_INDEX_OFFSET;
			}

			if (radio_reg_mode == LR11XX_SYSTEM_REG_MODE_DCDC) {
				*pwr_consumption_in_ua =
					ral_lr11xx_convert_tx_dbm_to_ua_reg_mode_dcdc_hf_vreg
						[index];
			} else {
				return RAL_STATUS_UNSUPPORTED_FEATURE;
			}
		} else {
			return RAL_STATUS_UNSUPPORTED_FEATURE;
		}
	} else {
		return RAL_STATUS_UNKNOWN_VALUE;
	}

	return RAL_STATUS_OK;
}

__weak ral_status_t ral_lr11xx_bsp_get_instantaneous_gfsk_rx_power_consumption(
	const void *context, lr11xx_system_reg_mode_t radio_reg_mode, bool rx_boosted,
	uint32_t *pwr_consumption_in_ua)
{
	if (radio_reg_mode == LR11XX_SYSTEM_REG_MODE_DCDC) {
		*pwr_consumption_in_ua = rx_boosted ? LR11XX_GFSK_RX_BOOSTED_CONSUMPTION_DCDC
						    : LR11XX_GFSK_RX_CONSUMPTION_DCDC;
	} else {
		/* TODO: find the good values */
		*pwr_consumption_in_ua = rx_boosted ? LR11XX_GFSK_RX_BOOSTED_CONSUMPTION_LDO
						    : LR11XX_GFSK_RX_CONSUMPTION_LDO;
	}

	return RAL_STATUS_OK;
}

__weak ral_status_t ral_lr11xx_bsp_get_instantaneous_lora_rx_power_consumption(
	const void *context, lr11xx_system_reg_mode_t radio_reg_mode, bool rx_boosted,
	uint32_t *pwr_consumption_in_ua)
{
	if (radio_reg_mode == LR11XX_SYSTEM_REG_MODE_DCDC) {
		*pwr_consumption_in_ua = rx_boosted ? LR11XX_LORA_RX_BOOSTED_CONSUMPTION_DCDC
						    : LR11XX_LORA_RX_CONSUMPTION_DCDC;
	} else {
		/* TODO: find the good values */
		*pwr_consumption_in_ua = rx_boosted ? LR11XX_LORA_RX_BOOSTED_CONSUMPTION_LDO
						    : LR11XX_LORA_RX_CONSUMPTION_LDO;
	}

	return RAL_STATUS_OK;
}
