/*
 *Copyright (c) 2024 Semtech Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "ral_lr11xx_bsp.h"
#include "lr11xx_hal_context.h"
#include "radio_utilities.h"
#include "lr11xx_radio.h"

LOG_MODULE_DECLARE(lr11xx_hal, CONFIG_LORA_BASICS_MODEM_DRIVERS_LOG_LEVEL);

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

void ral_lr11xx_bsp_get_lora_cad_det_peak(ral_lora_sf_t sf, ral_lora_bw_t bw,
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
#if defined(ADD_APP_GEOLOCATION)
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
