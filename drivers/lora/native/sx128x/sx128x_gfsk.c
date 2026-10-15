/*
 * Copyright (c) 2026 Giuseppe Fabiano <gfabiano40@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 *
 * GFSK modem of the SX128x. Unlike the sub-GHz parts, the chip has no free
 * bit rate or deviation registers: both come from short lists in the
 * datasheet, so a lora_modem_config_gfsk is translated into list entries
 * here, and anything that falls between them is refused rather than rounded
 * silently into a different link.
 */

#include <zephyr/drivers/lora.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(sx128x, CONFIG_LORA_LOG_LEVEL);

#include "sx128x.h"
#include "sx128x_gfsk.h"
#include "sx128x_hal.h"
#include "sx128x_regs.h"

/*
 * ModulationParam1 pairs a bit rate with a double-sideband channel filter
 * (DS.SX1280-1.W.APP Rev 3.3, Table 14-1). Entries for one bit rate are kept
 * from the tightest filter to the widest.
 */
static const struct {
	uint32_t bps;
	uint32_t filter_hz;
	uint8_t code;
} sx128x_gfsk_rates[] = {
	{125000, 300000, SX128X_GFSK_BR_125_BW_300},
	{250000, 300000, SX128X_GFSK_BR_250_BW_300},
	{250000, 600000, SX128X_GFSK_BR_250_BW_600},
	{400000, 600000, SX128X_GFSK_BR_400_BW_600},
	{400000, 1200000, SX128X_GFSK_BR_400_BW_1200},
	{500000, 600000, SX128X_GFSK_BR_500_BW_600},
	{500000, 1200000, SX128X_GFSK_BR_500_BW_1200},
	{800000, 1200000, SX128X_GFSK_BR_800_BW_1200},
	{800000, 2400000, SX128X_GFSK_BR_800_BW_2400},
	{1000000, 1200000, SX128X_GFSK_BR_1000_BW_1200},
	{1000000, 2400000, SX128X_GFSK_BR_1000_BW_2400},
	{1600000, 2400000, SX128X_GFSK_BR_1600_BW_2400},
	{2000000, 2400000, SX128X_GFSK_BR_2000_BW_2400},
};

/* The three ModulationParams bytes, in command order */
struct sx128x_gfsk_mod {
	uint8_t rate;
	uint8_t index;
	uint8_t shaping;
};

BUILD_ASSERT(sizeof(struct sx128x_gfsk_mod) == 3, "sent to the chip as is");

static int sx128x_gfsk_encode_rate(const struct lora_modem_config_gfsk *cfg, uint8_t *code)
{
	ARRAY_FOR_EACH(sx128x_gfsk_rates, i) {
		if (sx128x_gfsk_rates[i].bps != cfg->bitrate) {
			continue;
		}
		/* Asking for a filter no wider than one the chip has */
		if (cfg->bandwidth <= sx128x_gfsk_rates[i].filter_hz) {
			*code = sx128x_gfsk_rates[i].code;
			return 0;
		}
	}

	LOG_ERR("No GFSK mode for %u bit/s within %u Hz", cfg->bitrate, cfg->bandwidth);
	return -ENOTSUP;
}

/*
 * ModulationParam2 is the modulation index h = 2 * deviation / bit rate
 * (Table 14-2): 0.35, then 0.5 up to 4.0 every 0.25. Work in hundredths of
 * h and take the closest entry, as long as the request is nearer to the list
 * than half a step.
 */
static int sx128x_gfsk_encode_index(const struct lora_modem_config_gfsk *cfg, uint8_t *code)
{
	uint32_t h = (uint32_t)DIV_ROUND_CLOSEST(200ULL * cfg->freq_deviation, cfg->bitrate);

	if (h >= 28U && h < 43U) {
		*code = SX128X_GFSK_MOD_IND_0_35;
	} else if (h >= 43U && h <= 412U) {
		*code = (uint8_t)MIN(DIV_ROUND_CLOSEST(MAX(h, 50U) - 50U, 25U) + 1U,
				     SX128X_GFSK_MOD_IND_4_00);
	} else {
		LOG_ERR("GFSK deviation %u Hz at %u bit/s is outside h 0.35..4.0",
			cfg->freq_deviation, cfg->bitrate);
		return -ENOTSUP;
	}

	return 0;
}

/* ModulationParam3 (Table 14-3): only BT 0.5, BT 1.0 or no filter */
static int sx128x_gfsk_encode_shaping(const struct lora_modem_config_gfsk *cfg, uint8_t *code)
{
	switch (cfg->pulse_shape) {
	case LORA_GFSK_PULSE_SHAPE_NONE:
		*code = SX128X_GFSK_BT_OFF;
		break;
	case LORA_GFSK_PULSE_SHAPE_BT_0_5:
		*code = SX128X_GFSK_BT_0_5;
		break;
	case LORA_GFSK_PULSE_SHAPE_BT_1_0:
		*code = SX128X_GFSK_BT_1_0;
		break;
	default:
		LOG_ERR("GFSK pulse shape %d not available on this chip", cfg->pulse_shape);
		return -ENOTSUP;
	}

	return 0;
}

/* Check the parts of the frame format the chip can carry */
static int sx128x_gfsk_check_frame(const struct lora_modem_config_gfsk *cfg)
{
	if (cfg->frequency < SX128X_FREQ_MIN_HZ || cfg->frequency > SX128X_FREQ_MAX_HZ) {
		LOG_ERR("%u Hz is outside the 2.4 GHz band", cfg->frequency);
		return -EINVAL;
	}

	/* PacketParam1 counts the preamble in 4-bit steps up to 32 bits */
	if (cfg->preamble_len == 0U ||
	    cfg->preamble_len * BITS_PER_BYTE > SX128X_GFSK_PREAMBLE_MAX_BITS) {
		LOG_ERR("GFSK preamble of %u bytes, the chip takes 1 to %u", cfg->preamble_len,
			SX128X_GFSK_PREAMBLE_MAX_BITS / BITS_PER_BYTE);
		return cfg->preamble_len == 0U ? -EINVAL : -ENOTSUP;
	}

	if (cfg->sync_word_len > SX128X_GFSK_SYNC_WORD_MAX_LEN) {
		LOG_ERR("GFSK sync word of %u bytes, the chip takes up to %u", cfg->sync_word_len,
			SX128X_GFSK_SYNC_WORD_MAX_LEN);
		return -ENOTSUP;
	}

	if (cfg->fixed_len && cfg->payload_len == 0U) {
		LOG_ERR("Fixed-length GFSK without a payload length");
		return -EINVAL;
	}

	return 0;
}

static int sx128x_gfsk_encode(const struct lora_modem_config_gfsk *cfg, struct sx128x_gfsk_mod *mod)
{
	int ret;

	ret = sx128x_gfsk_check_frame(cfg);
	if (ret == 0) {
		ret = sx128x_gfsk_encode_rate(cfg, &mod->rate);
	}
	if (ret == 0) {
		ret = sx128x_gfsk_encode_index(cfg, &mod->index);
	}
	if (ret == 0) {
		ret = sx128x_gfsk_encode_shaping(cfg, &mod->shaping);
	}

	return ret;
}

int sx128x_set_gfsk_packet_params(const struct device *dev,
				  const struct lora_modem_config_gfsk *cfg, uint8_t payload_len)
{
	/* With sync detection off the length field is ignored, but must still
	 * hold a legal value.
	 */
	uint8_t sync_len = MAX(cfg->sync_word_len, 1U);
	uint8_t params[7] = {
		SX128X_GFSK_PREAMBLE_BITS(cfg->preamble_len * BITS_PER_BYTE),
		SX128X_GFSK_SYNC_WORD_LEN(sync_len),
		cfg->sync_word_len > 0U ? SX128X_GFSK_SYNC_WORD_1 : SX128X_GFSK_SYNC_WORD_OFF,
		cfg->fixed_len ? SX128X_GFSK_PACKET_FIXED : SX128X_GFSK_PACKET_VARIABLE,
		/* Fixed frames have no length field on air: both ends use this */
		cfg->fixed_len ? cfg->payload_len : payload_len,
		cfg->packet_crc_disable ? SX128X_GFSK_CRC_OFF : SX128X_GFSK_CRC_2_BYTES,
		cfg->whitening ? SX128X_GFSK_WHITENING_ON : SX128X_GFSK_WHITENING_OFF,
	};

	return sx12xx_hal_write_cmd(dev, SX128X_CMD_SET_PACKET_PARAMS, params, sizeof(params));
}

int sx128x_lora_config_gfsk(const struct device *dev, const struct lora_modem_config_gfsk *cfg)
{
	struct sx128x_data *data = dev->data;
	struct sx128x_gfsk_mod mod;
	int ret;

	ret = sx128x_gfsk_encode(cfg, &mod);
	if (ret < 0) {
		return ret;
	}

	ret = sx128x_config_begin(dev);
	if (ret < 0) {
		return ret;
	}

	ret = sx128x_set_packet_type(dev, SX128X_PACKET_TYPE_GFSK);
	if (ret == 0) {
		ret = sx128x_config_carrier(dev, cfg->frequency, cfg->tx_power);
	}
	if (ret == 0) {
		ret = sx12xx_hal_write_cmd(dev, SX128X_CMD_SET_MODULATION_PARAMS, (uint8_t *)&mod,
					   sizeof(mod));
	}
	if (ret == 0) {
		/* Receive side: the longest frame a variable-length link accepts */
		ret = sx128x_set_gfsk_packet_params(dev, cfg, SX128X_MAX_PAYLOAD_LEN);
	}
	if (ret == 0 && cfg->sync_word_len > 0U) {
		/* A sync word shorter than five bytes is the tail of the register */
		ret = sx12xx_hal_write_regs(dev,
					    SX128X_REG_GFSK_SYNC_WORD_1_END - cfg->sync_word_len,
					    cfg->sync_word, cfg->sync_word_len);
	}

	if (ret == 0) {
		data->gfsk_config = *cfg;
		data->gfsk = true;
		data->config_valid = true;
		LOG_DBG("GFSK %u Hz, %u bit/s, mod 0x%02x 0x%02x 0x%02x", cfg->frequency,
			cfg->bitrate, mod.rate, mod.index, mod.shaping);
	}

	sx128x_config_end(dev);
	return ret;
}
