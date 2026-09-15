/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file test_boot.c
 * @brief cs47l63_boot_bringup(): reset, boot-done, identification and trim
 */

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/ztest.h>

#include "cs47l63_boot.h"
#include "cs47l63_regs.h"

#include "cs47l63_emul.h"

#define CODEC DEVICE_DT_GET(DT_NODELABEL(cs47l63_boot))
#define EMUL  EMUL_DT_GET(DT_NODELABEL(cs47l63_boot))

static const struct gpio_dt_spec reset_gpio =
	GPIO_DT_SPEC_GET(DT_NODELABEL(cs47l63_boot), reset_gpios);

/** The OTP variant whose trim block the driver knows how to write. */
#define OTPID_TRIMMED 0x8U

/** Boot-done poll bound: 20 reads at 10 ms. */
#define BOOT_POLL_MAX 20U

/** The trim block, address and value, exactly as the vendor states it. */
static const struct {
	uint32_t addr;
	uint32_t val;
} k_trim[] = {
	{CS47L63_DAC_IF_CONTROL_1, 0x1DB10000},  {CS47L63_DAC_IF_TEST_1, 0x700249B8},
	{CS47L63_HP_OCD_CTRL1, 0x00010000},      {CS47L63_HP_OCD_TEST1, 0x000005FF},
	{CS47L63_MICBIAS_TST_CTRL1, 0x04150415}, {CS47L63_MICBIAS_TST_CTRL4, 0x00000415},
};

/** The reset line's configuration before the driver ever touched it. */
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

	zassert_ok(cs47l63_boot_bringup(CODEC));

	zassert_ok(gpio_emul_flags_get_dt(&reset_gpio, &flags));
	zassert_not_equal(flags & GPIO_OUTPUT, 0, "reset must be driven, not left floating");

	/* gpio_emul_output_get_dt() reads the line itself and does not apply the
	 * GPIO_ACTIVE_LOW from devicetree, so the released state of this
	 * active-low reset is a high line.
	 */
	zassert_equal(gpio_emul_output_get_dt(&reset_gpio), 1,
		      "reset must be released when bring-up returns, or the part stays held");
}

ZTEST(cs47l63_boot, test_waits_for_boot_done_before_reading_the_id)
{
	int boot_idx;
	int devid_idx;

	zassert_ok(cs47l63_boot_bringup(CODEC));

	boot_idx = cs47l63_emul_read_index(EMUL, CS47L63_IRQ1_EINT_2, 0);
	devid_idx = cs47l63_emul_read_index(EMUL, CS47L63_DEVID, 0);

	zassert_true(boot_idx >= 0, "boot-done must be polled");
	zassert_true(devid_idx > boot_idx,
		     "the register file is only trustworthy once boot-done has asserted");
}

ZTEST(cs47l63_boot, test_boot_done_that_never_asserts_times_out)
{
	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_EINT_2, 0);

	zassert_equal(-ETIMEDOUT, cs47l63_boot_bringup(CODEC),
		      "a part that never reports boot-done is not running");

	/* The injection took effect: the flag really was absent for every one
	 * of the polls the driver is allowed, and it gave up rather than
	 * looping.
	 */
	zassert_equal(cs47l63_emul_read_count(EMUL, CS47L63_IRQ1_EINT_2), BOOT_POLL_MAX,
		      "the poll must be bounded at 20 reads");
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_IRQ1_EINT_2), 0,
		      "boot-done must have stayed clear throughout");
	zassert_equal(cs47l63_emul_read_count(EMUL, CS47L63_DEVID), 0,
		      "nothing may be read out of a part that never booted");
}

ZTEST(cs47l63_boot, test_device_id_is_read_and_accepted)
{
	zassert_ok(cs47l63_boot_bringup(CODEC));

	zassert_true(cs47l63_emul_read_count(EMUL, CS47L63_DEVID) >= 1,
		     "the part must be identified, not assumed");
	zassert_true(cs47l63_emul_read_count(EMUL, CS47L63_REVID) >= 1,
		     "the revision is read alongside it");
}

ZTEST(cs47l63_boot, test_all_zero_device_id_fails_initialisation)
{
	/* All zeros is what a bus with no part answering on it reads back. */
	cs47l63_emul_set_reg(EMUL, CS47L63_DEVID, 0);

	zassert_equal(-ENODEV, cs47l63_boot_bringup(CODEC),
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
	/* All ones is what a floating MISO reads back. */
	cs47l63_emul_set_reg(EMUL, CS47L63_DEVID, CS47L63_DEVID_MASK);

	zassert_equal(-ENODEV, cs47l63_boot_bringup(CODEC));
	zassert_equal(cs47l63_emul_read_count(EMUL, CS47L63_DEVID), 1);
	zassert_equal(cs47l63_emul_write_count(EMUL, CS47L63_DAC_IF_CONTROL_1), 0);
}

ZTEST(cs47l63_boot, test_id_read_that_fails_on_the_bus_propagates)
{
	cs47l63_emul_fail_at(EMUL, CS47L63_DEVID);

	zassert_equal(-EIO, cs47l63_boot_bringup(CODEC),
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

	zassert_ok(cs47l63_boot_bringup(CODEC));

	/* Unlock is the pair 0x55, 0xAA written to both key registers; lock is
	 * 0xCC, 0x33 the same way.
	 */
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
	/* Any variant other than 8 has its trim in silicon; writing the block
	 * anyway would move a part that was already correct.
	 */
	cs47l63_emul_set_reg(EMUL, CS47L63_OTPID, 0x1);

	zassert_ok(cs47l63_boot_bringup(CODEC));

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

	zassert_equal(-EIO, cs47l63_boot_bringup(CODEC));

	zassert_equal(cs47l63_emul_write_count(EMUL, CS47L63_HP_OCD_CTRL1), 0,
		      "the injection took effect: the armed write never landed");
	zassert_equal(cs47l63_emul_write_count(EMUL, CS47L63_TEST_KEY_CTRL), 4,
		      "the lock pair must go out even on the way out of a failure");
	zassert_true(cs47l63_emul_nth_write(EMUL, CS47L63_TEST_KEY_CTRL, 3, &val));
	zassert_equal(val, CS47L63_KEY_LOCK_CODE1,
		      "the region must be left locked, never open after an aborted trim");
}
