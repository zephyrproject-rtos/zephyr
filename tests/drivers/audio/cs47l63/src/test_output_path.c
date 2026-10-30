/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file test_output_path.c
 * @brief OUT1L: the mixer, the headphone amplifier, volume and mute
 *
 * One output channel: "left" and "right" select which receive slot feeds OUT1L.
 */

#include <errno.h>
#include <stdint.h>

#include <zephyr/audio/codec.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include "cs47l63.h"

#include "cs47l63_emul.h"

#define CODEC DEVICE_DT_GET(DT_NODELABEL(cs47l63_out))
#define EMUL  EMUL_DT_GET(DT_NODELABEL(cs47l63_out))

#define OUT1L_VOL_MASK 0x000000FFU

/** Margin over the deadline of the start check. */
#define START_WAIT_MS (CS47L63_START_TIMEOUT_MS + 100U)

/** The driver's CS47L63_OUT_POLL_MS: the start check runs again after it. */
#define OUT_POLL_MS 2U

/** A property set may wait for at most one tick on a running start check. */
#define ONE_TICK 1

/** An output terminal value the part does not have: the line output. */
#define OUTPUT_HP     0U
#define OUTPUT_ABSENT 1U


/** OUT1L_VOL is 0.5 dB per step. */
#define VOL_CODE_0DB   0x80U
#define VOL_CODE_M20DB 0x58U
#define VOL_CODE_M64DB 0x00U

#define VOL_M20DB    (-20)
#define BELOW_MIN_DB 36
#define ABOVE_MAX_DB 12

#define ASP1_RX_EN_BOTH (CS47L63_ASP1_RX1_EN | CS47L63_ASP1_RX2_EN)

#define MIX_SLOT(src) (((uint32_t)CS47L63_OUT1LMIX_VOL_0DB << CS47L63_OUT1LMIX_VOL_SHIFT) | (src))
/** One of two routed slots: each at -6 dB, so their sum cannot clip. */
#define MIX_PAIR(src) (((uint32_t)CS47L63_OUT1LMIX_VOL_M6DB << CS47L63_OUT1LMIX_VOL_SHIFT) | (src))

/** Makes FLL1 report lock and the amplifier report enabled, then starts and waits. */
static void start_and_confirm(void)
{
	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_STS_6, CS47L63_FLL1_LOCK_STS1);
	cs47l63_emul_set_reg(EMUL, CS47L63_OUTPUT_STATUS_1, CS47L63_OUT1L_EN_STS);

	audio_codec_start_output(CODEC);
	(void)k_msleep((int32_t)START_WAIT_MS);

	zassert_not_equal(cs47l63_emul_get_reg(EMUL, CS47L63_OUTPUT_ENABLE_1) & CS47L63_OUT1L_EN,
			  0U, "the amplifier enable must be on the part before the level is");
}

/** Log index of the first write to @p addr from log index @p from whose @p mask bits are
 *  @p want, or -1.
 */
static int first_write_from(int from, uint32_t addr, uint32_t mask, uint32_t want)
{
	const uint32_t n = cs47l63_emul_write_count(EMUL, addr);
	uint32_t val;
	int idx;

	for (uint32_t i = 0U; i < n; i++) {
		zassert_true(cs47l63_emul_nth_write(EMUL, addr, i, &val));
		idx = cs47l63_emul_write_index(EMUL, addr, i);
		if ((idx >= from) && ((val & mask) == want)) {
			return idx;
		}
	}

	return -1;
}

static bool sysclk_on(void)
{
	return (cs47l63_emul_get_reg(EMUL, CS47L63_SYSTEM_CLOCK1) & CS47L63_SYSCLK_EN) != 0U;
}

static void output_before_each(void *fixture)
{
	ARG_UNUSED(fixture);

	cs47l63_emul_reset(EMUL);
	zassert_ok(cs47l63_test_configure(CODEC, AUDIO_ROUTE_PLAYBACK_CAPTURE));
}

static void output_after_each(void *fixture)
{
	ARG_UNUSED(fixture);

	audio_codec_stop_output(CODEC);
}

ZTEST_SUITE(cs47l63_out, NULL, NULL, output_before_each, output_after_each, NULL);

static ZTEST(cs47l63_out, test_configure_leaves_the_amplifier_down_and_muted)
{
	uint32_t val;

	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_OUTPUT_ENABLE_1) & CS47L63_OUT1L_EN, 0U,
		      "configure() must not start audio");

	val = cs47l63_emul_get_reg(EMUL, CS47L63_OUT1L_VOLUME_1);
	zassert_not_equal(val & CS47L63_OUT1L_MUTE, 0U, "the output boots muted");
	zassert_equal(val & OUT1L_VOL_MASK, VOL_CODE_0DB,
		      "the cached level is 0 dB until something sets it");
	zassert_not_equal(val & CS47L63_OUT_VU, 0U,
			  "OUT_VU is what latches the field; a write without it does nothing");
}

static ZTEST(cs47l63_out, test_start_routes_both_receive_slots_into_the_one_channel)
{
	audio_codec_start_output(CODEC);

	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_OUT1L_INPUT1),
		      MIX_PAIR(CS47L63_MIXER_SRC_ASP1RX1), "two slots summed: each at -6 dB");
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_OUT1L_INPUT2),
		      MIX_PAIR(CS47L63_MIXER_SRC_ASP1RX2));
}

static void configure_mono(void)
{
	struct audio_codec_cfg cfg = cs47l63_test_cfg(AUDIO_ROUTE_PLAYBACK_CAPTURE);

	cfg.dai_cfg.i2s.channels = 1U;
	zassert_ok(audio_codec_configure(CODEC, &cfg));
}

static void assert_mono_mixer_at_0db(void)
{
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_OUT1L_INPUT1),
		      MIX_SLOT(CS47L63_MIXER_SRC_ASP1RX1), "the one slot runs at 0 dB");
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_OUT1L_INPUT2),
		      MIX_SLOT(CS47L63_MIXER_SRC_NONE),
		      "the slot the stream does not carry is off");
}

static ZTEST(cs47l63_out, test_mono_start_feeds_the_one_slot_at_0db)
{
	configure_mono();
	audio_codec_start_output(CODEC);

	assert_mono_mixer_at_0db();
}

static ZTEST(cs47l63_out, test_mono_route_all_leaves_the_second_slot_off)
{
	configure_mono();
	audio_codec_start_output(CODEC);
	zassert_ok(audio_codec_route_output(CODEC, AUDIO_CHANNEL_ALL, OUTPUT_HP));

	assert_mono_mixer_at_0db();
}

static ZTEST(cs47l63_out, test_configure_leaves_sysclk_and_every_path_off)
{
	zassert_false(sysclk_on(), "SYSCLK waits for a stream to start");
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_ASP1_ENABLES1), 0U, "no slot enabled");
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_OUT1L_INPUT1),
		      MIX_SLOT(CS47L63_MIXER_SRC_NONE), "no mixer source");
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_OUT1L_INPUT2),
		      MIX_SLOT(CS47L63_MIXER_SRC_NONE));
}

static ZTEST(cs47l63_out, test_output_start_enables_sysclk_before_any_path)
{
	int sysclk;
	int rx;
	int src1;
	int src2;
	int amp;

	audio_codec_start_output(CODEC);

	sysclk = first_write_from(0, CS47L63_SYSTEM_CLOCK1, CS47L63_SYSCLK_EN, CS47L63_SYSCLK_EN);
	rx = first_write_from(0, CS47L63_ASP1_ENABLES1, ASP1_RX_EN_BOTH, ASP1_RX_EN_BOTH);
	src1 = first_write_from(0, CS47L63_OUT1L_INPUT1, CS47L63_OUT1L_SRC_MASK,
				CS47L63_MIXER_SRC_ASP1RX1);
	src2 = first_write_from(0, CS47L63_OUT1L_INPUT2, CS47L63_OUT1L_SRC_MASK,
				CS47L63_MIXER_SRC_ASP1RX2);
	amp = first_write_from(0, CS47L63_OUTPUT_ENABLE_1, CS47L63_OUT1L_EN, CS47L63_OUT1L_EN);

	zassert_true(sysclk >= 0, "the start enables SYSCLK");
	zassert_true(sysclk < rx, "before the receive slots");
	zassert_true((rx < src1) && (rx < src2), "the slots before the mixer takes them");
	zassert_true((src1 < amp) && (src2 < amp), "the mixer before the amplifier");
}

static ZTEST(cs47l63_out, test_route_output_on_a_stopped_port_is_only_cached)
{
	const uint32_t writes = cs47l63_emul_write_count(EMUL, CS47L63_OUT1L_INPUT1) +
				cs47l63_emul_write_count(EMUL, CS47L63_OUT1L_INPUT2);

	zassert_ok(audio_codec_route_output(CODEC, AUDIO_CHANNEL_FRONT_LEFT, OUTPUT_HP));
	zassert_equal(cs47l63_emul_write_count(EMUL, CS47L63_OUT1L_INPUT1) +
			      cs47l63_emul_write_count(EMUL, CS47L63_OUT1L_INPUT2),
		      writes, "a stopped output writes no mixer source");

	audio_codec_start_output(CODEC);
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_OUT1L_INPUT1),
		      MIX_SLOT(CS47L63_MIXER_SRC_ASP1RX1), "the start applies the cached route");
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_OUT1L_INPUT2),
		      MIX_SLOT(CS47L63_MIXER_SRC_NONE));
}

static ZTEST(cs47l63_out, test_last_stop_clears_slots_and_sources_before_sysclk)
{
	int mark;
	int off;
	int on;
	int idx;

	audio_codec_start_output(CODEC);
	mark = (int)cs47l63_emul_xfer_count(EMUL);
	audio_codec_stop_output(CODEC);

	zassert_false(sysclk_on(), "the last stop takes SYSCLK down");
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_ASP1_ENABLES1) & ASP1_RX_EN_BOTH, 0U);
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_OUT1L_INPUT1),
		      MIX_SLOT(CS47L63_MIXER_SRC_NONE));
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_OUT1L_INPUT2),
		      MIX_SLOT(CS47L63_MIXER_SRC_NONE));
	off = first_write_from(mark, CS47L63_SYSTEM_CLOCK1, CS47L63_SYSCLK_EN, 0U);
	idx = first_write_from(mark, CS47L63_ASP1_ENABLES1, ASP1_RX_EN_BOTH, 0U);
	zassert_true((idx >= 0) && (idx < off), "the receive slots go off before SYSCLK");
	idx = first_write_from(mark, CS47L63_OUT1L_INPUT1, CS47L63_OUT1L_SRC_MASK,
			       CS47L63_MIXER_SRC_NONE);
	zassert_true((idx >= 0) && (idx < off), "and the mixer sources");
	idx = first_write_from(mark, CS47L63_OUTPUT_ENABLE_1, CS47L63_OUT1L_EN, 0U);
	zassert_true((idx >= 0) && (idx < off), "and the amplifier");

	mark = (int)cs47l63_emul_xfer_count(EMUL);
	audio_codec_start_output(CODEC);

	on = first_write_from(mark, CS47L63_SYSTEM_CLOCK1, CS47L63_SYSCLK_EN, CS47L63_SYSCLK_EN);
	zassert_true(on >= 0, "a second start enables SYSCLK again");
	zassert_true(first_write_from(mark, CS47L63_ASP1_ENABLES1, ASP1_RX_EN_BOTH,
				      ASP1_RX_EN_BOTH) > on,
		     "and writes the receive slots again after it");
	zassert_true(first_write_from(mark, CS47L63_OUT1L_INPUT1, CS47L63_OUT1L_SRC_MASK,
				      CS47L63_MIXER_SRC_ASP1RX1) > on,
		     "and the mixer source");
}

static ZTEST(cs47l63_out, test_failed_output_start_leaves_sysclk_off)
{
	cs47l63_emul_fail_write_at(EMUL, CS47L63_OUTPUT_ENABLE_1);

	audio_codec_start_output(CODEC);

	zassert_true(cs47l63_emul_fail_hits(EMUL) > 0U,
		     "the injection took effect: the amplifier enable failed");
	zassert_true(first_write_from(0, CS47L63_SYSTEM_CLOCK1, CS47L63_SYSCLK_EN,
				      CS47L63_SYSCLK_EN) >= 0,
		     "the premise: the start enabled SYSCLK");
	zassert_false(sysclk_on(), "nothing runs, so SYSCLK is off again");
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_ASP1_ENABLES1) & ASP1_RX_EN_BOTH, 0U);
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_OUT1L_INPUT1),
		      MIX_SLOT(CS47L63_MIXER_SRC_NONE));
}

static ZTEST(cs47l63_out, test_start_enables_the_amplifier_and_unmutes_once_it_is_up)
{
	uint32_t val;

	start_and_confirm();

	val = cs47l63_emul_get_reg(EMUL, CS47L63_OUT1L_VOLUME_1);
	zassert_equal(val & CS47L63_OUT1L_MUTE, 0U,
		      "the level goes on the pins only once the stage reports enabled");
	zassert_equal(val & OUT1L_VOL_MASK, VOL_CODE_0DB);
	zassert_true(cs47l63_emul_read_count(EMUL, CS47L63_OUTPUT_STATUS_1) >= 1U,
		     "the enable must be confirmed against the part, not assumed");
}

static ZTEST(cs47l63_out, test_amplifier_that_never_comes_up_leaves_the_output_muted)
{
	uint32_t val;

	/* The clock is there but the analog stage never reports enabled. */
	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_STS_6, CS47L63_FLL1_LOCK_STS1);
	cs47l63_emul_set_reg(EMUL, CS47L63_OUTPUT_STATUS_1, 0U);

	audio_codec_start_output(CODEC);
	(void)k_msleep((int32_t)START_WAIT_MS);

	zassert_true(cs47l63_emul_read_count(EMUL, CS47L63_OUTPUT_STATUS_1) > 1U,
		     "the injection took effect: the status was read again and stayed low");

	val = cs47l63_emul_get_reg(EMUL, CS47L63_OUT1L_VOLUME_1);
	zassert_not_equal(val & CS47L63_OUT1L_MUTE, 0U,
			  "a stage that never came up must not be unmuted");
}

static ZTEST(cs47l63_out, test_start_check_unmutes_within_a_poll_of_the_stage_coming_up)
{
	const uint32_t poll_ticks = k_ms_to_ticks_ceil32(OUT_POLL_MS);
	int64_t start;
	int64_t elapsed = -1;

	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_STS_6, CS47L63_FLL1_LOCK_STS1);
	cs47l63_emul_set_reg(EMUL, CS47L63_OUTPUT_STATUS_1, CS47L63_OUT1L_EN_STS);

	start = k_uptime_ticks();
	audio_codec_start_output(CODEC);
	for (uint32_t i = 0U; i < k_ms_to_ticks_ceil32(START_WAIT_MS); i++) {
		if ((cs47l63_emul_get_reg(EMUL, CS47L63_OUT1L_VOLUME_1) & CS47L63_OUT1L_MUTE) ==
		    0U) {
			elapsed = k_uptime_ticks() - start;
			break;
		}
		(void)k_sleep(K_TICKS(1));
	}

	zassert_true(elapsed >= 0, "the output was unmuted");
	zassert_true(elapsed <= (int64_t)(poll_ticks + 1U),
		     "unmuted %lld ticks after the start, not within one poll",
		     (long long)elapsed);
}

static ZTEST(cs47l63_out, test_start_check_never_holds_the_codec_for_a_tick)
{
	const uint32_t ticks = k_ms_to_ticks_ceil32(START_WAIT_MS);
	audio_property_value_t unmute = {.mute = false};
	int64_t longest = 0;
	int64_t before;

	/* Locked, but the stage never reports up: the check runs to its deadline. */
	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_STS_6, CS47L63_FLL1_LOCK_STS1);
	cs47l63_emul_set_reg(EMUL, CS47L63_OUTPUT_STATUS_1, 0U);
	audio_codec_start_output(CODEC);

	for (uint32_t i = 0U; i < ticks; i++) {
		before = k_uptime_ticks();
		zassert_ok(audio_codec_set_property(CODEC, AUDIO_PROPERTY_OUTPUT_MUTE,
						    AUDIO_CHANNEL_ALL, unmute));
		longest = MAX(longest, k_uptime_ticks() - before);
		(void)k_sleep(K_TICKS(1));
	}

	zassert_true(cs47l63_emul_read_count(EMUL, CS47L63_OUTPUT_STATUS_1) > 1U,
		     "the premise: the check ran while the property was set");
	zassert_true(longest <= ONE_TICK, "a property set waited %lld ticks on the check",
		     (long long)longest);
}

static ZTEST(cs47l63_out, test_reconfigure_cancels_a_pending_start)
{
	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_STS_6, CS47L63_FLL1_LOCK_STS1);
	cs47l63_emul_set_reg(EMUL, CS47L63_OUTPUT_STATUS_1, CS47L63_OUT1L_EN_STS);

	audio_codec_start_output(CODEC);
	zassert_ok(cs47l63_test_configure(CODEC, AUDIO_ROUTE_PLAYBACK_CAPTURE));
	(void)k_msleep((int32_t)START_WAIT_MS);

	zassert_equal(cs47l63_emul_read_count(EMUL, CS47L63_OUTPUT_STATUS_1), 0U,
		      "the start check armed before the reconfigure must never run");
	zassert_not_equal(cs47l63_emul_get_reg(EMUL, CS47L63_OUT1L_VOLUME_1) & CS47L63_OUT1L_MUTE,
			  0U, "a reconfigured output stays muted until it is started again");
}

static uint32_t write_total(void)
{
	const uint32_t count = cs47l63_emul_xfer_count(EMUL);
	struct cs47l63_emul_xfer xfer;
	uint32_t n = 0U;

	for (uint32_t i = 0U; i < count; i++) {
		zassert_true(cs47l63_emul_xfer_get(EMUL, i, &xfer));
		n += xfer.write ? 1U : 0U;
	}

	return n;
}

static ZTEST(cs47l63_out, test_failed_reconfigure_leaves_nothing_running)
{
	audio_property_value_t vol = {.vol = VOL_M20DB};
	uint32_t writes;

	start_and_confirm();
	zassert_ok(audio_codec_start(CODEC, AUDIO_DAI_DIR_RX));

	cs47l63_emul_reset(EMUL);
	cs47l63_emul_fail_at(EMUL, CS47L63_ASP1_CONTROL2);
	zassert_equal(-EIO, cs47l63_test_configure(CODEC, AUDIO_ROUTE_PLAYBACK_CAPTURE));
	zassert_equal(cs47l63_emul_write_count(EMUL, CS47L63_ASP1_CONTROL2), 0U,
		      "the injection took effect: the serial port was never configured");
	writes = write_total();

	zassert_ok(audio_codec_route_input(CODEC, AUDIO_CHANNEL_ALL, 0U));
	zassert_ok(audio_codec_set_property(CODEC, AUDIO_PROPERTY_INPUT_VOLUME, AUDIO_CHANNEL_ALL,
					    vol));
	zassert_ok(audio_codec_set_property(CODEC, AUDIO_PROPERTY_OUTPUT_VOLUME, AUDIO_CHANNEL_ALL,
					    vol));

	zassert_equal(write_total(), writes, "a part left stopped by the reset is not written");
}

static ZTEST(cs47l63_out, test_volume_across_the_envelope)
{
	static const struct {
		int db;
		uint32_t code;
	} k_steps[] = {
		{0, VOL_CODE_0DB},
		{VOL_M20DB, VOL_CODE_M20DB},
		{CS47L63_OUT_VOLUME_MIN_DB, VOL_CODE_M64DB},
	};

	start_and_confirm();

	for (size_t i = 0U; i < ARRAY_SIZE(k_steps); i++) {
		audio_property_value_t val = {.vol = k_steps[i].db};
		uint32_t reg;

		zassert_ok(audio_codec_set_property(CODEC, AUDIO_PROPERTY_OUTPUT_VOLUME,
						    AUDIO_CHANNEL_ALL, val));

		reg = cs47l63_emul_get_reg(EMUL, CS47L63_OUT1L_VOLUME_1);
		zassert_equal(reg & OUT1L_VOL_MASK, k_steps[i].code,
			      "%d dB must encode to 0x%02x at 0.5 dB per step", k_steps[i].db,
			      k_steps[i].code);
		zassert_not_equal(reg & CS47L63_OUT_VU, 0U, "every volume write carries OUT_VU");
		zassert_equal(reg & CS47L63_OUT1L_MUTE, 0U, "a volume set must not mute");
	}
}

static ZTEST(cs47l63_out, test_volume_outside_the_envelope_is_clamped_not_wrapped)
{
	audio_property_value_t val;

	start_and_confirm();

	val.vol = CS47L63_OUT_VOLUME_MIN_DB - BELOW_MIN_DB;
	zassert_ok(audio_codec_set_property(CODEC, AUDIO_PROPERTY_OUTPUT_VOLUME, AUDIO_CHANNEL_ALL,
					    val));
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_OUT1L_VOLUME_1) & OUT1L_VOL_MASK,
		      VOL_CODE_M64DB, "below the floor must clamp to the floor, not wrap high");

	val.vol = CS47L63_OUT_VOLUME_MAX_DB + ABOVE_MAX_DB;
	zassert_ok(audio_codec_set_property(CODEC, AUDIO_PROPERTY_OUTPUT_VOLUME, AUDIO_CHANNEL_ALL,
					    val));
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_OUT1L_VOLUME_1) & OUT1L_VOL_MASK,
		      VOL_CODE_0DB, "above the ceiling must clamp to the ceiling");
}

static ZTEST(cs47l63_out, test_volume_set_while_stopped_survives_to_the_next_start)
{
	audio_property_value_t val = {.vol = VOL_M20DB};
	uint32_t writes_before = cs47l63_emul_write_count(EMUL, CS47L63_OUT1L_VOLUME_1);

	zassert_ok(audio_codec_set_property(CODEC, AUDIO_PROPERTY_OUTPUT_VOLUME, AUDIO_CHANNEL_ALL,
					    val));

	zassert_equal(cs47l63_emul_write_count(EMUL, CS47L63_OUT1L_VOLUME_1), writes_before,
		      "a stopped output must not be brought up to a level by a property set");

	start_and_confirm();

	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_OUT1L_VOLUME_1) & OUT1L_VOL_MASK,
		      VOL_CODE_M20DB, "the start must apply the level that was asked for");
}

static ZTEST(cs47l63_out, test_mute_round_trip_returns_to_the_cached_level)
{
	audio_property_value_t vol = {.vol = VOL_M20DB};
	audio_property_value_t mute = {.mute = true};
	uint32_t reg;

	start_and_confirm();

	zassert_ok(audio_codec_set_property(CODEC, AUDIO_PROPERTY_OUTPUT_VOLUME, AUDIO_CHANNEL_ALL,
					    vol));
	zassert_ok(audio_codec_set_property(CODEC, AUDIO_PROPERTY_OUTPUT_MUTE, AUDIO_CHANNEL_ALL,
					    mute));

	reg = cs47l63_emul_get_reg(EMUL, CS47L63_OUT1L_VOLUME_1);
	zassert_not_equal(reg & CS47L63_OUT1L_MUTE, 0U, "mute must reach the part");
	zassert_equal(reg & OUT1L_VOL_MASK, VOL_CODE_M20DB, "muting must not discard the level");

	mute.mute = false;
	zassert_ok(audio_codec_set_property(CODEC, AUDIO_PROPERTY_OUTPUT_MUTE, AUDIO_CHANNEL_ALL,
					    mute));

	reg = cs47l63_emul_get_reg(EMUL, CS47L63_OUT1L_VOLUME_1);
	zassert_equal(reg & CS47L63_OUT1L_MUTE, 0U);
	zassert_equal(reg & OUT1L_VOL_MASK, VOL_CODE_M20DB,
		      "unmuting returns to the level last set, not to a default");
}

static ZTEST(cs47l63_out, test_stop_mutes_before_it_takes_the_amplifier_down)
{
	int mute_idx;
	int disable_idx;
	uint32_t val;

	start_and_confirm();

	/* Clear the status so the shutdown wait sees the stage go down. */
	cs47l63_emul_set_reg(EMUL, CS47L63_OUTPUT_STATUS_1, 0U);
	audio_codec_stop_output(CODEC);

	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_OUTPUT_ENABLE_1) & CS47L63_OUT1L_EN, 0U,
		      "the amplifier must be disabled");

	val = cs47l63_emul_get_reg(EMUL, CS47L63_OUT1L_VOLUME_1);
	zassert_not_equal(val & CS47L63_OUT1L_MUTE, 0U, "and left muted");

	mute_idx = cs47l63_emul_write_index(EMUL, CS47L63_OUT1L_VOLUME_1,
					    cs47l63_emul_write_count(EMUL, CS47L63_OUT1L_VOLUME_1) -
						    1U);
	disable_idx = cs47l63_emul_write_index(
		EMUL, CS47L63_OUTPUT_ENABLE_1,
		cs47l63_emul_write_count(EMUL, CS47L63_OUTPUT_ENABLE_1) - 1U);
	zassert_true(mute_idx < disable_idx,
		     "the stage must be silent while it collapses, not after");
}

static ZTEST(cs47l63_out, test_stop_without_a_clock_does_not_wait_for_a_status_that_cannot_change)
{
	uint32_t reads_before;

	start_and_confirm();

	/* No FLL1 lock: nothing updates the status bit, so no wait. */
	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_STS_6, 0U);
	reads_before = cs47l63_emul_read_count(EMUL, CS47L63_OUTPUT_STATUS_1);

	audio_codec_stop_output(CODEC);

	zassert_equal(cs47l63_emul_read_count(EMUL, CS47L63_OUTPUT_STATUS_1), reads_before,
		      "an unlocked loop means the write is taken at its word");
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_OUTPUT_ENABLE_1) & CS47L63_OUT1L_EN, 0U,
		      "the amplifier still goes down");
}

static ZTEST(cs47l63_out, test_route_output_selects_one_slot_or_both)
{
	audio_codec_start_output(CODEC);

	zassert_ok(audio_codec_route_output(CODEC, AUDIO_CHANNEL_FRONT_LEFT,
					    OUTPUT_HP));
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_OUT1L_INPUT1),
		      MIX_SLOT(CS47L63_MIXER_SRC_ASP1RX1));
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_OUT1L_INPUT2),
		      MIX_SLOT(CS47L63_MIXER_SRC_NONE),
		      "the unused slot must be muted, not left on the previous source");

	zassert_ok(audio_codec_route_output(CODEC, AUDIO_CHANNEL_HEADPHONE_RIGHT,
					    OUTPUT_HP));
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_OUT1L_INPUT1),
		      MIX_SLOT(CS47L63_MIXER_SRC_ASP1RX2));

	zassert_ok(audio_codec_route_output(CODEC, AUDIO_CHANNEL_ALL, OUTPUT_HP));
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_OUT1L_INPUT1),
		      MIX_PAIR(CS47L63_MIXER_SRC_ASP1RX1), "both slots: each at -6 dB");
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_OUT1L_INPUT2),
		      MIX_PAIR(CS47L63_MIXER_SRC_ASP1RX2));
}

static ZTEST(cs47l63_out, test_absent_terminal_and_unaddressable_channel_are_refused)
{
	uint32_t writes_before = cs47l63_emul_write_count(EMUL, CS47L63_OUT1L_INPUT1);

	zassert_equal(-ENOTSUP,
		      audio_codec_route_output(CODEC, AUDIO_CHANNEL_ALL, OUTPUT_ABSENT));
	zassert_equal(-ENOTSUP, audio_codec_route_output(CODEC, AUDIO_CHANNEL_LFE,
							 OUTPUT_HP));

	zassert_equal(cs47l63_emul_write_count(EMUL, CS47L63_OUT1L_INPUT1), writes_before,
		      "a refused route must leave the mixer alone");
}

static ZTEST(cs47l63_out, test_output_level_accepts_the_channels_route_output_does)
{
	static const audio_channel_t k_channels[] = {
		AUDIO_CHANNEL_FRONT_LEFT,
		AUDIO_CHANNEL_FRONT_RIGHT,
		AUDIO_CHANNEL_HEADPHONE_LEFT,
		AUDIO_CHANNEL_HEADPHONE_RIGHT,
	};
	audio_property_value_t vol = {.vol = VOL_M20DB};
	audio_property_value_t mute = {.mute = true};

	start_and_confirm();

	for (size_t i = 0U; i < ARRAY_SIZE(k_channels); i++) {
		vol.vol = VOL_M20DB - (int)i;
		zassert_ok(audio_codec_set_property(CODEC, AUDIO_PROPERTY_OUTPUT_VOLUME,
						    k_channels[i], vol),
			   "channel %d names the one output", k_channels[i]);
		zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_OUT1L_VOLUME_1) & OUT1L_VOL_MASK,
			      (uint32_t)((int32_t)VOL_CODE_0DB + (2 * vol.vol)),
			      "the level reached OUT1L");
		zassert_ok(audio_codec_set_property(CODEC, AUDIO_PROPERTY_OUTPUT_MUTE,
						    k_channels[i], mute));
		zassert_equal((cs47l63_emul_get_reg(EMUL, CS47L63_OUT1L_VOLUME_1) &
			       CS47L63_OUT1L_MUTE) != 0U,
			      mute.mute, "and so did the mute");
		mute.mute = !mute.mute;
	}
}

static ZTEST(cs47l63_out, test_per_side_property_and_unknown_property_refused)
{
	audio_property_value_t val = {.vol = 0};
	uint32_t writes_before = cs47l63_emul_write_count(EMUL, CS47L63_OUT1L_VOLUME_1);

	zassert_equal(-EINVAL,
		      audio_codec_set_property(CODEC, AUDIO_PROPERTY_OUTPUT_VOLUME,
					       AUDIO_CHANNEL_LFE, val),
		      "a channel the output cannot name is refused");
	zassert_equal(-EINVAL,
		      audio_codec_set_property(CODEC, AUDIO_PROPERTY_OUTPUT_MUTE,
					       AUDIO_CHANNEL_REAR_LEFT, val));
	zassert_equal(
		-ENOTSUP,
		audio_codec_set_property(CODEC, AUDIO_PROPERTY_EQ_GAIN, AUDIO_CHANNEL_ALL, val),
		"an unsupported property is refused, never accepted and dropped");

	zassert_equal(cs47l63_emul_write_count(EMUL, CS47L63_OUT1L_VOLUME_1), writes_before);
}

static ZTEST(cs47l63_out, test_apply_properties_has_nothing_to_commit)
{
	uint32_t xfers_before = cs47l63_emul_xfer_count(EMUL);

	zassert_ok(audio_codec_apply_properties(CODEC));
	zassert_equal(cs47l63_emul_xfer_count(EMUL), xfers_before,
		      "every property is latched by OUT_VU as it is written");
}

static ZTEST(cs47l63_out, test_stop_disables_the_amplifier_after_a_failed_mute)
{
	start_and_confirm();

	cs47l63_emul_set_reg(EMUL, CS47L63_OUTPUT_STATUS_1, 0U);
	cs47l63_emul_fail_write_at(EMUL, CS47L63_OUT1L_VOLUME_1);

	zassert_equal(audio_codec_stop(CODEC, AUDIO_DAI_DIR_TX), -EIO,
		      "the mute error is the one reported");
	zassert_equal(cs47l63_emul_fail_hits(EMUL), 1U,
		      "the injection took effect: the mute write was attempted and failed");
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_OUTPUT_ENABLE_1) & CS47L63_OUT1L_EN, 0U,
		      "a failed mute must not leave the amplifier enabled");
}

static ZTEST(cs47l63_out, test_a_failed_input_stop_still_disables_the_amplifier)
{
	start_and_confirm();
	zassert_ok(audio_codec_start(CODEC, AUDIO_DAI_DIR_RX));

	cs47l63_emul_set_reg(EMUL, CS47L63_OUTPUT_STATUS_1, 0U);
	cs47l63_emul_fail_write_at(EMUL, CS47L63_INPUT_CONTROL);

	zassert_equal(audio_codec_stop(CODEC, AUDIO_DAI_DIR_TXRX), -EIO,
		      "the input stop error is the one reported");
	zassert_equal(cs47l63_emul_fail_hits(EMUL), 1U,
		      "the injection took effect: the input disable was attempted and failed");
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_OUTPUT_ENABLE_1) & CS47L63_OUT1L_EN, 0U,
		      "a failed input stop must not leave the amplifier enabled");
}
