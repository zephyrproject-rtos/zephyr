/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file test_output_path.c
 * @brief OUT1L: the mixer, the headphone amplifier, volume and mute
 *
 * This part has one output channel driving a differential headphone load, so
 * "left" and "right" select which receive slot feeds it rather than which of
 * two amplifiers is addressed.
 *
 * Starting the amplifier is deliberately split in two: the enable is written
 * when the caller asks, and the wait for OUT1L_EN_STS happens later, once the
 * caller's I2S transfer has given FLL1 a reference and SYSCLK exists. The
 * helper below drives both halves, which is what a real stream does.
 */

#include <errno.h>
#include <stdint.h>

#include <zephyr/audio/codec.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include "cs47l63_clock.h"
#include "cs47l63_out.h"
#include "cs47l63_regs.h"

#include "cs47l63_emul.h"

#define CODEC DEVICE_DT_GET(DT_NODELABEL(cs47l63_out))
#define EMUL  EMUL_DT_GET(DT_NODELABEL(cs47l63_out))

/** Margin over the settle the driver waits before completing a start. */
#define START_WAIT_MS (CS47L63_FLL_LOCK_SETTLE_MS + 100)

/** Reads the driver makes before it gives up on the amplifier enable. */
#define OUT_POLL_MAX 50U

/** Upper bound on that poll, in milliseconds, and the granularity of the wait.
 *  Each read sleeps 2 ms, and a sleep shorter than the tick costs a whole tick,
 *  so the bound allows for the tick rather than for the nominal 100 ms.
 */
#define OUT_POLL_WAIT_MS 3000
#define OUT_POLL_STEP_MS 20

/** OUT1L_VOL is 0.5 dB per step, and 0x80 is 0 dB. */
#define VOL_CODE_0DB   0x80U
#define VOL_CODE_M20DB 0x58U
#define VOL_CODE_M64DB 0x00U

/** A mixer slot: its source, at the 0 dB mix volume. */
#define MIX_SLOT(src) (((uint32_t)CS47L63_OUT1LMIX_VOL_0DB << CS47L63_OUT1LMIX_VOL_SHIFT) | (src))

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

/**
 * @brief Start the output and let the deferred half complete.
 *
 * Both conditions the deferred check looks at are made true first: FLL1 reports
 * lock, and the amplifier reports enabled. Without them the check raises a
 * fault instead of completing the start, which is a different test.
 */
static void start_and_confirm(void)
{
	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_STS_6, CS47L63_FLL1_LOCK_STS1);
	cs47l63_emul_set_reg(EMUL, CS47L63_OUTPUT_STATUS_1, CS47L63_OUT1L_EN_STS);

	audio_codec_start_output(CODEC);
	k_msleep(START_WAIT_MS);

	zassert_not_equal(cs47l63_emul_get_reg(EMUL, CS47L63_OUTPUT_ENABLE_1) & CS47L63_OUT1L_EN, 0,
			  "the amplifier enable must be on the part before the level is");
}

static void before_each(void *fixture)
{
	ARG_UNUSED(fixture);

	cs47l63_emul_reset(EMUL);
	configure();
}

static void after_each(void *fixture)
{
	ARG_UNUSED(fixture);

	/* Cancels the deferred start check, so it cannot run inside the next
	 * case and write a volume that case did not ask for.
	 */
	audio_codec_stop_output(CODEC);
}

ZTEST_SUITE(cs47l63_out, NULL, NULL, before_each, after_each, NULL);

ZTEST(cs47l63_out, test_configure_leaves_the_amplifier_down_and_muted)
{
	uint32_t val;

	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_OUTPUT_ENABLE_1) & CS47L63_OUT1L_EN, 0,
		      "configure() must not start audio");

	val = cs47l63_emul_get_reg(EMUL, CS47L63_OUT1L_VOLUME_1);
	zassert_not_equal(val & CS47L63_OUT1L_MUTE, 0, "the output boots muted");
	zassert_equal(val & CS47L63_OUT1L_VOL_MASK, VOL_CODE_0DB,
		      "the cached level is 0 dB until something sets it");
	zassert_not_equal(val & CS47L63_OUT_VU, 0,
			  "OUT_VU is what latches the field; a write without it does nothing");
}

ZTEST(cs47l63_out, test_configure_routes_both_receive_slots_into_the_one_channel)
{
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_OUT1L_INPUT1),
		      MIX_SLOT(CS47L63_MIXER_SRC_ASP1RX1));
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_OUT1L_INPUT2),
		      MIX_SLOT(CS47L63_MIXER_SRC_ASP1RX2));
}

ZTEST(cs47l63_out, test_start_enables_the_amplifier_and_unmutes_once_it_is_up)
{
	uint32_t val;

	start_and_confirm();

	val = cs47l63_emul_get_reg(EMUL, CS47L63_OUT1L_VOLUME_1);
	zassert_equal(val & CS47L63_OUT1L_MUTE, 0,
		      "the level goes on the pins only once the stage reports enabled");
	zassert_equal(val & CS47L63_OUT1L_VOL_MASK, VOL_CODE_0DB);
	zassert_true(cs47l63_emul_read_count(EMUL, CS47L63_OUTPUT_STATUS_1) >= 1,
		     "the enable must be confirmed against the part, not assumed");
}

ZTEST(cs47l63_out, test_amplifier_that_never_comes_up_leaves_the_output_muted)
{
	uint32_t val;

	/* The clock is there but the analog stage never reports enabled. */
	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_STS_6, CS47L63_FLL1_LOCK_STS1);
	cs47l63_emul_set_reg(EMUL, CS47L63_OUTPUT_STATUS_1, 0);

	audio_codec_start_output(CODEC);

	/* Wait for the poll to run out rather than for a fixed delay, so the
	 * mute below is asserted against a driver that gave up and not against
	 * one that is merely still waiting.
	 */
	for (int i = 0; i < OUT_POLL_WAIT_MS / OUT_POLL_STEP_MS &&
			cs47l63_emul_read_count(EMUL, CS47L63_OUTPUT_STATUS_1) < OUT_POLL_MAX;
	     i++) {
		k_msleep(OUT_POLL_STEP_MS);
	}

	zassert_equal(cs47l63_emul_read_count(EMUL, CS47L63_OUTPUT_STATUS_1), OUT_POLL_MAX,
		      "the injection took effect: the status was polled to the bound and "
		      "stayed low throughout");

	val = cs47l63_emul_get_reg(EMUL, CS47L63_OUT1L_VOLUME_1);
	zassert_not_equal(val & CS47L63_OUT1L_MUTE, 0,
			  "a stage that never came up must not be unmuted");
}

ZTEST(cs47l63_out, test_volume_across_the_envelope)
{
	static const struct {
		int db;
		uint32_t code;
	} k_steps[] = {
		{0, VOL_CODE_0DB},
		{-20, VOL_CODE_M20DB},
		{CS47L63_VOLUME_MIN_DB, VOL_CODE_M64DB},
	};

	start_and_confirm();

	for (size_t i = 0; i < ARRAY_SIZE(k_steps); i++) {
		audio_property_value_t val = {.vol = k_steps[i].db};
		uint32_t reg;

		zassert_ok(audio_codec_set_property(CODEC, AUDIO_PROPERTY_OUTPUT_VOLUME,
						    AUDIO_CHANNEL_ALL, val));

		reg = cs47l63_emul_get_reg(EMUL, CS47L63_OUT1L_VOLUME_1);
		zassert_equal(reg & CS47L63_OUT1L_VOL_MASK, k_steps[i].code,
			      "%d dB must encode to 0x%02x at 0.5 dB per step", k_steps[i].db,
			      k_steps[i].code);
		zassert_not_equal(reg & CS47L63_OUT_VU, 0, "every volume write carries OUT_VU");
		zassert_equal(reg & CS47L63_OUT1L_MUTE, 0, "a volume set must not mute");
	}
}

ZTEST(cs47l63_out, test_volume_outside_the_envelope_is_clamped_not_wrapped)
{
	audio_property_value_t val;

	start_and_confirm();

	val.vol = CS47L63_VOLUME_MIN_DB - 36;
	zassert_ok(audio_codec_set_property(CODEC, AUDIO_PROPERTY_OUTPUT_VOLUME, AUDIO_CHANNEL_ALL,
					    val));
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_OUT1L_VOLUME_1) & CS47L63_OUT1L_VOL_MASK,
		      VOL_CODE_M64DB, "below the floor must clamp to the floor, not wrap high");

	val.vol = CS47L63_VOLUME_MAX_DB + 12;
	zassert_ok(audio_codec_set_property(CODEC, AUDIO_PROPERTY_OUTPUT_VOLUME, AUDIO_CHANNEL_ALL,
					    val));
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_OUT1L_VOLUME_1) & CS47L63_OUT1L_VOL_MASK,
		      VOL_CODE_0DB, "above the ceiling must clamp to the ceiling");
}

ZTEST(cs47l63_out, test_volume_set_while_stopped_survives_to_the_next_start)
{
	audio_property_value_t val = {.vol = -20};
	uint32_t writes_before = cs47l63_emul_write_count(EMUL, CS47L63_OUT1L_VOLUME_1);

	zassert_ok(audio_codec_set_property(CODEC, AUDIO_PROPERTY_OUTPUT_VOLUME, AUDIO_CHANNEL_ALL,
					    val));

	zassert_equal(cs47l63_emul_write_count(EMUL, CS47L63_OUT1L_VOLUME_1), writes_before,
		      "a stopped output must not be brought up to a level by a property set");

	start_and_confirm();

	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_OUT1L_VOLUME_1) & CS47L63_OUT1L_VOL_MASK,
		      VOL_CODE_M20DB, "the start must apply the level that was asked for");
}

ZTEST(cs47l63_out, test_mute_round_trip_returns_to_the_cached_level)
{
	audio_property_value_t vol = {.vol = -20};
	audio_property_value_t mute = {.mute = true};
	uint32_t reg;

	start_and_confirm();

	zassert_ok(audio_codec_set_property(CODEC, AUDIO_PROPERTY_OUTPUT_VOLUME, AUDIO_CHANNEL_ALL,
					    vol));
	zassert_ok(audio_codec_set_property(CODEC, AUDIO_PROPERTY_OUTPUT_MUTE, AUDIO_CHANNEL_ALL,
					    mute));

	reg = cs47l63_emul_get_reg(EMUL, CS47L63_OUT1L_VOLUME_1);
	zassert_not_equal(reg & CS47L63_OUT1L_MUTE, 0, "mute must reach the part");
	zassert_equal(reg & CS47L63_OUT1L_VOL_MASK, VOL_CODE_M20DB,
		      "muting must not discard the level");

	mute.mute = false;
	zassert_ok(audio_codec_set_property(CODEC, AUDIO_PROPERTY_OUTPUT_MUTE, AUDIO_CHANNEL_ALL,
					    mute));

	reg = cs47l63_emul_get_reg(EMUL, CS47L63_OUT1L_VOLUME_1);
	zassert_equal(reg & CS47L63_OUT1L_MUTE, 0);
	zassert_equal(reg & CS47L63_OUT1L_VOL_MASK, VOL_CODE_M20DB,
		      "unmuting returns to the level last set, not to a default");
}

ZTEST(cs47l63_out, test_stop_mutes_before_it_takes_the_amplifier_down)
{
	int mute_idx;
	int disable_idx;
	uint32_t val;

	start_and_confirm();

	/* Clear the status so the shutdown wait sees the stage go down. */
	cs47l63_emul_set_reg(EMUL, CS47L63_OUTPUT_STATUS_1, 0);
	audio_codec_stop_output(CODEC);

	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_OUTPUT_ENABLE_1) & CS47L63_OUT1L_EN, 0,
		      "the amplifier must be disabled");

	val = cs47l63_emul_get_reg(EMUL, CS47L63_OUT1L_VOLUME_1);
	zassert_not_equal(val & CS47L63_OUT1L_MUTE, 0, "and left muted");

	mute_idx = cs47l63_emul_write_index(EMUL, CS47L63_OUT1L_VOLUME_1,
					    cs47l63_emul_write_count(EMUL, CS47L63_OUT1L_VOLUME_1) -
						    1);
	disable_idx = cs47l63_emul_write_index(
		EMUL, CS47L63_OUTPUT_ENABLE_1,
		cs47l63_emul_write_count(EMUL, CS47L63_OUTPUT_ENABLE_1) - 1);
	zassert_true(mute_idx < disable_idx,
		     "the stage must be silent while it collapses, not after");
}

ZTEST(cs47l63_out, test_stop_without_a_clock_does_not_wait_for_a_status_that_cannot_change)
{
	uint32_t reads_before;

	start_and_confirm();

	/* The I2S transfer that fed FLL1 has stopped, so nothing is left
	 * running to update the amplifier's status bit. Waiting on it could
	 * only ever time out.
	 */
	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_STS_6, 0);
	reads_before = cs47l63_emul_read_count(EMUL, CS47L63_OUTPUT_STATUS_1);

	audio_codec_stop_output(CODEC);

	zassert_equal(cs47l63_emul_read_count(EMUL, CS47L63_OUTPUT_STATUS_1), reads_before,
		      "an unlocked loop means the write is taken at its word");
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_OUTPUT_ENABLE_1) & CS47L63_OUT1L_EN, 0,
		      "the amplifier still goes down");
}

ZTEST(cs47l63_out, test_route_output_selects_one_slot_or_both)
{
	zassert_ok(cs47l63_out_route_output(CODEC, AUDIO_CHANNEL_FRONT_LEFT, CS47L63_OUTPUT_HP));
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_OUT1L_INPUT1),
		      MIX_SLOT(CS47L63_MIXER_SRC_ASP1RX1));
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_OUT1L_INPUT2),
		      MIX_SLOT(CS47L63_MIXER_SRC_NONE),
		      "the unused slot must be muted, not left on the previous source");

	zassert_ok(
		cs47l63_out_route_output(CODEC, AUDIO_CHANNEL_HEADPHONE_RIGHT, CS47L63_OUTPUT_HP));
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_OUT1L_INPUT1),
		      MIX_SLOT(CS47L63_MIXER_SRC_ASP1RX2));

	zassert_ok(cs47l63_out_route_output(CODEC, AUDIO_CHANNEL_ALL, CS47L63_OUTPUT_HP));
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_OUT1L_INPUT1),
		      MIX_SLOT(CS47L63_MIXER_SRC_ASP1RX1));
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_OUT1L_INPUT2),
		      MIX_SLOT(CS47L63_MIXER_SRC_ASP1RX2));
}

ZTEST(cs47l63_out, test_absent_terminal_and_unaddressable_channel_are_refused)
{
	uint32_t writes_before = cs47l63_emul_write_count(EMUL, CS47L63_OUT1L_INPUT1);

	/* The line terminal does not exist on this part. Routing it to the
	 * headphone anyway would be indistinguishable, at the far end, from
	 * the driver having got the terminal wrong.
	 */
	zassert_equal(-ENOTSUP,
		      cs47l63_out_route_output(CODEC, AUDIO_CHANNEL_ALL, CS47L63_OUTPUT_LINE));
	zassert_equal(-ENOTSUP,
		      cs47l63_out_route_output(CODEC, AUDIO_CHANNEL_LFE, CS47L63_OUTPUT_HP));

	zassert_equal(cs47l63_emul_write_count(EMUL, CS47L63_OUT1L_INPUT1), writes_before,
		      "a refused route must leave the mixer alone");
}

ZTEST(cs47l63_out, test_per_side_property_and_unknown_property_refused)
{
	audio_property_value_t val = {.vol = 0};
	uint32_t writes_before = cs47l63_emul_write_count(EMUL, CS47L63_OUT1L_VOLUME_1);

	zassert_equal(-EINVAL,
		      audio_codec_set_property(CODEC, AUDIO_PROPERTY_OUTPUT_VOLUME,
					       AUDIO_CHANNEL_FRONT_LEFT, val),
		      "one physical channel: a per-side level cannot be expressed");
	zassert_equal(
		-ENOTSUP,
		audio_codec_set_property(CODEC, AUDIO_PROPERTY_EQ_GAIN, AUDIO_CHANNEL_ALL, val),
		"an unsupported property is refused, never accepted and dropped");

	zassert_equal(cs47l63_emul_write_count(EMUL, CS47L63_OUT1L_VOLUME_1), writes_before);
}

ZTEST(cs47l63_out, test_apply_properties_has_nothing_to_commit)
{
	uint32_t xfers_before = cs47l63_emul_xfer_count(EMUL);

	zassert_ok(audio_codec_apply_properties(CODEC));
	zassert_equal(cs47l63_emul_xfer_count(EMUL), xfers_before,
		      "every property is latched by OUT_VU as it is written");
}
