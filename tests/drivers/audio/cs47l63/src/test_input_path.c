/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file test_input_path.c
 * @brief IN2, the analog line pair, and the two ASP1 transmit mixers
 *
 * IN1 is the PDM microphone pair, which needs a bias supply and a clock this
 * driver never configures. Asking for it is refused rather than silently
 * served from IN2, so a board wired for microphones fails loudly instead of
 * recording the wrong pins.
 */

#include <errno.h>
#include <stdint.h>

#include <zephyr/audio/codec.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/ztest.h>

#include "cs47l63_in.h"
#include "cs47l63_regs.h"

#include "cs47l63_emul.h"

#define CODEC DEVICE_DT_GET(DT_NODELABEL(cs47l63_in))
#define EMUL  EMUL_DT_GET(DT_NODELABEL(cs47l63_in))

#define IN2_EN_BOTH     (CS47L63_IN2L_EN | CS47L63_IN2R_EN)
#define ASP1_TX_EN_BOTH (CS47L63_ASP1_TX1_EN | CS47L63_ASP1_TX2_EN)
#define ASP1_RX_EN_BOTH (CS47L63_ASP1_RX1_EN | CS47L63_ASP1_RX2_EN)

/** A transmit mixer slot: its source, at the 0 dB mix volume. */
#define MIX_SLOT(src) (((uint32_t)CS47L63_OUT1LMIX_VOL_0DB << CS47L63_OUT1LMIX_VOL_SHIFT) | (src))

/** Digital 0 dB and analog PGA 0 dB, the level the front end runs unmuted at. */
#define IN2_LEVEL_0DB                                                                              \
	(((uint32_t)CS47L63_IN2_VOL_0DB << CS47L63_IN2_VOL_SHIFT) |                                \
	 ((uint32_t)CS47L63_IN2_PGA_VOL_0DB << CS47L63_IN2_PGA_VOL_SHIFT))

static void configure(void)
{
	struct audio_codec_cfg cfg = {
		.mclk_freq = 6144000,
		.dai_type = AUDIO_DAI_TYPE_I2S,
		.dai_cfg.i2s = {
			.word_size = 16,
			.channels = 2,
			.format = I2S_FMT_DATA_FORMAT_I2S,
			.frame_clk_freq = 48000,
		},
	};

	zassert_ok(audio_codec_configure(CODEC, &cfg));
}

static void before_each(void *fixture)
{
	ARG_UNUSED(fixture);

	cs47l63_emul_reset(EMUL);
	configure();
}

ZTEST_SUITE(cs47l63_in, NULL, NULL, before_each, NULL, NULL);

ZTEST(cs47l63_in, test_configure_leaves_the_front_end_quiet)
{
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_INPUT_CONTROL) & IN2_EN_BOTH, 0,
		      "an image that only plays back still leaves the ADC pins quiet");
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_ASP1_ENABLES1) & ASP1_TX_EN_BOTH, 0,
		      "nothing reaches ASP1DOUT until a start");
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_ASP1TX1_INPUT1),
		      MIX_SLOT(CS47L63_MIXER_SRC_NONE));
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_ASP1TX2_INPUT1),
		      MIX_SLOT(CS47L63_MIXER_SRC_NONE));
	zassert_not_equal(cs47l63_emul_get_reg(EMUL, CS47L63_IN2L_CONTROL2) & CS47L63_IN2_MUTE, 0,
			  "and the front end muted");
}

ZTEST(cs47l63_in, test_start_brings_up_the_analog_line_pair)
{
	uint32_t val;

	zassert_ok(audio_codec_start(CODEC, AUDIO_DAI_DIR_RX));

	val = cs47l63_emul_get_reg(EMUL, CS47L63_INPUT2_CONTROL1);
	zassert_equal(val & CS47L63_IN2_MODE, 0,
		      "IN2_MODE 0 selects the analog input, not the PDM one");
	zassert_equal((val & CS47L63_IN2_OSR_MASK) >> CS47L63_IN2_OSR_SHIFT, CS47L63_IN2_OSR_3M072,
		      "the 48 kHz family runs the front end at 3.072 MHz oversampling");

	zassert_equal((cs47l63_emul_get_reg(EMUL, CS47L63_IN2L_CONTROL1) & CS47L63_IN2_SRC_MASK) >>
			      CS47L63_IN2_SRC_SHIFT,
		      CS47L63_IN2_SRC_SINGLE_ENDED, "the boards wire the pair single-ended");
	zassert_equal((cs47l63_emul_get_reg(EMUL, CS47L63_IN2R_CONTROL1) & CS47L63_IN2_SRC_MASK) >>
			      CS47L63_IN2_SRC_SHIFT,
		      CS47L63_IN2_SRC_SINGLE_ENDED);

	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_INPUT_CONTROL) & IN2_EN_BOTH, IN2_EN_BOTH,
		      "both halves of the pair must be enabled");
}

ZTEST(cs47l63_in, test_start_unmutes_at_0db_and_strobes_the_update)
{
	int level_idx;
	int vu_idx;

	zassert_ok(audio_codec_start(CODEC, AUDIO_DAI_DIR_RX));

	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_IN2L_CONTROL2), IN2_LEVEL_0DB,
		      "digital and analog gain at 0 dB, unmuted");
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_IN2R_CONTROL2), IN2_LEVEL_0DB);

	/* IN_VU is what latches the two volume fields. A write without it
	 * leaves them changed and the front end at the previous level.
	 */
	level_idx =
		cs47l63_emul_write_index(EMUL, CS47L63_IN2R_CONTROL2,
					 cs47l63_emul_write_count(EMUL, CS47L63_IN2R_CONTROL2) - 1);
	vu_idx = cs47l63_emul_write_index(EMUL, CS47L63_INPUT_CONTROL3,
					  cs47l63_emul_write_count(EMUL, CS47L63_INPUT_CONTROL3) -
						  1);
	zassert_true(vu_idx > level_idx, "the strobe must follow the fields it latches");
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_INPUT_CONTROL3) & CS47L63_IN_VU,
		      CS47L63_IN_VU);
}

ZTEST(cs47l63_in, test_start_routes_the_pair_to_the_two_transmit_slots)
{
	zassert_ok(audio_codec_start(CODEC, AUDIO_DAI_DIR_RX));

	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_ASP1TX1_INPUT1),
		      MIX_SLOT(CS47L63_MIXER_SRC_IN2L));
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_ASP1TX2_INPUT1),
		      MIX_SLOT(CS47L63_MIXER_SRC_IN2R));
}

ZTEST(cs47l63_in, test_start_does_not_take_the_playback_slots_down_with_it)
{
	uint32_t val;

	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_ASP1_ENABLES1) & ASP1_RX_EN_BOTH,
		      ASP1_RX_EN_BOTH, "the premise: configure() enabled the receive slots");

	zassert_ok(audio_codec_start(CODEC, AUDIO_DAI_DIR_RX));

	val = cs47l63_emul_get_reg(EMUL, CS47L63_ASP1_ENABLES1);
	zassert_equal(val & ASP1_TX_EN_BOTH, ASP1_TX_EN_BOTH, "the transmit slots come up");
	zassert_equal(val & ASP1_RX_EN_BOTH, ASP1_RX_EN_BOTH,
		      "and the receive slots in the same register survive it");
}

ZTEST(cs47l63_in, test_start_does_not_wait_for_a_status_the_clock_cannot_produce)
{
	zassert_ok(audio_codec_start(CODEC, AUDIO_DAI_DIR_RX));

	/* The caller starts the I2S transfer that feeds FLL1 only after this
	 * returns, so at this point SYSCLK does not run and no input status
	 * could ever appear.
	 */
	zassert_equal(cs47l63_emul_read_count(EMUL, CS47L63_IRQ1_STS_6), 0,
		      "the capture side has no second half to wait for");
}

ZTEST(cs47l63_in, test_stop_returns_the_stage_to_its_boot_state)
{
	zassert_ok(audio_codec_start(CODEC, AUDIO_DAI_DIR_RX));
	zassert_ok(audio_codec_stop(CODEC, AUDIO_DAI_DIR_RX));

	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_ASP1_ENABLES1) & ASP1_TX_EN_BOTH, 0);
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_ASP1_ENABLES1) & ASP1_RX_EN_BOTH,
		      ASP1_RX_EN_BOTH, "stopping capture must not stop playback");
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_INPUT_CONTROL) & IN2_EN_BOTH, 0);
	zassert_not_equal(cs47l63_emul_get_reg(EMUL, CS47L63_IN2L_CONTROL2) & CS47L63_IN2_MUTE, 0);
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_ASP1TX1_INPUT1),
		      MIX_SLOT(CS47L63_MIXER_SRC_NONE),
		      "a stop then start must not depend on the previous route");
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_ASP1TX2_INPUT1),
		      MIX_SLOT(CS47L63_MIXER_SRC_NONE));
}

ZTEST(cs47l63_in, test_route_while_running_takes_effect_immediately)
{
	zassert_ok(audio_codec_start(CODEC, AUDIO_DAI_DIR_RX));

	zassert_ok(audio_codec_route_input(CODEC, AUDIO_CHANNEL_FRONT_LEFT, CS47L63_INPUT_LINE));
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_ASP1TX1_INPUT1),
		      MIX_SLOT(CS47L63_MIXER_SRC_IN2L));
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_ASP1TX2_INPUT1),
		      MIX_SLOT(CS47L63_MIXER_SRC_IN2L),
		      "a mono source must arrive on both halves of the stereo frame");

	zassert_ok(audio_codec_route_input(CODEC, AUDIO_CHANNEL_FRONT_RIGHT, CS47L63_INPUT_LINE));
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_ASP1TX1_INPUT1),
		      MIX_SLOT(CS47L63_MIXER_SRC_IN2R));
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_ASP1TX2_INPUT1),
		      MIX_SLOT(CS47L63_MIXER_SRC_IN2R));
}

ZTEST(cs47l63_in, test_route_while_stopped_is_remembered_for_the_next_start)
{
	uint32_t writes_before = cs47l63_emul_write_count(EMUL, CS47L63_ASP1TX1_INPUT1);

	zassert_ok(audio_codec_route_input(CODEC, AUDIO_CHANNEL_FRONT_RIGHT, CS47L63_INPUT_LINE));

	zassert_equal(cs47l63_emul_write_count(EMUL, CS47L63_ASP1TX1_INPUT1), writes_before,
		      "a stopped stage must not have its mixers written");

	zassert_ok(audio_codec_start(CODEC, AUDIO_DAI_DIR_RX));

	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_ASP1TX1_INPUT1),
		      MIX_SLOT(CS47L63_MIXER_SRC_IN2R),
		      "the start must apply the route that was chosen while stopped");
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_ASP1TX2_INPUT1),
		      MIX_SLOT(CS47L63_MIXER_SRC_IN2R));
}

ZTEST(cs47l63_in, test_pdm_terminal_and_unaddressable_channel_are_refused)
{
	uint32_t writes_before;

	zassert_ok(audio_codec_start(CODEC, AUDIO_DAI_DIR_RX));
	writes_before = cs47l63_emul_write_count(EMUL, CS47L63_ASP1TX1_INPUT1);

	zassert_equal(-ENOTSUP,
		      audio_codec_route_input(CODEC, AUDIO_CHANNEL_ALL, CS47L63_INPUT_PDM),
		      "IN1 needs a bias supply and a clock this driver never configures");
	zassert_equal(-ENOTSUP,
		      audio_codec_route_input(CODEC, AUDIO_CHANNEL_REAR_LEFT, CS47L63_INPUT_LINE));

	zassert_equal(cs47l63_emul_write_count(EMUL, CS47L63_ASP1TX1_INPUT1), writes_before,
		      "a refused route must leave the mixers alone");
}
