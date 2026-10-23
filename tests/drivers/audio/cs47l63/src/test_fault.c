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
#include <zephyr/sys/atomic.h>
#include <zephyr/ztest.h>

#include "cs47l63.h"

#include "cs47l63_emul.h"

#define CODEC DEVICE_DT_GET(DT_NODELABEL(cs47l63_fault))
#define EMUL  EMUL_DT_GET(DT_NODELABEL(cs47l63_fault))

#define EINT_1_WATCHED (CS47L63_SYSCLK_ERR_EINT1 | CS47L63_SYSCLK_FAIL_EINT1)
#define EINT_6_WATCHED (CS47L63_FLL1_REF_LOST_EINT1 | CS47L63_FLL1_LOCK_FALL_EINT1)

#define EINT_1_CLOCK EINT_1_WATCHED

#define CLOCK_ERROR ((uint32_t)AUDIO_CODEC_ERROR_CLOCK)

/** The driver's CS47L63_OUT_POLL_MS: the start check runs again after it. */
#define OUT_POLL_MS 2U

/** Margin over the deadline of the start check. */
#define START_WAIT_MS (CS47L63_START_TIMEOUT_MS + 100U)

#define MID_SETTLE_WAIT_MS (CS47L63_START_TIMEOUT_MS / 2U)

static atomic_t s_cb_errors;
static atomic_t s_cb_calls;
static atomic_t s_cb_other_dev_calls;
/* IRQ1_EINT_1 reads before the last poll_faults(); the output start polls too. */
static uint32_t s_eint1_reads;

static void fault_cb(const struct device *dev, uint32_t errors)
{
	if (dev != CODEC) {
		(void)atomic_inc(&s_cb_other_dev_calls);
	}
	(void)atomic_set(&s_cb_errors, (atomic_val_t)errors);
	(void)atomic_inc(&s_cb_calls);
}

static void stopping_cb(const struct device *dev, uint32_t errors)
{
	audio_codec_stop_output(dev);
	(void)atomic_set(&s_cb_errors, (atomic_val_t)errors);
	(void)atomic_inc(&s_cb_calls);
}

static void restarting_cb(const struct device *dev, uint32_t errors)
{
	(void)atomic_set(&s_cb_errors, (atomic_val_t)errors);
	if (atomic_inc(&s_cb_calls) == 0) {
		audio_codec_stop_output(dev);
		audio_codec_start_output(dev);
	}
}

static uint32_t cb_errors(void)
{
	return (uint32_t)atomic_get(&s_cb_errors);
}

static atomic_val_t cb_calls(void)
{
	zassert_equal(atomic_get(&s_cb_other_dev_calls), 0,
		      "the callback must receive the codec it was registered on");

	return atomic_get(&s_cb_calls);
}

/** Waits, one tick at a time, for a report or for the check's deadline to pass. */
static void wait_for_fault_report(void)
{
	for (uint32_t i = 0U; i < k_ms_to_ticks_ceil32(START_WAIT_MS); i++) {
		if (atomic_get(&s_cb_calls) != 0) {
			break;
		}
		(void)k_sleep(K_TICKS(1));
	}
}

/** Starts the output and waits for the deferred check to confirm it. */
static void mark_out_running(void)
{
	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_STS_6, CS47L63_FLL1_LOCK_STS1);
	cs47l63_emul_set_reg(EMUL, CS47L63_OUTPUT_STATUS_1, CS47L63_OUT1L_EN_STS);
	audio_codec_start_output(CODEC);
	(void)k_msleep((int32_t)START_WAIT_MS);

	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_OUT1L_VOLUME_1) & CS47L63_OUT1L_MUTE, 0U,
		      "the premise: the start check confirmed the output");
	zassert_equal(cb_calls(), 0, "the premise: the start itself raised nothing");
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
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_IRQ1_EINT_1) & EINT_1_WATCHED, 0U,
		      "a latch left set would be re-reported long after the condition passed");
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_IRQ1_EINT_6) & EINT_6_WATCHED, 0U);
}

static void fault_before_each(void *fixture)
{
	ARG_UNUSED(fixture);

	cs47l63_emul_reset(EMUL);
	zassert_ok(cs47l63_test_configure(CODEC, AUDIO_ROUTE_PLAYBACK_CAPTURE));

	(void)atomic_set(&s_cb_errors, 0);
	(void)atomic_set(&s_cb_calls, 0);
	(void)atomic_set(&s_cb_other_dev_calls, 0);
	zassert_ok(audio_codec_register_error_callback(CODEC, fault_cb));
}

static void fault_after_each(void *fixture)
{
	ARG_UNUSED(fixture);

	audio_codec_stop_output(CODEC);
	zassert_ok(audio_codec_register_error_callback(CODEC, NULL));
	zassert_equal(atomic_get(&s_cb_other_dev_calls), 0,
		      "the callback must receive the codec it was registered on");
}

ZTEST_SUITE(cs47l63_fault, NULL, NULL, fault_before_each, fault_after_each, NULL);

static ZTEST(cs47l63_fault, test_callback_registration_and_clear_are_implemented)
{
	zassert_ok(audio_codec_register_error_callback(CODEC, fault_cb));
	zassert_ok(audio_codec_clear_errors(CODEC));
}

static ZTEST(cs47l63_fault, test_an_output_short_circuit_is_neither_reported_nor_cleared)
{
	mark_out_running();

	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_EINT_1, CS47L63_OUT1L_SC_EINT1);

	poll_faults();

	zassert_true(cs47l63_emul_read_count(EMUL, CS47L63_IRQ1_EINT_1) > s_eint1_reads,
		     "the injection was there to see: the flags were read");
	zassert_equal(cb_calls(), 0, "the driver does not report an output short");
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_IRQ1_EINT_1), CS47L63_OUT1L_SC_EINT1,
		      "nor clear a latch it does not watch");
}

static ZTEST(cs47l63_fault, test_no_fault_no_callback)
{
	uint32_t writes_before;

	mark_out_running();
	writes_before = cs47l63_emul_write_count(EMUL, CS47L63_IRQ1_EINT_1);

	poll_faults();

	zassert_equal(cb_calls(), 0, "nothing was raised, so nothing may be reported");
	zassert_equal(cs47l63_emul_write_count(EMUL, CS47L63_IRQ1_EINT_1), writes_before,
		      "with no flags set there is nothing to clear either");
}

static ZTEST(cs47l63_fault, test_a_still_present_fault_is_not_reported_twice)
{
	mark_out_running();

	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_EINT_1, CS47L63_SYSCLK_FAIL_EINT1);
	poll_faults();
	zassert_equal(cb_calls(), 1);

	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_EINT_1, CS47L63_SYSCLK_FAIL_EINT1);
	poll_faults();

	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_IRQ1_EINT_1) & EINT_1_WATCHED, 0U,
		      "the injection took effect: the second latch was read and cleared too");
	zassert_equal(cb_calls(), 1, "once per occurrence, not once per poll");
}

static ZTEST(cs47l63_fault, test_a_latched_fault_survives_a_poll_that_no_longer_sees_it)
{
	mark_out_running();

	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_EINT_1, CS47L63_SYSCLK_FAIL_EINT1);
	poll_faults();
	zassert_equal(cb_calls(), 1);
	zassert_equal(cb_errors(), CLOCK_ERROR);

	/* A quiet poll must not clear the shadow. */
	poll_faults();
	zassert_equal(cb_calls(), 1, "a quiet poll is not an event");

	inject_clock_latches();
	poll_faults();

	assert_latches_were_read_and_cleared();
	zassert_equal(cb_calls(), 1, "the fault is still latched, so it is not new");
}

static ZTEST(cs47l63_fault, test_clear_errors_clears_the_part_as_well_as_the_shadow)
{
	mark_out_running();

	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_EINT_1, CS47L63_SYSCLK_FAIL_EINT1);
	poll_faults();
	zassert_equal(cb_calls(), 1);

	/* Flags left set on the part catch a clear that only forgets locally. */
	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_EINT_1, EINT_1_WATCHED);
	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_EINT_6, EINT_6_WATCHED);

	zassert_ok(audio_codec_clear_errors(CODEC));

	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_IRQ1_EINT_1) & EINT_1_WATCHED, 0U,
		      "the part's own flags must be cleared, or the next poll re-reports them");
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_IRQ1_EINT_6) & EINT_6_WATCHED, 0U);

	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_EINT_1, CS47L63_SYSCLK_FAIL_EINT1);
	poll_faults();

	zassert_equal(cb_calls(), 2, "clearing must make the shadow forget too");
	zassert_equal(cb_errors(), CLOCK_ERROR);
}

static ZTEST(cs47l63_fault, test_start_output_polls_for_faults)
{
	mark_out_running();

	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_EINT_1, CS47L63_SYSCLK_FAIL_EINT1);

	audio_codec_start_output(CODEC);

	zassert_equal(cb_calls(), 1, "start_output must poll the fault flags");
	zassert_equal(cb_errors(), CLOCK_ERROR);
}

static ZTEST(cs47l63_fault, test_start_polls_for_faults_when_it_starts_the_output)
{
	uint32_t reads = cs47l63_emul_read_count(EMUL, CS47L63_IRQ1_EINT_1);

	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_EINT_1, CS47L63_SYSCLK_FAIL_EINT1);

	zassert_ok(audio_codec_start(CODEC, AUDIO_DAI_DIR_TX));

	zassert_true(cs47l63_emul_read_count(EMUL, CS47L63_IRQ1_EINT_1) > reads,
		     "a direction start of the output must poll the fault flags");
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_IRQ1_EINT_1) & EINT_1_WATCHED, 0U);
	zassert_equal(cb_calls(), 0, "the output was not running yet");
}

static ZTEST(cs47l63_fault, test_volume_set_polls_for_faults)
{
	audio_property_value_t val = {.vol = 0};

	mark_out_running();

	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_EINT_1, CS47L63_SYSCLK_FAIL_EINT1);

	zassert_ok(audio_codec_set_property(CODEC, AUDIO_PROPERTY_OUTPUT_VOLUME, AUDIO_CHANNEL_ALL,
					    val));

	zassert_equal(cb_calls(), 1, "a property set must poll the fault flags");
	zassert_equal(cb_errors(), CLOCK_ERROR);
}

static ZTEST(cs47l63_fault, test_bus_failure_during_a_check_reports_nothing)
{
	mark_out_running();

	/* The first read sees a clock fault; the second fails, so nothing is reported. */
	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_EINT_1, CS47L63_SYSCLK_FAIL_EINT1);
	cs47l63_emul_fail_at(EMUL, CS47L63_IRQ1_EINT_6);

	poll_faults();

	zassert_true(cs47l63_emul_read_count(EMUL, CS47L63_IRQ1_EINT_1) > s_eint1_reads,
		     "the check ran");
	zassert_equal(cs47l63_emul_fail_hits(EMUL), 1U,
		      "the injection took effect: the EINT_6 read was attempted and failed");
	zassert_equal(cb_calls(), 0, "a bus failure is not a codec fault");
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_IRQ1_EINT_1), CS47L63_SYSCLK_FAIL_EINT1,
		      "a failed check leaves the latch for the next one");
}

static ZTEST(cs47l63_fault, test_no_callback_still_latches_and_clears)
{
	mark_out_running();
	zassert_ok(audio_codec_register_error_callback(CODEC, NULL));

	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_EINT_1, CS47L63_SYSCLK_FAIL_EINT1);
	poll_faults();

	assert_latches_were_read_and_cleared();
	zassert_equal(cb_calls(), 0, "an unregistered callback must not be called");

	/* Latched anyway, so the same condition reports nothing new. */
	zassert_ok(audio_codec_register_error_callback(CODEC, fault_cb));
	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_EINT_1, CS47L63_SYSCLK_FAIL_EINT1);
	poll_faults();

	zassert_equal(cb_calls(), 0, "nothing accumulated while no callback was registered");
}


static ZTEST(cs47l63_fault, test_a_sysclk_latch_from_the_start_window_is_not_reported)
{
	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_STS_6, CS47L63_FLL1_LOCK_STS1);
	cs47l63_emul_set_reg(EMUL, CS47L63_OUTPUT_STATUS_1, CS47L63_OUT1L_EN_STS);
	audio_codec_start_output(CODEC);

	/* The edge SYSCLK_EN makes before MCLK1 runs, latched before the check confirms. */
	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_EINT_1, CS47L63_SYSCLK_FAIL_EINT1);
	(void)k_msleep((int32_t)START_WAIT_MS);
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_OUT1L_VOLUME_1) & CS47L63_OUT1L_MUTE, 0U,
		      "the premise: the start check confirmed the output");

	poll_faults();

	zassert_true(cs47l63_emul_read_count(EMUL, CS47L63_IRQ1_EINT_1) > s_eint1_reads,
		     "the flags were read");
	zassert_equal(cb_calls(), 0, "a latch from the start window is not a fault");
}

static ZTEST(cs47l63_fault, test_clock_latches_are_not_reported_while_the_output_is_stopped)
{
	inject_clock_latches();

	poll_faults();

	/* Read and cleared, so the silence below is not a missing poll. */
	assert_latches_were_read_and_cleared();
	zassert_equal(cb_calls(), 0,
		      "a clock that stopped because the stream stopped is not a fault");
}

static ZTEST(cs47l63_fault, test_clock_latches_are_reported_while_the_output_is_running)
{
	mark_out_running();

	inject_clock_latches();

	poll_faults();

	assert_latches_were_read_and_cleared();
	zassert_equal(cb_calls(), 1, "the same latches, with the output up, are a real fault");
	zassert_equal(cb_errors(), CLOCK_ERROR);
}

static ZTEST(cs47l63_fault, test_a_reference_that_never_arrives_is_reported_at_the_deadline)
{
	const uint32_t poll_ticks = k_ms_to_ticks_ceil32(OUT_POLL_MS);
	int64_t start;
	int64_t elapsed;

	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_STS_6, 0U);
	cs47l63_emul_set_reg(EMUL, CS47L63_OUTPUT_STATUS_1, 0U);

	start = k_uptime_ticks();
	audio_codec_start_output(CODEC);
	wait_for_fault_report();
	elapsed = k_uptime_ticks() - start;

	zassert_true(elapsed >= (int64_t)k_ms_to_ticks_floor32(CS47L63_START_TIMEOUT_MS),
		     "reported after %lld ticks, before the deadline", (long long)elapsed);
	zassert_true(elapsed <= (int64_t)(k_ms_to_ticks_ceil32(CS47L63_START_TIMEOUT_MS) +
					  poll_ticks + 1U),
		     "reported %lld ticks after the start, later than one poll past the deadline",
		     (long long)elapsed);

	zassert_true(cs47l63_emul_read_count(EMUL, CS47L63_IRQ1_STS_6) >= 1U,
		     "the injection took effect: the lock bit was read and was clear");
	zassert_equal(cb_calls(), 1, "an unlocked loop at the deadline is a real fault");
	zassert_equal(cb_errors(), CLOCK_ERROR);
}

static ZTEST(cs47l63_fault, test_an_amplifier_that_never_enables_raises_no_callback)
{
	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_STS_6, CS47L63_FLL1_LOCK_STS1);
	cs47l63_emul_set_reg(EMUL, CS47L63_OUTPUT_STATUS_1, 0U);

	audio_codec_start_output(CODEC);
	(void)k_msleep((int32_t)START_WAIT_MS);

	zassert_true(cs47l63_emul_read_count(EMUL, CS47L63_OUTPUT_STATUS_1) > 1U,
		     "the injection took effect: the status was polled and stayed low");
	zassert_equal(cb_calls(), 0, "the class API has no bit for a dead output: it is logged");
}

static ZTEST(cs47l63_fault,
	     test_a_callback_that_stops_the_output_is_not_undone_by_the_deferred_check)
{
	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_STS_6, 0U);
	cs47l63_emul_set_reg(EMUL, CS47L63_OUTPUT_STATUS_1, CS47L63_OUT1L_EN_STS);
	zassert_ok(audio_codec_register_error_callback(CODEC, stopping_cb));

	audio_codec_start_output(CODEC);
	wait_for_fault_report();

	zassert_equal(cb_errors(), (uint32_t)AUDIO_CODEC_ERROR_CLOCK,
		      "the injection took effect: the unlocked loop reached the callback");
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_OUTPUT_ENABLE_1) & CS47L63_OUT1L_EN, 0U,
		      "the callback's stop took the amplifier down");
	zassert_not_equal(cs47l63_emul_get_reg(EMUL, CS47L63_OUT1L_VOLUME_1) & CS47L63_OUT1L_MUTE,
			  0U, "the check must not unmute an output the callback stopped");
}

static ZTEST(cs47l63_fault, test_a_callback_that_restarts_the_output_leaves_it_to_the_new_check)
{
	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_STS_6, 0U);
	cs47l63_emul_set_reg(EMUL, CS47L63_OUTPUT_STATUS_1, CS47L63_OUT1L_EN_STS);
	zassert_ok(audio_codec_register_error_callback(CODEC, restarting_cb));

	audio_codec_start_output(CODEC);
	wait_for_fault_report();
	(void)k_msleep((int32_t)MID_SETTLE_WAIT_MS);

	zassert_equal(cb_errors(), (uint32_t)AUDIO_CODEC_ERROR_CLOCK,
		      "the injection took effect: the unlocked loop reached the callback");
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_OUTPUT_ENABLE_1) & CS47L63_OUT1L_EN,
		      CS47L63_OUT1L_EN, "the callback's start brought the amplifier back up");
	zassert_not_equal(cs47l63_emul_get_reg(EMUL, CS47L63_OUT1L_VOLUME_1) & CS47L63_OUT1L_MUTE,
			  0U, "the superseded check must not confirm the restarted output");

	(void)k_msleep((int32_t)START_WAIT_MS);

	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_OUT1L_VOLUME_1) & CS47L63_OUT1L_MUTE, 0U,
		      "the restart's own check confirmed the output");
}

static ZTEST(cs47l63_fault, test_a_failed_eint6_clear_still_reports_and_clears_eint1)
{
	mark_out_running();

	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_EINT_1, CS47L63_SYSCLK_FAIL_EINT1);
	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_EINT_6, CS47L63_FLL1_REF_LOST_EINT1);
	cs47l63_emul_fail_write_at(EMUL, CS47L63_IRQ1_EINT_6);

	poll_faults();

	zassert_equal(cs47l63_emul_fail_hits(EMUL), 1U,
		      "the injection took effect: the EINT_6 clear was attempted and failed");
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_IRQ1_EINT_1) & EINT_1_WATCHED, 0U);
	zassert_equal(cb_calls(), 1, "a failed clear must not lose the fault already decoded");
	zassert_equal(cb_errors(), CLOCK_ERROR);
}

static ZTEST(cs47l63_fault, test_a_failed_eint1_clear_still_clears_eint6_and_reports)
{
	mark_out_running();

	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_EINT_1, CS47L63_SYSCLK_FAIL_EINT1);
	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_EINT_6, CS47L63_FLL1_REF_LOST_EINT1);
	cs47l63_emul_fail_write_at(EMUL, CS47L63_IRQ1_EINT_1);

	poll_faults();

	zassert_equal(cs47l63_emul_fail_hits(EMUL), 1U,
		      "the injection took effect: the EINT_1 clear was attempted and failed");
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_IRQ1_EINT_6) & EINT_6_WATCHED, 0U,
		      "the EINT_6 clear is still attempted");
	zassert_equal(cb_calls(), 1, "a failed clear must not lose the fault already decoded");
	zassert_equal(cb_errors(), CLOCK_ERROR);
}

static ZTEST(cs47l63_fault, test_a_status_read_failure_is_not_a_dead_output)
{
	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_STS_6, CS47L63_FLL1_LOCK_STS1);
	cs47l63_emul_fail_at(EMUL, CS47L63_OUTPUT_STATUS_1);

	audio_codec_start_output(CODEC);
	(void)k_msleep((int32_t)START_WAIT_MS);

	zassert_true(cs47l63_emul_fail_hits(EMUL) > 0U,
		     "the injection took effect: the deferred check read the status and failed");
	zassert_equal(cs47l63_emul_fail_hits(EMUL), 1U, "a failed read ends the check");
	zassert_equal(cb_calls(), 0, "a bus error is not a dead output");
}

static ZTEST(cs47l63_fault, test_a_volume_write_failure_after_the_stage_is_up_is_not_a_dead_output)
{
	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_STS_6, CS47L63_FLL1_LOCK_STS1);
	cs47l63_emul_set_reg(EMUL, CS47L63_OUTPUT_STATUS_1, CS47L63_OUT1L_EN_STS);
	cs47l63_emul_fail_write_at(EMUL, CS47L63_OUT1L_VOLUME_1);

	audio_codec_start_output(CODEC);
	(void)k_msleep((int32_t)START_WAIT_MS);

	zassert_true(cs47l63_emul_fail_hits(EMUL) > 0U,
		     "the injection took effect: the stage was up and the level write failed");
	zassert_equal(cb_calls(), 0, "a bus error is not a dead output");
}
