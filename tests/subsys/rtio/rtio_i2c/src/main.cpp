/* Copyright (c) 2024 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#include "blocking_emul.hpp"

#include <zephyr/drivers/i2c.h>
#include <zephyr/fff.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/ztest.h>

DEFINE_FFF_GLOBALS;

static const struct device *i2c_dev = DEVICE_DT_GET(DT_NODELABEL(i2c0));
I2C_DT_IODEV_DEFINE(blocking_emul_iodev, DT_NODELABEL(blocking_emul));

RTIO_DEFINE(test_rtio_ctx, 4, 4);

static void rtio_i2c_before(void *fixture)
{
	ARG_UNUSED(fixture);
	RESET_FAKE(blocking_emul_i2c_transfer);
	FFF_RESET_HISTORY();

	rtio_sqe_drop_all(&test_rtio_ctx);

	struct rtio_cqe *cqe;

	while ((cqe = rtio_cqe_consume(&test_rtio_ctx)) != NULL) {
		rtio_cqe_release(&test_rtio_ctx, cqe);
	}
}

ZTEST_SUITE(rtio_i2c, NULL, NULL, rtio_i2c_before, NULL, NULL);

ZTEST(rtio_i2c, test_emulated_api_uses_fallback_submit)
{
	zassert_not_null(i2c_dev->api);
	zassert_equal_ptr(i2c_iodev_submit_fallback,
			  ((const struct i2c_driver_api *)i2c_dev->api)->iodev_submit);
}

ZTEST(rtio_i2c, test_fallback_submit_tx)
{
	uint8_t data[] = {0x01, 0x02, 0x03};
	struct i2c_msg msg = {
		.buf = data,
		.len = ARRAY_SIZE(data),
		.flags = I2C_MSG_WRITE | I2C_MSG_STOP,
	};

	blocking_emul_i2c_transfer_fake.custom_fake =
		[&msg](const struct emul *, struct i2c_msg *msgs, int msg_count, int) {
			zassert_equal(1, msg_count);
			zassert_equal(msg.len, msgs[0].len);
			zassert_mem_equal(msg.buf, msgs[0].buf, msg.len);
			zassert_equal(msg.flags, msgs[0].flags);
			return 0;
		};

	struct rtio_sqe *sqe = i2c_rtio_copy(&test_rtio_ctx, &blocking_emul_iodev, &msg, 1);

	zassert_not_null(sqe);
	zassert_ok(rtio_submit(&test_rtio_ctx, 1));
	zassert_equal(1, blocking_emul_i2c_transfer_fake.call_count);

	struct rtio_cqe *cqe = rtio_cqe_consume_block(&test_rtio_ctx);

	zassert_ok(cqe->result);
	rtio_cqe_release(&test_rtio_ctx, cqe);
}

ZTEST(rtio_i2c, test_fallback_submit_invalid_op)
{
	struct rtio_sqe *sqe = rtio_sqe_acquire(&test_rtio_ctx);

	zassert_not_null(sqe);
	sqe->op = UINT8_MAX;
	sqe->prio = RTIO_PRIO_NORM;
	sqe->iodev = &blocking_emul_iodev;
	sqe->userdata = NULL;

	zassert_ok(rtio_submit(&test_rtio_ctx, 1));
	zassert_equal(0, blocking_emul_i2c_transfer_fake.call_count);

	struct rtio_cqe *cqe = rtio_cqe_consume_block(&test_rtio_ctx);

	zassert_equal(-EIO, cqe->result);
	rtio_cqe_release(&test_rtio_ctx, cqe);
}

ZTEST(rtio_i2c, test_fallback_submit_tiny_tx)
{
	uint8_t data[] = {0x01, 0x02, 0x03};
	struct rtio_sqe *sqe = rtio_sqe_acquire(&test_rtio_ctx);

	blocking_emul_i2c_transfer_fake.custom_fake =
		[&data](const struct emul *, struct i2c_msg *msgs, int msg_count, int) {
			zassert_equal(1, msg_count);
			zassert_equal(ARRAY_SIZE(data), msgs[0].len);
			zassert_mem_equal(data, msgs[0].buf, msgs[0].len);
			zassert_equal(I2C_MSG_WRITE | I2C_MSG_STOP, msgs[0].flags);
			return 0;
		};

	zassert_not_null(sqe);

	rtio_sqe_prep_tiny_write(sqe, &blocking_emul_iodev, RTIO_PRIO_NORM, data, ARRAY_SIZE(data),
				 NULL);
	zassert_ok(rtio_submit(&test_rtio_ctx, 1));
	zassert_equal(1, blocking_emul_i2c_transfer_fake.call_count);

	struct rtio_cqe *cqe = rtio_cqe_consume_block(&test_rtio_ctx);

	zassert_ok(cqe->result);

	rtio_cqe_release(&test_rtio_ctx, cqe);
}

ZTEST(rtio_i2c, test_fallback_submit_txrx)
{
	uint8_t tx_data[] = {0x01, 0x02, 0x03};
	uint8_t rx_data[ARRAY_SIZE(tx_data)] = {0};
	struct rtio_sqe *sqe = rtio_sqe_acquire(&test_rtio_ctx);

	blocking_emul_i2c_transfer_fake.custom_fake =
		[&tx_data](const struct emul *, struct i2c_msg *msgs, int msg_count, int) {
			zassert_equal(2, msg_count);
			// First message should be a 'tx'
			zassert_equal(ARRAY_SIZE(tx_data), msgs[0].len);
			zassert_mem_equal(tx_data, msgs[0].buf, msgs[0].len);
			zassert_equal(I2C_MSG_WRITE, msgs[0].flags);
			// Second message should be an 'rx'
			zassert_equal(ARRAY_SIZE(tx_data), msgs[1].len);
			zassert_equal(I2C_MSG_READ | I2C_MSG_STOP, msgs[1].flags);
			for (uint8_t i = 0; i < msgs[1].len; ++i) {
				msgs[1].buf[i] = msgs[0].buf[i];
			}
			return 0;
		};

	zassert_not_null(sqe);
	rtio_sqe_prep_transceive(sqe, &blocking_emul_iodev, RTIO_PRIO_NORM, tx_data, rx_data,
				 ARRAY_SIZE(tx_data), NULL);
	zassert_ok(rtio_submit(&test_rtio_ctx, 1));
	zassert_equal(1, blocking_emul_i2c_transfer_fake.call_count);

	struct rtio_cqe *cqe = rtio_cqe_consume_block(&test_rtio_ctx);

	zassert_ok(cqe->result);
	zassert_mem_equal(tx_data, rx_data, ARRAY_SIZE(tx_data));

	rtio_cqe_release(&test_rtio_ctx, cqe);
}

ZTEST(rtio_i2c, test_fallback_submit_rx)
{
	uint8_t expected_buffer[] = {0x00, 0x01, 0x02};
	uint8_t buffer[ARRAY_SIZE(expected_buffer)] = {0};
	struct i2c_msg msg = {
		.buf = buffer,
		.len = ARRAY_SIZE(buffer),
		.flags = I2C_MSG_READ | I2C_MSG_STOP,
	};

	blocking_emul_i2c_transfer_fake.custom_fake =
		[&msg](const struct emul *, struct i2c_msg *msgs, int msg_count, int) {
			zassert_equal(1, msg_count);
			zassert_equal(msg.len, msgs[0].len);
			zassert_equal(msg.flags, msgs[0].flags);
			for (uint8_t i = 0; i < msg.len; ++i) {
				msgs[0].buf[i] = i;
			}
			return 0;
		};

	struct rtio_sqe *sqe = i2c_rtio_copy(&test_rtio_ctx, &blocking_emul_iodev, &msg, 1);

	zassert_not_null(sqe);
	zassert_ok(rtio_submit(&test_rtio_ctx, 1));
	zassert_equal(1, blocking_emul_i2c_transfer_fake.call_count);

	struct rtio_cqe *cqe = rtio_cqe_consume_block(&test_rtio_ctx);

	zassert_ok(cqe->result);
	zassert_mem_equal(buffer, expected_buffer, ARRAY_SIZE(expected_buffer));

	rtio_cqe_release(&test_rtio_ctx, cqe);
}

ZTEST(rtio_i2c, test_fallback_transaction_error)
{
	uint8_t buffer[3];
	struct rtio_sqe *phase1 = rtio_sqe_acquire(&test_rtio_ctx);
	struct rtio_sqe *phase2 = rtio_sqe_acquire(&test_rtio_ctx);

	blocking_emul_i2c_transfer_fake.return_val = -EIO;

	zassert_not_null(phase1);
	zassert_not_null(phase2);

	rtio_sqe_prep_read(phase1, &blocking_emul_iodev, RTIO_PRIO_NORM, buffer, ARRAY_SIZE(buffer),
			   NULL);
	rtio_sqe_prep_read(phase2, &blocking_emul_iodev, RTIO_PRIO_NORM, buffer, ARRAY_SIZE(buffer),
			   NULL);

	phase1->flags |= RTIO_SQE_TRANSACTION;

	zassert_ok(rtio_submit(&test_rtio_ctx, 2));
	zassert_equal(1, blocking_emul_i2c_transfer_fake.call_count);

	struct rtio_cqe *cqe = rtio_cqe_consume_block(&test_rtio_ctx);

	zassert_equal(-EIO, cqe->result);

	rtio_cqe_release(&test_rtio_ctx, cqe);

	// We have another CQE for the transaction that must be cleared out.
	cqe = rtio_cqe_consume_block(&test_rtio_ctx);
	rtio_cqe_release(&test_rtio_ctx, cqe);
}

ZTEST(rtio_i2c, test_fallback_transaction)
{
	uint8_t buffer[3];
	struct rtio_sqe *phase1 = rtio_sqe_acquire(&test_rtio_ctx);
	struct rtio_sqe *phase2 = rtio_sqe_acquire(&test_rtio_ctx);

	zassert_not_null(phase1);
	zassert_not_null(phase2);

	rtio_sqe_prep_read(phase1, &blocking_emul_iodev, RTIO_PRIO_NORM, buffer, ARRAY_SIZE(buffer),
			   NULL);
	rtio_sqe_prep_read(phase2, &blocking_emul_iodev, RTIO_PRIO_NORM, buffer, ARRAY_SIZE(buffer),
			   NULL);

	phase1->flags |= RTIO_SQE_TRANSACTION;

	zassert_ok(rtio_submit(&test_rtio_ctx, 2));
	zassert_equal(2, blocking_emul_i2c_transfer_fake.call_count);

	struct rtio_cqe *cqe;

	// Check the first part of the transaction.
	cqe = rtio_cqe_consume_block(&test_rtio_ctx);
	zassert_ok(cqe->result);
	rtio_cqe_release(&test_rtio_ctx, cqe);

	// We have another CQE for the transaction that must be cleared out.
	cqe = rtio_cqe_consume_block(&test_rtio_ctx);
	zassert_ok(cqe->result);
	rtio_cqe_release(&test_rtio_ctx, cqe);
}

ZTEST(rtio_i2c, test_work_queue_overflow)
{
	BUILD_ASSERT(CONFIG_RTIO_WORKQ_POOL_ITEMS == 2);

	uint8_t data[][2] = {
		{0x01, 0x02},
		{0x03, 0x04},
		{0x05, 0x06},
	};
	struct i2c_msg msg[] = {
		{
			.buf = data[0],
			.len = 2,
			.flags = I2C_MSG_WRITE | I2C_MSG_STOP,
		},
		{
			.buf = data[1],
			.len = 2,
			.flags = I2C_MSG_READ | I2C_MSG_STOP,
		},
		{
			.buf = data[2],
			.len = 2,
			.flags = I2C_MSG_READ | I2C_MSG_ADDR_10_BITS | I2C_MSG_STOP,
		},
	};

	BUILD_ASSERT(ARRAY_SIZE(data) == ARRAY_SIZE(msg));

	blocking_emul_i2c_transfer_fake.custom_fake =
		[&msg](const struct emul *, struct i2c_msg *msgs, int msg_count, int) {
			zassert_equal(1, msg_count);

			int msg_idx = i2c_is_read_op(&msgs[0]) ? 1 : 0;

			zassert_equal(msg[msg_idx].len, msgs[0].len);
			zassert_mem_equal(msg[msg_idx].buf, msgs[0].buf, msg[msg_idx].len,
					  "Expected [0x%02x, 0x%02x] but got [0x%02x, 0x%02x]",
					  msg[msg_idx].buf[0], msg[msg_idx].buf[1], msgs[0].buf[0],
					  msgs[0].buf[1]);
			zassert_equal(msg[msg_idx].flags, msgs[0].flags);
			return 0;
		};

	struct rtio_sqe *sqe_write =
		i2c_rtio_copy(&test_rtio_ctx, &blocking_emul_iodev, &msg[0], 1);
	struct rtio_sqe *sqe_read = i2c_rtio_copy(&test_rtio_ctx, &blocking_emul_iodev, &msg[1], 1);
	struct rtio_sqe *sqe_dropped =
		i2c_rtio_copy(&test_rtio_ctx, &blocking_emul_iodev, &msg[2], 1);

	zassert_not_null(sqe_write);
	zassert_not_null(sqe_read);
	zassert_not_null(sqe_dropped);

	/* Add userdata so we can match these up with the CQEs */
	sqe_write->userdata = &msg[0];
	sqe_read->userdata = &msg[1];
	sqe_dropped->userdata = &msg[2];

	zassert_ok(rtio_submit(&test_rtio_ctx, 3));
	zassert_equal(2, blocking_emul_i2c_transfer_fake.call_count);

	struct rtio_cqe *cqe[] = {
		rtio_cqe_consume_block(&test_rtio_ctx),
		rtio_cqe_consume_block(&test_rtio_ctx),
		rtio_cqe_consume_block(&test_rtio_ctx),
	};

	/*
	 * We need to make sure that we got back results for all 3 messages and that there are no
	 * duplicates
	 */
	uint8_t msg_seen_mask = 0;
	for (unsigned int i = 0; i < ARRAY_SIZE(cqe); ++i) {
		int msg_idx = (struct i2c_msg *)cqe[i]->userdata - msg;

		zassert_true(msg_idx >= 0 && msg_idx < 3);
		msg_seen_mask |= BIT(msg_idx);
		if (msg_idx == 0 || msg_idx == 1) {
			/* Expect the first 2 to succeed */
			zassert_ok(cqe[i]->result);
		} else {
			zassert_equal(-ENOMEM, cqe[i]->result);
		}
	}

	/* Make sure bits 0, 1, and 2 were set. */
	zassert_equal(0x7, msg_seen_mask);

	rtio_cqe_release(&test_rtio_ctx, cqe[0]);
	rtio_cqe_release(&test_rtio_ctx, cqe[1]);
	rtio_cqe_release(&test_rtio_ctx, cqe[2]);
}
