/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file test_irq.c
 * @brief Faults reported from the irq-gpios line, without a control call
 *
 * The emulator drives the pin from the flags the masks leave enabled.
 */

#include <errno.h>
#include <stdint.h>

#include <zephyr/audio/codec.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/ztest.h>

#include "cs47l63.h"

#include "cs47l63_emul.h"

#define CODEC DEVICE_DT_GET(DT_NODELABEL(cs47l63_irq))
#define EMUL  EMUL_DT_GET(DT_NODELABEL(cs47l63_irq))

#define EINT_1_WATCHED                                                                             \
	(CS47L63_OUT1L_SC_EINT1 | CS47L63_SYSCLK_ERR_EINT1 | CS47L63_SYSCLK_FAIL_EINT1)
#define EINT_6_WATCHED (CS47L63_FLL1_REF_LOST_EINT1 | CS47L63_FLL1_LOCK_FALL_EINT1)

#define START_WAIT_MS (CS47L63_FLL_LOCK_SETTLE_MS + 100U)

/** Bound on a report that has no other signal; a service takes a few emulated transfers. */
#define REPORT_WAIT_MS 500U

#define SETTLE_MS 100U

#define PIN_POLL_MS    10U
#define PIN_POLL_COUNT 50U

#define MICB1B_ON (CS47L63_MICB1B_EN | CS47L63_MICB1B_SRC)

static const struct gpio_dt_spec s_irq = GPIO_DT_SPEC_GET(DT_NODELABEL(cs47l63_irq), irq_gpios);

static K_SEM_DEFINE(s_cb_sem, 0, K_SEM_MAX_LIMIT);
static atomic_t s_cb_errors;
static atomic_t s_cb_calls;
static atomic_t s_cb_other_dev_calls;
static void (*s_cb_hook)(void);

static void fault_cb(const struct device *dev, uint32_t errors)
{
	if (dev != CODEC) {
		(void)atomic_inc(&s_cb_other_dev_calls);
	}
	(void)atomic_set(&s_cb_errors, (atomic_val_t)errors);
	(void)atomic_inc(&s_cb_calls);
	if (s_cb_hook != NULL) {
		s_cb_hook();
	}
	k_sem_give(&s_cb_sem);
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

/** Nothing in the test calls the driver while it waits. */
static void wait_for_report(void)
{
	zassert_ok(k_sem_take(&s_cb_sem, K_MSEC(REPORT_WAIT_MS)),
		   "no fault was reported from the interrupt line");
}

static void wait_for_pin_released(void)
{
	for (uint32_t i = 0U; i < PIN_POLL_COUNT; i++) {
		if (!cs47l63_emul_get_irq_asserted(EMUL)) {
			return;
		}
		(void)k_msleep((int32_t)PIN_POLL_MS);
	}
}

static bool int_enabled(void)
{
	gpio_flags_t flags;

	zassert_ok(gpio_emul_flags_get_dt(&s_irq, &flags));

	return (flags & GPIO_INT_ENABLE) != 0U;
}

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

/* Nothing here blocks between the attach and the configure. The attach asserts the pin, because
 * boot done is latched and unmasked at reset, and queues a service if the driver is still armed
 * from the previous case. The configure disarms and cancels it before this thread yields.
 */
static void before_each(void *fixture)
{
	ARG_UNUSED(fixture);

	cs47l63_emul_reset(EMUL);
	cs47l63_emul_irq_attach(EMUL, &s_irq);
	zassert_ok(cs47l63_test_configure(CODEC, AUDIO_ROUTE_PLAYBACK_CAPTURE));

	(void)atomic_set(&s_cb_errors, 0);
	(void)atomic_set(&s_cb_calls, 0);
	(void)atomic_set(&s_cb_other_dev_calls, 0);
	s_cb_hook = NULL;
	k_sem_reset(&s_cb_sem);
	zassert_ok(audio_codec_register_error_callback(CODEC, fault_cb));
}

static void after_each(void *fixture)
{
	ARG_UNUSED(fixture);

	audio_codec_stop_output(CODEC);
	s_cb_hook = NULL;
	zassert_ok(audio_codec_register_error_callback(CODEC, NULL));
}

ZTEST_SUITE(cs47l63_irq, NULL, NULL, before_each, after_each, NULL);

static ZTEST(cs47l63_irq, test_clock_latch_while_running_is_reported_without_a_poll)
{
	mark_out_running();

	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_EINT_1, CS47L63_SYSCLK_FAIL_EINT1);
	wait_for_report();
	wait_for_pin_released();

	zassert_equal(cb_calls(), 1, "one occurrence, one report");
	zassert_equal(cb_errors(), CS47L63_ERROR_CLOCK);
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_IRQ1_EINT_1) & EINT_1_WATCHED, 0U,
		      "the latch was cleared, or the line would stay asserted");
	zassert_false(cs47l63_emul_get_irq_asserted(EMUL), "the pin was released");
}

static ZTEST(cs47l63_irq, test_configure_unmasks_exactly_the_watched_sources)
{
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_IRQ1_MASK_1),
		      CS47L63_EMUL_MASK_1_DEFAULT & ~EINT_1_WATCHED);
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_IRQ1_MASK_6),
		      CS47L63_EMUL_MASK_6_DEFAULT & ~EINT_6_WATCHED);
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_IRQ1_MASK_2),
		      CS47L63_EMUL_MASK_2_DEFAULT | CS47L63_BOOT_DONE_EINT1,
		      "boot done is the one flag unmasked at reset and is masked here");
}

static ZTEST(cs47l63_irq, test_pin_is_released_after_configure)
{
	zassert_not_equal(cs47l63_emul_get_reg(EMUL, CS47L63_IRQ1_EINT_2) & CS47L63_BOOT_DONE_EINT1,
			  0U, "the premise: boot done is still latched");
	zassert_false(cs47l63_emul_get_irq_asserted(EMUL));
	zassert_equal(gpio_pin_get_dt(&s_irq), 0, "the pin reads inactive");
}

static ZTEST(cs47l63_irq, test_idle_clock_latch_is_cleared_and_not_reported)
{
	const uint32_t writes = cs47l63_emul_write_count(EMUL, CS47L63_IRQ1_EINT_6);

	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_EINT_6, CS47L63_FLL1_REF_LOST_EINT1);
	wait_for_pin_released();
	(void)k_msleep((int32_t)SETTLE_MS);

	zassert_equal(cs47l63_emul_write_count(EMUL, CS47L63_IRQ1_EINT_6), writes + 1U,
		      "the latch was serviced");
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_IRQ1_EINT_6) & EINT_6_WATCHED, 0U);
	zassert_false(cs47l63_emul_get_irq_asserted(EMUL));
	zassert_equal(cb_calls(), 0, "a clock that went with a stopped stream is not a fault");
}

static ZTEST(cs47l63_irq, test_overcurrent_latch_reports_overcurrent)
{
	mark_out_running();

	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_EINT_1, CS47L63_OUT1L_SC_EINT1);
	wait_for_report();

	zassert_equal(cb_calls(), 1);
	zassert_equal(cb_errors(), (uint32_t)AUDIO_CODEC_ERROR_OVERCURRENT);
}

static void raise_overcurrent(void)
{
	if (atomic_get(&s_cb_calls) == 1) {
		cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_EINT_1, CS47L63_OUT1L_SC_EINT1);
	}
}

static ZTEST(cs47l63_irq, test_a_second_source_raised_during_service_is_reported_too)
{
	mark_out_running();
	s_cb_hook = raise_overcurrent;

	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_EINT_1, CS47L63_SYSCLK_FAIL_EINT1);
	wait_for_report();
	wait_for_report();
	wait_for_pin_released();

	zassert_equal(cb_calls(), 2);
	zassert_equal(cb_errors(), CS47L63_ERROR_CLOCK | (uint32_t)AUDIO_CODEC_ERROR_OVERCURRENT,
		      "the second report carries both sources");
	zassert_false(cs47l63_emul_get_irq_asserted(EMUL));
}

static ZTEST(cs47l63_irq, test_a_latch_landing_between_read_and_clear_is_not_lost)
{
	uint32_t eint1_reads;
	uint32_t eint1_writes;
	uint32_t eint6_writes;

	mark_out_running();
	eint1_reads = cs47l63_emul_read_count(EMUL, CS47L63_IRQ1_EINT_1);
	eint1_writes = cs47l63_emul_write_count(EMUL, CS47L63_IRQ1_EINT_1);
	eint6_writes = cs47l63_emul_write_count(EMUL, CS47L63_IRQ1_EINT_6);

	/* Pass 1 has read EINT_1 by the time it reads EINT_6, so it clears only EINT_6. */
	cs47l63_emul_inject_on_read(EMUL, CS47L63_IRQ1_EINT_6, 0U, CS47L63_IRQ1_EINT_1,
				    CS47L63_SYSCLK_ERR_EINT1);
	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_EINT_6, CS47L63_FLL1_LOCK_FALL_EINT1);
	wait_for_report();
	wait_for_pin_released();
	(void)k_msleep((int32_t)SETTLE_MS);

	zassert_equal(cs47l63_emul_read_count(EMUL, CS47L63_IRQ1_EINT_1), eint1_reads + 2U,
		      "the latch that landed after the read needs a second pass");
	zassert_equal(cs47l63_emul_write_count(EMUL, CS47L63_IRQ1_EINT_1), eint1_writes + 1U);
	zassert_equal(cs47l63_emul_write_count(EMUL, CS47L63_IRQ1_EINT_6), eint6_writes + 1U);
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_IRQ1_EINT_1) & EINT_1_WATCHED, 0U);
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_IRQ1_EINT_6) & EINT_6_WATCHED, 0U);
	zassert_false(cs47l63_emul_get_irq_asserted(EMUL));
}

static ZTEST(cs47l63_irq, test_a_stuck_line_is_serviced_a_bounded_number_of_times)
{
	const uint32_t reads = cs47l63_emul_read_count(EMUL, CS47L63_IRQ1_EINT_1);
	uint32_t after;

	cs47l63_emul_irq_force(EMUL, true);
	(void)k_msleep((int32_t)SETTLE_MS);

	after = cs47l63_emul_read_count(EMUL, CS47L63_IRQ1_EINT_1);
	zassert_equal(after, reads + CS47L63_IRQ_PASSES_MAX,
		      "one service, a bounded number of passes");
	zassert_false(int_enabled(), "a line nothing can clear is given up on");

	(void)k_msleep((int32_t)SETTLE_MS);
	zassert_equal(cs47l63_emul_read_count(EMUL, CS47L63_IRQ1_EINT_1), after,
		      "and no longer serviced");
	zassert_equal(cb_calls(), 0);
}

static ZTEST(cs47l63_irq, test_a_line_already_asserted_when_armed_is_serviced)
{
	const uint32_t reads = cs47l63_emul_read_count(EMUL, CS47L63_IRQ1_EINT_1);

	/* Asserted before the edge interrupt is enabled and never released: no edge follows. */
	cs47l63_emul_irq_force(EMUL, true);
	zassert_ok(cs47l63_test_configure(CODEC, AUDIO_ROUTE_PLAYBACK_CAPTURE));
	(void)k_msleep((int32_t)SETTLE_MS);

	zassert_true(cs47l63_emul_read_count(EMUL, CS47L63_IRQ1_EINT_1) >=
			     reads + CS47L63_IRQ_PASSES_MAX,
		     "the line was serviced without an edge");
	zassert_false(int_enabled(), "and given up on, as nothing clears it");
}

static ZTEST(cs47l63_irq, test_failed_configure_leaves_the_interrupt_disarmed)
{
	zassert_true(int_enabled(), "the premise: the first configure armed it");

	cs47l63_emul_fail_at(EMUL, CS47L63_IRQ1_MASK_1);
	zassert_true(cs47l63_test_configure(CODEC, AUDIO_ROUTE_PLAYBACK_CAPTURE) < 0);
	zassert_equal(cs47l63_emul_fail_hits(EMUL), 1U,
		      "the injection took effect: the unmask was refused");

	zassert_false(int_enabled());

	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_EINT_1, CS47L63_OUT1L_SC_EINT1);
	(void)k_msleep((int32_t)SETTLE_MS);
	zassert_equal(cb_calls(), 0, "nothing listens to a line that was not armed");
}

static ZTEST(cs47l63_irq, test_reconfigure_rearms)
{
	/* The emulator does not model a reset restoring the masks; this does it for the part. */
	cs47l63_emul_reset(EMUL);
	cs47l63_emul_irq_attach(EMUL, &s_irq);
	zassert_ok(cs47l63_test_configure(CODEC, AUDIO_ROUTE_PLAYBACK_CAPTURE));

	zassert_true(cs47l63_emul_write_count(EMUL, CS47L63_IRQ1_MASK_1) >= 1U);
	zassert_true(cs47l63_emul_write_count(EMUL, CS47L63_IRQ1_MASK_2) >= 1U);
	zassert_true(cs47l63_emul_write_count(EMUL, CS47L63_IRQ1_MASK_6) >= 1U);

	mark_out_running();
	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_EINT_1, CS47L63_SYSCLK_FAIL_EINT1);
	wait_for_report();

	zassert_equal(cb_calls(), 1);
	zassert_equal(cb_errors(), CS47L63_ERROR_CLOCK);
}

static ZTEST(cs47l63_irq, test_micbias_off_keeps_the_watched_sources_unmasked)
{
	const uint32_t mask1 = cs47l63_emul_get_reg(EMUL, CS47L63_IRQ1_MASK_1);

	zassert_ok(audio_codec_route_input(CODEC, AUDIO_CHANNEL_ALL, (uint32_t)CS47L63_INPUT_PDM));
	zassert_ok(audio_codec_start(CODEC, AUDIO_DAI_DIR_RX));
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_MICBIAS_CTRL5) & MICB1B_ON, MICB1B_ON,
		      "the premise: the microphone supply is up");

	zassert_ok(audio_codec_stop(CODEC, AUDIO_DAI_DIR_RX));

	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_MICBIAS_CTRL5) & MICB1B_ON, 0U,
		      "the premise: the supply went down");
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_IRQ1_MASK_1), mask1,
		      "the supply's own mask dance restores exactly what it saved");
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_IRQ1_MASK_1) & EINT_1_WATCHED, 0U);
}

static ZTEST(cs47l63_irq, test_a_polled_check_after_an_interrupt_report_does_not_report_again)
{
	audio_property_value_t unmute = {.mute = false};

	mark_out_running();
	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_EINT_1, CS47L63_OUT1L_SC_EINT1);
	wait_for_report();

	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_EINT_1, CS47L63_OUT1L_SC_EINT1);
	wait_for_pin_released();
	zassert_ok(audio_codec_set_property(CODEC, AUDIO_PROPERTY_OUTPUT_MUTE, AUDIO_CHANNEL_ALL,
					    unmute));

	zassert_equal(cb_calls(), 1, "the sticky mask spans both paths");
}
