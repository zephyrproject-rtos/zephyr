/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file test_input_path.c
 * @brief The two input terminals and the two ASP1 transmit mixers
 *
 * IN2 is the analog line pair; IN1 runs in digital mode for a PDM microphone on MICBIAS1B.
 */

#include <errno.h>
#include <stdint.h>

#include <zephyr/audio/codec.h>
#include <zephyr/device.h>
#include <zephyr/drivers/audio/cs47l63.h>
#include <zephyr/ztest.h>

#include "cs47l63.h"

#include "cs47l63_emul.h"

#define INPUT_HPF_CONTROL 0x00004244U
/** Analog PGA gain, D7-D1; not written in digital mode. */
#define IN1_PGA_VOL_MASK  0x000000FEU

#define CODEC DEVICE_DT_GET(DT_NODELABEL(cs47l63_in))
#define EMUL  EMUL_DT_GET(DT_NODELABEL(cs47l63_in))

#define IN2_EN_BOTH     (CS47L63_IN2L_EN | CS47L63_IN2R_EN)
#define IN1_EN_BOTH     (CS47L63_IN1L_EN | CS47L63_IN1R_EN)
#define MICB1B_ON       (CS47L63_MICB1B_EN | CS47L63_MICB1B_SRC)
#define ASP1_TX_EN_BOTH (CS47L63_ASP1_TX1_EN | CS47L63_ASP1_TX2_EN)
#define ASP1_RX_EN_BOTH (CS47L63_ASP1_RX1_EN | CS47L63_ASP1_RX2_EN)

#define MIX_SLOT(src) (((uint32_t)CS47L63_OUT1LMIX_VOL_0DB << CS47L63_OUT1LMIX_VOL_SHIFT) | (src))

/** Digital 0 dB and analog PGA 0 dB, the level the front end runs unmuted at. */
#define IN2_LEVEL_0DB                                                                              \
	(((uint32_t)CS47L63_IN_VOL_0DB << CS47L63_IN2_VOL_SHIFT) |                                 \
	 ((uint32_t)CS47L63_IN2_PGA_VOL_0DB << CS47L63_IN2_PGA_VOL_SHIFT))

/** Digital 0 dB, unmuted; the PGA field below it is left at its reset value. */
#define IN1_LEVEL_0DB ((uint32_t)CS47L63_IN_VOL_0DB << CS47L63_IN1_VOL_SHIFT)

/** Reset values the PDM tests start from (DS1249F2 register map). */
#define INPUT1_CONTROL1_RESET 0x00050020U
#define IN1_CONTROL2_RESET    0x10800080U
#define IRQ1_MASK_1_RESET     0x00021F10U

/** INPUT1_CONTROL1 bit 5, which must stay set (DS1249F2 section 4.2.6). */
#define INPUT1_CONTROL1_BIT5 0x00000020U

/** CLK_32K_EN with CLK_32K_SRC = 10, SYSCLK auto-divided (DS1249F2 table 4-48). */
#define CLOCK32K_ON_SYSCLK (CS47L63_CLK_32K_EN | CS47L63_CLK_32K_SRC_SYSCLK)

#define INPUT_UNKNOWN 2U

#define MIXER_SRC_IN1L 0x010U
#define MIXER_SRC_IN1R 0x011U

BUILD_ASSERT(CS47L63_MIXER_SRC_IN1L == MIXER_SRC_IN1L, "IN1L signal path, DS1249F2 mixer sources");
BUILD_ASSERT(CS47L63_MIXER_SRC_IN1R == MIXER_SRC_IN1R, "IN1R signal path");

/** @brief Log index of the last write to @p addr, or -1 when there was none. */
static int last_write(uint32_t addr)
{
	uint32_t n = cs47l63_emul_write_count(EMUL, addr);

	return (n == 0U) ? -1 : cs47l63_emul_write_index(EMUL, addr, n - 1U);
}

static void input_before_each(void *fixture)
{
	ARG_UNUSED(fixture);

	cs47l63_emul_reset(EMUL);
	cs47l63_emul_set_reg(EMUL, CS47L63_INPUT1_CONTROL1, INPUT1_CONTROL1_RESET);
	cs47l63_emul_set_reg(EMUL, CS47L63_IN1L_CONTROL2, IN1_CONTROL2_RESET);
	cs47l63_emul_set_reg(EMUL, CS47L63_IN1R_CONTROL2, IN1_CONTROL2_RESET);
	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_MASK_1, IRQ1_MASK_1_RESET);
	zassert_ok(cs47l63_test_configure(CODEC, AUDIO_ROUTE_PLAYBACK_CAPTURE));
}

static void start_pdm(audio_channel_t channel)
{
	zassert_ok(audio_codec_route_input(CODEC, channel, (uint32_t)CS47L63_INPUT_PDM));
	zassert_ok(audio_codec_start(CODEC, AUDIO_DAI_DIR_RX));
}

ZTEST_SUITE(cs47l63_in, NULL, NULL, input_before_each, NULL, NULL);

static ZTEST(cs47l63_in, test_public_header_ids_select_the_terminal)
{
	start_pdm(AUDIO_CHANNEL_ALL);
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_INPUT_CONTROL) & IN1_EN_BOTH, IN1_EN_BOTH,
		      "CS47L63_INPUT_PDM must enable IN1L/IN1R");

	zassert_ok(audio_codec_route_input(CODEC, AUDIO_CHANNEL_ALL, (uint32_t)CS47L63_INPUT_LINE));
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_INPUT_CONTROL) & IN2_EN_BOTH, IN2_EN_BOTH,
		      "CS47L63_INPUT_LINE must enable IN2L/IN2R");
}

static ZTEST(cs47l63_in, test_configure_leaves_the_front_end_quiet)
{
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_INPUT_CONTROL) & IN2_EN_BOTH, 0U,
		      "an image that only plays back still leaves the ADC pins quiet");
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_ASP1_ENABLES1) & ASP1_TX_EN_BOTH, 0U,
		      "nothing reaches ASP1DOUT until a start");
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_ASP1TX1_INPUT1),
		      MIX_SLOT(CS47L63_MIXER_SRC_NONE));
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_ASP1TX2_INPUT1),
		      MIX_SLOT(CS47L63_MIXER_SRC_NONE));
	zassert_not_equal(cs47l63_emul_get_reg(EMUL, CS47L63_IN2L_CONTROL2) & CS47L63_IN2_MUTE, 0U,
			  "and the front end muted");
}

static ZTEST(cs47l63_in, test_start_brings_up_the_analog_line_pair)
{
	uint32_t val;

	zassert_ok(audio_codec_start(CODEC, AUDIO_DAI_DIR_RX));

	val = cs47l63_emul_get_reg(EMUL, CS47L63_INPUT2_CONTROL1);
	zassert_equal(val & CS47L63_IN2_MODE, 0U,
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

static ZTEST(cs47l63_in, test_start_unmutes_at_0db_and_strobes_the_update)
{
	int level_idx;
	int vu_idx;

	zassert_ok(audio_codec_start(CODEC, AUDIO_DAI_DIR_RX));

	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_IN2L_CONTROL2), IN2_LEVEL_0DB,
		      "digital and analog gain at 0 dB, unmuted");
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_IN2R_CONTROL2), IN2_LEVEL_0DB);

	level_idx = cs47l63_emul_write_index(EMUL, CS47L63_IN2R_CONTROL2,
					     cs47l63_emul_write_count(EMUL, CS47L63_IN2R_CONTROL2) -
						     1U);
	vu_idx = cs47l63_emul_write_index(EMUL, CS47L63_INPUT_CONTROL3,
					  cs47l63_emul_write_count(EMUL, CS47L63_INPUT_CONTROL3) -
						  1U);
	zassert_true(vu_idx > level_idx, "the strobe must follow the fields it latches");
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_INPUT_CONTROL3) & CS47L63_IN_VU,
		      CS47L63_IN_VU);
}

static ZTEST(cs47l63_in, test_start_routes_the_pair_to_the_two_transmit_slots)
{
	zassert_ok(audio_codec_start(CODEC, AUDIO_DAI_DIR_RX));

	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_ASP1TX1_INPUT1),
		      MIX_SLOT(CS47L63_MIXER_SRC_IN2L));
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_ASP1TX2_INPUT1),
		      MIX_SLOT(CS47L63_MIXER_SRC_IN2R));
}

static ZTEST(cs47l63_in, test_start_does_not_take_the_playback_slots_down_with_it)
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

static ZTEST(cs47l63_in, test_start_does_not_wait_for_a_status_the_clock_cannot_produce)
{
	zassert_ok(audio_codec_start(CODEC, AUDIO_DAI_DIR_RX));

	zassert_equal(cs47l63_emul_read_count(EMUL, CS47L63_IRQ1_STS_6), 0U,
		      "the capture side has no second half to wait for");
}

static ZTEST(cs47l63_in, test_stop_returns_the_stage_to_its_boot_state)
{
	zassert_ok(audio_codec_start(CODEC, AUDIO_DAI_DIR_RX));
	zassert_ok(audio_codec_stop(CODEC, AUDIO_DAI_DIR_RX));

	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_ASP1_ENABLES1) & ASP1_TX_EN_BOTH, 0U);
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_ASP1_ENABLES1) & ASP1_RX_EN_BOTH,
		      ASP1_RX_EN_BOTH, "stopping capture must not stop playback");
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_INPUT_CONTROL) & IN2_EN_BOTH, 0U);
	zassert_not_equal(cs47l63_emul_get_reg(EMUL, CS47L63_IN2L_CONTROL2) & CS47L63_IN2_MUTE, 0U);
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_ASP1TX1_INPUT1),
		      MIX_SLOT(CS47L63_MIXER_SRC_NONE),
		      "a stop then start must not depend on the previous route");
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_ASP1TX2_INPUT1),
		      MIX_SLOT(CS47L63_MIXER_SRC_NONE));
}

static ZTEST(cs47l63_in, test_route_while_running_takes_effect_immediately)
{
	zassert_ok(audio_codec_start(CODEC, AUDIO_DAI_DIR_RX));

	zassert_ok(audio_codec_route_input(CODEC, AUDIO_CHANNEL_FRONT_LEFT,
					   (uint32_t)CS47L63_INPUT_LINE));
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_ASP1TX1_INPUT1),
		      MIX_SLOT(CS47L63_MIXER_SRC_IN2L));
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_ASP1TX2_INPUT1),
		      MIX_SLOT(CS47L63_MIXER_SRC_IN2L),
		      "a mono source must arrive on both halves of the stereo frame");

	zassert_ok(audio_codec_route_input(CODEC, AUDIO_CHANNEL_FRONT_RIGHT,
					   (uint32_t)CS47L63_INPUT_LINE));
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_ASP1TX1_INPUT1),
		      MIX_SLOT(CS47L63_MIXER_SRC_IN2R));
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_ASP1TX2_INPUT1),
		      MIX_SLOT(CS47L63_MIXER_SRC_IN2R));
}

static ZTEST(cs47l63_in, test_route_while_stopped_is_remembered_for_the_next_start)
{
	uint32_t writes_before = cs47l63_emul_write_count(EMUL, CS47L63_ASP1TX1_INPUT1);

	zassert_ok(audio_codec_route_input(CODEC, AUDIO_CHANNEL_FRONT_RIGHT,
					   (uint32_t)CS47L63_INPUT_LINE));

	zassert_equal(cs47l63_emul_write_count(EMUL, CS47L63_ASP1TX1_INPUT1), writes_before,
		      "a stopped stage must not have its mixers written");

	zassert_ok(audio_codec_start(CODEC, AUDIO_DAI_DIR_RX));

	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_ASP1TX1_INPUT1),
		      MIX_SLOT(CS47L63_MIXER_SRC_IN2R),
		      "the start must apply the route that was chosen while stopped");
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_ASP1TX2_INPUT1),
		      MIX_SLOT(CS47L63_MIXER_SRC_IN2R));
}

static ZTEST(cs47l63_in, test_unknown_terminal_and_unaddressable_channel_are_refused)
{
	uint32_t xfers_before;

	zassert_ok(audio_codec_start(CODEC, AUDIO_DAI_DIR_RX));
	xfers_before = cs47l63_emul_xfer_count(EMUL);

	zassert_equal(-ENOTSUP, audio_codec_route_input(CODEC, AUDIO_CHANNEL_ALL, INPUT_UNKNOWN),
		      "a terminal the driver cannot name is refused, not served from IN2");
	zassert_equal(-ENOTSUP, audio_codec_route_input(CODEC, AUDIO_CHANNEL_REAR_LEFT,
							(uint32_t)CS47L63_INPUT_LINE));
	zassert_equal(-ENOTSUP, audio_codec_route_input(CODEC, AUDIO_CHANNEL_REAR_LEFT,
							(uint32_t)CS47L63_INPUT_PDM));

	zassert_equal(cs47l63_emul_xfer_count(EMUL), xfers_before,
		      "a refused route must not touch the part at all");
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_INPUT_CONTROL) & IN2_EN_BOTH, IN2_EN_BOTH,
		      "and the running line pair keeps running");
}

static ZTEST(cs47l63_in, test_configure_leaves_the_microphone_path_quiet)
{
	/* A part left with the microphone up by a previous configuration. */
	cs47l63_emul_set_reg(EMUL, CS47L63_MICBIAS_CTRL5, MICB1B_ON);
	cs47l63_emul_set_reg(EMUL, CS47L63_INPUT_CONTROL, IN1_EN_BOTH);
	cs47l63_emul_set_reg(EMUL, CS47L63_IN1L_CONTROL2, IN1_LEVEL_0DB);
	cs47l63_emul_set_reg(EMUL, CS47L63_IN1R_CONTROL2, IN1_LEVEL_0DB);

	zassert_ok(cs47l63_test_configure(CODEC, AUDIO_ROUTE_PLAYBACK_CAPTURE));

	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_MICBIAS_CTRL5) & MICB1B_ON, 0U,
		      "MICBIAS1B off");
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_INPUT_CONTROL) & IN1_EN_BOTH, 0U,
		      "IN1 disabled");
	zassert_not_equal(cs47l63_emul_get_reg(EMUL, CS47L63_IN1L_CONTROL2) & CS47L63_IN1_MUTE, 0U,
			  "IN1L muted");
	zassert_not_equal(cs47l63_emul_get_reg(EMUL, CS47L63_IN1R_CONTROL2) & CS47L63_IN1_MUTE, 0U,
			  "IN1R muted");
}

static ZTEST(cs47l63_in, test_pdm_start_writes_the_bring_up_sequence_in_order)
{
	int clk32k_idx;
	int micbias_idx;
	int mode_idx;
	int level_l_idx;
	int level_r_idx;
	int vu_idx;
	int enable_idx;
	int mixer_idx;
	int slots_idx;

	/* IN1_OSR seeded at 000, not its reset 101, so the OSR write has to be
	 * made rather than inherited; bit 5 stays set as the part requires.
	 */
	cs47l63_emul_set_reg(EMUL, CS47L63_INPUT1_CONTROL1, INPUT1_CONTROL1_BIT5);

	start_pdm(AUDIO_CHANNEL_ALL);

	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_CLOCK32K) &
			      (CS47L63_CLK_32K_EN | CS47L63_CLK_32K_SRC_MASK),
		      CLOCK32K_ON_SYSCLK,
		      "the MICBIAS current limit runs from the 32 kHz clock, taken from SYSCLK");

	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_MICBIAS_CTRL5) & MICB1B_ON, MICB1B_ON,
		      "MICBIAS1B on, sourced from VDD_A: the DK grounds VDD_LDO");
	zassert_equal(
		(cs47l63_emul_get_reg(EMUL, CS47L63_INPUT1_CONTROL1) & CS47L63_IN1_OSR_MASK) >>
			CS47L63_IN1_OSR_SHIFT,
		CS47L63_IN1_OSR_3M072, "IN1_OSR 101: a 3.072 MHz PDM clock");
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_INPUT1_CONTROL1),
		      INPUT1_CONTROL1_BIT5 | (CS47L63_IN1_OSR_3M072 << CS47L63_IN1_OSR_SHIFT) |
			      CS47L63_IN1_MODE,
		      "digital mode at OSR 101, every other bit as it was");
	zassert_not_equal(cs47l63_emul_get_reg(EMUL, CS47L63_INPUT1_CONTROL1) &
				  INPUT1_CONTROL1_BIT5,
			  0U, "bit 5 must stay set at all times");
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_IN1L_CONTROL2),
		      IN1_LEVEL_0DB | (IN1_CONTROL2_RESET & IN1_PGA_VOL_MASK),
		      "unmuted at 0 dB, the PGA field untouched");
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_IN1R_CONTROL2),
		      IN1_LEVEL_0DB | (IN1_CONTROL2_RESET & IN1_PGA_VOL_MASK));
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_INPUT_CONTROL) & IN1_EN_BOTH, IN1_EN_BOTH);
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_INPUT_CONTROL) & IN2_EN_BOTH, 0U,
		      "the line pair stays down");
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_ASP1_ENABLES1) & ASP1_TX_EN_BOTH,
		      ASP1_TX_EN_BOTH);

	clk32k_idx = last_write(CS47L63_CLOCK32K);
	micbias_idx = last_write(CS47L63_MICBIAS_CTRL5);
	mode_idx = last_write(CS47L63_INPUT1_CONTROL1);
	level_l_idx = last_write(CS47L63_IN1L_CONTROL2);
	level_r_idx = last_write(CS47L63_IN1R_CONTROL2);
	vu_idx = last_write(CS47L63_INPUT_CONTROL3);
	enable_idx = last_write(CS47L63_INPUT_CONTROL);
	mixer_idx = last_write(CS47L63_ASP1TX2_INPUT1);
	slots_idx = last_write(CS47L63_ASP1_ENABLES1);

	zassert_true((clk32k_idx >= 0) && (clk32k_idx < micbias_idx),
		     "the 32 kHz clock is enabled before MICBIAS1B");
	zassert_true(micbias_idx < mode_idx, "the microphone is powered before its clock starts");
	zassert_true((mode_idx < level_l_idx) && (mode_idx < level_r_idx),
		     "digital mode before the level");
	zassert_true((level_l_idx < vu_idx) && (level_r_idx < vu_idx) && (vu_idx < enable_idx),
		     "both halves unmuted and latched before the path is enabled");
	zassert_true(enable_idx < mixer_idx, "the path enabled before the mixers take it");
	zassert_true(mixer_idx < slots_idx, "the transmit slots enabled last");
}

static ZTEST(cs47l63_in, test_pdm_route_maps_left_right_and_all_like_the_line_pair)
{
	start_pdm(AUDIO_CHANNEL_ALL);
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_ASP1TX1_INPUT1),
		      MIX_SLOT(CS47L63_MIXER_SRC_IN1L));
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_ASP1TX2_INPUT1),
		      MIX_SLOT(CS47L63_MIXER_SRC_IN1R));

	zassert_ok(audio_codec_route_input(CODEC, AUDIO_CHANNEL_FRONT_LEFT,
					   (uint32_t)CS47L63_INPUT_PDM));
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_ASP1TX1_INPUT1),
		      MIX_SLOT(CS47L63_MIXER_SRC_IN1L));
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_ASP1TX2_INPUT1),
		      MIX_SLOT(CS47L63_MIXER_SRC_IN1L), "a mono microphone reaches both slots");

	zassert_ok(audio_codec_route_input(CODEC, AUDIO_CHANNEL_FRONT_RIGHT,
					   (uint32_t)CS47L63_INPUT_PDM));
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_ASP1TX1_INPUT1),
		      MIX_SLOT(CS47L63_MIXER_SRC_IN1R));
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_ASP1TX2_INPUT1),
		      MIX_SLOT(CS47L63_MIXER_SRC_IN1R));
}

static ZTEST(cs47l63_in, test_pdm_route_within_the_terminal_only_rewrites_the_mixers)
{
	uint32_t micbias_writes;
	uint32_t enable_writes;

	start_pdm(AUDIO_CHANNEL_ALL);
	micbias_writes = cs47l63_emul_write_count(EMUL, CS47L63_MICBIAS_CTRL5);
	enable_writes = cs47l63_emul_write_count(EMUL, CS47L63_INPUT_CONTROL);

	zassert_ok(audio_codec_route_input(CODEC, AUDIO_CHANNEL_FRONT_LEFT,
					   (uint32_t)CS47L63_INPUT_PDM));

	zassert_equal(cs47l63_emul_write_count(EMUL, CS47L63_MICBIAS_CTRL5), micbias_writes,
		      "the microphone supply is not cycled by a channel change");
	zassert_equal(cs47l63_emul_write_count(EMUL, CS47L63_INPUT_CONTROL), enable_writes,
		      "nor is the path");
}

static ZTEST(cs47l63_in, test_pdm_stop_masks_the_short_circuit_flag_around_micbias_off)
{
	uint32_t unmasked = IRQ1_MASK_1_RESET & ~CS47L63_MICB_SC_MASK1;
	uint32_t mask_writes;
	uint32_t micbias_writes;
	uint32_t eint_writes;
	uint32_t val;
	int mask_idx = -1;
	int off_idx = -1;
	int clear_idx = -1;
	int restore_idx;

	/* Start from MICB_SC unmasked, as a host servicing the IRQ would have
	 * it, so the mask the stop puts up is a write that can be seen.
	 */
	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_MASK_1, unmasked);
	start_pdm(AUDIO_CHANNEL_ALL);
	zassert_ok(audio_codec_stop(CODEC, AUDIO_DAI_DIR_RX));

	mask_writes = cs47l63_emul_write_count(EMUL, CS47L63_IRQ1_MASK_1);
	for (uint32_t n = 0U; n < mask_writes; n++) {
		zassert_true(cs47l63_emul_nth_write(EMUL, CS47L63_IRQ1_MASK_1, n, &val));
		if ((val & CS47L63_MICB_SC_MASK1) != 0U) {
			mask_idx = cs47l63_emul_write_index(EMUL, CS47L63_IRQ1_MASK_1, n);
			break;
		}
	}
	micbias_writes = cs47l63_emul_write_count(EMUL, CS47L63_MICBIAS_CTRL5);
	for (uint32_t n = 0U; n < micbias_writes; n++) {
		zassert_true(cs47l63_emul_nth_write(EMUL, CS47L63_MICBIAS_CTRL5, n, &val));
		if ((val & CS47L63_MICB1B_EN) == 0U) {
			off_idx = cs47l63_emul_write_index(EMUL, CS47L63_MICBIAS_CTRL5, n);
		}
	}
	eint_writes = cs47l63_emul_write_count(EMUL, CS47L63_IRQ1_EINT_1);
	for (uint32_t n = 0U; n < eint_writes; n++) {
		zassert_true(cs47l63_emul_nth_write(EMUL, CS47L63_IRQ1_EINT_1, n, &val));
		if ((val & CS47L63_MICB_SC_EINT1) != 0U) {
			clear_idx = cs47l63_emul_write_index(EMUL, CS47L63_IRQ1_EINT_1, n);
		}
	}
	restore_idx = last_write(CS47L63_IRQ1_MASK_1);

	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_CLOCK32K) & CS47L63_CLK_32K_EN, 0U,
		      "the 32 kHz clock goes down with the supply it served");
	zassert_true(last_write(CS47L63_CLOCK32K) > off_idx, "and only after MICBIAS1B is off");
	zassert_true((mask_idx >= 0) && (mask_idx < off_idx),
		     "MICB_SC masked before MICBIAS1B goes down: disabling it raises the flag");
	zassert_true(off_idx < clear_idx, "the flag the disable raised is cleared");
	zassert_true(clear_idx < restore_idx, "and only then is the mask put back");
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_IRQ1_MASK_1), unmasked,
		      "the interrupt mask ends as it was found");
}

static ZTEST(cs47l63_in, test_pdm_stop_returns_the_stage_to_its_boot_state)
{
	start_pdm(AUDIO_CHANNEL_ALL);
	zassert_ok(audio_codec_stop(CODEC, AUDIO_DAI_DIR_RX));

	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_ASP1_ENABLES1) & ASP1_TX_EN_BOTH, 0U);
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_ASP1_ENABLES1) & ASP1_RX_EN_BOTH,
		      ASP1_RX_EN_BOTH, "stopping capture must not stop playback");
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_INPUT_CONTROL) & IN1_EN_BOTH, 0U);
	zassert_not_equal(cs47l63_emul_get_reg(EMUL, CS47L63_IN1L_CONTROL2) & CS47L63_IN1_MUTE, 0U);
	zassert_not_equal(cs47l63_emul_get_reg(EMUL, CS47L63_IN1R_CONTROL2) & CS47L63_IN1_MUTE, 0U);
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_MICBIAS_CTRL5) & MICB1B_ON, 0U);
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_IRQ1_MASK_1), IRQ1_MASK_1_RESET,
		      "MICB_SC left masked, as the part resets");
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_ASP1TX1_INPUT1),
		      MIX_SLOT(CS47L63_MIXER_SRC_NONE));
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_ASP1TX2_INPUT1),
		      MIX_SLOT(CS47L63_MIXER_SRC_NONE));
}

static ZTEST(cs47l63_in, test_switching_line_to_pdm_while_running_moves_the_path)
{
	zassert_ok(audio_codec_start(CODEC, AUDIO_DAI_DIR_RX));
	zassert_ok(audio_codec_route_input(CODEC, AUDIO_CHANNEL_ALL, (uint32_t)CS47L63_INPUT_PDM));

	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_INPUT_CONTROL) & IN2_EN_BOTH, 0U,
		      "the line pair is taken down");
	zassert_not_equal(cs47l63_emul_get_reg(EMUL, CS47L63_IN2L_CONTROL2) & CS47L63_IN2_MUTE, 0U,
			  "and muted");
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_INPUT_CONTROL) & IN1_EN_BOTH, IN1_EN_BOTH,
		      "the microphone path runs in its place");
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_MICBIAS_CTRL5) & MICB1B_ON, MICB1B_ON);
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_ASP1TX1_INPUT1),
		      MIX_SLOT(CS47L63_MIXER_SRC_IN1L));
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_ASP1_ENABLES1) & ASP1_TX_EN_BOTH,
		      ASP1_TX_EN_BOTH, "still streaming");

	zassert_ok(audio_codec_route_input(CODEC, AUDIO_CHANNEL_ALL, (uint32_t)CS47L63_INPUT_LINE));

	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_INPUT_CONTROL) & IN1_EN_BOTH, 0U,
		      "and back: the microphone path down");
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_MICBIAS_CTRL5) & MICB1B_ON, 0U,
		      "its supply off");
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_INPUT_CONTROL) & IN2_EN_BOTH, IN2_EN_BOTH,
		      "the line pair running again");
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_ASP1TX1_INPUT1),
		      MIX_SLOT(CS47L63_MIXER_SRC_IN2L));
}

static ZTEST(cs47l63_in, test_pdm_bus_error_propagates_and_stops_the_sequence)
{
	uint32_t enable_writes = cs47l63_emul_write_count(EMUL, CS47L63_INPUT_CONTROL);
	uint32_t micbias_writes = cs47l63_emul_write_count(EMUL, CS47L63_MICBIAS_CTRL5);

	zassert_ok(audio_codec_route_input(CODEC, AUDIO_CHANNEL_ALL, (uint32_t)CS47L63_INPUT_PDM));
	cs47l63_emul_fail_at(EMUL, CS47L63_INPUT1_CONTROL1);

	zassert_equal(-EIO, audio_codec_start(CODEC, AUDIO_DAI_DIR_RX));

	/* The step before INPUT1_CONTROL1 ran, so the injected failure was reached. */
	zassert_true(cs47l63_emul_write_count(EMUL, CS47L63_MICBIAS_CTRL5) > micbias_writes,
		     "MICBIAS1B was switched on before the failing step");
	zassert_equal(cs47l63_emul_read_count(EMUL, CS47L63_INPUT1_CONTROL1), 0U,
		      "the failed read of INPUT1_CONTROL1 was not retried into success");
	zassert_equal(cs47l63_emul_write_count(EMUL, CS47L63_INPUT_CONTROL), enable_writes,
		      "nothing after the failing step is written, and nothing is retried");
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_ASP1_ENABLES1) & ASP1_TX_EN_BOTH, 0U);

	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_MICBIAS_CTRL5) & MICB1B_ON, 0U,
		      "MICBIAS1B is off again after the failure");
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_CLOCK32K) & CS47L63_CLK_32K_EN, 0U,
		      "and so is the 32 kHz clock");
}

static ZTEST(cs47l63_in, test_pdm_failure_enabling_micbias_rolls_the_32k_clock_back)
{
	zassert_ok(audio_codec_route_input(CODEC, AUDIO_CHANNEL_ALL, (uint32_t)CS47L63_INPUT_PDM));
	cs47l63_emul_fail_at(EMUL, CS47L63_MICBIAS_CTRL5);

	zassert_equal(-EIO, audio_codec_start(CODEC, AUDIO_DAI_DIR_RX));

	zassert_true(cs47l63_emul_write_count(EMUL, CS47L63_CLOCK32K) >= 1U,
		     "the 32 kHz clock was enabled before the failing MICBIAS write");
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_CLOCK32K) & CS47L63_CLK_32K_EN, 0U,
		      "and is off again after it");
}

static ZTEST(cs47l63_in, test_line_path_never_touches_the_32k_clock)
{
	zassert_ok(audio_codec_start(CODEC, AUDIO_DAI_DIR_RX));
	zassert_ok(audio_codec_route_input(CODEC, AUDIO_CHANNEL_FRONT_LEFT,
					   (uint32_t)CS47L63_INPUT_LINE));
	zassert_ok(audio_codec_stop(CODEC, AUDIO_DAI_DIR_RX));

	/* Counted from the emulator reset, so configure() is included. */
	zassert_equal(cs47l63_emul_write_count(EMUL, CS47L63_CLOCK32K), 0U);
	zassert_equal(cs47l63_emul_read_count(EMUL, CS47L63_CLOCK32K), 0U);
}

/* Input level: whole dB, clamped (DS1249F2 table 4-5). */

#define IN_VOL_CODE_P31DB 0xBEU
#define IN_VOL_CODE_M6DB  0x74U
#define IN_VOL_CODE_M10DB 0x6CU
#define IN_VOL_CODE_M12DB 0x68U
#define IN_VOL_CODE_M20DB 0x58U
#define IN_VOL_CODE_M64DB 0x00U

#define IN_VOL_M6DB         (-6)
#define IN_VOL_M10DB        (-10)
#define IN_VOL_M12DB        (-12)
#define IN_VOL_M20DB        (-20)
#define IN_VOL_ABOVE_MAX_DB 100
#define IN_VOL_BELOW_MIN_DB (-100)

static int set_in_volume(audio_channel_t channel, int db)
{
	audio_property_value_t val = {.vol = db};

	return audio_codec_set_property(CODEC, AUDIO_PROPERTY_INPUT_VOLUME, channel, val);
}

static int set_in_mute(audio_channel_t channel, bool mute)
{
	audio_property_value_t val = {.mute = mute};

	return audio_codec_set_property(CODEC, AUDIO_PROPERTY_INPUT_MUTE, channel, val);
}

static uint32_t vol_field(uint32_t addr)
{
	return (cs47l63_emul_get_reg(EMUL, addr) & CS47L63_IN1_VOL_MASK) >> CS47L63_IN1_VOL_SHIFT;
}

static bool muted(uint32_t addr)
{
	return (cs47l63_emul_get_reg(EMUL, addr) & CS47L63_IN1_MUTE) != 0U;
}

/** Writes to every register an input level or mute could land in. */
static uint32_t level_writes(void)
{
	return cs47l63_emul_write_count(EMUL, CS47L63_IN1L_CONTROL2) +
	       cs47l63_emul_write_count(EMUL, CS47L63_IN1R_CONTROL2) +
	       cs47l63_emul_write_count(EMUL, CS47L63_IN2L_CONTROL2) +
	       cs47l63_emul_write_count(EMUL, CS47L63_IN2R_CONTROL2) +
	       cs47l63_emul_write_count(EMUL, CS47L63_INPUT_CONTROL3);
}

static ZTEST(cs47l63_in, test_input_level_set_while_stopped_is_only_cached)
{
	uint32_t writes_before = level_writes();

	zassert_ok(set_in_volume(AUDIO_CHANNEL_ALL, IN_VOL_M20DB));
	zassert_ok(set_in_mute(AUDIO_CHANNEL_ALL, true));

	zassert_equal(level_writes(), writes_before, "a stopped input must not be written");

	zassert_ok(audio_codec_start(CODEC, AUDIO_DAI_DIR_RX));

	zassert_equal(vol_field(CS47L63_IN2L_CONTROL2), IN_VOL_CODE_M20DB,
		      "the start applies the cached level");
	zassert_equal(vol_field(CS47L63_IN2R_CONTROL2), IN_VOL_CODE_M20DB);
	zassert_true(muted(CS47L63_IN2L_CONTROL2), "and the cached mute");
	zassert_true(muted(CS47L63_IN2R_CONTROL2));
}

static ZTEST(cs47l63_in, test_input_level_set_while_running_goes_to_the_active_terminal)
{
	uint32_t in1_writes;
	int level_idx;

	zassert_ok(audio_codec_start(CODEC, AUDIO_DAI_DIR_RX));
	in1_writes = cs47l63_emul_write_count(EMUL, CS47L63_IN1L_CONTROL2) +
		     cs47l63_emul_write_count(EMUL, CS47L63_IN1R_CONTROL2);

	zassert_ok(set_in_volume(AUDIO_CHANNEL_ALL, IN_VOL_M10DB));

	zassert_equal(vol_field(CS47L63_IN2L_CONTROL2), IN_VOL_CODE_M10DB);
	zassert_equal(vol_field(CS47L63_IN2R_CONTROL2), IN_VOL_CODE_M10DB);
	zassert_false(muted(CS47L63_IN2L_CONTROL2));
	level_idx = last_write(CS47L63_IN2R_CONTROL2);
	zassert_true(last_write(CS47L63_INPUT_CONTROL3) > level_idx, "IN_VU latches the new level");
	zassert_equal(cs47l63_emul_write_count(EMUL, CS47L63_IN1L_CONTROL2) +
			      cs47l63_emul_write_count(EMUL, CS47L63_IN1R_CONTROL2),
		      in1_writes, "the idle microphone path is not written");

	zassert_ok(audio_codec_route_input(CODEC, AUDIO_CHANNEL_ALL, (uint32_t)CS47L63_INPUT_PDM));
	zassert_ok(set_in_mute(AUDIO_CHANNEL_ALL, true));

	zassert_true(muted(CS47L63_IN1L_CONTROL2), "PDM running: the mute lands on IN1");
	zassert_true(muted(CS47L63_IN1R_CONTROL2));
	zassert_true(last_write(CS47L63_INPUT_CONTROL3) > last_write(CS47L63_IN1R_CONTROL2));
	zassert_equal(vol_field(CS47L63_IN1L_CONTROL2), IN_VOL_CODE_M10DB,
		      "at the level already set");
}

static ZTEST(cs47l63_in, test_pdm_start_applies_the_cached_level_in_bring_up_order)
{
	zassert_ok(set_in_volume(AUDIO_CHANNEL_ALL, IN_VOL_M6DB));

	start_pdm(AUDIO_CHANNEL_ALL);

	zassert_equal(vol_field(CS47L63_IN1L_CONTROL2), IN_VOL_CODE_M6DB);
	zassert_equal(vol_field(CS47L63_IN1R_CONTROL2), IN_VOL_CODE_M6DB);
	zassert_false(muted(CS47L63_IN1L_CONTROL2));
	zassert_false(muted(CS47L63_IN1R_CONTROL2));
	zassert_true(
		(last_write(CS47L63_IN1L_CONTROL2) < last_write(CS47L63_INPUT_CONTROL)) &&
			(last_write(CS47L63_IN1R_CONTROL2) < last_write(CS47L63_INPUT_CONTROL)),
		"the level is set before the path is enabled, as without a cached level");
	zassert_true(last_write(CS47L63_INPUT_CONTROL3) < last_write(CS47L63_INPUT_CONTROL));
}

static ZTEST(cs47l63_in, test_stop_mutes_the_part_but_keeps_the_user_mute_state)
{
	zassert_ok(audio_codec_start(CODEC, AUDIO_DAI_DIR_RX));
	zassert_ok(audio_codec_stop(CODEC, AUDIO_DAI_DIR_RX));
	zassert_true(muted(CS47L63_IN2L_CONTROL2), "a stopped input is muted on the part");

	zassert_ok(audio_codec_start(CODEC, AUDIO_DAI_DIR_RX));
	zassert_false(muted(CS47L63_IN2L_CONTROL2),
		      "the stop's mute is not the user's: the restart is unmuted");

	zassert_ok(set_in_mute(AUDIO_CHANNEL_ALL, true));
	zassert_ok(audio_codec_stop(CODEC, AUDIO_DAI_DIR_RX));
	zassert_ok(audio_codec_start(CODEC, AUDIO_DAI_DIR_RX));
	zassert_true(muted(CS47L63_IN2L_CONTROL2), "and a user mute survives the stop");
	zassert_true(muted(CS47L63_IN2R_CONTROL2));
}

static ZTEST(cs47l63_in, test_route_switch_carries_the_level_to_the_new_terminal)
{
	zassert_ok(audio_codec_start(CODEC, AUDIO_DAI_DIR_RX));
	zassert_ok(set_in_volume(AUDIO_CHANNEL_ALL, IN_VOL_M12DB));

	zassert_ok(audio_codec_route_input(CODEC, AUDIO_CHANNEL_ALL, (uint32_t)CS47L63_INPUT_PDM));

	zassert_equal(vol_field(CS47L63_IN1L_CONTROL2), IN_VOL_CODE_M12DB);
	zassert_equal(vol_field(CS47L63_IN1R_CONTROL2), IN_VOL_CODE_M12DB);
	zassert_false(muted(CS47L63_IN1L_CONTROL2));
	zassert_true(muted(CS47L63_IN2L_CONTROL2), "the line pair it left is muted");
}

static ZTEST(cs47l63_in, test_input_property_on_one_channel_refused_with_nothing_written)
{
	uint32_t xfers_before;

	zassert_ok(audio_codec_start(CODEC, AUDIO_DAI_DIR_RX));
	xfers_before = cs47l63_emul_xfer_count(EMUL);

	zassert_equal(-EINVAL, set_in_volume(AUDIO_CHANNEL_FRONT_LEFT, IN_VOL_M6DB));
	zassert_equal(-EINVAL, set_in_mute(AUDIO_CHANNEL_FRONT_RIGHT, true));

	zassert_equal(cs47l63_emul_xfer_count(EMUL), xfers_before,
		      "a refused property must not touch the part");
}

static ZTEST(cs47l63_in, test_input_volume_is_clamped_to_the_field_envelope)
{
	zassert_ok(audio_codec_start(CODEC, AUDIO_DAI_DIR_RX));

	zassert_ok(set_in_volume(AUDIO_CHANNEL_ALL, IN_VOL_ABOVE_MAX_DB));
	zassert_equal(vol_field(CS47L63_IN2L_CONTROL2), IN_VOL_CODE_P31DB,
		      "+31 dB, the top whole dB below the reserved codes 0xC0-0xFF");

	zassert_ok(set_in_volume(AUDIO_CHANNEL_ALL, IN_VOL_BELOW_MIN_DB));
	zassert_equal(vol_field(CS47L63_IN2L_CONTROL2), IN_VOL_CODE_M64DB, "-64 dB, never wrapped");
}

static ZTEST(cs47l63_in, test_configure_enables_the_input_hpf_on_both_paths)
{
	static const uint32_t k_control1[] = {
		CS47L63_IN1L_CONTROL1,
		CS47L63_IN1R_CONTROL1,
		CS47L63_IN2L_CONTROL1,
		CS47L63_IN2R_CONTROL1,
	};

	for (size_t i = 0U; i < ARRAY_SIZE(k_control1); i++) {
		zassert_not_equal(cs47l63_emul_get_reg(EMUL, k_control1[i]) & CS47L63_IN_HPF, 0U,
				  "HPF enabled on 0x%05x", k_control1[i]);
	}
	zassert_equal(cs47l63_emul_write_count(EMUL, INPUT_HPF_CONTROL), 0U,
		      "the cut-off stays at its reset value");

	/* The line pair's start rewrites IN2n_CONTROL1 for its source field. */
	zassert_ok(audio_codec_start(CODEC, AUDIO_DAI_DIR_RX));
	zassert_true(cs47l63_emul_write_count(EMUL, CS47L63_IN2L_CONTROL1) >= 2U,
		     "the premise: the start wrote IN2L_CONTROL1 after configure did");
	zassert_not_equal(cs47l63_emul_get_reg(EMUL, CS47L63_IN2L_CONTROL1) & CS47L63_IN_HPF, 0U,
			  "the line start keeps the HPF on IN2L");
	zassert_not_equal(cs47l63_emul_get_reg(EMUL, CS47L63_IN2R_CONTROL1) & CS47L63_IN_HPF, 0U,
			  "and on IN2R");
	zassert_ok(audio_codec_stop(CODEC, AUDIO_DAI_DIR_RX));

	start_pdm(AUDIO_CHANNEL_ALL);
	zassert_ok(audio_codec_stop(CODEC, AUDIO_DAI_DIR_RX));
	zassert_ok(audio_codec_start(CODEC, AUDIO_DAI_DIR_RX));
	for (size_t i = 0U; i < ARRAY_SIZE(k_control1); i++) {
		zassert_not_equal(cs47l63_emul_get_reg(EMUL, k_control1[i]) & CS47L63_IN_HPF, 0U,
				  "a start does not take the HPF off 0x%05x", k_control1[i]);
	}
}

/** @brief Whether any write to INPUT_CONTROL ever set all of @p bits. */
static bool was_enabled(uint32_t bits)
{
	const uint32_t writes = cs47l63_emul_write_count(EMUL, CS47L63_INPUT_CONTROL);
	uint32_t val;

	for (uint32_t n = 0U; n < writes; n++) {
		zassert_true(cs47l63_emul_nth_write(EMUL, CS47L63_INPUT_CONTROL, n, &val));
		if ((val & bits) == bits) {
			return true;
		}
	}

	return false;
}

static ZTEST(cs47l63_in, test_failed_line_start_leaves_the_pair_stopped)
{
	cs47l63_emul_fail_at(EMUL, CS47L63_ASP1_ENABLES1);

	zassert_equal(-EIO, audio_codec_start(CODEC, AUDIO_DAI_DIR_RX),
		      "the transmit-slot enable is the failing step");
	zassert_true(was_enabled(IN2_EN_BOTH), "the premise: the pair was enabled before it");

	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_INPUT_CONTROL) & IN2_EN_BOTH, 0U,
		      "and is disabled again");
	zassert_true(muted(CS47L63_IN2L_CONTROL2), "and muted");
	zassert_true(muted(CS47L63_IN2R_CONTROL2));
}

static ZTEST(cs47l63_in, test_failed_pdm_start_leaves_the_microphone_stopped)
{
	zassert_ok(audio_codec_route_input(CODEC, AUDIO_CHANNEL_ALL, (uint32_t)CS47L63_INPUT_PDM));
	cs47l63_emul_fail_at(EMUL, CS47L63_ASP1_ENABLES1);

	zassert_equal(-EIO, audio_codec_start(CODEC, AUDIO_DAI_DIR_RX));
	zassert_true(was_enabled(IN1_EN_BOTH), "the premise: IN1 was enabled before it");

	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_INPUT_CONTROL) & IN1_EN_BOTH, 0U);
	zassert_true(muted(CS47L63_IN1L_CONTROL2));
	zassert_true(muted(CS47L63_IN1R_CONTROL2));
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_MICBIAS_CTRL5) & MICB1B_ON, 0U,
		      "the microphone supply is off");
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_CLOCK32K) & CS47L63_CLK_32K_EN, 0U);
}

static ZTEST(cs47l63_in, test_failed_output_start_does_not_leave_the_input_it_started_running)
{
	cs47l63_emul_fail_at(EMUL, CS47L63_OUTPUT_ENABLE_1);

	zassert_equal(-EIO, audio_codec_start(CODEC, AUDIO_DAI_DIR_RX | AUDIO_DAI_DIR_TX),
		      "the output enable is the failing step");
	zassert_true(was_enabled(IN2_EN_BOTH), "the premise: the pair was enabled before it");

	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_INPUT_CONTROL) & IN2_EN_BOTH, 0U,
		      "the pair this call enabled is disabled again");
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_ASP1_ENABLES1) & ASP1_TX_EN_BOTH, 0U);
}

static ZTEST(cs47l63_in, test_failed_output_start_keeps_the_input_that_was_already_running)
{
	zassert_ok(audio_codec_start(CODEC, AUDIO_DAI_DIR_RX));
	cs47l63_emul_fail_at(EMUL, CS47L63_OUTPUT_ENABLE_1);

	zassert_equal(-EIO, audio_codec_start(CODEC, AUDIO_DAI_DIR_RX | AUDIO_DAI_DIR_TX));

	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_INPUT_CONTROL) & IN2_EN_BOTH, IN2_EN_BOTH,
		      "a capture that was running before the call keeps running");
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_ASP1_ENABLES1) & ASP1_TX_EN_BOTH,
		      ASP1_TX_EN_BOTH);
}

static ZTEST(cs47l63_in, test_failed_route_switch_leaves_both_paths_stopped)
{
	zassert_ok(audio_codec_start(CODEC, AUDIO_DAI_DIR_RX));
	cs47l63_emul_fail_at(EMUL, CS47L63_INPUT1_CONTROL1);

	zassert_equal(-EIO, audio_codec_route_input(CODEC, AUDIO_CHANNEL_ALL,
						    (uint32_t)CS47L63_INPUT_PDM));

	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_INPUT_CONTROL) &
			      (IN1_EN_BOTH | IN2_EN_BOTH),
		      0U, "neither path is left enabled");
	zassert_true(muted(CS47L63_IN2L_CONTROL2), "the line pair it left is muted");
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_MICBIAS_CTRL5) & MICB1B_ON, 0U);
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_ASP1_ENABLES1) & ASP1_TX_EN_BOTH, 0U);
}

static ZTEST(cs47l63_in, test_failed_stop_still_mutes_and_disables)
{
	zassert_ok(audio_codec_start(CODEC, AUDIO_DAI_DIR_RX));
	cs47l63_emul_fail_at(EMUL, CS47L63_ASP1_ENABLES1);

	zassert_equal(-EIO, audio_codec_stop(CODEC, AUDIO_DAI_DIR_RX),
		      "the first error is the one reported");

	zassert_true(muted(CS47L63_IN2L_CONTROL2), "the steps after the failure still ran");
	zassert_true(muted(CS47L63_IN2R_CONTROL2));
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_INPUT_CONTROL) & IN2_EN_BOTH, 0U);
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_ASP1TX1_INPUT1),
		      MIX_SLOT(CS47L63_MIXER_SRC_NONE));
}

static ZTEST(cs47l63_in, test_mono_capture_enables_only_the_first_transmit_slot)
{
	struct audio_codec_cfg cfg = cs47l63_test_cfg(AUDIO_ROUTE_CAPTURE);

	cfg.dai_cfg.i2s.channels = 1U;
	zassert_ok(audio_codec_configure(CODEC, &cfg));

	zassert_ok(audio_codec_start(CODEC, AUDIO_DAI_DIR_RX));

	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_ASP1_ENABLES1) & ASP1_TX_EN_BOTH,
		      CS47L63_ASP1_TX1_EN, "a mono frame has no second slot to drive");
}
