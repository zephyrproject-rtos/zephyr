/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file test_dai_configure.c
 * @brief ASP1, the serial port the SoC's I2S is wired to
 *
 * Reference stream: 48 kHz stereo 16-bit, SoC as clock controller. SAMPLE_RATE_n 0x03 is 48 kHz
 * and ASP1_FMT 010 is I2S (DS1249F2 table 4-48 and the ASP1_CONTROL2 table).
 */

#include <errno.h>
#include <stdint.h>

#include <zephyr/audio/codec.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/ztest.h>

#include "cs47l63.h"

#include "cs47l63_emul.h"

#define CODEC DEVICE_DT_GET(DT_NODELABEL(cs47l63_dai))
#define EMUL  EMUL_DT_GET(DT_NODELABEL(cs47l63_dai))

#define RATE_24K_HZ           24000U
#define RATE_16K_HZ           16000U
#define RATE_44K1_HZ          44100U
#define WORD_BITS_24          24U
#define WORD_BITS_32          32U
#define WORD_BITS_UNSUPPORTED 20U
#define CHANNELS_MONO         1U
#define CHANNELS_UNSUPPORTED  4U

#define RATE_CODE_48K 0x03U
#define RATE_CODE_24K 0x02U
#define RATE_CODE_16K 0x12U

#define STALE_RATE_CODE     0x0BU
#define STALE_RATE_SLOT_SEL 2U

/** Both slot widths, the format code, and every clock-controller and inversion bit low. */
#define EXPECT_ASP1_CONTROL2                                                                       \
	((CS47L63_EMUL_WORD_BITS << CS47L63_ASP1_RX_WIDTH_SHIFT) |                                 \
	 (CS47L63_EMUL_WORD_BITS << CS47L63_ASP1_TX_WIDTH_SHIFT) |                                 \
	 ((uint32_t)CS47L63_ASP1_FMT_I2S << CS47L63_ASP1_FMT_SHIFT))

static struct i2s_config s_i2s;

static void dai_before_each(void *fixture)
{
	ARG_UNUSED(fixture);

	cs47l63_emul_reset(EMUL);
	s_i2s = cs47l63_test_cfg(AUDIO_ROUTE_PLAYBACK_CAPTURE).dai_cfg.i2s;
}

ZTEST_SUITE(cs47l63_dai, NULL, NULL, dai_before_each, NULL, NULL);

static ZTEST(cs47l63_dai, test_48k_stereo_16bit_solves_to_the_datasheet_codes)
{
	struct cs47l63_dai_solution sol;

	zassert_ok(cs47l63_dai_solve(AUDIO_DAI_TYPE_I2S, &s_i2s, &sol));

	zassert_equal(sol.rate_code, RATE_CODE_48K, "SAMPLE_RATE_n 0x03 is 48 kHz");
	zassert_equal(sol.word_len, CS47L63_EMUL_WORD_BITS, "one 16-bit word per slot");
	zassert_equal(sol.enables, CS47L63_ASP1_RX1_EN | CS47L63_ASP1_RX2_EN,
		      "a stereo stream fills both receive slots");
}

static ZTEST(cs47l63_dai, test_other_supported_rates_and_widths)
{
	struct cs47l63_dai_solution sol;

	s_i2s.frame_clk_freq = RATE_24K_HZ;
	zassert_ok(cs47l63_dai_solve(AUDIO_DAI_TYPE_I2S, &s_i2s, &sol));
	zassert_equal(sol.rate_code, RATE_CODE_24K, "SAMPLE_RATE_n 0x02 is 24 kHz");

	s_i2s.frame_clk_freq = RATE_16K_HZ;
	zassert_ok(cs47l63_dai_solve(AUDIO_DAI_TYPE_I2S, &s_i2s, &sol));
	zassert_equal(sol.rate_code, RATE_CODE_16K, "SAMPLE_RATE_n 0x12 is 16 kHz");

	s_i2s.frame_clk_freq = CS47L63_EMUL_RATE_HZ;
	s_i2s.word_size = WORD_BITS_24;
	zassert_ok(cs47l63_dai_solve(AUDIO_DAI_TYPE_I2S, &s_i2s, &sol));
	zassert_equal(sol.word_len, WORD_BITS_24);

	s_i2s.word_size = WORD_BITS_32;
	zassert_ok(cs47l63_dai_solve(AUDIO_DAI_TYPE_I2S, &s_i2s, &sol));
	zassert_equal(sol.word_len, WORD_BITS_32);
}

static ZTEST(cs47l63_dai, test_mono_uses_one_receive_slot)
{
	struct cs47l63_dai_solution sol;

	s_i2s.channels = CHANNELS_MONO;

	zassert_ok(cs47l63_dai_solve(AUDIO_DAI_TYPE_I2S, &s_i2s, &sol));
	zassert_equal(sol.enables, CS47L63_ASP1_RX1_EN,
		      "a mono stream must not enable the second slot");
}

static ZTEST(cs47l63_dai, test_unsupported_format_rejected)
{
	struct cs47l63_dai_solution sol;

	zassert_equal(-ENOTSUP, cs47l63_dai_solve(AUDIO_DAI_TYPE_LEFT_JUSTIFIED, &s_i2s, &sol),
		      "only I2S is implemented on ASP1");
	zassert_equal(-ENOTSUP, cs47l63_dai_solve(AUDIO_DAI_TYPE_PCMA, &s_i2s, &sol));
	zassert_equal(-ENOTSUP, cs47l63_dai_solve(AUDIO_DAI_TYPE_RIGHT_JUSTIFIED, &s_i2s, &sol));
}

static ZTEST(cs47l63_dai, test_i2s_format_other_than_standard_i2s_rejected)
{
	static const i2s_fmt_t k_formats[] = {
		I2S_FMT_DATA_FORMAT_LEFT_JUSTIFIED,
		I2S_FMT_DATA_FORMAT_RIGHT_JUSTIFIED,
		I2S_FMT_DATA_FORMAT_PCM_SHORT,
		(uint32_t)I2S_FMT_DATA_FORMAT_I2S | I2S_FMT_DATA_ORDER_LSB,
		(uint32_t)I2S_FMT_DATA_FORMAT_I2S | I2S_FMT_BIT_CLK_INV,
		(uint32_t)I2S_FMT_DATA_FORMAT_I2S | I2S_FMT_FRAME_CLK_INV,
	};
	struct cs47l63_dai_solution sol;

	for (size_t i = 0U; i < ARRAY_SIZE(k_formats); i++) {
		s_i2s.format = k_formats[i];
		zassert_equal(-ENOTSUP, cs47l63_dai_solve(AUDIO_DAI_TYPE_I2S, &s_i2s, &sol),
			      "format 0x%x must not be programmed as standard I2S",
			      (uint32_t)k_formats[i]);
	}
}

static ZTEST(cs47l63_dai, test_unsupported_word_size_rejected)
{
	struct cs47l63_dai_solution sol;

	s_i2s.word_size = WORD_BITS_UNSUPPORTED;
	zassert_equal(-ENOTSUP, cs47l63_dai_solve(AUDIO_DAI_TYPE_I2S, &s_i2s, &sol),
		      "20 bits has no ASP1 word-length encoding");
}

static ZTEST(cs47l63_dai, test_unsupported_frame_rate_rejected)
{
	struct cs47l63_dai_solution sol;

	s_i2s.frame_clk_freq = RATE_44K1_HZ;
	zassert_equal(-ENOTSUP, cs47l63_dai_solve(AUDIO_DAI_TYPE_I2S, &s_i2s, &sol),
		      "the 44.1 kHz family needs a SYSCLK this driver does not select");
}

static ZTEST(cs47l63_dai, test_unsupported_channel_count_rejected)
{
	struct cs47l63_dai_solution sol;

	s_i2s.channels = CHANNELS_UNSUPPORTED;
	zassert_equal(-ENOTSUP, cs47l63_dai_solve(AUDIO_DAI_TYPE_I2S, &s_i2s, &sol));

	s_i2s.channels = 0U;
	zassert_equal(-ENOTSUP, cs47l63_dai_solve(AUDIO_DAI_TYPE_I2S, &s_i2s, &sol));
}

static ZTEST(cs47l63_dai, test_codec_as_clock_target_is_the_accepted_role)
{
	struct cs47l63_dai_solution sol;

	s_i2s.options = I2S_OPT_BIT_CLK_TARGET | I2S_OPT_FRAME_CLK_TARGET;
	zassert_ok(cs47l63_dai_solve(AUDIO_DAI_TYPE_I2S, &s_i2s, &sol),
		   "both *_TARGET: the codec is the clock target, as the port is wired");
}

static ZTEST(cs47l63_dai, test_codec_as_clock_controller_rejected)
{
	struct cs47l63_dai_solution sol;

	s_i2s.options = I2S_OPT_BIT_CLK_CONTROLLER | I2S_OPT_FRAME_CLK_CONTROLLER;
	zassert_equal(-ENOTSUP, cs47l63_dai_solve(AUDIO_DAI_TYPE_I2S, &s_i2s, &sol),
		      "options 0: the codec as controller of both clocks");

	s_i2s.options = I2S_OPT_BIT_CLK_TARGET;
	zassert_equal(-ENOTSUP, cs47l63_dai_solve(AUDIO_DAI_TYPE_I2S, &s_i2s, &sol),
		      "mixed: the codec would drive the frame clock");

	s_i2s.options = I2S_OPT_FRAME_CLK_TARGET;
	zassert_equal(-ENOTSUP, cs47l63_dai_solve(AUDIO_DAI_TYPE_I2S, &s_i2s, &sol),
		      "mixed: the codec would drive the bit clock");
}

static ZTEST(cs47l63_dai, test_configure_as_controller_writes_nothing)
{
	struct audio_codec_cfg cfg = cs47l63_test_cfg(AUDIO_ROUTE_PLAYBACK_CAPTURE);

	cfg.dai_cfg.i2s.options = I2S_OPT_BIT_CLK_CONTROLLER | I2S_OPT_FRAME_CLK_CONTROLLER;

	zassert_equal(-ENOTSUP, audio_codec_configure(CODEC, &cfg));
	zassert_equal(cs47l63_emul_xfer_count(EMUL), 0U,
		      "refused before the part is touched at all");
}

static ZTEST(cs47l63_dai, test_null_arguments_rejected)
{
	struct cs47l63_dai_solution sol;

	zassert_equal(-EINVAL, cs47l63_dai_solve(AUDIO_DAI_TYPE_I2S, NULL, &sol));
	zassert_equal(-EINVAL, cs47l63_dai_solve(AUDIO_DAI_TYPE_I2S, &s_i2s, NULL));
}

static ZTEST(cs47l63_dai, test_apply_muxes_the_four_asp1_pads)
{
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

	zassert_ok(cs47l63_test_configure(CODEC, AUDIO_ROUTE_PLAYBACK_CAPTURE));

	/* The pads reset as GPIOs. */
	for (size_t i = 0U; i < ARRAY_SIZE(k_pads); i++) {
		uint32_t val;

		zassert_true(cs47l63_emul_nth_write(EMUL, k_pads[i].addr, 0U, &val),
			     "pad 0x%05x must be muxed to its ASP function", k_pads[i].addr);
		zassert_equal(val, k_pads[i].val, "pad 0x%05x got the wrong configuration",
			      k_pads[i].addr);
	}
}

static ZTEST(cs47l63_dai, test_apply_writes_the_rate_into_one_slot_and_points_the_port_at_it)
{
	/* A different rate left behind, so clearing it must be a write. */
	cs47l63_emul_set_reg(EMUL, CS47L63_SAMPLE_RATE1, STALE_RATE_CODE);
	cs47l63_emul_set_reg(EMUL, CS47L63_ASP1_CONTROL1,
			     STALE_RATE_SLOT_SEL << CS47L63_ASP1_RATE_SHIFT);

	zassert_ok(cs47l63_test_configure(CODEC, AUDIO_ROUTE_PLAYBACK_CAPTURE));

	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_SAMPLE_RATE1) & CS47L63_SAMPLE_RATE_MASK,
		      RATE_CODE_48K, "the global rate slot must carry the 48 kHz code");
	zassert_equal(
		(cs47l63_emul_get_reg(EMUL, CS47L63_ASP1_CONTROL1) & CS47L63_ASP1_RATE_MASK) >>
			CS47L63_ASP1_RATE_SHIFT,
		CS47L63_ASP1_RATE_SEL_SAMPLE_RATE1,
		"the port must follow the slot the rate was written into");
}

static ZTEST(cs47l63_dai, test_apply_writes_format_widths_and_target_mode)
{
	/* Clock-controller and inversion bits preset, so they must be written down. */
	cs47l63_emul_set_reg(EMUL, CS47L63_ASP1_CONTROL2,
			     CS47L63_ASP1_BCLK_MSTR | CS47L63_ASP1_FSYNC_MSTR |
				     CS47L63_ASP1_BCLK_INV | CS47L63_ASP1_FSYNC_INV);

	zassert_ok(cs47l63_test_configure(CODEC, AUDIO_ROUTE_PLAYBACK_CAPTURE));

	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_ASP1_CONTROL2), EXPECT_ASP1_CONTROL2,
		      "widths, format code, and every clock-controller and inversion bit low");
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_ASP1_DATA_CONTROL1) &
			      CS47L63_ASP1_TX_WL_MASK,
		      CS47L63_EMUL_WORD_BITS, "ASP1_TX_WL is the word length, not the slot width");
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_ASP1_DATA_CONTROL5) &
			      CS47L63_ASP1_RX_WL_MASK,
		      CS47L63_EMUL_WORD_BITS);
}

static ZTEST(cs47l63_dai, test_apply_disables_every_slot_before_the_format)
{
	zassert_ok(cs47l63_test_configure(CODEC, AUDIO_ROUTE_PLAYBACK_CAPTURE));

	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_ASP1_ENABLES1), 0U,
		      "the slots wait for a start");
	zassert_true(cs47l63_emul_write_index(EMUL, CS47L63_ASP1_ENABLES1, 0U) <
			     cs47l63_emul_write_index(EMUL, CS47L63_ASP1_CONTROL2, 0U),
		     "no slot is enabled while the format changes");
}

static ZTEST(cs47l63_dai, test_bypass_route_rejected_before_any_write)
{
	zassert_equal(-ENOTSUP, cs47l63_test_configure(CODEC, AUDIO_ROUTE_BYPASS));
	zassert_equal(cs47l63_emul_xfer_count(EMUL), 0U, "a rejected route touches nothing");
}

static ZTEST(cs47l63_dai, test_capture_route_leaves_the_output_mixer_alone)
{
	zassert_ok(cs47l63_test_configure(CODEC, AUDIO_ROUTE_CAPTURE));

	zassert_true(cs47l63_emul_write_count(EMUL, CS47L63_ASP1_CONTROL2) >= 1U,
		     "the premise: configure() reached the hardware");
	zassert_equal(cs47l63_emul_write_count(EMUL, CS47L63_OUT1L_INPUT1), 0U);
	zassert_equal(cs47l63_emul_write_count(EMUL, CS47L63_OUT1L_INPUT2), 0U);
}

static ZTEST(cs47l63_dai, test_playback_route_routes_the_output_mixer)
{
	zassert_ok(cs47l63_test_configure(CODEC, AUDIO_ROUTE_PLAYBACK));

	zassert_equal(cs47l63_emul_write_count(EMUL, CS47L63_OUT1L_INPUT1), 1U);
	zassert_equal(cs47l63_emul_write_count(EMUL, CS47L63_OUT1L_INPUT2), 1U);
}

static ZTEST(cs47l63_dai, test_reconfiguring_the_only_port_boots_the_part_again)
{
	zassert_ok(cs47l63_test_configure(CODEC, AUDIO_ROUTE_PLAYBACK));
	zassert_ok(cs47l63_test_configure(CODEC, AUDIO_ROUTE_PLAYBACK));

	zassert_equal(cs47l63_emul_read_count(EMUL, CS47L63_IRQ1_EINT_2), 2U,
		      "one boot-done poll per reset");
	zassert_equal(cs47l63_emul_read_count(EMUL, CS47L63_DEVID), 2U,
		      "the part is identified again after each reset");
	zassert_equal(cs47l63_emul_read_count(EMUL, CS47L63_SAMPLE_RATE1), 2U,
		      "and its rate is set up again");
}

static ZTEST(cs47l63_dai, test_capture_start_refused_on_a_playback_route)
{
	uint32_t xfers;

	zassert_ok(cs47l63_test_configure(CODEC, AUDIO_ROUTE_PLAYBACK));
	xfers = cs47l63_emul_xfer_count(EMUL);

	zassert_equal(-EINVAL, audio_codec_start(CODEC, AUDIO_DAI_DIR_RX));
	zassert_equal(cs47l63_emul_xfer_count(EMUL), xfers, "a refused start touches nothing");
}

static ZTEST(cs47l63_dai, test_playback_start_refused_on_a_capture_route)
{
	uint32_t xfers;

	zassert_ok(cs47l63_test_configure(CODEC, AUDIO_ROUTE_CAPTURE));
	xfers = cs47l63_emul_xfer_count(EMUL);

	zassert_equal(-EINVAL, audio_codec_start(CODEC, AUDIO_DAI_DIR_TX));
	zassert_equal(cs47l63_emul_xfer_count(EMUL), xfers, "a refused start touches nothing");
}

static ZTEST(cs47l63_dai, test_both_starts_allowed_on_a_playback_capture_route)
{
	zassert_ok(cs47l63_test_configure(CODEC, AUDIO_ROUTE_PLAYBACK_CAPTURE));

	zassert_ok(audio_codec_start(CODEC, AUDIO_DAI_DIR_RX));
	zassert_ok(audio_codec_start(CODEC, AUDIO_DAI_DIR_TX));

	zassert_ok(audio_codec_stop(CODEC, AUDIO_DAI_DIR_TX));
	zassert_ok(audio_codec_stop(CODEC, AUDIO_DAI_DIR_RX));
}

static ZTEST(cs47l63_dai, test_start_output_writes_nothing_on_a_capture_route)
{
	uint32_t xfers;

	zassert_ok(cs47l63_test_configure(CODEC, AUDIO_ROUTE_CAPTURE));
	xfers = cs47l63_emul_xfer_count(EMUL);

	audio_codec_start_output(CODEC);
	zassert_equal(cs47l63_emul_xfer_count(EMUL), xfers, "no playback on a capture route");
}

static ZTEST(cs47l63_dai, test_failed_configure_leaves_nothing_startable)
{
	uint32_t xfers;

	zassert_ok(cs47l63_test_configure(CODEC, AUDIO_ROUTE_PLAYBACK_CAPTURE));

	cs47l63_emul_reset(EMUL);
	cs47l63_emul_fail_at(EMUL, CS47L63_ASP1_CONTROL2);
	zassert_equal(-EIO, cs47l63_test_configure(CODEC, AUDIO_ROUTE_PLAYBACK_CAPTURE));
	zassert_equal(cs47l63_emul_write_count(EMUL, CS47L63_ASP1_CONTROL2), 0U,
		      "the injection took effect: the serial port was never configured");
	xfers = cs47l63_emul_xfer_count(EMUL);

	zassert_equal(-EINVAL, audio_codec_start(CODEC, AUDIO_DAI_DIR_RX));
	zassert_equal(-EINVAL, audio_codec_start(CODEC, AUDIO_DAI_DIR_TX));
	audio_codec_start_output(CODEC);
	zassert_equal(cs47l63_emul_xfer_count(EMUL), xfers, "a half-configured part stays stopped");
}
