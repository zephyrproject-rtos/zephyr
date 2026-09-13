/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file test_fault.c
 * @brief Fault decode, the sticky shadow, and the error callback
 *
 * Every flag this driver watches is a write-1-to-clear edge latch, which the
 * emulator models. That is what makes "reported once per occurrence" different
 * from "reported once per poll", and it is what the cases below turn on.
 *
 * The last two cases are the regression anchor for a real defect: the clock
 * latches must not be reported while the output is stopped. FLL1's reference
 * is the SoC's I2S master clock, which stops with the stream, so REF_LOST,
 * LOCK_FALL and the SYSCLK flags all latch on the way down by design. Reporting
 * them tells the application its codec has failed every time it stops playing.
 */

#include <errno.h>
#include <stdint.h>

#include <zephyr/audio/codec.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include "cs47l63_clock.h"
#include "cs47l63_fault.h"
#include "cs47l63_out.h"
#include "cs47l63_regs.h"

#include "cs47l63_emul.h"

#define CODEC DEVICE_DT_GET(DT_NODELABEL(cs47l63_fault))
#define EMUL  EMUL_DT_GET(DT_NODELABEL(cs47l63_fault))

/** Every flag the driver reads out of each interrupt register. */
#define EINT_1_WATCHED                                                                             \
	(CS47L63_OUT1L_SC_EINT1 | CS47L63_SYSCLK_ERR_EINT1 | CS47L63_SYSCLK_FAIL_EINT1)
#define EINT_6_WATCHED (CS47L63_FLL1_REF_LOST_EINT1 | CS47L63_FLL1_LOCK_FALL_EINT1)

/** The clock half alone: everything except the short-circuit flag. */
#define EINT_1_CLOCK (CS47L63_SYSCLK_ERR_EINT1 | CS47L63_SYSCLK_FAIL_EINT1)

/**
 * @brief Upper bound on the deferred check, in milliseconds.
 *
 * The settle the driver waits before it looks, plus the amplifier poll it may
 * then spend giving up. That poll is 50 reads at a 2 ms sleep each, and a sleep
 * shorter than the tick costs a whole tick, so the bound has to allow for the
 * tick and not for the nominal 100 ms.
 */
#define FAULT_REPORT_WAIT_MS (CS47L63_FLL_LOCK_SETTLE_MS + 3000)

/** Granularity of the wait below. */
#define FAULT_REPORT_POLL_MS 20

static uint32_t s_cb_errors;
static int s_cb_calls;

static void fault_cb(const struct device *dev, uint32_t errors)
{
	zassert_equal(dev, CODEC, "the callback must receive the codec it was registered on");
	s_cb_errors = errors;
	s_cb_calls++;
}

/**
 * @brief Wait, bounded, for the deferred check to report a fault.
 *
 * Waiting on the outcome rather than on a fixed delay: the check runs on the
 * system work queue after a settle, and how long it then takes to give up
 * depends on the tick rate rather than on anything this test controls. A fixed
 * sleep that is a tick too short reads as "no fault reported", which is the
 * exact answer a broken driver would give.
 */
static void wait_for_fault_report(void)
{
	for (int i = 0; i < FAULT_REPORT_WAIT_MS / FAULT_REPORT_POLL_MS && s_cb_calls == 0; i++) {
		k_msleep(FAULT_REPORT_POLL_MS);
	}
}

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
 * @brief Bring the output up without the half-second the class API waits.
 *
 * The state that matters here is output_running, which the deferred check sets
 * once the amplifier reports enabled. Reaching it directly keeps the fault
 * cases about faults.
 */
static void mark_output_running(void)
{
	cs47l63_emul_set_reg(EMUL, CS47L63_OUTPUT_STATUS_1, CS47L63_OUT1L_EN_STS);
	zassert_ok(cs47l63_out_confirm_start(CODEC));
}

/** @brief Latch the clock flags on the part, and prove they are there. */
static void inject_clock_latches(void)
{
	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_EINT_1, EINT_1_CLOCK);
	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_EINT_6, EINT_6_WATCHED);

	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_IRQ1_EINT_1), EINT_1_CLOCK,
		      "the injection must be on the part before the driver looks");
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_IRQ1_EINT_6), EINT_6_WATCHED);
}

/** @brief Assert the driver consumed the latches it was given. */
static void assert_latches_were_read_and_cleared(void)
{
	zassert_true(cs47l63_emul_read_count(EMUL, CS47L63_IRQ1_EINT_1) >= 1,
		     "the flags must be read, not ignored");
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_IRQ1_EINT_1) & EINT_1_WATCHED, 0,
		      "a latch left set would be re-reported long after the condition passed");
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_IRQ1_EINT_6) & EINT_6_WATCHED, 0);
}

static void before_each(void *fixture)
{
	ARG_UNUSED(fixture);

	cs47l63_emul_reset(EMUL);
	configure();

	s_cb_errors = 0;
	s_cb_calls = 0;
	zassert_ok(audio_codec_register_error_callback(CODEC, fault_cb));
}

static void after_each(void *fixture)
{
	ARG_UNUSED(fixture);

	/* Drops any deferred start check, so it cannot fire inside the next
	 * case and report a fault that case did not raise.
	 */
	audio_codec_stop_output(CODEC);
	zassert_ok(audio_codec_register_error_callback(CODEC, NULL));
}

ZTEST_SUITE(cs47l63_fault, NULL, NULL, before_each, after_each, NULL);

ZTEST(cs47l63_fault, test_callback_registration_and_clear_are_implemented)
{
	zassert_ok(audio_codec_register_error_callback(CODEC, fault_cb));
	zassert_ok(audio_codec_clear_errors(CODEC));
}

ZTEST(cs47l63_fault, test_short_circuit_is_reported_as_overcurrent)
{
	mark_output_running();

	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_EINT_1, CS47L63_OUT1L_SC_EINT1);
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_IRQ1_EINT_1), CS47L63_OUT1L_SC_EINT1,
		      "the injection must be on the part before the driver looks");

	zassert_ok(cs47l63_fault_check(CODEC));

	assert_latches_were_read_and_cleared();
	zassert_equal(s_cb_calls, 1, "the callback must fire exactly once for a fresh fault");
	zassert_equal(s_cb_errors, AUDIO_CODEC_ERROR_OVERCURRENT);
}

ZTEST(cs47l63_fault, test_no_fault_no_callback)
{
	uint32_t writes_before;

	mark_output_running();
	writes_before = cs47l63_emul_write_count(EMUL, CS47L63_IRQ1_EINT_1);

	zassert_ok(cs47l63_fault_check(CODEC));

	zassert_equal(s_cb_calls, 0, "nothing was raised, so nothing may be reported");
	zassert_equal(cs47l63_emul_write_count(EMUL, CS47L63_IRQ1_EINT_1), writes_before,
		      "with no flags set there is nothing to clear either");
}

ZTEST(cs47l63_fault, test_a_still_present_fault_is_not_reported_twice)
{
	mark_output_running();

	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_EINT_1, CS47L63_OUT1L_SC_EINT1);
	zassert_ok(cs47l63_fault_check(CODEC));
	zassert_equal(s_cb_calls, 1);

	/* The condition is still present, so the edge latch sets again. */
	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_EINT_1, CS47L63_OUT1L_SC_EINT1);
	zassert_ok(cs47l63_fault_check(CODEC));

	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_IRQ1_EINT_1) & EINT_1_WATCHED, 0,
		      "the injection took effect: the second latch was read and cleared too");
	zassert_equal(s_cb_calls, 1, "once per occurrence, not once per poll");
}

ZTEST(cs47l63_fault, test_a_latched_fault_survives_a_poll_that_no_longer_sees_it)
{
	mark_output_running();

	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_EINT_1, CS47L63_OUT1L_SC_EINT1);
	zassert_ok(cs47l63_fault_check(CODEC));
	zassert_equal(s_cb_calls, 1);
	zassert_equal(s_cb_errors, AUDIO_CODEC_ERROR_OVERCURRENT);

	/* A poll with nothing set. The shadow must not be forgotten by it. */
	zassert_ok(cs47l63_fault_check(CODEC));
	zassert_equal(s_cb_calls, 1, "a quiet poll is not an event");

	/* A different, genuinely new fault. The mask handed to the callback is
	 * the accumulated one, so the earlier bit showing up here is the proof
	 * that it survived the quiet poll in between.
	 */
	inject_clock_latches();
	zassert_ok(cs47l63_fault_check(CODEC));

	zassert_equal(s_cb_calls, 2);
	zassert_equal(s_cb_errors, AUDIO_CODEC_ERROR_OVERCURRENT | CS47L63_ERROR_CLOCK,
		      "the latched over-current must still be in the reported mask");
}

ZTEST(cs47l63_fault, test_clear_errors_clears_the_part_as_well_as_the_shadow)
{
	mark_output_running();

	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_EINT_1, CS47L63_OUT1L_SC_EINT1);
	zassert_ok(cs47l63_fault_check(CODEC));
	zassert_equal(s_cb_calls, 1);

	/* Leave flags set on the part, so a clear that only forgot locally
	 * would be caught by the register check below.
	 */
	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_EINT_1, EINT_1_WATCHED);
	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_EINT_6, EINT_6_WATCHED);

	zassert_ok(audio_codec_clear_errors(CODEC));

	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_IRQ1_EINT_1) & EINT_1_WATCHED, 0,
		      "the part's own flags must be cleared, or the next poll re-reports them");
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_IRQ1_EINT_6) & EINT_6_WATCHED, 0);

	/* The same fault again must now count as fresh. */
	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_EINT_1, CS47L63_OUT1L_SC_EINT1);
	zassert_ok(cs47l63_fault_check(CODEC));

	zassert_equal(s_cb_calls, 2, "clearing must make the shadow forget too");
	zassert_equal(s_cb_errors, AUDIO_CODEC_ERROR_OVERCURRENT);
}

ZTEST(cs47l63_fault, test_start_output_polls_for_faults)
{
	mark_output_running();

	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_EINT_1, CS47L63_OUT1L_SC_EINT1);

	/* The driver does not service the part's interrupt line, so the class
	 * operations that already touch the output stage stand in for one.
	 */
	audio_codec_start_output(CODEC);

	zassert_equal(s_cb_calls, 1, "start_output must poll the fault flags");
	zassert_equal(s_cb_errors, AUDIO_CODEC_ERROR_OVERCURRENT);
}

ZTEST(cs47l63_fault, test_volume_set_polls_for_faults)
{
	audio_property_value_t val = {.vol = 0};

	mark_output_running();

	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_EINT_1, CS47L63_OUT1L_SC_EINT1);

	zassert_ok(audio_codec_set_property(CODEC, AUDIO_PROPERTY_OUTPUT_VOLUME, AUDIO_CHANNEL_ALL,
					    val));

	zassert_equal(s_cb_calls, 1, "a property set must poll the fault flags");
	zassert_equal(s_cb_errors, AUDIO_CODEC_ERROR_OVERCURRENT);
}

ZTEST(cs47l63_fault, test_bus_failure_during_a_check_propagates_and_reports_nothing)
{
	mark_output_running();

	cs47l63_emul_fail_at(EMUL, CS47L63_IRQ1_EINT_1);

	zassert_equal(-EIO, cs47l63_fault_check(CODEC));

	zassert_equal(cs47l63_emul_read_count(EMUL, CS47L63_IRQ1_EINT_1), 0,
		      "the injection took effect: the read never completed");
	zassert_equal(s_cb_calls, 0, "a bus failure is not a codec fault");
}

ZTEST(cs47l63_fault, test_no_callback_still_latches_and_clears)
{
	mark_output_running();
	zassert_ok(audio_codec_register_error_callback(CODEC, NULL));

	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_EINT_1, CS47L63_OUT1L_SC_EINT1);
	zassert_ok(cs47l63_fault_check(CODEC));

	assert_latches_were_read_and_cleared();
	zassert_equal(s_cb_calls, 0, "an unregistered callback must not be called");

	/* The fault was latched all the same, so re-registering and polling the
	 * same still-present condition reports nothing new.
	 */
	zassert_ok(audio_codec_register_error_callback(CODEC, fault_cb));
	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_EINT_1, CS47L63_OUT1L_SC_EINT1);
	zassert_ok(cs47l63_fault_check(CODEC));

	zassert_equal(s_cb_calls, 0, "nothing accumulated while no callback was registered");
}

ZTEST(cs47l63_fault, test_over_current_is_reported_even_while_the_output_is_stopped)
{
	/* A short circuit is a real fault whatever the stream is doing. This is
	 * the control for the two cases below: it proves a stopped output is
	 * still polled and still reports.
	 */
	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_EINT_1, CS47L63_OUT1L_SC_EINT1);

	zassert_ok(cs47l63_fault_check(CODEC));

	assert_latches_were_read_and_cleared();
	zassert_equal(s_cb_calls, 1);
	zassert_equal(s_cb_errors, AUDIO_CODEC_ERROR_OVERCURRENT);
}

ZTEST(cs47l63_fault, test_clock_latches_are_not_reported_while_the_output_is_stopped)
{
	/* The regression anchor. Stopping a route takes FLL1's reference away
	 * by design, so these four latches say nothing except that the stream
	 * stopped.
	 */
	inject_clock_latches();

	zassert_ok(cs47l63_fault_check(CODEC));

	/* The flags were read and cleared, so the silence below is a decision
	 * the driver made and not a poll it never performed.
	 */
	assert_latches_were_read_and_cleared();
	zassert_equal(s_cb_calls, 0,
		      "a clock that stopped because the stream stopped is not a fault");
}

ZTEST(cs47l63_fault, test_clock_latches_are_reported_while_the_output_is_running)
{
	mark_output_running();

	inject_clock_latches();

	zassert_ok(cs47l63_fault_check(CODEC));

	assert_latches_were_read_and_cleared();
	zassert_equal(s_cb_calls, 1, "the same latches, with the output up, are a real fault");
	zassert_equal(s_cb_errors, CS47L63_ERROR_CLOCK,
		      "the class API has no bit for a clock failure, so the driver adds one");
}

ZTEST(cs47l63_fault, test_a_reference_that_never_arrives_is_reported_by_the_deferred_check)
{
	/* The other half of the anchor: suppressing the latches while stopped
	 * must not lose a reference that genuinely never turns up. The deferred
	 * check reads the live lock bit once the stream has had time to start.
	 */
	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_STS_6, 0);
	cs47l63_emul_set_reg(EMUL, CS47L63_OUTPUT_STATUS_1, CS47L63_OUT1L_EN_STS);

	audio_codec_start_output(CODEC);
	wait_for_fault_report();

	zassert_true(cs47l63_emul_read_count(EMUL, CS47L63_IRQ1_STS_6) >= 1,
		     "the injection took effect: the lock bit was read and was clear");
	zassert_equal(s_cb_calls, 1, "an unlocked loop after the settle is a real fault");
	zassert_equal(s_cb_errors, CS47L63_ERROR_CLOCK);
}

ZTEST(cs47l63_fault, test_an_amplifier_that_never_enables_is_reported_by_the_deferred_check)
{
	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_STS_6, CS47L63_FLL1_LOCK_STS1);
	cs47l63_emul_set_reg(EMUL, CS47L63_OUTPUT_STATUS_1, 0);

	audio_codec_start_output(CODEC);
	wait_for_fault_report();

	zassert_true(cs47l63_emul_read_count(EMUL, CS47L63_OUTPUT_STATUS_1) > 1,
		     "the injection took effect: the status was polled and stayed low");
	zassert_equal(s_cb_calls, 1);
	zassert_equal(s_cb_errors, CS47L63_ERROR_OUTPUT,
		      "a stage that never came up is a dead output, not a clock problem");
}
