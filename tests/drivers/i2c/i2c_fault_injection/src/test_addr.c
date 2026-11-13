/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Address-phase faults.
 *
 * These need only a controller and an address that nothing answers, so they
 * run on any board that sets the i2c-dut alias.  They cover writes, reads and
 * write-reads to that address, repeated and alternating NACKs, and the 10-bit,
 * general call (0x00) and reserved addresses.  The test thread waits at most
 * the test wait limit (CONFIG_I2C_FAULT_TEST_TIMEOUT_MS) for each transfer.
 *
 * Where a condition asserts an error, the accepted values are the ones
 * i2c_fault_assert_error() documents.
 */

#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include "i2c_fault.h"
#include "i2c_timing.h"

/*
 * Buffers used by transfers on the worker thread must outlive the test that
 * submitted them, because a transfer still running at the wait limit is left
 * running and the controller may still be reading or writing them.  They are
 * file scope for that reason.
 */
static uint8_t tx_buf[I2C_FAULT_PAYLOAD_LEN];
static uint8_t rx_buf[I2C_FAULT_PAYLOAD_LEN];
static uint8_t probe_byte;

/* Run a transfer on the worker thread and assert it failed cleanly. */
static void expect_error(struct i2c_msg *msgs, uint8_t num_msgs, uint16_t addr,
			 const char *condition)
{
	const struct i2c_fault_ctx *ctx = i2c_fault_ctx();
	struct i2c_fault_outcome outcome;
	struct i2c_fault_xfer xfer = {
		.bus = ctx->dut,
		.addr = addr,
		.msgs = msgs,
		.num_msgs = num_msgs,
	};

	i2c_fault_transfer_bounded(&xfer, condition, &outcome);
	i2c_fault_assert_error(outcome.ret, condition);
} /* expect_error() */

ZTEST_SUITE(i2c_fault_addr, NULL, i2c_fault_setup, i2c_fault_before, NULL, NULL);

/**
 * @brief A write to an address nothing answers must report an error.
 *
 * @details
 * Test a write of I2C_FAULT_PAYLOAD_LEN bytes to the absent address found at
 * setup, whose address byte nothing acknowledges.  The driver should report
 * the NACK as an error within the wait limit and leave the bus usable for the
 * next transfer.  Catches a driver that reports success after an address NACK,
 * or that leaves the controller unable to start again.
 *
 * Requires: one I2C controller.  No fixture.
 */
ZTEST(i2c_fault_addr, test_addr_nack_write)
{
	const struct i2c_fault_ctx *ctx = i2c_fault_ctx();
	struct i2c_msg msg;

	msg.buf = tx_buf;
	msg.len = sizeof(tx_buf);
	msg.flags = I2C_MSG_WRITE | I2C_MSG_STOP;

	expect_error(&msg, 1U, ctx->absent_addr, "address NACK on write");
	i2c_fault_assert_healthy("address NACK on write");
} /* test_addr_nack_write() */

/**
 * @brief A read from an address nothing answers must report an error.
 *
 * @details
 * Test a read of I2C_FAULT_PAYLOAD_LEN bytes from the absent address.  The
 * driver should report the address NACK as an error within the wait limit
 * and leave the bus usable.  Catches a driver that waits for read data that
 * never arrives, or that returns success with nothing read.
 *
 * Requires: one I2C controller.  No fixture.
 */
ZTEST(i2c_fault_addr, test_addr_nack_read)
{
	const struct i2c_fault_ctx *ctx = i2c_fault_ctx();
	struct i2c_msg msg;

	msg.buf = rx_buf;
	msg.len = sizeof(rx_buf);
	msg.flags = I2C_MSG_READ | I2C_MSG_STOP;

	expect_error(&msg, 1U, ctx->absent_addr, "address NACK on read");
	i2c_fault_assert_healthy("address NACK on read");
} /* test_addr_nack_read() */

/**
 * @brief A write-read must report an error rather than stale data.
 *
 * @details
 * Test a write-read to the absent address: a one-byte write, then a repeated
 * START and a read of I2C_FAULT_PAYLOAD_LEN bytes.  The NACK on the first
 * address phase should end the whole transfer with an error, within the wait
 * limit, and leave the bus usable.  Catches a driver that still starts the
 * read after the failed write and reports success with data left from an
 * earlier transfer.
 *
 * Requires: one I2C controller.  No fixture.
 */
ZTEST(i2c_fault_addr, test_addr_nack_write_read)
{
	const struct i2c_fault_ctx *ctx = i2c_fault_ctx();
	struct i2c_fault_outcome outcome;
	struct i2c_msg msgs[2];
	struct i2c_fault_xfer xfer;

	msgs[0].buf = tx_buf;
	msgs[0].len = 1U;
	msgs[0].flags = I2C_MSG_WRITE;
	msgs[1].buf = rx_buf;
	msgs[1].len = sizeof(rx_buf);
	msgs[1].flags = I2C_MSG_RESTART | I2C_MSG_READ | I2C_MSG_STOP;

	xfer.bus = ctx->dut;
	xfer.addr = ctx->absent_addr;
	xfer.msgs = msgs;
	xfer.num_msgs = ARRAY_SIZE(msgs);

	i2c_fault_transfer_bounded(&xfer, "address NACK on write-read", &outcome);
	i2c_fault_assert_error(outcome.ret, "address NACK on write-read");
	i2c_fault_assert_healthy("address NACK on write-read");
} /* test_addr_nack_write_read() */

/**
 * @brief A NACK must not latch the controller state machine.
 *
 * @details
 * Test CONFIG_I2C_FAULT_TEST_REPEAT_COUNT consecutive one-byte writes to the
 * absent address, with nothing in between.
 *
 * Every attempt should fail with an error i2c_fault_assert_error() accepts,
 * and the bus should be usable afterwards.  Each attempt is checked and
 * recorded on its own.  The API does not say that repeated attempts return
 * the same value, so the values are not compared.
 *
 * Catches a driver that carries state out of the first failure, so a later
 * attempt returns success or an error the API does not document.
 *
 * Requires: one I2C controller.  No fixture.
 */
ZTEST(i2c_fault_addr, test_addr_nack_persistent)
{
	const struct i2c_fault_ctx *ctx = i2c_fault_ctx();
	struct i2c_fault_outcome outcome;
	struct i2c_fault_xfer xfer;
	struct i2c_msg msg;

	msg.buf = tx_buf;
	msg.len = 1U;
	msg.flags = I2C_MSG_WRITE | I2C_MSG_STOP;

	xfer.bus = ctx->dut;
	xfer.addr = ctx->absent_addr;
	xfer.msgs = &msg;
	xfer.num_msgs = 1U;

	for (uint32_t i = 0U; i < CONFIG_I2C_FAULT_TEST_REPEAT_COUNT; i++) {
		msg.flags = I2C_MSG_WRITE | I2C_MSG_STOP;

		i2c_fault_transfer_bounded(&xfer, "repeated address NACK", &outcome);
		i2c_fault_assert_error(outcome.ret, "repeated address NACK");
	}

	i2c_fault_assert_healthy("repeated address NACK");
} /* test_addr_nack_persistent() */

/**
 * @brief A NACK must not disturb the transfer that follows it.
 *
 * @details
 * Test CONFIG_I2C_FAULT_TEST_REPEAT_COUNT rounds of four transfers, in this
 * order: a one-byte write to the absent address, a zero-length write to the
 * known-good address, a write-read to the absent address, and another
 * zero-length write to the known-good address.
 *
 * Every transfer to the absent address should fail with an error
 * i2c_fault_assert_error() accepts, and every one to the known-good address
 * should succeed.
 *
 * Catches a driver that carries state from a failed transfer into the next
 * one, such as a completion left pending by the NACK that the following
 * transfer picks up as its own.
 *
 * Requires: one I2C controller and one device that answers.  No fixture.
 */
ZTEST(i2c_fault_addr, test_addr_nack_alternating)
{
	const struct i2c_fault_ctx *ctx = i2c_fault_ctx();
	struct i2c_fault_outcome outcome;
	struct i2c_fault_xfer xfer;
	struct i2c_msg msgs[2];

	i2c_fault_require_present();

	xfer.bus = ctx->dut;
	xfer.addr = ctx->absent_addr;
	xfer.msgs = msgs;

	for (uint32_t i = 0U; i < CONFIG_I2C_FAULT_TEST_REPEAT_COUNT; i++) {
		msgs[0].buf = tx_buf;
		msgs[0].len = 1U;
		msgs[0].flags = I2C_MSG_WRITE | I2C_MSG_STOP;
		xfer.num_msgs = 1U;

		i2c_fault_transfer_bounded(&xfer, "alternating NACK, write", &outcome);
		i2c_fault_assert_error(outcome.ret, "alternating NACK, write");

		i2c_fault_assert_healthy("alternating NACK, after a failed write");

		msgs[0].buf = tx_buf;
		msgs[0].len = 1U;
		msgs[0].flags = I2C_MSG_WRITE;
		msgs[1].buf = rx_buf;
		msgs[1].len = sizeof(rx_buf);
		msgs[1].flags = I2C_MSG_RESTART | I2C_MSG_READ | I2C_MSG_STOP;
		xfer.num_msgs = ARRAY_SIZE(msgs);

		i2c_fault_transfer_bounded(&xfer, "alternating NACK, write-read", &outcome);
		i2c_fault_assert_error(outcome.ret, "alternating NACK, write-read");

		i2c_fault_assert_healthy("alternating NACK, after a failed write-read");
	}
} /* test_addr_nack_alternating() */

/**
 * @brief A 10-bit address nothing answers must fail like a 7-bit one.
 *
 * @details
 * Test a one-byte write carrying I2C_MSG_ADDR_10_BITS to
 * CONFIG_I2C_FAULT_TEST_ABSENT_ADDR_10BIT.  The driver should report the
 * address NACK as an error within the wait limit and leave the bus usable.
 * -ENOTSUP or -ENOSYS is taken to mean the controller has no 10-bit support,
 * and the test skips.  Catches a driver that reports success for a
 * 10-bit address nothing acknowledges.
 *
 * The I2C API notes that not every controller supports 10-bit addressing but
 * does not say what such a driver returns.  A driver that rejects the message
 * with any other negative value passes here without a 10-bit address reaching
 * the bus.
 *
 * Before the data write, a zero-length 10-bit write checks that nothing
 * answers the address, so no data byte reaches a device there.  If something
 * answers, the test skips and names the address; set
 * CONFIG_I2C_FAULT_TEST_ABSENT_ADDR_10BIT to another one.
 *
 * Requires: one I2C controller.  No fixture.
 */
ZTEST(i2c_fault_addr, test_addr_nack_10bit)
{
	const struct i2c_fault_ctx *ctx = i2c_fault_ctx();
	struct i2c_fault_outcome outcome;
	struct i2c_fault_xfer xfer;
	struct i2c_msg msg;

	xfer.bus = ctx->dut;
	xfer.addr = (uint16_t)CONFIG_I2C_FAULT_TEST_ABSENT_ADDR_10BIT;
	xfer.msgs = &msg;
	xfer.num_msgs = 1U;

	msg.buf = &probe_byte;
	msg.len = 0U;
	msg.flags = I2C_MSG_WRITE | I2C_MSG_STOP | I2C_MSG_ADDR_10_BITS;

	i2c_fault_transfer_bounded(&xfer, "10-bit address probe", &outcome);
	i2c_fault_record("10-bit address probe", outcome.ret);

	if ((outcome.ret == -ENOTSUP) || (outcome.ret == -ENOSYS)) {
		TC_PRINT("  the controller does not support 10-bit addressing\n");
		ztest_test_skip();
		return;
	}

	/* A device answers this address, so the data write is not sent to it. */
	if (outcome.ret == 0) {
		TC_PRINT("  10-bit address 0x%03x answered a zero-length write\n",
			 (unsigned int)CONFIG_I2C_FAULT_TEST_ABSENT_ADDR_10BIT);
		ztest_test_skip();
		return;
	}

	msg.buf = tx_buf;
	msg.len = 1U;
	msg.flags = I2C_MSG_WRITE | I2C_MSG_STOP | I2C_MSG_ADDR_10_BITS;

	i2c_fault_transfer_bounded(&xfer, "10-bit address NACK", &outcome);
	i2c_fault_record("10-bit address NACK", outcome.ret);

	if ((outcome.ret == -ENOTSUP) || (outcome.ret == -ENOSYS)) {
		TC_PRINT("  the controller does not support 10-bit "
			 "addressing\n");
		ztest_test_skip();
		return;
	}

	zassert_true(outcome.ret < 0, "10-bit address NACK: 0x%03x answered",
		     (unsigned int)CONFIG_I2C_FAULT_TEST_ABSENT_ADDR_10BIT);
	i2c_fault_assert_healthy("10-bit address NACK");
} /* test_addr_nack_10bit() */

/**
 * @brief The general call address must not hang or corrupt the bus.
 *
 * @details
 * Test a zero-length write to the general call address, 0x00.  The write
 * carries no command byte, so no device that listens to general calls acts
 * on it.  Whether anything acknowledges depends on the board, so the result is
 * recorded rather than asserted; what is required is that the driver returns
 * within the wait limit and leaves the bus usable.  Catches a driver that does
 * not return, or cannot start the next transfer, after addressing 0x00.
 *
 * Requires: one I2C controller.  No fixture.
 */
ZTEST(i2c_fault_addr, test_addr_general_call)
{
	const struct i2c_fault_ctx *ctx = i2c_fault_ctx();
	struct i2c_fault_outcome outcome;
	struct i2c_fault_xfer xfer;
	struct i2c_msg msg;

	/* Zero length keeps the attempt non-destructive to any listener. */
	msg.buf = &probe_byte;
	msg.len = 0U;
	msg.flags = I2C_MSG_WRITE | I2C_MSG_STOP;

	xfer.bus = ctx->dut;
	xfer.addr = 0x00U;
	xfer.msgs = &msg;
	xfer.num_msgs = 1U;

	i2c_fault_transfer_bounded(&xfer, "general call address", &outcome);
	i2c_fault_record("general call address", outcome.ret);
	i2c_fault_assert_healthy("general call address");
} /* test_addr_general_call() */

/**
 * @brief A zero-length write to each reserved address must return and leave
 * the bus usable.
 *
 * @details
 * Test a zero-length write to each of 0x01 to 0x07 and 0x78 to 0x7F in turn.
 * The I2C-bus specification (UM10204 Rev. 7.0, Table 4) reserves these for
 * CBUS, a different bus format, future purposes, Hs-mode controller codes,
 * device ID, and 10-bit addressing.  Address 0x00, the general call, is
 * tested by test_addr_general_call().
 *
 * Neither the Zephyr API nor the specification requires a controller to
 * reject a reserved address; the specification allows one to be used as a
 * target address when its reserved purpose is unused.  Each result is
 * recorded, not asserted.  What is required is that every attempt returns
 * within the wait limit and the bus is usable at the end.  Catches a driver
 * that hangs on an address it does not expect, or is left unable to start
 * the next transfer.
 *
 * Requires: one I2C controller.  No fixture.
 */
ZTEST(i2c_fault_addr, test_addr_reserved)
{
	static const uint16_t reserved[] = {
		0x01U, 0x02U, 0x03U, 0x04U, 0x05U, 0x06U, 0x07U,
		0x78U, 0x79U, 0x7AU, 0x7BU, 0x7CU, 0x7DU, 0x7EU, 0x7FU,
	};
	const struct i2c_fault_ctx *ctx = i2c_fault_ctx();
	struct i2c_fault_outcome outcome;
	struct i2c_fault_xfer xfer;
	struct i2c_msg msg;

	msg.buf = &probe_byte;
	msg.len = 0U;

	xfer.bus = ctx->dut;
	xfer.msgs = &msg;
	xfer.num_msgs = 1U;

	for (size_t i = 0U; i < ARRAY_SIZE(reserved); i++) {
		msg.flags = I2C_MSG_WRITE | I2C_MSG_STOP;
		xfer.addr = reserved[i];

		i2c_fault_transfer_bounded(&xfer, "reserved address", &outcome);
		TC_PRINT("  reserved address 0x%02x: driver returned %d\n", reserved[i],
			 outcome.ret);
	}

	i2c_fault_assert_healthy("reserved address");
} /* test_addr_reserved() */
