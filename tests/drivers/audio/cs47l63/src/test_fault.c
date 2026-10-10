/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file test_fault.c
 * @brief Fault decode, the sticky shadow, and the error callback
 *
 * Watched flags are write-1-to-clear edge latches, so a fault is reported once per
 * occurrence, not once per poll.
 */

#include <errno.h>
#include <stdint.h>

#include <zephyr/audio/codec.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include "cs47l63.h"

#include "cs47l63_emul.h"

#define CODEC DEVICE_DT_GET(DT_NODELABEL(cs47l63_fault))
#define EMUL  EMUL_DT_GET(DT_NODELABEL(cs47l63_fault))

#define EINT_1_WATCHED                                                                             \
	(CS47L63_OUT1L_SC_EINT1 | CS47L63_SYSCLK_ERR_EINT1 | CS47L63_SYSCLK_FAIL_EINT1)
#define EINT_6_WATCHED (CS47L63_FLL1_REF_LOST_EINT1 | CS47L63_FLL1_LOCK_FALL_EINT1)

#define EINT_1_CLOCK (CS47L63_SYSCLK_ERR_EINT1 | CS47L63_SYSCLK_FAIL_EINT1)

/** Settle plus the 50 x 2 ms amplifier poll, each sleep possibly rounded up to a tick. */
#define FAULT_REPORT_WAIT_MS (CS47L63_FLL_LOCK_SETTLE_MS + 3000)

/** Margin over the settle the driver waits before completing a start. */
#define START_WAIT_MS (CS47L63_FLL_LOCK_SETTLE_MS + 100)

#define FAULT_REPORT_POLL_MS 20

static uint32_t s_cb_errors;
static int s_cb_calls;
/* IRQ1_EINT_1 reads before the last poll_faults(); the output start polls too. */
static uint32_t s_eint1_reads;

static void fault_cb(const struct device *dev, uint32_t errors)
{
	zassert_equal(dev, CODEC, "the callback must receive the codec it was registered on");
	s_cb_errors = errors;
	s_cb_calls++;
}

/** Waits on the outcome, bounded: the check's duration depends on the tick rate. */
static void wait_for_fault_report(void)
{
	for (int i = 0; i < FAULT_REPORT_WAIT_MS / FAULT_REPORT_POLL_MS && s_cb_calls == 0; i++) {
		k_msleep(FAULT_REPORT_POLL_MS);
	}
}

/** Starts the output and waits for the deferred check to confirm it. */
static void mark_output_running(void)
{
	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_STS_6, CS47L63_FLL1_LOCK_STS1);
	cs47l63_emul_set_reg(EMUL, CS47L63_OUTPUT_STATUS_1, CS47L63_OUT1L_EN_STS);
	audio_codec_start_output(CODEC);
	k_msleep(START_WAIT_MS);

	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_OUT1L_VOLUME_1) & CS47L63_OUT1L_MUTE, 0,
		      "the premise: the start check confirmed the output");
	zassert_equal(s_cb_calls, 0, "the premise: the start itself raised nothing");
}

/** Polls the fault flags through an output mute set that changes nothing. */
static void poll_faults(void)
{
	audio_property_value_t unmute = {.mute = false};

	s_eint1_reads = cs47l63_emul_read_count(EMUL, CS47L63_IRQ1_EINT_1);
	zassert_ok(audio_codec_set_property(CODEC, AUDIO_PROPERTY_OUTPUT_MUTE, AUDIO_CHANNEL_ALL,
					    unmute));
}

static void inject_clock_latches(void)
{
	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_EINT_1, EINT_1_CLOCK);
	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_EINT_6, EINT_6_WATCHED);

	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_IRQ1_EINT_1), EINT_1_CLOCK,
		      "the injection must be on the part before the driver looks");
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_IRQ1_EINT_6), EINT_6_WATCHED);
}

static void assert_latches_were_read_and_cleared(void)
{
	zassert_true(cs47l63_emul_read_count(EMUL, CS47L63_IRQ1_EINT_1) > s_eint1_reads,
		     "the flags must be read, not ignored");
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_IRQ1_EINT_1) & EINT_1_WATCHED, 0,
		      "a latch left set would be re-reported long after the condition passed");
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_IRQ1_EINT_6) & EINT_6_WATCHED, 0);
}

static void before_each(void *fixture)
{
	ARG_UNUSED(fixture);

	cs47l63_emul_reset(EMUL);
	zassert_ok(cs47l63_test_configure(CODEC, AUDIO_ROUTE_PLAYBACK_CAPTURE));

	s_cb_errors = 0;
	s_cb_calls = 0;
	zassert_ok(audio_codec_register_error_callback(CODEC, fault_cb));
}

static void after_each(void *fixture)
{
	ARG_UNUSED(fixture);

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

	poll_faults();

	assert_latches_were_read_and_cleared();
	zassert_equal(s_cb_calls, 1, "the callback must fire exactly once for a fresh fault");
	zassert_equal(s_cb_errors, AUDIO_CODEC_ERROR_OVERCURRENT);
}

ZTEST(cs47l63_fault, test_no_fault_no_callback)
{
	uint32_t writes_before;

	mark_output_running();
	writes_before = cs47l63_emul_write_count(EMUL, CS47L63_IRQ1_EINT_1);

	poll_faults();

	zassert_equal(s_cb_calls, 0, "nothing was raised, so nothing may be reported");
	zassert_equal(cs47l63_emul_write_count(EMUL, CS47L63_IRQ1_EINT_1), writes_before,
		      "with no flags set there is nothing to clear either");
}

ZTEST(cs47l63_fault, test_a_still_present_fault_is_not_reported_twice)
{
	mark_output_running();

	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_EINT_1, CS47L63_OUT1L_SC_EINT1);
	poll_faults();
	zassert_equal(s_cb_calls, 1);

	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_EINT_1, CS47L63_OUT1L_SC_EINT1);
	poll_faults();

	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_IRQ1_EINT_1) & EINT_1_WATCHED, 0,
		      "the injection took effect: the second latch was read and cleared too");
	zassert_equal(s_cb_calls, 1, "once per occurrence, not once per poll");
}

ZTEST(cs47l63_fault, test_a_latched_fault_survives_a_poll_that_no_longer_sees_it)
{
	mark_output_running();

	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_EINT_1, CS47L63_OUT1L_SC_EINT1);
	poll_faults();
	zassert_equal(s_cb_calls, 1);
	zassert_equal(s_cb_errors, AUDIO_CODEC_ERROR_OVERCURRENT);

	/* A quiet poll must not clear the shadow. */
	poll_faults();
	zassert_equal(s_cb_calls, 1, "a quiet poll is not an event");

	/* The callback mask is accumulated, so the earlier bit here proves it survived. */
	inject_clock_latches();
	poll_faults();

	zassert_equal(s_cb_calls, 2);
	zassert_equal(s_cb_errors, AUDIO_CODEC_ERROR_OVERCURRENT | CS47L63_ERROR_CLOCK,
		      "the latched over-current must still be in the reported mask");
}

ZTEST(cs47l63_fault, test_clear_errors_clears_the_part_as_well_as_the_shadow)
{
	mark_output_running();

	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_EINT_1, CS47L63_OUT1L_SC_EINT1);
	poll_faults();
	zassert_equal(s_cb_calls, 1);

	/* Flags left set on the part catch a clear that only forgets locally. */
	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_EINT_1, EINT_1_WATCHED);
	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_EINT_6, EINT_6_WATCHED);

	zassert_ok(audio_codec_clear_errors(CODEC));

	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_IRQ1_EINT_1) & EINT_1_WATCHED, 0,
		      "the part's own flags must be cleared, or the next poll re-reports them");
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_IRQ1_EINT_6) & EINT_6_WATCHED, 0);

	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_EINT_1, CS47L63_OUT1L_SC_EINT1);
	poll_faults();

	zassert_equal(s_cb_calls, 2, "clearing must make the shadow forget too");
	zassert_equal(s_cb_errors, AUDIO_CODEC_ERROR_OVERCURRENT);
}

ZTEST(cs47l63_fault, test_start_output_polls_for_faults)
{
	mark_output_running();

	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_EINT_1, CS47L63_OUT1L_SC_EINT1);

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

ZTEST(cs47l63_fault, test_bus_failure_during_a_check_reports_nothing)
{
	mark_output_running();

	/* The first read sees an overcurrent; the second fails, so nothing is reported. */
	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_EINT_1, CS47L63_OUT1L_SC_EINT1);
	cs47l63_emul_fail_at(EMUL, CS47L63_IRQ1_EINT_6);

	poll_faults();

	zassert_true(cs47l63_emul_read_count(EMUL, CS47L63_IRQ1_EINT_1) > s_eint1_reads,
		     "the check ran");
	zassert_equal(cs47l63_emul_fail_hits(EMUL), 1,
		      "the injection took effect: the EINT_6 read was attempted and failed");
	zassert_equal(s_cb_calls, 0, "a bus failure is not a codec fault");
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_IRQ1_EINT_1), CS47L63_OUT1L_SC_EINT1,
		      "a failed check leaves the latch for the next one");
}

ZTEST(cs47l63_fault, test_no_callback_still_latches_and_clears)
{
	mark_output_running();
	zassert_ok(audio_codec_register_error_callback(CODEC, NULL));

	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_EINT_1, CS47L63_OUT1L_SC_EINT1);
	poll_faults();

	assert_latches_were_read_and_cleared();
	zassert_equal(s_cb_calls, 0, "an unregistered callback must not be called");

	/* Latched anyway, so the same condition reports nothing new. */
	zassert_ok(audio_codec_register_error_callback(CODEC, fault_cb));
	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_EINT_1, CS47L63_OUT1L_SC_EINT1);
	poll_faults();

	zassert_equal(s_cb_calls, 0, "nothing accumulated while no callback was registered");
}

ZTEST(cs47l63_fault, test_over_current_is_reported_even_while_the_output_is_stopped)
{
	/* Control for the two cases below: a stopped output still reports a short. */
	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_EINT_1, CS47L63_OUT1L_SC_EINT1);

	poll_faults();

	assert_latches_were_read_and_cleared();
	zassert_equal(s_cb_calls, 1);
	zassert_equal(s_cb_errors, AUDIO_CODEC_ERROR_OVERCURRENT);
}

ZTEST(cs47l63_fault, test_clock_latches_are_not_reported_while_the_output_is_stopped)
{
	inject_clock_latches();

	poll_faults();

	/* Read and cleared, so the silence below is not a missing poll. */
	assert_latches_were_read_and_cleared();
	zassert_equal(s_cb_calls, 0,
		      "a clock that stopped because the stream stopped is not a fault");
}

ZTEST(cs47l63_fault, test_clock_latches_are_reported_while_the_output_is_running)
{
	mark_output_running();

	inject_clock_latches();

	poll_faults();

	assert_latches_were_read_and_cleared();
	zassert_equal(s_cb_calls, 1, "the same latches, with the output up, are a real fault");
	zassert_equal(s_cb_errors, CS47L63_ERROR_CLOCK,
		      "the class API has no bit for a clock failure, so the driver adds one");
}

ZTEST(cs47l63_fault, test_a_reference_that_never_arrives_is_reported_by_the_deferred_check)
{
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
