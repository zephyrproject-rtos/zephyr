/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * API misuse.
 *
 * These need only a controller.  They test that drivers adhere to the I2C
 * API when a caller passes arguments the API forbids or leaves undefined, or
 * a transfer of no messages, which it defines as a no-op.  They also register
 * a target on a controller that may not implement the target API.
 *
 * Where the API defines the result, the test asserts it.  Where it does not,
 * the test records the value the driver returned and checks that the bus is
 * still usable.
 */

#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/ztest_error_hook.h>

#include "i2c_fault.h"
#include "i2c_timing.h"

/* Speed code 0x6 sits between I2C_SPEED_ULTRA and the I2C_SPEED_DT sentinel. */
#define UNASSIGNED_SPEED 0x6U

static uint8_t tx_buf[I2C_FAULT_PAYLOAD_LEN];

/*
 * test_misuse_null_buffer() runs in user mode, where the suite context in
 * main.c is not readable, so the two values it needs are copied into the
 * ztest partition.
 */
struct i2c_fault_api_misuse_fixture {
	const struct device *dut;
	uint16_t absent_addr;
};

/*
 * Run the shared setup, copy the controller and the absent address into a
 * fixture in the ztest partition, and grant every thread access to the
 * controller, so test_misuse_null_buffer() can use both from user mode.
 */
static void *api_misuse_setup(void)
{
	static ZTEST_DMEM struct i2c_fault_api_misuse_fixture fixture;
	const struct i2c_fault_ctx *ctx;

	(void)i2c_fault_setup();
	ctx = i2c_fault_ctx();

	fixture.dut = ctx->dut;
	fixture.absent_addr = ctx->absent_addr;
	k_object_access_all_grant(fixture.dut);

	return &fixture;
} /* api_misuse_setup() */

ZTEST_SUITE(i2c_fault_api_misuse, NULL, api_misuse_setup, i2c_fault_before, NULL, NULL);

/*
 * Run the NULL-buffer write of test_misuse_null_buffer() on the worker thread,
 * so a driver that never returns from it stalls the worker rather than the
 * test thread.
 */
static void null_buffer_unvalidated(const struct i2c_fault_api_misuse_fixture *fixture)
{
	struct i2c_fault_outcome outcome;
	struct i2c_fault_xfer xfer;
	struct i2c_msg msg;

	msg.buf = NULL;
	msg.len = sizeof(tx_buf);
	msg.flags = I2C_MSG_WRITE | I2C_MSG_STOP;

	xfer.bus = fixture->dut;
	xfer.addr = fixture->absent_addr;
	xfer.msgs = &msg;
	xfer.num_msgs = 1U;

	i2c_fault_transfer_bounded(&xfer, "NULL buffer with non-zero length", &outcome);
	i2c_fault_record("NULL buffer with non-zero length", outcome.ret);
	i2c_fault_assert_healthy("NULL buffer with non-zero length");
} /* null_buffer_unvalidated() */

/**
 * @brief A NULL buffer with a non-zero length.
 *
 * @details
 * Test a write from a user thread whose buffer is NULL and whose length is
 * not.  The syscall handler in i2c_handlers.c should reject it before the
 * driver is reached, by killing the calling thread with K_OOPS(), so the call
 * must not return.  Catches a syscall path that hands an unvalidated caller
 * buffer to the driver.
 *
 * The handler checks the calling thread's access to the controller before it
 * checks the buffer, and either failure kills the thread the same way.  The
 * same message with a valid buffer is therefore sent first and must return.
 * That shows the thread has access, so the kill on the NULL write can only
 * come from the buffer check.
 *
 * Without CONFIG_USERSPACE nothing validates the buffer, and a driver may
 * dereference it or hand it to a DMA engine.  The test then skips unless
 * CONFIG_I2C_FAULT_TEST_UNSAFE_API_MISUSE is set.  With the option set, the
 * same write is sent to the absent address on the worker thread.  The API
 * defines no result for it, so the value the driver returned is recorded,
 * and the driver should return within the wait limit and leave the bus
 * usable.
 *
 * Requires: one I2C controller, and either CONFIG_USERSPACE or the opt-in.
 */
ZTEST_USER_F(i2c_fault_api_misuse, test_misuse_null_buffer)
{
	static ZTEST_BMEM uint8_t payload[I2C_FAULT_PAYLOAD_LEN];
	struct i2c_msg msg;
	int ret;

	if (!IS_ENABLED(CONFIG_USERSPACE)) {
		if (!IS_ENABLED(CONFIG_I2C_FAULT_TEST_UNSAFE_API_MISUSE)) {
			TC_PRINT("  without CONFIG_USERSPACE the API does not "
				 "validate this argument and the driver may "
				 "dereference it; enable "
				 "CONFIG_I2C_FAULT_TEST_UNSAFE_API_MISUSE to "
				 "run this test anyway\n");
			ztest_test_skip();
			return;
		}

		null_buffer_unvalidated(fixture);
		return;
	}

	msg.buf = payload;
	msg.len = sizeof(payload);
	msg.flags = I2C_MSG_WRITE | I2C_MSG_STOP;

	ret = i2c_transfer(fixture->dut, &msg, 1U, fixture->absent_addr);
	zassert_true(ret < 0,
		     "NULL buffer: the absent address 0x%02x answered a write "
		     "from a user thread (%d)",
		     fixture->absent_addr, ret);

	msg.buf = NULL;

	ztest_set_fault_valid(true);
	ret = i2c_transfer(fixture->dut, &msg, 1U, fixture->absent_addr);

	/*
	 * The flag is keyed to the thread, and ztest runs every case on the
	 * same thread object, so it is cleared before failing.
	 */
	ztest_set_fault_valid(false);
	zassert_unreachable("a NULL buffer of %u bytes from a user thread "
			    "reached the driver and returned %d",
			    msg.len, ret);
} /* test_misuse_null_buffer() */

/**
 * @brief A transfer with no messages.
 *
 * @details
 * Test i2c_transfer() with num_msgs of zero and an otherwise valid message
 * array.  The I2C API documents that no transfer occurs in that case, so the
 * call should return 0 without touching the bus.  Callers that build message
 * arrays at run time can produce an empty one and rely on it being a no-op.
 * Catches a change that makes an empty transfer return an error.
 *
 * The test runs in supervisor mode.  Under CONFIG_USERSPACE the syscall
 * handler in i2c_handlers.c kills a user thread that passes num_msgs of zero,
 * with K_OOPS().
 *
 * Requires: one I2C controller.  No fixture.
 */
ZTEST(i2c_fault_api_misuse, test_misuse_zero_messages)
{
	const struct i2c_fault_ctx *ctx = i2c_fault_ctx();
	struct i2c_msg msg;
	int ret;

	msg.buf = tx_buf;
	msg.len = sizeof(tx_buf);
	msg.flags = I2C_MSG_WRITE | I2C_MSG_STOP;

	ret = i2c_transfer(ctx->dut, &msg, 0U, ctx->absent_addr);
	i2c_fault_record("transfer with zero messages", ret);
	zassert_ok(ret,
		   "a transfer of no messages is defined as a no-op "
		   "returning 0, got %d",
		   ret);
	i2c_fault_assert_healthy("transfer with zero messages");
} /* test_misuse_zero_messages() */

/**
 * @brief A zero-length message in the middle of a sequence.
 *
 * @details
 * Test a three-message write to the absent address whose middle message has
 * a valid buffer and a length of zero.  The API neither forbids an empty
 * message nor defines its result, so the value is recorded; what is required
 * is that the driver returns within the test wait limit
 * (CONFIG_I2C_FAULT_TEST_TIMEOUT_MS) and leaves the bus usable.  Catches a
 * driver that miscounts messages around the empty one, by dividing by its
 * length, treating zero as "use the whole buffer", or advancing its index
 * twice, and hangs or leaves the controller unusable as a result.
 *
 * Requires: one I2C controller.  No fixture.
 */
ZTEST(i2c_fault_api_misuse, test_misuse_zero_length_middle)
{
	const struct i2c_fault_ctx *ctx = i2c_fault_ctx();
	struct i2c_fault_outcome outcome;
	struct i2c_fault_xfer xfer;
	struct i2c_msg msgs[3];

	msgs[0].buf = tx_buf;
	msgs[0].len = 1U;
	msgs[0].flags = I2C_MSG_WRITE;
	msgs[1].buf = tx_buf;
	msgs[1].len = 0U;
	msgs[1].flags = I2C_MSG_WRITE;
	msgs[2].buf = tx_buf;
	msgs[2].len = 1U;
	msgs[2].flags = I2C_MSG_WRITE | I2C_MSG_STOP;

	xfer.bus = ctx->dut;
	xfer.addr = ctx->absent_addr;
	xfer.msgs = msgs;
	xfer.num_msgs = ARRAY_SIZE(msgs);

	i2c_fault_transfer_bounded(&xfer, "zero-length message mid-sequence", &outcome);
	i2c_fault_record("zero-length message mid-sequence", outcome.ret);
	i2c_fault_assert_healthy("zero-length message mid-sequence");
} /* test_misuse_zero_length_middle() */

/**
 * @brief A speed code the API does not assign.
 *
 * @details
 * Test i2c_configure() with speed code 0x6, the one value of the three-bit
 * speed field that the I2C API leaves unassigned, between I2C_SPEED_ULTRA and
 * the I2C_SPEED_DT sentinel.  The API does not say what an unassigned code
 * should produce, so the result is recorded.  The configuration read
 * beforehand with i2c_get_config() should then restore, and the bus should
 * be usable.  Catches a driver that accepts the code, programs an unintended
 * clock divider, and cannot be returned to its previous configuration.
 *
 * Requires: a driver implementing i2c_get_config().  No fixture.
 */
ZTEST(i2c_fault_api_misuse, test_misuse_invalid_speed)
{
	const struct i2c_fault_ctx *ctx = i2c_fault_ctx();
	uint32_t original;
	uint32_t bad;
	int ret;

	ret = i2c_get_config(ctx->dut, &original);
	if (ret == -ENOSYS) {
		TC_PRINT("  the driver does not implement i2c_get_config()\n");
		ztest_test_skip();
		return;
	}

	zassert_ok(ret, "unassigned speed code: i2c_get_config() failed (%d)", ret);

	bad = (original & ~(uint32_t)I2C_SPEED_MASK) | I2C_SPEED_SET(UNASSIGNED_SPEED);

	ret = i2c_configure(ctx->dut, bad);
	i2c_fault_record("unassigned speed code", ret);

	zassert_ok(i2c_configure(ctx->dut, original),
		   "unassigned speed code: the original configuration "
		   "0x%08x could not be restored",
		   original);
	i2c_fault_assert_healthy("unassigned speed code");
} /* test_misuse_invalid_speed() */

/**
 * @brief The target API on a controller that may not implement it.
 *
 * @details
 * Test i2c_target_register() with a no-op target at the absent address, then
 * i2c_target_unregister() if registration succeeded.  Registration should
 * return a value the I2C API documents: 0, -EINVAL, -EIO or -ENOSYS, where
 * -ENOSYS ends the test.  After 0, unregistration should succeed and the
 * controller should still complete transfers.  Catches a driver that
 * registers a target but cannot remove it, or that leaves the controller
 * unusable for transfers once the target is gone.
 *
 * Requires: one I2C controller.  No fixture.
 */
ZTEST(i2c_fault_api_misuse, test_misuse_target_api_unsupported)
{
	const struct i2c_fault_ctx *ctx = i2c_fault_ctx();
	struct i2c_target_config cfg = {
		.callbacks = &i2c_fault_noop_callbacks,
	};
	int ret;

	/*
	 * An address that answers already would collide with a real device,
	 * so the absent address is reused here.
	 */
	cfg.address = ctx->absent_addr;

	ret = i2c_target_register(ctx->dut, &cfg);
	i2c_fault_record("target registration", ret);

	if (ret == -ENOSYS) {
		return;
	}

	zassert_true((ret == 0) || (ret == -EINVAL) || (ret == -EIO),
		     "target registration returned %d; the I2C API documents 0, "
		     "-EINVAL, -EIO and -ENOSYS",
		     ret);

	if (ret == 0) {
		zassert_ok(i2c_target_unregister(ctx->dut, &cfg),
			   "a target that registered could not be "
			   "unregistered");
	}

	i2c_fault_assert_healthy("target registration");
} /* test_misuse_target_api_unsupported() */
