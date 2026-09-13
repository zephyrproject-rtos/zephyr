/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file test_dai_configure.c
 * @brief ASP1, the serial port the SoC's I2S is wired to
 *
 * The reference stream throughout is 48 kHz stereo at 16 bits, with the SoC as
 * bit-clock and frame-sync master. The field codes asserted are the
 * datasheet's: SAMPLE_RATE_n 0x03 is 48 kHz and ASP1_FMT 010 is I2S mode
 * (DS1249F2, the SAMPLE_RATE1 and ASP1_CONTROL2 register tables).
 */

#include <errno.h>
#include <stdint.h>

#include <zephyr/audio/codec.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/ztest.h>

#include "cs47l63_dai.h"
#include "cs47l63_regs.h"

#include "cs47l63_emul.h"

#define CODEC DEVICE_DT_GET(DT_NODELABEL(cs47l63_dai))
#define EMUL  EMUL_DT_GET(DT_NODELABEL(cs47l63_dai))

#define WORD_BITS 16U

/** Both slot widths, the format code, and every master and inversion bit low. */
#define EXPECT_ASP1_CONTROL2                                                                       \
	((WORD_BITS << CS47L63_ASP1_RX_WIDTH_SHIFT) | (WORD_BITS << CS47L63_ASP1_TX_WIDTH_SHIFT) | \
	 ((uint32_t)CS47L63_ASP1_FMT_I2S << CS47L63_ASP1_FMT_SHIFT))

/** The four pads that carry ASP1: DOUT is driven by the codec, the rest are inputs. */
static const struct {
	uint32_t addr;
	uint32_t val;
} k_pads[] = {
	{CS47L63_GPIO1_CTRL1, CS47L63_GP_CTRL1_ASP_PAD},
	{CS47L63_GPIO2_CTRL1, CS47L63_GP_CTRL1_ASP_PAD | CS47L63_GP_DIR_INPUT},
	{CS47L63_GPIO3_CTRL1, CS47L63_GP_CTRL1_ASP_PAD | CS47L63_GP_DIR_INPUT},
	{CS47L63_GPIO4_CTRL1, CS47L63_GP_CTRL1_ASP_PAD | CS47L63_GP_DIR_INPUT},
};

static struct i2s_config s_i2s;

static void before_each(void *fixture)
{
	ARG_UNUSED(fixture);

	cs47l63_emul_reset(EMUL);

	s_i2s = (struct i2s_config){
		.word_size = WORD_BITS,
		.channels = 2,
		.format = I2S_FMT_DATA_FORMAT_I2S,
		.options = 0,
		.frame_clk_freq = 48000,
	};
}

ZTEST_SUITE(cs47l63_dai, NULL, NULL, before_each, NULL, NULL);

ZTEST(cs47l63_dai, test_48k_stereo_16bit_solves_to_the_datasheet_codes)
{
	struct cs47l63_dai_solution sol;

	zassert_ok(cs47l63_dai_solve(AUDIO_DAI_TYPE_I2S, &s_i2s, &sol));

	zassert_equal(sol.rate_code, 0x03, "SAMPLE_RATE_n 0x03 is 48 kHz");
	zassert_equal(sol.fmt, 0x2, "ASP1_FMT 010 is I2S mode");
	zassert_equal(sol.slot_width, WORD_BITS, "one 16-bit word per slot");
	zassert_equal(sol.word_len, WORD_BITS);
	zassert_equal(sol.enables, CS47L63_ASP1_RX1_EN | CS47L63_ASP1_RX2_EN,
		      "a stereo stream fills both receive slots");
}

ZTEST(cs47l63_dai, test_other_supported_rates_and_widths)
{
	struct cs47l63_dai_solution sol;

	s_i2s.frame_clk_freq = 24000;
	zassert_ok(cs47l63_dai_solve(AUDIO_DAI_TYPE_I2S, &s_i2s, &sol));
	zassert_equal(sol.rate_code, 0x02, "SAMPLE_RATE_n 0x02 is 24 kHz");

	s_i2s.frame_clk_freq = 16000;
	zassert_ok(cs47l63_dai_solve(AUDIO_DAI_TYPE_I2S, &s_i2s, &sol));
	zassert_equal(sol.rate_code, 0x12, "SAMPLE_RATE_n 0x12 is 16 kHz");

	s_i2s.frame_clk_freq = 48000;
	s_i2s.word_size = 24;
	zassert_ok(cs47l63_dai_solve(AUDIO_DAI_TYPE_I2S, &s_i2s, &sol));
	zassert_equal(sol.slot_width, 24);
	zassert_equal(sol.word_len, 24);

	s_i2s.word_size = 32;
	zassert_ok(cs47l63_dai_solve(AUDIO_DAI_TYPE_I2S, &s_i2s, &sol));
	zassert_equal(sol.slot_width, 32);
}

ZTEST(cs47l63_dai, test_mono_uses_one_receive_slot)
{
	struct cs47l63_dai_solution sol;

	s_i2s.channels = 1;

	zassert_ok(cs47l63_dai_solve(AUDIO_DAI_TYPE_I2S, &s_i2s, &sol));
	zassert_equal(sol.enables, CS47L63_ASP1_RX1_EN,
		      "a mono stream must not enable the second slot");
}

ZTEST(cs47l63_dai, test_unsupported_format_rejected)
{
	struct cs47l63_dai_solution sol;

	zassert_equal(-ENOTSUP, cs47l63_dai_solve(AUDIO_DAI_TYPE_LEFT_JUSTIFIED, &s_i2s, &sol),
		      "only I2S is implemented on ASP1");
	zassert_equal(-ENOTSUP, cs47l63_dai_solve(AUDIO_DAI_TYPE_PCMA, &s_i2s, &sol));
	zassert_equal(-ENOTSUP, cs47l63_dai_solve(AUDIO_DAI_TYPE_RIGHT_JUSTIFIED, &s_i2s, &sol));
}

ZTEST(cs47l63_dai, test_unsupported_word_size_rejected)
{
	struct cs47l63_dai_solution sol;

	s_i2s.word_size = 20;
	zassert_equal(-ENOTSUP, cs47l63_dai_solve(AUDIO_DAI_TYPE_I2S, &s_i2s, &sol),
		      "20 bits has no ASP1 word-length encoding");
}

ZTEST(cs47l63_dai, test_unsupported_frame_rate_rejected)
{
	struct cs47l63_dai_solution sol;

	s_i2s.frame_clk_freq = 44100;
	zassert_equal(-ENOTSUP, cs47l63_dai_solve(AUDIO_DAI_TYPE_I2S, &s_i2s, &sol),
		      "the 44.1 kHz family needs a SYSCLK this driver does not select");
}

ZTEST(cs47l63_dai, test_unsupported_channel_count_rejected)
{
	struct cs47l63_dai_solution sol;

	s_i2s.channels = 4;
	zassert_equal(-ENOTSUP, cs47l63_dai_solve(AUDIO_DAI_TYPE_I2S, &s_i2s, &sol));

	s_i2s.channels = 0;
	zassert_equal(-ENOTSUP, cs47l63_dai_solve(AUDIO_DAI_TYPE_I2S, &s_i2s, &sol));
}

ZTEST(cs47l63_dai, test_codec_as_clock_master_rejected)
{
	struct cs47l63_dai_solution sol;

	/* The SoC drives both clocks on this board. A second driver on either
	 * net is a hardware fault, so the request is refused rather than
	 * quietly corrected to slave.
	 */
	s_i2s.options = I2S_OPT_BIT_CLK_TARGET;
	zassert_equal(-ENOTSUP, cs47l63_dai_solve(AUDIO_DAI_TYPE_I2S, &s_i2s, &sol));

	s_i2s.options = I2S_OPT_FRAME_CLK_TARGET;
	zassert_equal(-ENOTSUP, cs47l63_dai_solve(AUDIO_DAI_TYPE_I2S, &s_i2s, &sol));
}

ZTEST(cs47l63_dai, test_null_arguments_rejected)
{
	struct cs47l63_dai_solution sol;

	zassert_equal(-EINVAL, cs47l63_dai_solve(AUDIO_DAI_TYPE_I2S, NULL, &sol));
	zassert_equal(-EINVAL, cs47l63_dai_solve(AUDIO_DAI_TYPE_I2S, &s_i2s, NULL));
	zassert_equal(-EINVAL, cs47l63_dai_apply(CODEC, NULL));
	zassert_equal(cs47l63_emul_xfer_count(EMUL), 0, "a rejected apply writes nothing");
}

ZTEST(cs47l63_dai, test_apply_muxes_the_four_asp1_pads)
{
	struct cs47l63_dai_solution sol;

	zassert_ok(cs47l63_dai_solve(AUDIO_DAI_TYPE_I2S, &s_i2s, &sol));
	zassert_ok(cs47l63_dai_apply(CODEC, &sol));

	/* The pads come out of reset as GPIOs. A port configured without this
	 * is correct in every other register and still silent on the wire.
	 */
	for (size_t i = 0; i < ARRAY_SIZE(k_pads); i++) {
		uint32_t val;

		zassert_true(cs47l63_emul_nth_write(EMUL, k_pads[i].addr, 0, &val),
			     "pad 0x%05x must be muxed to its ASP function", k_pads[i].addr);
		zassert_equal(val, k_pads[i].val, "pad 0x%05x got the wrong configuration",
			      k_pads[i].addr);
	}
}

ZTEST(cs47l63_dai, test_apply_writes_the_rate_into_one_slot_and_points_the_port_at_it)
{
	struct cs47l63_dai_solution sol;

	/* Leave a different rate behind in both registers, so clearing them is
	 * a write the driver has to make rather than a reset value it inherits.
	 */
	cs47l63_emul_set_reg(EMUL, CS47L63_SAMPLE_RATE1, 0x0B);
	cs47l63_emul_set_reg(EMUL, CS47L63_ASP1_CONTROL1, 0x00000200);

	zassert_ok(cs47l63_dai_solve(AUDIO_DAI_TYPE_I2S, &s_i2s, &sol));
	zassert_ok(cs47l63_dai_apply(CODEC, &sol));

	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_SAMPLE_RATE1) & CS47L63_SAMPLE_RATE_MASK,
		      0x03, "the global rate slot must carry the 48 kHz code");
	zassert_equal(
		(cs47l63_emul_get_reg(EMUL, CS47L63_ASP1_CONTROL1) & CS47L63_ASP1_RATE_MASK) >>
			CS47L63_ASP1_RATE_SHIFT,
		CS47L63_ASP1_RATE_SEL_SAMPLE_RATE1,
		"the port must follow the slot the rate was written into");
}

ZTEST(cs47l63_dai, test_apply_writes_format_widths_and_slave_mode)
{
	struct cs47l63_dai_solution sol;

	/* Come in with the codec configured as master and both clocks
	 * inverted: those bits must be written down, not merely left alone.
	 */
	cs47l63_emul_set_reg(EMUL, CS47L63_ASP1_CONTROL2,
			     CS47L63_ASP1_BCLK_MSTR | CS47L63_ASP1_FSYNC_MSTR |
				     CS47L63_ASP1_BCLK_INV | CS47L63_ASP1_FSYNC_INV);

	zassert_ok(cs47l63_dai_solve(AUDIO_DAI_TYPE_I2S, &s_i2s, &sol));
	zassert_ok(cs47l63_dai_apply(CODEC, &sol));

	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_ASP1_CONTROL2), EXPECT_ASP1_CONTROL2,
		      "widths, format code, and every master and inversion bit low");
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_ASP1_DATA_CONTROL1) &
			      CS47L63_ASP1_TX_WL_MASK,
		      WORD_BITS, "ASP1_TX_WL is the word length, not the slot width");
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_ASP1_DATA_CONTROL5) &
			      CS47L63_ASP1_RX_WL_MASK,
		      WORD_BITS);
}

ZTEST(cs47l63_dai, test_apply_enables_the_receive_slots_last)
{
	struct cs47l63_dai_solution sol;
	int enables_idx;

	zassert_ok(cs47l63_dai_solve(AUDIO_DAI_TYPE_I2S, &s_i2s, &sol));
	zassert_ok(cs47l63_dai_apply(CODEC, &sol));

	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_ASP1_ENABLES1),
		      CS47L63_ASP1_RX1_EN | CS47L63_ASP1_RX2_EN,
		      "both receive slots carry audio for a stereo stream");

	enables_idx = cs47l63_emul_write_index(EMUL, CS47L63_ASP1_ENABLES1, 0);
	zassert_true(enables_idx > cs47l63_emul_write_index(EMUL, CS47L63_ASP1_CONTROL2, 0),
		     "the format must be settled before the slots start carrying data");
}
