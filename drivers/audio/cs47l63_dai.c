/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file cs47l63_dai.c
 * @brief CS47L63 ASP1 serial-port solver and apply path
 *
 * The rate code, the I2S format code and the ASP1 pin pad configuration are the
 * ones the Apache-2.0 vendor bring-up script states in words
 * (modules/hal/cirrus-logic/cs47l63/config/wisce_init.txt); the field masks are
 * from cs47l63_spec.h.
 */

#include "cs47l63_dai.h"

#include <errno.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/sys/util.h>

#include "cs47l63_bus.h"
#include "cs47l63_regs.h"

#define LOG_LEVEL CONFIG_AUDIO_CODEC_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(cs47l63);

/** The four pads that carry ASP1 on this board, in pin order. */
static const struct {
	uint32_t addr;
	uint32_t val;
} k_asp1_pads[] = {
	/* ASP1DOUT is driven by the codec; the other three are driven by the
	 * nRF and are inputs here.
	 */
	{CS47L63_GPIO1_CTRL1, CS47L63_GP_CTRL1_ASP_PAD},
	{CS47L63_GPIO2_CTRL1, CS47L63_GP_CTRL1_ASP_PAD | CS47L63_GP_DIR_INPUT},
	{CS47L63_GPIO3_CTRL1, CS47L63_GP_CTRL1_ASP_PAD | CS47L63_GP_DIR_INPUT},
	{CS47L63_GPIO4_CTRL1, CS47L63_GP_CTRL1_ASP_PAD | CS47L63_GP_DIR_INPUT},
};

static int rate_code(uint32_t frame_clk_hz, uint8_t *code)
{
	static const struct {
		uint32_t hz;
		uint8_t code;
	} k_rates[] = {
		{48000U, CS47L63_SAMPLE_RATE_48K},
		{24000U, CS47L63_SAMPLE_RATE_24K},
		{16000U, CS47L63_SAMPLE_RATE_16K},
	};

	for (size_t i = 0; i < ARRAY_SIZE(k_rates); i++) {
		if (k_rates[i].hz == frame_clk_hz) {
			*code = k_rates[i].code;
			return 0;
		}
	}

	return -ENOTSUP;
}

int cs47l63_dai_solve(audio_dai_type_t dai_type, const struct i2s_config *i2s,
		      struct cs47l63_dai_solution *out)
{
	uint8_t code;
	int ret;

	if (i2s == NULL || out == NULL) {
		return -EINVAL;
	}

	if (dai_type != AUDIO_DAI_TYPE_I2S) {
		LOG_ERR("Only I2S is implemented on ASP1, got DAI type %d", dai_type);
		return -ENOTSUP;
	}

	/* Refused, never corrected: the nRF drives both clocks on this board,
	 * and a second driver on either net is a hardware fault.
	 */
	if ((i2s->options & (I2S_OPT_BIT_CLK_TARGET | I2S_OPT_FRAME_CLK_TARGET)) != 0) {
		LOG_ERR("Codec-as-master requested; ASP1 is always a slave here");
		return -ENOTSUP;
	}

	switch (i2s->word_size) {
	case 16:
	case 24:
	case 32:
		break;
	default:
		LOG_ERR("Unsupported word size %u", i2s->word_size);
		return -ENOTSUP;
	}

	ret = rate_code(i2s->frame_clk_freq, &code);
	if (ret < 0) {
		LOG_ERR("No rate-field encoding for %u Hz", i2s->frame_clk_freq);
		return ret;
	}

	switch (i2s->channels) {
	case 1:
		out->enables = CS47L63_ASP1_RX1_EN;
		break;
	case 2:
		out->enables = CS47L63_ASP1_RX1_EN | CS47L63_ASP1_RX2_EN;
		break;
	default:
		LOG_ERR("Unsupported channel count %u", i2s->channels);
		return -ENOTSUP;
	}

	out->rate_code = code;
	out->fmt = CS47L63_ASP1_FMT_I2S;
	out->slot_width = (uint8_t)i2s->word_size;
	out->word_len = (uint8_t)i2s->word_size;

	return 0;
}

int cs47l63_dai_apply(const struct device *dev, const struct cs47l63_dai_solution *sol)
{
	int ret;

	if (sol == NULL) {
		return -EINVAL;
	}

	for (size_t i = 0; i < ARRAY_SIZE(k_asp1_pads); i++) {
		ret = cs47l63_bus_write_reg(dev, k_asp1_pads[i].addr, k_asp1_pads[i].val);
		if (ret < 0) {
			return ret;
		}
	}

	/* One decision, two registers: the global slot carries the rate and the
	 * port is pointed at that slot. Computing the rate twice is how the two
	 * silently disagree.
	 */
	ret = cs47l63_bus_update_reg(dev, CS47L63_SAMPLE_RATE1, CS47L63_SAMPLE_RATE_MASK,
				     sol->rate_code);
	if (ret < 0) {
		return ret;
	}

	ret = cs47l63_bus_update_reg(dev, CS47L63_ASP1_CONTROL1, CS47L63_ASP1_RATE_MASK,
				     CS47L63_ASP1_RATE_SEL_SAMPLE_RATE1 << CS47L63_ASP1_RATE_SHIFT);
	if (ret < 0) {
		return ret;
	}

	/* Slot widths, format, and the four master/inversion bits written
	 * explicitly to zero rather than left at whatever reset or a previous
	 * configure left behind.
	 */
	ret = cs47l63_bus_update_reg(
		dev, CS47L63_ASP1_CONTROL2,
		CS47L63_ASP1_RX_WIDTH_MASK | CS47L63_ASP1_TX_WIDTH_MASK | CS47L63_ASP1_FMT_MASK |
			CS47L63_ASP1_BCLK_INV | CS47L63_ASP1_BCLK_MSTR | CS47L63_ASP1_FSYNC_INV |
			CS47L63_ASP1_FSYNC_MSTR,
		((uint32_t)sol->slot_width << CS47L63_ASP1_RX_WIDTH_SHIFT) |
			((uint32_t)sol->slot_width << CS47L63_ASP1_TX_WIDTH_SHIFT) |
			((uint32_t)sol->fmt << CS47L63_ASP1_FMT_SHIFT));
	if (ret < 0) {
		return ret;
	}

	ret = cs47l63_bus_update_reg(dev, CS47L63_ASP1_DATA_CONTROL1, CS47L63_ASP1_TX_WL_MASK,
				     sol->word_len);
	if (ret < 0) {
		return ret;
	}

	ret = cs47l63_bus_update_reg(dev, CS47L63_ASP1_DATA_CONTROL5, CS47L63_ASP1_RX_WL_MASK,
				     sol->word_len);
	if (ret < 0) {
		return ret;
	}

	return cs47l63_bus_write_reg(dev, CS47L63_ASP1_ENABLES1, sol->enables);
}
