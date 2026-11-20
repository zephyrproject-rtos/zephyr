/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * I2C fault injection suite: setup, address selection and the transfer
 * helpers that run on the worker thread.
 */

#include <errno.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include "i2c_fault.h"
#include "i2c_timing.h"

static struct i2c_fault_ctx ctx;

const struct i2c_fault_ctx *i2c_fault_ctx(void)
{
	return &ctx;
} /* i2c_fault_ctx() */

void i2c_fault_skip_unbounded(void)
{
	TC_PRINT("\n  the API documents a bounded return only for a driver "
		 "selecting I2C_TRANSFER_TIMEOUT_SUPPORTED with a non-zero "
		 "timeout below CONFIG_I2C_FAULT_TEST_TIMEOUT_MS, so this "
		 "condition skips\n");
	ztest_test_skip();
} /* i2c_fault_skip_unbounded() */

int i2c_fault_xfer_job(void *arg)
{
	const struct i2c_fault_xfer *xfer = arg;

	return i2c_transfer(xfer->bus, xfer->msgs, xfer->num_msgs, xfer->addr);
} /* i2c_fault_xfer_job() */

/*
 * Storage for the transfer given to the worker thread.
 *
 * A job still running at the test wait limit
 * (CONFIG_I2C_FAULT_TEST_TIMEOUT_MS) is left running, so anything the
 * controller may still touch has to outlive the test thread's stack frame.
 * The message array is copied here; the buffers the messages point at are the
 * caller's responsibility and have static storage throughout the suite.
 */
static struct i2c_msg pinned_msgs[I2C_FAULT_MAX_MSGS];
static struct i2c_fault_xfer pinned_xfer;

/*
 * Copy the caller's message array into static storage and point pinned_xfer
 * at it, so a hung transfer on the worker never reads descriptors from a
 * stack frame that has already returned.  Call it only while no job is
 * stuck; otherwise it overwrites the descriptors the stuck job is using.
 */
static void pin_transfer(const struct i2c_fault_xfer *xfer)
{
	zassert_true(xfer->num_msgs <= ARRAY_SIZE(pinned_msgs),
		     "a transfer of %u messages exceeds I2C_FAULT_MAX_MSGS", xfer->num_msgs);

	(void)memcpy(pinned_msgs, xfer->msgs, (size_t)xfer->num_msgs * sizeof(pinned_msgs[0]));

	pinned_xfer.bus = xfer->bus;
	pinned_xfer.addr = xfer->addr;
	pinned_xfer.msgs = pinned_msgs;
	pinned_xfer.num_msgs = xfer->num_msgs;
} /* pin_transfer() */

void i2c_fault_transfer_bounded(const struct i2c_fault_xfer *xfer, const char *condition,
				struct i2c_fault_outcome *out)
{
	pin_transfer(xfer);
	i2c_timing_run_or_fail(i2c_fault_xfer_job, &pinned_xfer, condition, out);
} /* i2c_fault_transfer_bounded() */

/*
 * Send a zero-length write to addr on the worker thread and return the
 * outcome for the caller to check.  A probe that does not return then cannot
 * stall the test thread.
 */
static int probe_bounded(uint16_t addr, struct i2c_fault_outcome *out)
{
	static uint8_t pinned_scratch;
	struct i2c_msg msg;
	struct i2c_fault_xfer xfer;

	msg.buf = &pinned_scratch;
	msg.len = 0U;
	msg.flags = I2C_MSG_WRITE | I2C_MSG_STOP;

	xfer.bus = ctx.dut;
	xfer.addr = addr;
	xfer.msgs = &msg;
	xfer.num_msgs = 1U;

	pin_transfer(&xfer);

	return i2c_timing_run_bounded(i2c_fault_xfer_job, &pinned_xfer, out);
} /* probe_bounded() */

/*
 * Byte-mode callbacks for i2c_fault_noop_callbacks.  Each returns 0, and the
 * read callbacks supply 0xFF.
 */
static int noop_write_requested(struct i2c_target_config *config)
{
	ARG_UNUSED(config);

	return 0;
} /* noop_write_requested() */

static int noop_write_received(struct i2c_target_config *config, uint8_t val)
{
	ARG_UNUSED(config);
	ARG_UNUSED(val);

	return 0;
} /* noop_write_received() */

static int noop_read_requested(struct i2c_target_config *config, uint8_t *val)
{
	ARG_UNUSED(config);

	*val = 0xFFU;

	return 0;
} /* noop_read_requested() */

static int noop_read_processed(struct i2c_target_config *config, uint8_t *val)
{
	ARG_UNUSED(config);

	*val = 0xFFU;

	return 0;
} /* noop_read_processed() */

static int noop_stop(struct i2c_target_config *config)
{
	ARG_UNUSED(config);

	return 0;
} /* noop_stop() */

const struct i2c_target_callbacks i2c_fault_noop_callbacks = {
	.write_requested = noop_write_requested,
	.read_requested = noop_read_requested,
	.write_received = noop_write_received,
	.read_processed = noop_read_processed,
	.stop = noop_stop,
};

void i2c_fault_record(const char *condition, int ret)
{
	TC_PRINT("  %s: driver returned %d\n", condition, ret);
} /* i2c_fault_record() */

void i2c_fault_assert_error(int ret, const char *condition)
{
	i2c_fault_record(condition, ret);

	if (IS_ENABLED(CONFIG_I2C_FAULT_TEST_RELAXED_ERRNO)) {
		zassert_true(ret < 0, "%s: expected a negative errno, got %d", condition, ret);
	} else {
		/*
		 * The I2C API documents -EIO for a failed transfer, and the I2C
		 * documentation's "Transfer Timeout" section documents
		 * -ETIMEDOUT for one that ran out of time.
		 */
		zassert_true((ret == -EIO) || (ret == -ETIMEDOUT),
			     "%s: the I2C API documents -EIO for a failed "
			     "transfer and -ETIMEDOUT for one that timed out, "
			     "got %d",
			     condition, ret);
	}
} /* i2c_fault_assert_error() */

void i2c_fault_assert_healthy(const char *condition)
{
	struct i2c_fault_outcome outcome;
	uint16_t addr = ctx.have_present ? ctx.present_addr : ctx.absent_addr;

	/*
	 * A driver that kept its bus lock after the fault would stall the run
	 * here, so this probe runs on the worker thread like a faulted transfer.
	 */
	(void)probe_bounded(addr, &outcome);
	I2C_FAULT_ASSERT_RETURNED(&outcome,
				  "%s: a probe of 0x%02x did not return within "
				  "%d ms after the fault",
				  condition, addr, CONFIG_I2C_FAULT_TEST_TIMEOUT_MS);

	if (ctx.have_present) {
		zassert_ok(outcome.ret,
			   "%s: the bus was left unusable, address 0x%02x "
			   "stopped answering (%d)",
			   condition, addr, outcome.ret);
		return;
	}

	/*
	 * With nothing on the bus to answer, the weaker check is that a
	 * transfer to the absent address still fails, rather than answering
	 * or hanging.
	 */
	zassert_true(outcome.ret < 0,
		     "%s: absent address 0x%02x began answering after the "
		     "fault (%d)",
		     condition, addr, outcome.ret);
} /* i2c_fault_assert_healthy() */

void i2c_fault_require_present(void)
{
	if (!ctx.have_present) {
		TC_PRINT("  no known-good address answered at setup\n");
		ztest_test_skip();
	}
} /* i2c_fault_require_present() */

/* Devicetree address of one enabled child of the controller under test. */
#define DUT_CHILD_ADDR(node_id) (uint16_t)DT_REG_ADDR(node_id),

/*
 * Probe addr on the worker thread during setup.  A probe that does not return
 * within the wait limit marks the bus unusable, so every test skips.
 */
static bool setup_probe(uint16_t addr, struct i2c_fault_outcome *outcome)
{
	(void)probe_bounded(addr, outcome);

	if (outcome->timed_out) {
		TC_PRINT("  a setup probe of 0x%02x did not return within %d ms\n", addr,
			 CONFIG_I2C_FAULT_TEST_TIMEOUT_MS);
		ctx.bus_unusable = true;
		return false;
	}

	return true;
} /* setup_probe() */

/*
 * Settle on the two addresses the suite works with, each verified by one
 * zero-length write before it is trusted.
 *
 * The absent address is CONFIG_I2C_FAULT_TEST_ABSENT_ADDR, by default 0x01,
 * the CBUS address, which I2C-bus compatible devices are not allowed to
 * respond to, though a system may assign it to a target when CBUS is not
 * used (UM10204 Rev. 7.0, Table 4 and section 3.1.12).  The known-good
 * address is CONFIG_I2C_FAULT_TEST_PRESENT_ADDR where set, and otherwise the
 * first enabled devicetree child of the controller that answers.  No other
 * address is probed.
 */
static void select_addresses(void)
{
	/* The trailing 0 keeps the array non-empty on a controller with no children. */
	static const uint16_t dt_addrs[] = {
		DT_FOREACH_CHILD_STATUS_OKAY(I2C_FAULT_DUT_NODE, DUT_CHILD_ADDR) 0U};
	struct i2c_fault_outcome outcome;

	ctx.absent_addr = (uint16_t)CONFIG_I2C_FAULT_TEST_ABSENT_ADDR;

	if (!setup_probe(ctx.absent_addr, &outcome)) {
		return;
	}

	/*
	 * Every test reads a refused zero-length write as an absent address.
	 * The API does not require a driver to support that probe, so a
	 * controller on which the absent address does not refuse it skips the
	 * run instead of failing it.  -ETIMEDOUT is not a refusal: the probe
	 * never completed, so it says nothing about the address.
	 */
	if ((outcome.ret >= 0) || (outcome.ret == -ETIMEDOUT)) {
		TC_PRINT("  absent address 0x%02x did not refuse a zero-length write (%d); "
			 "set CONFIG_I2C_FAULT_TEST_ABSENT_ADDR to an address nothing answers\n",
			 ctx.absent_addr, outcome.ret);
		ctx.probe_unusable = true;
		return;
	}

	if (CONFIG_I2C_FAULT_TEST_PRESENT_ADDR != 0) {
		if (setup_probe((uint16_t)CONFIG_I2C_FAULT_TEST_PRESENT_ADDR, &outcome) &&
		    (outcome.ret == 0)) {
			ctx.present_addr = (uint16_t)CONFIG_I2C_FAULT_TEST_PRESENT_ADDR;
			ctx.have_present = true;
		}
		return;
	}

	for (size_t i = 0U; (i < (ARRAY_SIZE(dt_addrs) - 1U)) && !ctx.have_present; i++) {
		/* A 10-bit child cannot be checked with a 7-bit probe. */
		if (dt_addrs[i] > 0x7FU) {
			continue;
		}

		if (!setup_probe(dt_addrs[i], &outcome)) {
			return;
		}

		if (outcome.ret == 0) {
			ctx.present_addr = dt_addrs[i];
			ctx.have_present = true;
		}
	}
} /* select_addresses() */

/* Report the addresses setup settled on, so a skipped test is explicable. */
static void report_environment(void)
{
	TC_PRINT("I2C fault injection environment\n");
	TC_PRINT("  controller under test: %s\n", ctx.dut->name);
	TC_PRINT("  absent address:        0x%02x\n", ctx.absent_addr);

	if (ctx.have_present) {
		TC_PRINT("  known-good address:    0x%02x\n", ctx.present_addr);
	} else {
		TC_PRINT("  known-good address:    none found\n");
	}
} /* report_environment() */

void *i2c_fault_setup(void)
{
	static bool ctx_valid;

	/*
	 * Each ztest suite calls this hook.  Only the first call sets up ctx,
	 * so every suite works with the same controller and addresses and the
	 * addresses are probed once.
	 */
	if (ctx_valid) {
		return NULL;
	}

	ctx.dut = DEVICE_DT_GET(I2C_FAULT_DUT_NODE);
	zassert_true(device_is_ready(ctx.dut), "controller under test is not ready");

	i2c_timing_harness_start();

	select_addresses();
	report_environment();

	ctx_valid = true;

	return NULL;
} /* i2c_fault_setup() */

void i2c_fault_before(void *fixture)
{
	ARG_UNUSED(fixture);

	/*
	 * A transfer that did not return within the wait limit may only have
	 * been slow.  Waiting for it once lets the run continue after a slow
	 * driver; one that still has not returned is treated as hung for the
	 * rest of the run.
	 */
	if (!ctx.hang_detected && i2c_timing_harness_stuck() && !i2c_timing_harness_recover()) {
		ctx.hang_detected = true;
	}

	if (ctx.hang_detected) {
		TC_PRINT("  an earlier transfer had still not returned %d ms "
			 "after the wait limit, so the bus state is unknown\n",
			 CONFIG_I2C_FAULT_TEST_TIMEOUT_MS);
		ztest_test_skip();
		return;
	}

	if (ctx.bus_unusable) {
		TC_PRINT("  a setup probe did not return within %d ms, so no address could be "
			 "verified\n",
			 CONFIG_I2C_FAULT_TEST_TIMEOUT_MS);
		ztest_test_skip();
		return;
	}

	if (ctx.probe_unusable) {
		TC_PRINT("  the absent address did not refuse a zero-length write, so no test "
			 "can run\n");
		ztest_test_skip();
		return;
	}
} /* i2c_fault_before() */

ZTEST_SUITE(i2c_fault_harness, NULL, i2c_fault_setup, i2c_fault_before, NULL, NULL);

/**
 * @brief The environment the rest of the suite depends on.
 *
 * @details
 * Test that the controller under test is ready and that the absent address
 * chosen at setup still refuses a zero-length write, sent on the worker
 * thread.  Every address-phase test depends on both, so this should pass
 * whenever the rest of the suite runs.  Catches a setup path that leaves the
 * suite running against a controller that is not ready, or against an
 * address that answers.
 *
 * Requires: one I2C controller.  No fixture.
 */
ZTEST(i2c_fault_harness, test_environment)
{
	struct i2c_fault_outcome outcome;

	zassert_true(device_is_ready(ctx.dut), "controller under test is not ready");

	(void)probe_bounded(ctx.absent_addr, &outcome);
	I2C_FAULT_ASSERT_RETURNED(&outcome,
				  "a probe of the absent address 0x%02x did not return "
				  "within %d ms",
				  ctx.absent_addr, CONFIG_I2C_FAULT_TEST_TIMEOUT_MS);
	zassert_true(outcome.ret < 0, "absent address 0x%02x answers", ctx.absent_addr);
} /* test_environment() */
