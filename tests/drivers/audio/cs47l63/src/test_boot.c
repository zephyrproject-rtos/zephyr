/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file test_boot.c
 * @brief Bring-up by configure(): reset, boot-done, identification and trim
 */

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>

#include <zephyr/audio/codec.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/ztest.h>

#include "cs47l63.h"

#include "cs47l63_emul.h"

#define CODEC DEVICE_DT_GET(DT_NODELABEL(cs47l63_boot))
#define EMUL  EMUL_DT_GET(DT_NODELABEL(cs47l63_boot))

static const struct gpio_dt_spec reset_gpio =
	GPIO_DT_SPEC_GET(DT_NODELABEL(cs47l63_boot), reset_gpios);

#define CODEC_RESET_FAIL DEVICE_DT_GET(DT_NODELABEL(cs47l63_reset_fail))
#define EMUL_RESET_FAIL  EMUL_DT_GET(DT_NODELABEL(cs47l63_reset_fail))

static const struct gpio_dt_spec reset_fail_gpio =
	GPIO_DT_SPEC_GET(DT_NODELABEL(cs47l63_reset_fail), reset_gpios);

#define OTPID_TRIMMED 0x8U

#define BOOT_POLL_MAX 20U

/** The vendor's cs47l63_otpid_8_patch() trim block. */
static const struct {
	uint32_t addr;
	uint32_t val;
} k_trim[] = {
	{CS47L63_DAC_IF_CONTROL_1, 0x1DB10000},  {CS47L63_DAC_IF_TEST_1, 0x700249B8},
	{CS47L63_HP_OCD_CTRL1, 0x00010000},      {CS47L63_HP_OCD_TEST1, 0x000005FF},
	{CS47L63_MICBIAS_TST_CTRL1, 0x04150415}, {CS47L63_MICBIAS_TST_CTRL4, 0x00000415},
};

static gpio_flags_t s_flags_before_first_bringup;

static void *suite_setup(void)
{
	zassert_ok(gpio_emul_flags_get_dt(&reset_gpio, &s_flags_before_first_bringup));

	return NULL;
}

static void before_each(void *fixture)
{
	ARG_UNUSED(fixture);

	cs47l63_emul_reset(EMUL);
}

ZTEST_SUITE(cs47l63_boot, NULL, suite_setup, before_each, NULL, NULL);

ZTEST(cs47l63_boot, test_reset_line_is_driven_and_released)
{
	gpio_flags_t flags;

	zassert_equal(s_flags_before_first_bringup & GPIO_OUTPUT, 0,
		      "the driver must not have driven reset before it was asked to");

	zassert_ok(cs47l63_test_configure(CODEC, AUDIO_ROUTE_PLAYBACK_CAPTURE));

	zassert_ok(gpio_emul_flags_get_dt(&reset_gpio, &flags));
	zassert_not_equal(flags & GPIO_OUTPUT, 0, "reset must be driven, not left floating");

	/* gpio_emul_output_get_dt() ignores GPIO_ACTIVE_LOW, so the released reset reads 1. */
	zassert_equal(gpio_emul_output_get_dt(&reset_gpio), 1,
		      "reset must be released when bring-up returns, or the part stays held");
}

ZTEST(cs47l63_boot, test_waits_for_boot_done_before_reading_the_id)
{
	int boot_idx;
	int devid_idx;

	zassert_ok(cs47l63_test_configure(CODEC, AUDIO_ROUTE_PLAYBACK_CAPTURE));

	boot_idx = cs47l63_emul_read_index(EMUL, CS47L63_IRQ1_EINT_2, 0);
	devid_idx = cs47l63_emul_read_index(EMUL, CS47L63_DEVID, 0);

	zassert_true(boot_idx >= 0, "boot-done must be polled");
	zassert_true(devid_idx > boot_idx,
		     "the register file is only trustworthy once boot-done has asserted");
}

ZTEST(cs47l63_boot, test_boot_done_that_never_asserts_times_out)
{
	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_EINT_2, 0);

	zassert_equal(-ETIMEDOUT, cs47l63_test_configure(CODEC, AUDIO_ROUTE_PLAYBACK_CAPTURE),
		      "a part that never reports boot-done is not running");

	zassert_equal(cs47l63_emul_read_count(EMUL, CS47L63_IRQ1_EINT_2), BOOT_POLL_MAX,
		      "the poll must be bounded at 20 reads");
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_IRQ1_EINT_2), 0,
		      "boot-done must have stayed clear throughout");
	zassert_equal(cs47l63_emul_read_count(EMUL, CS47L63_DEVID), 0,
		      "nothing may be read out of a part that never booted");
}

ZTEST(cs47l63_boot, test_device_id_is_read_and_accepted)
{
	zassert_ok(cs47l63_test_configure(CODEC, AUDIO_ROUTE_PLAYBACK_CAPTURE));

	zassert_true(cs47l63_emul_read_count(EMUL, CS47L63_DEVID) >= 1,
		     "the part must be identified, not assumed");
	zassert_true(cs47l63_emul_read_count(EMUL, CS47L63_REVID) >= 1,
		     "the revision is read alongside it");
}

ZTEST(cs47l63_boot, test_all_zero_device_id_fails_initialisation)
{
	cs47l63_emul_set_reg(EMUL, CS47L63_DEVID, 0);

	zassert_equal(-ENODEV, cs47l63_test_configure(CODEC, AUDIO_ROUTE_PLAYBACK_CAPTURE),
		      "an implausible ID must fail, and must fail distinguishably");

	zassert_equal(cs47l63_emul_read_count(EMUL, CS47L63_DEVID), 1,
		      "the injected ID was read exactly once");
	zassert_equal(cs47l63_emul_write_count(EMUL, CS47L63_DAC_IF_CONTROL_1), 0,
		      "no trim may be written into a part that is not the expected one");
	zassert_equal(cs47l63_emul_write_count(EMUL, CS47L63_TEST_KEY_CTRL), 0,
		      "and the register-region lock must never be opened");
}

ZTEST(cs47l63_boot, test_all_ones_device_id_fails_initialisation)
{
	cs47l63_emul_set_reg(EMUL, CS47L63_DEVID, CS47L63_DEVID_MASK);

	zassert_equal(-ENODEV, cs47l63_test_configure(CODEC, AUDIO_ROUTE_PLAYBACK_CAPTURE));
	zassert_equal(cs47l63_emul_read_count(EMUL, CS47L63_DEVID), 1);
	zassert_equal(cs47l63_emul_write_count(EMUL, CS47L63_DAC_IF_CONTROL_1), 0);
}

ZTEST(cs47l63_boot, test_wrong_device_id_fails_initialisation)
{
	struct cs47l63_emul_xfer xfer;
	int devid_idx;

	cs47l63_emul_set_reg(EMUL, CS47L63_DEVID, CS47L63_DEVID_CS47L63 - 1U);

	zassert_equal(-ENODEV, cs47l63_test_configure(CODEC, AUDIO_ROUTE_PLAYBACK_CAPTURE),
		      "a readable ID that is not the CS47L63's must fail");

	devid_idx = cs47l63_emul_read_index(EMUL, CS47L63_DEVID, 0);
	zassert_true(devid_idx >= 0);
	zassert_true(cs47l63_emul_xfer_get(EMUL, (uint32_t)devid_idx, &xfer));
	zassert_equal(xfer.val, CS47L63_DEVID_CS47L63 - 1U,
		      "the injection took effect: the wrong ID was served");
	for (uint32_t i = (uint32_t)devid_idx + 1U; i < cs47l63_emul_xfer_count(EMUL); i++) {
		zassert_true(cs47l63_emul_xfer_get(EMUL, i, &xfer));
		zassert_false(xfer.write, "no write may follow a rejected ID (reg 0x%05x)",
			      xfer.addr);
	}
}

ZTEST(cs47l63_boot, test_id_read_that_fails_on_the_bus_propagates)
{
	cs47l63_emul_fail_at(EMUL, CS47L63_DEVID);

	zassert_equal(-EIO, cs47l63_test_configure(CODEC, AUDIO_ROUTE_PLAYBACK_CAPTURE),
		      "a bus failure is not the same answer as a wrong part");
	zassert_equal(cs47l63_emul_read_count(EMUL, CS47L63_DEVID), 0,
		      "the injection took effect: the read never completed");
}

ZTEST(cs47l63_boot, test_trim_block_is_written_whole_and_behind_the_lock)
{
	uint32_t val;
	int unlock_idx;
	int lock_idx;

	cs47l63_emul_set_reg(EMUL, CS47L63_OTPID, OTPID_TRIMMED);

	zassert_ok(cs47l63_test_configure(CODEC, AUDIO_ROUTE_PLAYBACK_CAPTURE));

	zassert_equal(cs47l63_emul_write_count(EMUL, CS47L63_TEST_KEY_CTRL), 4);
	zassert_equal(cs47l63_emul_write_count(EMUL, CS47L63_USER_KEY_CTRL), 4);

	zassert_true(cs47l63_emul_nth_write(EMUL, CS47L63_TEST_KEY_CTRL, 0, &val));
	zassert_equal(val, CS47L63_KEY_UNLOCK_CODE0);
	zassert_true(cs47l63_emul_nth_write(EMUL, CS47L63_TEST_KEY_CTRL, 1, &val));
	zassert_equal(val, CS47L63_KEY_UNLOCK_CODE1);
	zassert_true(cs47l63_emul_nth_write(EMUL, CS47L63_TEST_KEY_CTRL, 2, &val));
	zassert_equal(val, CS47L63_KEY_LOCK_CODE0);
	zassert_true(cs47l63_emul_nth_write(EMUL, CS47L63_TEST_KEY_CTRL, 3, &val));
	zassert_equal(val, CS47L63_KEY_LOCK_CODE1);

	unlock_idx = cs47l63_emul_write_index(EMUL, CS47L63_USER_KEY_CTRL, 1);
	lock_idx = cs47l63_emul_write_index(EMUL, CS47L63_USER_KEY_CTRL, 2);

	for (size_t i = 0; i < ARRAY_SIZE(k_trim); i++) {
		int idx = cs47l63_emul_write_index(EMUL, k_trim[i].addr, 0);

		zassert_true(idx > unlock_idx && idx < lock_idx,
			     "trim register 0x%05x must be written inside the unlocked window",
			     k_trim[i].addr);
		zassert_true(cs47l63_emul_nth_write(EMUL, k_trim[i].addr, 0, &val));
		zassert_equal(val, k_trim[i].val, "trim register 0x%05x got the wrong value",
			      k_trim[i].addr);
	}
}

ZTEST(cs47l63_boot, test_untrimmed_otp_variant_gets_no_trim_block)
{
	cs47l63_emul_set_reg(EMUL, CS47L63_OTPID, 0x1);

	zassert_ok(cs47l63_test_configure(CODEC, AUDIO_ROUTE_PLAYBACK_CAPTURE));

	zassert_equal(cs47l63_emul_read_count(EMUL, CS47L63_OTPID), 1,
		      "the variant must be read, not assumed");
	zassert_equal(cs47l63_emul_write_count(EMUL, CS47L63_TEST_KEY_CTRL), 0,
		      "the lock stays closed when there is nothing to write behind it");

	for (size_t i = 0; i < ARRAY_SIZE(k_trim); i++) {
		zassert_equal(cs47l63_emul_write_count(EMUL, k_trim[i].addr), 0,
			      "trim register 0x%05x must not be touched", k_trim[i].addr);
	}
}

ZTEST(cs47l63_boot, test_failed_trim_write_relocks_the_region)
{
	uint32_t val;

	cs47l63_emul_set_reg(EMUL, CS47L63_OTPID, OTPID_TRIMMED);
	cs47l63_emul_fail_at(EMUL, CS47L63_HP_OCD_CTRL1);

	zassert_equal(-EIO, cs47l63_test_configure(CODEC, AUDIO_ROUTE_PLAYBACK_CAPTURE));

	zassert_equal(cs47l63_emul_write_count(EMUL, CS47L63_HP_OCD_CTRL1), 0,
		      "the injection took effect: the armed write never landed");
	zassert_equal(cs47l63_emul_write_count(EMUL, CS47L63_TEST_KEY_CTRL), 4,
		      "the lock pair must go out even on the way out of a failure");
	zassert_true(cs47l63_emul_nth_write(EMUL, CS47L63_TEST_KEY_CTRL, 3, &val));
	zassert_equal(val, CS47L63_KEY_LOCK_CODE1,
		      "the region must be left locked, never open after an aborted trim");
}

ZTEST(cs47l63_boot, test_failed_unlock_relocks_the_region)
{
	uint32_t val;

	cs47l63_emul_set_reg(EMUL, CS47L63_OTPID, OTPID_TRIMMED);
	cs47l63_emul_fail_at(EMUL, CS47L63_USER_KEY_CTRL);

	zassert_equal(-EIO, cs47l63_test_configure(CODEC, AUDIO_ROUTE_PLAYBACK_CAPTURE));

	zassert_equal(cs47l63_emul_write_count(EMUL, CS47L63_USER_KEY_CTRL), 0,
		      "the injection took effect: the unlock never completed");
	zassert_true(cs47l63_emul_nth_write(EMUL, CS47L63_TEST_KEY_CTRL, 1, &val),
		     "a lock must be attempted after a failed unlock");
	zassert_equal(val, CS47L63_KEY_LOCK_CODE0);
	zassert_equal(cs47l63_emul_write_count(EMUL, CS47L63_DAC_IF_CONTROL_1), 0,
		      "no trim may be written after a failed unlock");
}

ZTEST(cs47l63_boot, test_reset_line_that_cannot_be_driven_aborts_bringup)
{
	gpio_flags_t flags;

	cs47l63_emul_reset(EMUL_RESET_FAIL);

	zassert_equal(-ENOTSUP,
		      cs47l63_test_configure(CODEC_RESET_FAIL, AUDIO_ROUTE_PLAYBACK_CAPTURE),
		      "the GPIO error must reach the caller");

	zassert_ok(gpio_emul_flags_get_dt(&reset_fail_gpio, &flags));
	zassert_equal(flags & GPIO_OUTPUT, 0,
		      "the injection took effect: the line was never configured");
	zassert_equal(cs47l63_emul_xfer_count(EMUL_RESET_FAIL), 0,
		      "nothing may be polled or read from a part whose reset failed");
}

ZTEST(cs47l63_boot, test_start_before_configure_writes_nothing)
{
	/* The other cases configure CODEC; no configure of this node ever succeeds. */
	cs47l63_emul_reset(EMUL_RESET_FAIL);

	zassert_equal(-EINVAL, audio_codec_start(CODEC_RESET_FAIL, AUDIO_DAI_DIR_RX));
	zassert_equal(-EINVAL, audio_codec_start(CODEC_RESET_FAIL, AUDIO_DAI_DIR_TX));
	audio_codec_start_output(CODEC_RESET_FAIL);

	zassert_equal(cs47l63_emul_xfer_count(EMUL_RESET_FAIL), 0,
		      "a part never configured is not touched");
}

ZTEST(cs47l63_boot, test_configure_that_never_reset_the_part_keeps_it_stoppable)
{
	const struct cs47l63_config *config = CODEC_RESET_FAIL->config;
	audio_property_value_t mute = {.mute = true};
	audio_property_value_t vol = {.vol = -20};
	gpio_flags_t flags;
	uint32_t out_en;
	uint32_t in_en;

	/* No configure of this node gets past its reset, so the output and input a previous
	 * configuration left running are set directly, as cs47l63_out_confirm_start() and
	 * port_in_start() would.
	 */
	cs47l63_emul_reset(EMUL_RESET_FAIL);
	config->chip->output_running = true;
	config->port->in_started = true;
	config->chip->in_users[config->port->in_terminal] |= BIT(config->port->asp - 1U);

	zassert_equal(-ENOTSUP,
		      cs47l63_test_configure(CODEC_RESET_FAIL, AUDIO_ROUTE_PLAYBACK_CAPTURE));
	zassert_ok(gpio_emul_flags_get_dt(&reset_fail_gpio, &flags));
	zassert_equal(flags & GPIO_OUTPUT, 0,
		      "the injection took effect: the reset line was never driven");

	zassert_true(config->chip->output_running,
		     "an output the reset never reached is still live");
	zassert_true(config->port->in_started, "an input the reset never reached is still live");

	cs47l63_emul_reset(EMUL_RESET_FAIL);
	zassert_ok(audio_codec_set_property(CODEC_RESET_FAIL, AUDIO_PROPERTY_OUTPUT_VOLUME,
					    AUDIO_CHANNEL_ALL, vol));
	zassert_true(cs47l63_emul_write_count(EMUL_RESET_FAIL, CS47L63_OUT1L_VOLUME_1) >= 1,
		     "the level still reaches a live output");
	cs47l63_emul_reset(EMUL_RESET_FAIL);
	zassert_ok(audio_codec_set_property(CODEC_RESET_FAIL, AUDIO_PROPERTY_INPUT_MUTE,
					    AUDIO_CHANNEL_ALL, mute));
	zassert_true(cs47l63_emul_xfer_count(EMUL_RESET_FAIL) >= 1,
		     "the mute still reaches a live input");

	/* Enabled on the part, so the checks below prove stop cleared them. */
	cs47l63_emul_set_reg(EMUL_RESET_FAIL, CS47L63_OUTPUT_ENABLE_1, CS47L63_OUT1L_EN);
	cs47l63_emul_set_reg(EMUL_RESET_FAIL, CS47L63_INPUT_CONTROL,
			     CS47L63_IN2L_EN | CS47L63_IN2R_EN);

	audio_codec_stop_output(CODEC_RESET_FAIL);
	zassert_ok(audio_codec_stop(CODEC_RESET_FAIL, AUDIO_DAI_DIR_RX));
	out_en = cs47l63_emul_get_reg(EMUL_RESET_FAIL, CS47L63_OUTPUT_ENABLE_1);
	zassert_equal(out_en & CS47L63_OUT1L_EN, 0, "stop takes the amplifier down");
	in_en = cs47l63_emul_get_reg(EMUL_RESET_FAIL, CS47L63_INPUT_CONTROL) &
		(CS47L63_IN2L_EN | CS47L63_IN2R_EN | CS47L63_IN1L_EN | CS47L63_IN1R_EN);
	zassert_equal(in_en, 0, "stop takes the input down");
	zassert_false(config->chip->output_running);
	zassert_false(config->port->in_started);
}
