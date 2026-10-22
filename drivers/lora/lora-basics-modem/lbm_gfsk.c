/*
 * Copyright (c) 2025 Embeint Inc
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/lora.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(lbm_driver, CONFIG_LORA_LOG_LEVEL);

#include "lbm_common.h"
#include "lbm_gfsk.h"

/*
 * The CRC LoRaWAN's FSK datarate asks for is CRC-16-CCITT, whose seed and
 * polynomial these radios happen to reset to. The whitening seed is not:
 * the parts come up at 0x0100, and LoRaWAN wants every bit of the register
 * set instead.
 */
#define LBM_GFSK_CRC_SEED       0x1D0F
#define LBM_GFSK_CRC_POLYNOMIAL 0x1021
#define LBM_GFSK_WHITENING_SEED 0x01FF

static int lbm_gfsk_pulse_shape(enum lora_gfsk_pulse_shape shape, ral_gfsk_pulse_shape_t *out)
{
	switch (shape) {
	case LORA_GFSK_PULSE_SHAPE_NONE:
		*out = RAL_GFSK_PULSE_SHAPE_OFF;
		break;
	case LORA_GFSK_PULSE_SHAPE_BT_0_3:
		*out = RAL_GFSK_PULSE_SHAPE_BT_03;
		break;
	case LORA_GFSK_PULSE_SHAPE_BT_0_5:
		*out = RAL_GFSK_PULSE_SHAPE_BT_05;
		break;
	case LORA_GFSK_PULSE_SHAPE_BT_0_7:
		*out = RAL_GFSK_PULSE_SHAPE_BT_07;
		break;
	case LORA_GFSK_PULSE_SHAPE_BT_1_0:
		*out = RAL_GFSK_PULSE_SHAPE_BT_1;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

int lbm_lora_config_gfsk(const struct device *dev, const struct lora_modem_config_gfsk *gfsk)
{
	const struct lbm_lora_config_common *config = dev->config;
	struct lbm_lora_data_common *data = dev->data;
	ralf_params_gfsk_t params = {
		.rf_freq_in_hz = gfsk->frequency,
		.output_pwr_in_dbm = gfsk->tx_power,
		.sync_word = gfsk->sync_word,
		.crc_seed = LBM_GFSK_CRC_SEED,
		.crc_polynomial = LBM_GFSK_CRC_POLYNOMIAL,
		.whitening_seed = LBM_GFSK_WHITENING_SEED,
	};
	int ret;

	if (gfsk->sync_word_len > LORA_GFSK_SYNC_WORD_MAX) {
		return -EINVAL;
	}

	if (gfsk->preamble_len > LORA_GFSK_PREAMBLE_MAX) {
		return -EINVAL;
	}

	ret = lbm_gfsk_pulse_shape(gfsk->pulse_shape, &params.mod_params.pulse_shape);
	if (ret < 0) {
		return ret;
	}

	params.mod_params.br_in_bps = gfsk->bitrate;
	params.mod_params.fdev_in_hz = gfsk->freq_deviation;
	params.mod_params.bw_dsb_in_hz = gfsk->bandwidth;

	params.pkt_params.preamble_len_in_bits = gfsk->preamble_len * BITS_PER_BYTE;
	params.pkt_params.preamble_detector = RAL_GFSK_PREAMBLE_DETECTOR_MIN_8BITS;
	params.pkt_params.sync_word_len_in_bits = gfsk->sync_word_len * BITS_PER_BYTE;
	params.pkt_params.address_filtering = RAL_GFSK_ADDRESS_FILTERING_DISABLE;
	params.pkt_params.header_type =
		gfsk->fixed_len ? RAL_GFSK_PKT_FIX_LEN : RAL_GFSK_PKT_VAR_LEN;
	params.pkt_params.pld_len_in_bytes = gfsk->fixed_len ? gfsk->payload_len : UINT8_MAX;
	params.pkt_params.crc_type =
		gfsk->packet_crc_disable ? RAL_GFSK_CRC_OFF : RAL_GFSK_CRC_2_BYTES_INV;
	params.pkt_params.dc_free =
		gfsk->whitening ? RAL_GFSK_DC_FREE_WHITENING : RAL_GFSK_DC_FREE_OFF;

	/* Perform deferred radio initialization on first config */
	if (IS_ENABLED(CONFIG_LORA_BASICS_MODEM_DEFERRED_INIT) && !data->radio_initialized) {
		ret = lbm_driver_radio_init(dev);
		if (ret < 0) {
			return ret;
		}
		data->radio_initialized = true;
	}

	if (!lbm_modem_acquire(dev)) {
		return -EBUSY;
	}

	if (ralf_setup_gfsk(&config->ralf, &params) != RAL_STATUS_OK) {
		ret = -EIO;
		goto release;
	}

	data->gfsk_mod_params = params.mod_params;
	data->gfsk_pkt_params = params.pkt_params;
	data->gfsk = true;
	data->configured = true;
	ret = 0;

release:
	lbm_modem_release(dev);
	return ret;
}
