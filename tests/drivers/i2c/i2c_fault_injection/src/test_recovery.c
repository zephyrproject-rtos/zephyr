/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Held bus lines and bus recovery.
 *
 * A spare GPIO wired to the bus holds SDA, SCL or both low.  A held SDA
 * simulates a target that lost its place in a byte.  The tests check that a
 * transfer on a held bus fails rather than hangs, and that i2c_recover_bus()
 * reports a bus it cannot clear and clears one it can.  The recovery they
 * expect is the bus clear defined in the I2C-bus specification: with SDA
 * stuck low, the controller sends nine clock pulses (UM10204 Rev. 7.0,
 * section 3.1.16).
 *
 * The tests that hold a line need the i2c_bus_fault fixture.  The tests that
 * call i2c_recover_bus() on an idle bus need no wiring.  Every transfer and
 * every recovery call runs on the worker thread, so a driver that waits for
 * its bus lock with no timeout cannot block the test thread.
 */

#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include "bus_force.h"
#include "i2c_fault.h"
#include "i2c_timing.h"

static int write_job(void *arg)
{
	uint8_t tx_buf[I2C_FAULT_PAYLOAD_LEN] = {0};
	const struct i2c_fault_ctx *ctx = i2c_fault_ctx();

	ARG_UNUSED(arg);

	return i2c_write(ctx->dut, tx_buf, sizeof(tx_buf), ctx->present_addr);
} /* write_job() */

static int recover_job(void *arg)
{
	const struct i2c_fault_ctx *ctx = i2c_fault_ctx();

	ARG_UNUSED(arg);

	return i2c_recover_bus(ctx->dut);
} /* recover_job() */

/*
 * Skip the running test when the driver implements no recovery.  Calling
 * i2c_recover_bus() is the only way to find out, so this runs one recovery on
 * the idle bus.
 */
static void require_recovery(void)
{
	struct i2c_fault_outcome outcome;

	i2c_timing_run_or_fail(recover_job, NULL, "recovery support check", &outcome);

	if (outcome.ret == -ENOSYS) {
		TC_PRINT("  the driver implements no bus recovery\n");
		ztest_test_skip();
	}
} /* require_recovery() */

/*
 * Run a write to the known-good address on the worker thread with @p lines
 * held low, then release them before checking the result.  A driver that
 * returns only once the lines are free then gets one more wait limit to do
 * so, instead of leaving the worker stuck and the remaining tests skipped.
 */
static void transfer_while_held(uint32_t lines, const char *condition,
				struct i2c_fault_outcome *out)
{
	bool recovered;

	zassert_ok(bus_force_low(lines), "%s: could not hold the lines low", condition);
	zassert_ok(i2c_timing_submit(write_job, NULL), "%s: could not submit the transfer",
		   condition);

	(void)i2c_timing_collect(out);

	zassert_ok(bus_force_release(), "%s: could not release the lines", condition);
	recovered = i2c_timing_harness_recover();

	I2C_FAULT_ASSERT_RETURNED(out,
				  "%s: the transfer had not returned after %d ms, and %s once "
				  "the lines were released",
				  condition, CONFIG_I2C_FAULT_TEST_TIMEOUT_MS,
				  recovered ? "only returned" : "never returned");
} /* transfer_while_held() */

/* Release the lines after a test that failed while holding one. */
static void recovery_after(void *fixture)
{
	ARG_UNUSED(fixture);

	if (i2c_fault_ctx()->bus_force_ready) {
		(void)bus_force_release();
	}
} /* recovery_after() */

ZTEST_SUITE(i2c_fault_recovery, NULL, i2c_fault_setup, i2c_fault_before, recovery_after, NULL);

/**
 * @brief A transfer on a bus with SDA held low must fail, not hang.
 *
 * @details
 * Test a write of I2C_FAULT_PAYLOAD_LEN bytes to the known-good address while
 * a forcing GPIO holds SDA low.  With SDA low the controller cannot issue a
 * START.  The driver should return an error i2c_fault_assert_error() accepts
 * within the wait limit.  Catches a driver that waits forever for the bus to
 * go free, or that reports success for a write that never reached the bus.
 *
 * Requires: the forcing GPIOs and one device that answers.  Fixture:
 * i2c_bus_fault.
 */
ZTEST(i2c_fault_recovery, test_recovery_sda_held)
{
	struct i2c_fault_outcome outcome;

	i2c_fault_require_bus_force();
	i2c_fault_require_present();

	transfer_while_held(BUS_FORCE_SDA, "SDA held low", &outcome);
	i2c_fault_assert_error(outcome.ret, "SDA held low");
} /* test_recovery_sda_held() */

/**
 * @brief A transfer on a bus with SCL held low must fail, not hang.
 *
 * @details
 * Test a write of I2C_FAULT_PAYLOAD_LEN bytes to the known-good address while
 * a forcing GPIO holds SCL low, so the controller cannot produce a clock edge.
 * The driver should return an error i2c_fault_assert_error() accepts within
 * the wait limit.  Catches a driver that handles a held SDA but waits forever
 * on a held SCL.
 *
 * Requires: the forcing GPIOs and one device that answers.  Fixture:
 * i2c_bus_fault.
 */
ZTEST(i2c_fault_recovery, test_recovery_scl_held)
{
	struct i2c_fault_outcome outcome;

	i2c_fault_require_bus_force();
	i2c_fault_require_present();

	transfer_while_held(BUS_FORCE_SCL, "SCL held low", &outcome);
	i2c_fault_assert_error(outcome.ret, "SCL held low");
} /* test_recovery_scl_held() */

/**
 * @brief A transfer on a bus with both lines held low must fail, not hang.
 *
 * @details
 * Test a write of I2C_FAULT_PAYLOAD_LEN bytes to the known-good address while
 * the forcing GPIOs hold SDA and SCL low together.  The driver should return
 * an error i2c_fault_assert_error() accepts within the wait limit.  Catches a
 * driver that checks the two lines one after the other and handles each held
 * line alone, but not both at once.
 *
 * Requires: the forcing GPIOs and one device that answers.  Fixture:
 * i2c_bus_fault.
 */
ZTEST(i2c_fault_recovery, test_recovery_both_held)
{
	struct i2c_fault_outcome outcome;

	i2c_fault_require_bus_force();
	i2c_fault_require_present();

	transfer_while_held(BUS_FORCE_SDA | BUS_FORCE_SCL, "both lines held low", &outcome);
	i2c_fault_assert_error(outcome.ret, "both lines held low");
} /* test_recovery_both_held() */

/*
 * The I2C API documents -EBUSY for a bus that is not clear after the recovery
 * attempt, so that is what the strict mode requires.
 */
static void assert_recovery_failed(int ret, const char *condition)
{
	i2c_fault_record(condition, ret);

	if (IS_ENABLED(CONFIG_I2C_FAULT_TEST_RELAXED_ERRNO)) {
		zassert_true(ret < 0, "%s: recovery returned %d with SDA still held", condition,
			     ret);
	} else {
		zassert_equal(ret, -EBUSY,
			      "%s: the I2C API documents -EBUSY for a bus that is not clear "
			      "after recovery, got %d",
			      condition, ret);
	}
} /* assert_recovery_failed() */

/**
 * @brief Recovery must report a bus it could not clear.
 *
 * @details
 * Test i2c_recover_bus() while a forcing GPIO holds SDA low.  Clock pulses
 * cannot release a line a GPIO holds, so SDA is still low when the recovery
 * ends.  The driver should return -EBUSY within the wait limit.  Catches a
 * driver that reports success for a bus it did not clear, or that does not
 * return while the line stays low.
 *
 * Requires: the forcing GPIOs and a driver that implements recovery.
 * Fixture: i2c_bus_fault.
 */
ZTEST(i2c_fault_recovery, test_recovery_fails_while_sda_held)
{
	struct i2c_fault_outcome outcome;
	bool recovered;

	i2c_fault_require_bus_force();
	require_recovery();

	zassert_ok(bus_force_low(BUS_FORCE_SDA), "could not hold SDA low");
	zassert_ok(i2c_timing_submit(recover_job, NULL), "could not submit the recovery");

	(void)i2c_timing_collect(&outcome);

	zassert_ok(bus_force_release(), "could not release SDA");
	recovered = i2c_timing_harness_recover();

	I2C_FAULT_ASSERT_RETURNED(&outcome,
				  "recovery with SDA held: i2c_recover_bus() had not returned "
				  "after %d ms, and %s once SDA was released",
				  CONFIG_I2C_FAULT_TEST_TIMEOUT_MS,
				  recovered ? "only returned" : "never returned");
	assert_recovery_failed(outcome.ret, "recovery with SDA held");
} /* test_recovery_fails_while_sda_held() */

/**
 * @brief Recovery after a held line is released must succeed.
 *
 * @details
 * Test i2c_recover_bus() after a forcing GPIO has held SDA low for
 * CONFIG_I2C_FAULT_TEST_HOLD_MS and released it.  No transfer runs during the
 * hold.  Recovery should return 0, and the bus should then be usable.
 * Catches a driver whose recovery fails on a bus that has gone free, or
 * reports success but leaves the controller unable to start a transfer.
 *
 * Requires: the forcing GPIOs and a driver that implements recovery.
 * Fixture: i2c_bus_fault.
 */
ZTEST(i2c_fault_recovery, test_recovery_succeeds_after_release)
{
	struct i2c_fault_outcome outcome;

	i2c_fault_require_bus_force();
	require_recovery();

	zassert_ok(bus_force_low(BUS_FORCE_SDA), "could not hold SDA low");
	k_msleep(CONFIG_I2C_FAULT_TEST_HOLD_MS);
	zassert_ok(bus_force_release(), "could not release SDA");

	i2c_timing_run_or_fail(recover_job, NULL, "recovery after SDA release", &outcome);
	i2c_fault_record("recovery after SDA release", outcome.ret);
	zassert_ok(outcome.ret, "recovery after SDA release failed (%d)", outcome.ret);

	i2c_fault_assert_healthy("recovery after SDA release");
} /* test_recovery_succeeds_after_release() */

/**
 * @brief A driver without recovery must say so with -ENOSYS.
 *
 * @details
 * Test one call of i2c_recover_bus() on an idle bus.  The I2C API documents 0
 * for success and -ENOSYS for a driver that does not implement recovery.  It
 * also documents -EIO, but on a bus with nothing stuck that would report a
 * failed recovery, so the result should be 0 or -ENOSYS.  Catches a driver
 * without recovery that returns another error, which a caller cannot tell
 * apart from a recovery that failed.
 *
 * Requires: one I2C controller.  No fixture.
 */
ZTEST(i2c_fault_recovery, test_recovery_unsupported_reports_enosys)
{
	struct i2c_fault_outcome outcome;

	i2c_timing_run_or_fail(recover_job, NULL, "recovery support", &outcome);
	i2c_fault_record("recovery support", outcome.ret);
	zassert_true((outcome.ret == 0) || (outcome.ret == -ENOSYS),
		     "recovery on an idle bus returned %d; the I2C API documents 0 for "
		     "success and -ENOSYS for a driver without recovery",
		     outcome.ret);
} /* test_recovery_unsupported_reports_enosys() */

/**
 * @brief Repeated recovery on an idle bus must keep succeeding.
 *
 * @details
 * Test CONFIG_I2C_FAULT_TEST_REPEAT_COUNT consecutive calls of
 * i2c_recover_bus() on an idle bus.  Every call should return 0, and the bus
 * should be usable afterwards.  Catches a driver whose recovery leaves the
 * bus pins switched to GPIO or the controller disabled, or leaks pin or
 * controller state on each call, so that a later call or the next transfer
 * fails.
 *
 * Requires: a driver that implements recovery.  No fixture.
 */
ZTEST(i2c_fault_recovery, test_recovery_repeated)
{
	struct i2c_fault_outcome outcome;

	require_recovery();

	for (uint32_t i = 0U; i < CONFIG_I2C_FAULT_TEST_REPEAT_COUNT; i++) {
		i2c_timing_run_or_fail(recover_job, NULL, "repeated recovery", &outcome);

		if (i == 0U) {
			i2c_fault_record("repeated recovery", outcome.ret);
		}

		zassert_ok(outcome.ret, "repeated recovery: call %u of %u failed (%d)", i + 1U,
			   CONFIG_I2C_FAULT_TEST_REPEAT_COUNT, outcome.ret);
	}

	i2c_fault_assert_healthy("repeated recovery");
} /* test_recovery_repeated() */
