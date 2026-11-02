/*
 * Copyright (c) 2026 Carlo Caione <ccaione@baylibre.com>
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/lora.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(sx126x, CONFIG_LORA_LOG_LEVEL);

#include "sx126x.h"
#include "sx126x_gfsk.h"
#include "sx126x_hal.h"
#include "sx126x_regs.h"

/*
 * Receive bandwidths the GFSK modem offers, smallest first, stated across
 * both sidebands the way the datasheet does (DS.SX1261-2.W.APP Rev 2.2,
 * Table 4-2). A request takes the narrowest filter at least as wide, so the
 * receiver is never narrower than what was asked for.
 */
static const struct {
	uint32_t bandwidth;
	uint8_t reg;
} sx126x_gfsk_bw[] = {
	{4800, SX126X_GFSK_BW_4800},     {5800, SX126X_GFSK_BW_5800},
	{7300, SX126X_GFSK_BW_7300},     {9700, SX126X_GFSK_BW_9700},
	{11700, SX126X_GFSK_BW_11700},   {14600, SX126X_GFSK_BW_14600},
	{19500, SX126X_GFSK_BW_19500},   {23400, SX126X_GFSK_BW_23400},
	{29300, SX126X_GFSK_BW_29300},   {39000, SX126X_GFSK_BW_39000},
	{46900, SX126X_GFSK_BW_46900},   {58600, SX126X_GFSK_BW_58600},
	{78200, SX126X_GFSK_BW_78200},   {93800, SX126X_GFSK_BW_93800},
	{117300, SX126X_GFSK_BW_117300}, {156200, SX126X_GFSK_BW_156200},
	{187200, SX126X_GFSK_BW_187200}, {234300, SX126X_GFSK_BW_234300},
	{312000, SX126X_GFSK_BW_312000}, {373600, SX126X_GFSK_BW_373600},
	{467000, SX126X_GFSK_BW_467000},
};

static int sx126x_gfsk_bw_to_reg(uint32_t bandwidth, uint8_t *reg)
{
	for (int i = 0; i < ARRAY_SIZE(sx126x_gfsk_bw); i++) {
		if (sx126x_gfsk_bw[i].bandwidth >= bandwidth) {
			*reg = sx126x_gfsk_bw[i].reg;
			return 0;
		}
	}

	return -ENOTSUP;
}

static int sx126x_gfsk_pulse_shape_to_reg(enum lora_gfsk_pulse_shape shape, uint8_t *reg)
{
	switch (shape) {
	case LORA_GFSK_PULSE_SHAPE_NONE:
		*reg = SX126X_GFSK_PULSE_SHAPE_OFF;
		return 0;
	case LORA_GFSK_PULSE_SHAPE_BT_0_3:
		*reg = SX126X_GFSK_PULSE_SHAPE_BT_03;
		return 0;
	case LORA_GFSK_PULSE_SHAPE_BT_0_5:
		*reg = SX126X_GFSK_PULSE_SHAPE_BT_05;
		return 0;
	case LORA_GFSK_PULSE_SHAPE_BT_0_7:
		*reg = SX126X_GFSK_PULSE_SHAPE_BT_07;
		return 0;
	case LORA_GFSK_PULSE_SHAPE_BT_1_0:
		*reg = SX126X_GFSK_PULSE_SHAPE_BT_10;
		return 0;
	default:
		return -EINVAL;
	}
}

static int sx126x_validate_gfsk_config(const struct lora_modem_config_gfsk *gfsk)
{
	uint8_t reg;

	if (gfsk->bitrate < SX126X_GFSK_BR_MIN_BPS || gfsk->bitrate > SX126X_GFSK_BR_MAX_BPS) {
		LOG_ERR("GFSK bit rate out of range: %u", gfsk->bitrate);
		return -EINVAL;
	}

	if (gfsk->freq_deviation < SX126X_GFSK_FDEV_MIN_HZ ||
	    gfsk->freq_deviation > SX126X_GFSK_FDEV_MAX_HZ) {
		LOG_ERR("GFSK deviation out of range: %u", gfsk->freq_deviation);
		return -EINVAL;
	}

	if (gfsk->freq_deviation + gfsk->bitrate / 2 > SX126X_GFSK_FDEV_BR_MAX_HZ) {
		LOG_ERR("GFSK deviation and bit rate occupy too much bandwidth");
		return -EINVAL;
	}

	if (gfsk->sync_word_len > LORA_GFSK_SYNC_WORD_MAX) {
		LOG_ERR("GFSK sync word too long: %u", gfsk->sync_word_len);
		return -EINVAL;
	}

	if (gfsk->preamble_len > LORA_GFSK_PREAMBLE_MAX) {
		LOG_ERR("GFSK preamble too long: %u", gfsk->preamble_len);
		return -EINVAL;
	}

	if (sx126x_gfsk_bw_to_reg(gfsk->bandwidth, &reg) < 0) {
		LOG_ERR("Unsupported GFSK bandwidth: %u Hz", gfsk->bandwidth);
		return -ENOTSUP;
	}

	return sx126x_gfsk_pulse_shape_to_reg(gfsk->pulse_shape, &reg);
}

static int sx126x_set_gfsk_modulation_params(const struct device *dev,
					     const struct lora_modem_config_gfsk *gfsk)
{
	uint8_t buf[8];
	uint8_t bw_reg;
	uint8_t shape_reg;
	int ret;

	ret = sx126x_gfsk_bw_to_reg(gfsk->bandwidth, &bw_reg);
	if (ret < 0) {
		return ret;
	}

	ret = sx126x_gfsk_pulse_shape_to_reg(gfsk->pulse_shape, &shape_reg);
	if (ret < 0) {
		return ret;
	}

	sys_put_be24(SX126X_GFSK_BR_TO_REG(gfsk->bitrate), &buf[0]);
	buf[3] = shape_reg;
	buf[4] = bw_reg;
	sys_put_be24(SX126X_FREQ_TO_REG(gfsk->freq_deviation), &buf[5]);

	return sx12xx_hal_write_cmd(dev, SX126X_CMD_SET_MODULATION_PARAMS, buf, 8);
}

int sx126x_set_gfsk_packet_params(const struct device *dev,
				  const struct lora_modem_config_gfsk *gfsk, uint8_t payload_len)
{
	uint8_t buf[9];

	sys_put_be16(gfsk->preamble_len * BITS_PER_BYTE, &buf[0]);
	buf[2] = SX126X_GFSK_PREAMBLE_DETECT_8_BITS;
	buf[3] = gfsk->sync_word_len * BITS_PER_BYTE;
	buf[4] = SX126X_GFSK_ADDR_FILTER_OFF;
	buf[5] = gfsk->fixed_len ? SX126X_GFSK_PKT_FIX_LEN : SX126X_GFSK_PKT_VAR_LEN;
	/* A fixed-length frame carries no length byte, so the radio matches on this one */
	buf[6] = gfsk->fixed_len ? gfsk->payload_len : payload_len;
	buf[7] = gfsk->packet_crc_disable ? SX126X_GFSK_CRC_OFF : SX126X_GFSK_CRC_2_BYTES_INV;
	buf[8] = gfsk->whitening ? SX126X_GFSK_WHITENING_ON : SX126X_GFSK_WHITENING_OFF;

	return sx12xx_hal_write_cmd(dev, SX126X_CMD_SET_PACKET_PARAMS, buf, 9);
}

static int sx126x_config_gfsk(const struct device *dev, const struct lora_modem_config_gfsk *gfsk)
{
	uint8_t buf[2];
	int ret;

	ret = sx126x_set_gfsk_modulation_params(dev, gfsk);
	if (ret < 0) {
		return ret;
	}

	/* The payload length only matters for a fixed-length packet; a
	 * variable-length one carries its own and this caps what is accepted.
	 */
	ret = sx126x_set_gfsk_packet_params(
		dev, gfsk, gfsk->fixed_len ? gfsk->payload_len : SX126X_MAX_PAYLOAD_LEN);
	if (ret < 0) {
		return ret;
	}

	if (gfsk->sync_word_len > 0) {
		ret = sx12xx_hal_write_regs(dev, SX126X_REG_GFSK_SYNC_WORD, gfsk->sync_word,
					    gfsk->sync_word_len);
		if (ret < 0) {
			return ret;
		}
	}

	if (!gfsk->packet_crc_disable) {
		sys_put_be16(SX126X_GFSK_CRC_INIT_CCITT, buf);
		ret = sx12xx_hal_write_regs(dev, SX126X_REG_GFSK_CRC_INIT_MSB, buf, 2);
		if (ret < 0) {
			return ret;
		}

		sys_put_be16(SX126X_GFSK_CRC_POLY_CCITT, buf);
		ret = sx12xx_hal_write_regs(dev, SX126X_REG_GFSK_CRC_POLY_MSB, buf, 2);
		if (ret < 0) {
			return ret;
		}
	}

	if (gfsk->whitening) {
		/* Only the low bit of the high byte belongs to the seed. What
		 * else the register holds is the radio's, so read it back and
		 * leave it alone.
		 */
		ret = sx12xx_hal_read_regs(dev, SX126X_REG_GFSK_WHITENING_MSB, buf, 1);
		if (ret < 0) {
			return ret;
		}

		buf[0] = (buf[0] & ~SX126X_GFSK_WHITENING_MSB_MASK) |
			 ((SX126X_GFSK_WHITENING_INIT >> 8) & SX126X_GFSK_WHITENING_MSB_MASK);
		buf[1] = SX126X_GFSK_WHITENING_INIT & 0xFF;
		ret = sx12xx_hal_write_regs(dev, SX126X_REG_GFSK_WHITENING_MSB, buf, 2);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

/*
 * Frame a packet the way the configuration asks for. The two modems take
 * different parameters, and every send and receive needs the same choice
 * made, so it is made here rather than at each of them.
 */

int sx126x_lora_config_gfsk(const struct device *dev, const struct lora_modem_config_gfsk *gfsk)
{
	struct sx126x_data *data = dev->data;
	const struct sx126x_hal_config *hal_config = dev->config;
	int ret;

	ret = sx126x_validate_gfsk_config(gfsk);
	if (ret < 0) {
		return ret;
	}

	ret = sx126x_config_begin(dev);
	if (ret < 0) {
		return ret;
	}

	data->gfsk_config = *gfsk;

	ret = sx126x_set_packet_type(dev, SX126X_PACKET_TYPE_GFSK);
	if (ret < 0) {
		goto out;
	}

	ret = sx126x_config_carrier(dev, gfsk->frequency, gfsk->tx_power);
	if (ret < 0) {
		goto out;
	}

	ret = sx126x_config_gfsk(dev, gfsk);
	if (ret < 0) {
		goto out;
	}

	/* Boosting the receiver is a board property, not a modem one */
	ret = sx126x_set_rx_gain(dev, hal_config->rx_boosted);
	if (ret < 0) {
		goto out;
	}

	data->gfsk = true;
	data->config_valid = true;
	LOG_DBG("Config: GFSK freq=%u, br=%u, fdev=%u, bw=%u, power=%d", gfsk->frequency,
		gfsk->bitrate, gfsk->freq_deviation, gfsk->bandwidth, gfsk->tx_power);

out:
	sx126x_set_sleep(dev);
	k_mutex_unlock(&data->lock);
	return ret;
}
