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
}

ZTEST_SUITE(rtio_i2c, NULL, NULL, rtio_i2c_before, NULL, NULL);

ZTEST(rtio_i2c, test_emulated_api_does_not_implement_submit)
{
	zassert_not_null(i2c_dev->api);
	zassert_is_null(((const struct i2c_driver_api *)i2c_dev->api)->iodev_submit);
}

ZTEST(rtio_i2c, test_fallback_submit)
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

ZTEST(rtio_i2c, test_work_queue)
{
	BUILD_ASSERT(CONFIG_I2C_RTIO_MAX_PENDING_SYS_WORKQUEUE_REQUESTS == 2);

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
