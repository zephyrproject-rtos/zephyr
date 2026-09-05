/*
 * Copyright (c) 2025 Embeint Inc
 * Copyright (c) 2026 Jakub Rzeszutko <jakub.rzeszutko@verkada.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/lora.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/pm/device.h>

#include "ral.h"

#include "ralf_sx126x.h"
#include "ral_sx126x_bsp.h"
#include "sx126x_hal.h"
#include "sx126x.h"

#include "lbm_sx126x_common.h"

#define SX1261_TX_PWR_MAX 15
#define SX1261_TX_PWR_MIN -17
#define SX1262_TX_PWR_MAX 22
#define SX1262_TX_PWR_MIN -9


/* Copied from LBM sx126x.c */
enum sx126x_commands {
	/* Operational Modes Functions */
	SX126X_SET_SLEEP = 0x84,
	SX126X_SET_STANDBY = 0x80,
	SX126X_SET_FS = 0xC1,
	SX126X_SET_TX = 0x83,
	SX126X_SET_RX = 0x82,
	SX126X_SET_STOP_TIMER_ON_PREAMBLE = 0x9F,
	SX126X_SET_RX_DUTY_CYCLE = 0x94,
	SX126X_SET_CAD = 0xC5,
	SX126X_SET_TX_CONTINUOUS_WAVE = 0xD1,
	SX126X_SET_TX_INFINITE_PREAMBLE = 0xD2,
	SX126X_SET_REGULATOR_MODE = 0x96,
	SX126X_CALIBRATE = 0x89,
	SX126X_CALIBRATE_IMAGE = 0x98,
	SX126X_SET_PA_CFG = 0x95,
	SX126X_SET_RX_TX_FALLBACK_MODE = 0x93,
	/* Registers and buffer Access */
	SX126X_WRITE_REGISTER = 0x0D,
	SX126X_READ_REGISTER = 0x1D,
	SX126X_WRITE_BUFFER = 0x0E,
	SX126X_READ_BUFFER = 0x1E,
	/* DIO and IRQ Control Functions */
	SX126X_SET_DIO_IRQ_PARAMS = 0x08,
	SX126X_GET_IRQ_STATUS = 0x12,
	SX126X_CLR_IRQ_STATUS = 0x02,
	SX126X_SET_DIO2_AS_RF_SWITCH_CTRL = 0x9D,
	SX126X_SET_DIO3_AS_TCXO_CTRL = 0x97,
	/* RF Modulation and Packet-Related Functions */
	SX126X_SET_RF_FREQUENCY = 0x86,
	SX126X_SET_PKT_TYPE = 0x8A,
	SX126X_GET_PKT_TYPE = 0x11,
	SX126X_SET_TX_PARAMS = 0x8E,
	SX126X_SET_MODULATION_PARAMS = 0x8B,
	SX126X_SET_PKT_PARAMS = 0x8C,
	SX126X_SET_CAD_PARAMS = 0x88,
	SX126X_SET_BUFFER_BASE_ADDRESS = 0x8F,
	SX126X_SET_LORA_SYMB_NUM_TIMEOUT = 0xA0,
	/* Communication Status Information */
	SX126X_GET_STATUS = 0xC0,
	SX126X_GET_RX_BUFFER_STATUS = 0x13,
	SX126X_GET_PKT_STATUS = 0x14,
	SX126X_GET_RSSI_INST = 0x15,
	SX126X_GET_STATS = 0x10,
	SX126X_RESET_STATS = 0x00,
	/* Miscellaneous */
	SX126X_GET_DEVICE_ERRORS = 0x17,
	SX126X_CLR_DEVICE_ERRORS = 0x07,
};

LOG_MODULE_DECLARE(lbm_driver, CONFIG_LORA_LOG_LEVEL);

static int sx126x_wait_device_ready(const struct device *dev, k_timeout_t timeout)
{
	k_timepoint_t expiry = sys_timepoint_calc(timeout);

	do {
		if (!lbm_sx126x_is_busy(dev)) {
			return 0;
		}
		k_sleep(K_MSEC(1));
	} while (!sys_timepoint_expired(expiry));
	return -EAGAIN;
}

static int sx126x_ensure_device_ready(const struct device *dev, k_timeout_t timeout)
{
	const struct lbm_sx126x_config *config = dev->config;
	struct lbm_sx126x_data *data = dev->data;
	uint8_t get_status_cmd[2] = {SX126X_GET_STATUS, 0xFF};
	const struct spi_buf tx_bufs[] = {
		{
			.buf = (void *)get_status_cmd,
			.len = sizeof(get_status_cmd),
		},
	};
	const struct spi_buf_set tx_buf_set = {tx_bufs, .count = ARRAY_SIZE(tx_bufs)};
	int ret;

	if (data->asleep) {
		LOG_DBG("SLEEP -> ACTIVE");
		/* Re-enable the DIO1 interrupt */
		lbm_driver_dio1_irq_enable(dev);
		/* DO NOT USE sx126x_get_status as this will result in recursion */
		ret = spi_write_dt(&config->spi, &tx_buf_set);
		if (ret) {
			return ret;
		}
	}
	ret = sx126x_wait_device_ready(dev, timeout);
	data->asleep = false;
	return ret;
}

sx126x_hal_status_t sx126x_hal_write(const void *context, const uint8_t *command,
				     const uint16_t command_length, const uint8_t *data,
				     const uint16_t data_length)
{
	const struct device *dev = context;
	const struct lbm_sx126x_config *config = dev->config;
	struct lbm_sx126x_data *dev_data = dev->data;
	const struct spi_buf tx_bufs[] = {
		{
			.buf = (void *)command,
			.len = command_length,
		},
		{
			.buf = (void *)data,
			.len = data_length,
		},
	};
	const struct spi_buf_set tx_buf_set = {tx_bufs, .count = ARRAY_SIZE(tx_bufs)};
	int ret;

	LOG_DBG("CMD[0]=0x%02x CMD_LEN=%d DATA_LEN=%d", command[0], command_length, data_length);

	ret = sx126x_ensure_device_ready(dev, K_SECONDS(1));
	if (ret) {
		return SX126X_HAL_STATUS_ERROR;
	}

	ret = spi_write_dt(&config->spi, &tx_buf_set);
	if (ret) {
		return SX126X_HAL_STATUS_ERROR;
	}

	if (command[0] == SX126X_SET_SLEEP) {
		LOG_DBG("ACTIVE -> SLEEP");
		/* Disable the DIO1 interrupt to save power */
		lbm_driver_dio1_irq_disable(dev);
		dev_data->asleep = true;
		/* Wait for sleep to take effect */
		k_sleep(K_MSEC(1));
	}

	return SX126X_HAL_STATUS_OK;
}

sx126x_hal_status_t sx126x_hal_read(const void *context, const uint8_t *command,
				    const uint16_t command_length, uint8_t *data,
				    const uint16_t data_length)
{
	const struct device *dev = context;
	const struct lbm_sx126x_config *config = dev->config;
	const struct spi_buf tx_bufs[] = {
		{
			.buf = (uint8_t *)command,
			.len = command_length,
		},
		{
			.buf = NULL,
			.len = data_length,
		},
	};
	const struct spi_buf rx_bufs[] = {
		{
			.buf = NULL,
			.len = command_length,
		},
		{
			.buf = data,
			.len = data_length,
		},
	};
	const struct spi_buf_set tx_buf_set = {.buffers = tx_bufs, .count = ARRAY_SIZE(tx_bufs)};
	const struct spi_buf_set rx_buf_set = {.buffers = rx_bufs, .count = ARRAY_SIZE(rx_bufs)};
	int ret;

	LOG_DBG("CMD[0]=0x%02x DATA_LEN=%d", command[0], data_length);

	ret = sx126x_ensure_device_ready(dev, K_SECONDS(1));
	if (ret) {
		return SX126X_HAL_STATUS_ERROR;
	}

	ret = spi_transceive_dt(&config->spi, &tx_buf_set, &rx_buf_set);
	if (ret) {
		return SX126X_HAL_STATUS_ERROR;
	}
	return SX126X_HAL_STATUS_OK;
}

sx126x_hal_status_t sx126x_hal_reset(const void *context)
{
	const struct device *dev = context;

	LOG_DBG("");

	lbm_sx126x_reset(dev);

	return SX126X_HAL_STATUS_OK;
}

sx126x_hal_status_t sx126x_hal_wakeup(const void *context)
{
	const struct device *dev = context;
	int ret;

	LOG_DBG("");

	ret = sx126x_ensure_device_ready(dev, K_SECONDS(1));
	if (ret) {
		return SX126X_HAL_STATUS_ERROR;
	}
	return SX126X_HAL_STATUS_OK;
}

void ral_sx126x_bsp_get_reg_mode(const void *context, sx126x_reg_mod_t *reg_mode)
{
	const struct device *dev = context;
	const struct lbm_sx126x_config *config = dev->config;

	if (config->regulator_ldo) {
		*reg_mode = SX126X_REG_MODE_LDO;
	} else {
		*reg_mode = SX126X_REG_MODE_DCDC;
	}
}

void ral_sx126x_bsp_get_rf_switch_cfg(const void *context, bool *dio2_is_set_as_rf_switch)
{
	const struct device *dev = context;
	const struct lbm_sx126x_config *config = dev->config;

	*dio2_is_set_as_rf_switch = config->dio2_rf_switch;
}

void ral_sx126x_bsp_get_tx_cfg(const void *context,
			       const ral_sx126x_bsp_tx_cfg_input_params_t *input_params,
			       ral_sx126x_bsp_tx_cfg_output_params_t *output_params)

{
	const struct device *dev = context;
	const struct lbm_sx126x_config *config = dev->config;
	int16_t power = input_params->system_output_pwr_in_dbm;

	output_params->pa_ramp_time = SX126X_RAMP_40_US;
	output_params->pa_cfg.pa_lut = 0x01;

	if (config->variant == VARIANT_STM32WL) {
		/* Values from ST AN5457, chapter 5.1.2. */
		if (config->pa_output == SX126X_PA_OUTPUT_RFO_LP) {
			int8_t max_power = config->rfo_lp_max_power;

			power = MIN(power, max_power);
			output_params->pa_cfg.device_sel = 0x01;
			output_params->pa_cfg.hp_max = 0x00;
			if (max_power == 15) {
				output_params->pa_cfg.pa_duty_cycle = 0x06;
				power = 14 - (max_power - power);
			} else if (max_power == 10) {
				output_params->pa_cfg.pa_duty_cycle = 0x01;
				power = 13 - (max_power - power);
			} else {
				output_params->pa_cfg.pa_duty_cycle = 0x04;
				power = 14 - (max_power - power);
			}
			power = MAX(power, SX1261_TX_PWR_MIN);
		} else {
			int8_t max_power = config->rfo_hp_max_power;

			power = MIN(power, max_power);
			output_params->pa_cfg.device_sel = 0x00;
			if (max_power == 20) {
				output_params->pa_cfg.pa_duty_cycle = 0x03;
				output_params->pa_cfg.hp_max = 0x05;
				power = 22 - (max_power - power);
			} else if (max_power == 17) {
				output_params->pa_cfg.pa_duty_cycle = 0x02;
				output_params->pa_cfg.hp_max = 0x03;
				power = 22 - (max_power - power);
			} else if (max_power == 14) {
				output_params->pa_cfg.pa_duty_cycle = 0x02;
				output_params->pa_cfg.hp_max = 0x02;
				power = 14 - (max_power - power);
			} else {
				output_params->pa_cfg.pa_duty_cycle = 0x04;
				output_params->pa_cfg.hp_max = 0x07;
				power = 22 - (max_power - power);
			}
			power = MAX(power, SX1262_TX_PWR_MIN);
		}
		output_params->chip_output_pwr_in_dbm_configured = power;
		output_params->chip_output_pwr_in_dbm_expected = power;
	} else if (config->variant == VARIANT_SX1261) {
		power = CLAMP(power, SX1261_TX_PWR_MIN, SX1261_TX_PWR_MAX);
		output_params->pa_cfg.device_sel = 0x01;
		output_params->chip_output_pwr_in_dbm_configured = power;
		output_params->chip_output_pwr_in_dbm_expected = power;
		if (power == 15) {
			output_params->chip_output_pwr_in_dbm_configured = 14;
			output_params->pa_cfg.pa_duty_cycle = 0x06;
		} else {
			output_params->pa_cfg.pa_duty_cycle = 0x04;
		}
	} else {
		power = CLAMP(power, SX1262_TX_PWR_MIN, SX1262_TX_PWR_MAX);
		output_params->pa_cfg.device_sel = 0x00;
		output_params->pa_cfg.hp_max = 0x07;
		output_params->pa_cfg.pa_duty_cycle = 0x04;
		output_params->chip_output_pwr_in_dbm_configured = power;
		output_params->chip_output_pwr_in_dbm_expected = power;
	}
}

void ral_sx126x_bsp_get_xosc_cfg(const void *context, ral_xosc_cfg_t *xosc_cfg,
				 sx126x_tcxo_ctrl_voltages_t *supply_voltage,
				 uint32_t *startup_time_in_tick)
{
	const struct device *dev = context;
	const struct lbm_sx126x_config *config = dev->config;

	if (config->dio3_tcxo_voltage == UINT8_MAX) {
		*xosc_cfg = RAL_XOSC_CFG_XTAL;
	} else {
		*xosc_cfg = RAL_XOSC_CFG_TCXO_RADIO_CTRL;
		*supply_voltage = config->dio3_tcxo_voltage;
		/* From datasheet: 1 tick = 15.625 us = 65536 Hz */
		*startup_time_in_tick = z_tmcvt_32(config->dio3_tcxo_startup_delay_ms, Z_HZ_ms,
						   65536, true, true, false);
	}
}

void ral_sx126x_bsp_get_trim_cap(const void *context, uint8_t *trimming_cap_xta,
				 uint8_t *trimming_cap_xtb)
{
	/* Do nothing, let the driver choose the default values */
}

void ral_sx126x_bsp_get_rx_boost_cfg(const void *context, bool *rx_boost_is_activated)
{
	const struct device *dev = context;
	const struct lbm_sx126x_config *config = dev->config;

	*rx_boost_is_activated = config->rx_boosted;
}

void ral_sx126x_bsp_get_ocp_value(const void *context, uint8_t *ocp_in_step_of_2_5_ma)
{
	const struct device *dev = context;
	const struct lbm_sx126x_config *config = dev->config;

	if (config->variant != VARIANT_STM32WL) {
		/* Let the radio driver choose the default value */
		return;
	}

	/* 60 mA on the low-power output, 140 mA on the high-power one. */
	*ocp_in_step_of_2_5_ma = config->pa_output == SX126X_PA_OUTPUT_RFO_LP ? 0x18 : 0x38;
}

void ral_sx126x_bsp_get_lora_cad_det_peak(const void *context, ral_lora_sf_t sf, ral_lora_bw_t bw,
					  ral_lora_cad_symbs_t nb_symbol,
					  uint8_t *in_out_cad_det_peak)
{
	/* The DetPeak value set in the sx126x Radio Abstraction Layer is too close to the
	 * sensitivity for BW500 and SF>=9
	 */
	if (bw >= RAL_LORA_BW_500_KHZ) {
		if (sf >= RAL_LORA_SF9) {
			*in_out_cad_det_peak += 11;
		}
	}
}

ral_status_t ral_sx126x_bsp_get_instantaneous_tx_power_consumption(
	const void *context, const ral_sx126x_bsp_tx_cfg_output_params_t *tx_cfg_output_params,
	sx126x_reg_mod_t radio_reg_mode, uint32_t *pwr_consumption_in_ua)
{
	/* Not yet implemented, not relevant for LoRa */
	return RAL_STATUS_UNSUPPORTED_FEATURE;
}

ral_status_t ral_sx126x_bsp_get_instantaneous_gfsk_rx_power_consumption(
	const void *context, sx126x_reg_mod_t radio_reg_mode, bool rx_boosted,
	uint32_t *pwr_consumption_in_ua)
{
	/* Not yet implemented, not relevant for LoRa */
	return RAL_STATUS_UNSUPPORTED_FEATURE;
}

ral_status_t ral_sx126x_bsp_get_instantaneous_lora_rx_power_consumption(
	const void *context, sx126x_reg_mod_t radio_reg_mode, bool rx_boosted,
	uint32_t *pwr_consumption_in_ua)
{
	/* Not yet implemented, not relevant for LoRa */
	return RAL_STATUS_UNSUPPORTED_FEATURE;
}

void lbm_driver_antenna_configure(const struct device *dev, enum lbm_modem_mode mode)
{
	const struct lbm_sx126x_config *config = dev->config;

	/* Antenna / RF switch control */
	switch (mode) {
	case MODE_SLEEP:
		lbm_optional_gpio_set_dt(&config->ant_enable, 0);
		lbm_optional_gpio_set_dt(&config->rx_enable, 0);
		lbm_optional_gpio_set_dt(&config->tx_enable, 0);
		break;
	case MODE_TX:
	case MODE_CW:
		lbm_optional_gpio_set_dt(&config->rx_enable, 0);
		lbm_optional_gpio_set_dt(&config->tx_enable, 1);
		lbm_optional_gpio_set_dt(&config->ant_enable, 1);
		break;
	case MODE_RX:
	case MODE_RX_ASYNC:
	case MODE_CAD:
		lbm_optional_gpio_set_dt(&config->tx_enable, 0);
		lbm_optional_gpio_set_dt(&config->rx_enable, 1);
		lbm_optional_gpio_set_dt(&config->ant_enable, 1);
		break;
	}
}

int lbm_driver_radio_init(const struct device *dev)
{
	const struct lbm_sx126x_config *config = dev->config;
	ral_status_t status;
	int ret;

	/* Reset chip */
	status = ral_reset(&config->lbm_common.ralf.ral);
	if (status != RAL_STATUS_OK) {
		LOG_ERR("Reset failure (%d)", status);
		return -EIO;
	}

	/* Wait for chip to be ready */
	ret = sx126x_ensure_device_ready(dev, K_MSEC(100));
	if (ret) {
		LOG_ERR("Failed to return to ready after reset");
		return -EIO;
	}

	/* Common structure init */
	ret = lbm_lora_common_init(dev);
	if (ret < 0) {
		return ret;
	}

	/* Configure and enable interrupts */
	ret = lbm_sx126x_variant_init(dev);
	if (ret < 0) {
		return ret;
	}

	LOG_INF("Radio initialized");
	return 0;
}

static int sx126x_init(const struct device *dev)
{
	const struct lbm_sx126x_config *config = dev->config;
	struct lbm_sx126x_data *data = dev->data;
	int ret;

	/* Validate hardware is ready */
	if (!spi_is_ready_dt(&config->spi)) {
		LOG_ERR("SPI bus %s not ready", config->spi.bus->name);
		return -ENODEV;
	}

	/* Which of reset, busy and DIO1 are pins at all depends on the part, so
	 * the board glue claims those. The RF switch is wired the same way
	 * whether or not the radio is on-die, so it is handled here.
	 */
	ret = lbm_sx126x_pins_init(dev);
	if (ret < 0) {
		return ret;
	}

	if (config->ant_enable.port) {
		gpio_pin_configure_dt(&config->ant_enable, GPIO_OUTPUT_INACTIVE);
	}
	if (config->tx_enable.port) {
		gpio_pin_configure_dt(&config->tx_enable, GPIO_OUTPUT_INACTIVE);
	}
	if (config->rx_enable.port) {
		gpio_pin_configure_dt(&config->rx_enable, GPIO_OUTPUT_INACTIVE);
	}

	/* Initialize data structure */
	data->dev = dev;

	if (!IS_ENABLED(CONFIG_LORA_BASICS_MODEM_DEFERRED_INIT)) {
		return lbm_driver_radio_init(dev);
	}

	LOG_INF("Device initialized (radio initialization deferred)");
	return 0;
}

#define SX126X_SPI_OPERATION (SPI_WORD_SET(8) | SPI_OP_MODE_CONTROLLER | SPI_TRANSFER_MSB)

/* The properties a radio has wherever the core sits. What is left out is what
 * only a discrete part reaches over pins: reset, busy and DIO1.
 */
#define SX126X_CONFIG_COMMON(node_id)                                                              \
	.lbm_common.ralf = RALF_SX126X_INSTANTIATE(DEVICE_DT_GET(node_id)),                        \
	.lbm_common.force_ldro = DT_PROP(node_id, force_ldro),                                     \
	.spi = SPI_DT_SPEC_GET(node_id, SX126X_SPI_OPERATION),                                     \
	.ant_enable = GPIO_DT_SPEC_GET_OR(node_id, antenna_enable_gpios, {0}),                     \
	.tx_enable = GPIO_DT_SPEC_GET_OR(node_id, tx_enable_gpios, {0}),                           \
	.rx_enable = GPIO_DT_SPEC_GET_OR(node_id, rx_enable_gpios, {0}),                           \
	.dio3_tcxo_startup_delay_ms = DT_PROP_OR(node_id, tcxo_power_startup_delay_ms, 0),         \
	.dio3_tcxo_voltage = DT_PROP_OR(node_id, dio3_tcxo_voltage, UINT8_MAX),                    \
	.dio2_rf_switch = DT_PROP(node_id, dio2_tx_enable),                                        \
	.rx_boosted = DT_PROP(node_id, rx_boosted),                                                \
	.regulator_ldo = DT_PROP(node_id, regulator_ldo)

#define SX126X_DEVICE_DEFINE(node_id)                                                              \
	static struct lbm_sx126x_data data_##node_id;                                              \
	DEVICE_DT_DEFINE(node_id, sx126x_init, NULL, &data_##node_id, &config_##node_id,           \
			 POST_KERNEL, CONFIG_LORA_INIT_PRIORITY, &lbm_lora_api)

#define SX126X_DEFINE(node_id, sx_variant)                                                         \
	static const struct lbm_sx126x_config config_##node_id = {                                 \
		SX126X_CONFIG_COMMON(node_id),                                                     \
		.lbm_common.dio1 = GPIO_DT_SPEC_GET(node_id, dio1_gpios),                          \
		.reset = GPIO_DT_SPEC_GET(node_id, reset_gpios),                                   \
		.busy = GPIO_DT_SPEC_GET(node_id, busy_gpios),                                     \
		.variant = sx_variant,                                                             \
	};                                                                                         \
	SX126X_DEVICE_DEFINE(node_id)

#define SX1261_DEFINE(node_id) SX126X_DEFINE(node_id, VARIANT_SX1261)
#define SX1262_DEFINE(node_id) SX126X_DEFINE(node_id, VARIANT_SX1262)

DT_FOREACH_STATUS_OKAY(semtech_sx1261, SX1261_DEFINE);
DT_FOREACH_STATUS_OKAY(semtech_sx1262, SX1262_DEFINE);
DT_FOREACH_STATUS_OKAY(semtech_sx1268, SX1262_DEFINE);
DT_FOREACH_STATUS_OKAY(semtech_llcc68, SX1262_DEFINE);

#define STM32WL_DEFINE(node_id)                                                                    \
	static const struct lbm_sx126x_config config_##node_id = {                                 \
		SX126X_CONFIG_COMMON(node_id),                                                     \
		.variant = VARIANT_STM32WL,                                                        \
		.pa_output = DT_ENUM_IDX(node_id, power_amplifier_output),                         \
		.rfo_lp_max_power = DT_PROP(node_id, rfo_lp_max_power),                            \
		.rfo_hp_max_power = DT_PROP(node_id, rfo_hp_max_power),                            \
	};                                                                                         \
	SX126X_DEVICE_DEFINE(node_id)

DT_FOREACH_STATUS_OKAY(st_stm32wl_subghz_radio, STM32WL_DEFINE);
