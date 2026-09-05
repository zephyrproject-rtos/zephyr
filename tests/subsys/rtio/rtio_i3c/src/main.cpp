/* Copyright (c) 2026 Meta Platforms
 * SPDX-License-Identifier: Apache-2.0
 *
 * Exercises i3c_iodev_submit_fallback — the workqueue-based subsys helper
 * that controllers without a native RTIO submit fall back to. Modeled on
 * tests/subsys/rtio/rtio_i2c so the two paths can be compared side-by-side.
 *
 * The controller is zephyr,i3c-emul (which binds .iodev_submit to
 * i3c_iodev_submit_fallback). The peripheral on the bus is a custom
 * blocking_emul whose xfers callback is FFF-wrapped so each test can install
 * a capturing lambda and assert on the driver-supplied msgs.
 */

#include "blocking_emul.hpp"

#include <zephyr/drivers/i3c.h>
#include <zephyr/drivers/i3c/rtio.h>
#include <zephyr/fff.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/ztest.h>

DEFINE_FFF_GLOBALS;

static const struct device *i3c_dev = DEVICE_DT_GET(DT_NODELABEL(i3c0));
I3C_DT_IODEV_DEFINE(blocking_emul_iodev, DT_NODELABEL(blocking_emul));

RTIO_DEFINE(test_rtio_ctx, 4, 4);

static void rtio_i3c_before(void *fixture)
{
	ARG_UNUSED(fixture);
	RESET_FAKE(blocking_emul_i3c_xfers);
	RESET_FAKE(blocking_emul_i3c_do_ccc);
	FFF_RESET_HISTORY();

	rtio_sqe_drop_all(&test_rtio_ctx);

	struct rtio_cqe *cqe;

	while ((cqe = rtio_cqe_consume(&test_rtio_ctx)) != NULL) {
		rtio_cqe_release(&test_rtio_ctx, cqe);
	}
}

ZTEST_SUITE(rtio_i3c, NULL, NULL, rtio_i3c_before, NULL, NULL);

ZTEST(rtio_i3c, test_emulated_api_uses_fallback_submit)
{
	zassert_not_null(i3c_dev->api);
	zassert_equal_ptr(i3c_iodev_submit_fallback,
			  ((const struct i3c_driver_api *)i3c_dev->api)->iodev_submit);
}

ZTEST(rtio_i3c, test_fallback_submit_tx)
{
	uint8_t data[] = {0x01, 0x02, 0x03};
	struct i3c_msg msg = {
		.buf = data,
		.len = ARRAY_SIZE(data),
		.flags = I3C_MSG_WRITE | I3C_MSG_STOP,
	};

	blocking_emul_i3c_xfers_fake.custom_fake =
		[&msg](const struct emul *, struct i3c_msg *msgs, uint8_t num_msgs) {
			zassert_equal(1, num_msgs);
			zassert_equal(msg.len, msgs[0].len);
			zassert_mem_equal(msg.buf, msgs[0].buf, msg.len);
			zassert_equal(msg.flags, msgs[0].flags);
			return 0;
		};

	struct rtio_sqe *sqe = i3c_rtio_copy(&test_rtio_ctx, &blocking_emul_iodev, &msg, 1);

	zassert_not_null(sqe);
	zassert_ok(rtio_submit(&test_rtio_ctx, 1));
	zassert_equal(1, blocking_emul_i3c_xfers_fake.call_count);

	struct rtio_cqe *cqe = rtio_cqe_consume_block(&test_rtio_ctx);

	zassert_ok(cqe->result);
	rtio_cqe_release(&test_rtio_ctx, cqe);
}

ZTEST(rtio_i3c, test_fallback_submit_invalid_op)
{
	struct rtio_sqe *sqe = rtio_sqe_acquire(&test_rtio_ctx);

	zassert_not_null(sqe);
	sqe->op = UINT8_MAX;
	sqe->prio = RTIO_PRIO_NORM;
	sqe->iodev = &blocking_emul_iodev;
	sqe->userdata = NULL;

	zassert_ok(rtio_submit(&test_rtio_ctx, 1));
	zassert_equal(0, blocking_emul_i3c_xfers_fake.call_count);

	struct rtio_cqe *cqe = rtio_cqe_consume_block(&test_rtio_ctx);

	zassert_equal(-EIO, cqe->result);
	rtio_cqe_release(&test_rtio_ctx, cqe);
}

ZTEST(rtio_i3c, test_fallback_submit_tiny_tx)
{
	uint8_t data[] = {0x01, 0x02, 0x03};
	struct rtio_sqe *sqe = rtio_sqe_acquire(&test_rtio_ctx);

	blocking_emul_i3c_xfers_fake.custom_fake =
		[&data](const struct emul *, struct i3c_msg *msgs, uint8_t num_msgs) {
			zassert_equal(1, num_msgs);
			zassert_equal(ARRAY_SIZE(data), msgs[0].len);
			zassert_mem_equal(data, msgs[0].buf, msgs[0].len);
			zassert_equal(I3C_MSG_WRITE | I3C_MSG_STOP, msgs[0].flags);
			return 0;
		};

	zassert_not_null(sqe);

	rtio_sqe_prep_tiny_write(sqe, &blocking_emul_iodev, RTIO_PRIO_NORM, data, ARRAY_SIZE(data),
				 NULL);
	sqe->iodev_flags = RTIO_IODEV_I3C_STOP;
	zassert_ok(rtio_submit(&test_rtio_ctx, 1));
	zassert_equal(1, blocking_emul_i3c_xfers_fake.call_count);

	struct rtio_cqe *cqe = rtio_cqe_consume_block(&test_rtio_ctx);

	zassert_ok(cqe->result);
	rtio_cqe_release(&test_rtio_ctx, cqe);
}

ZTEST(rtio_i3c, test_fallback_submit_rx)
{
	uint8_t expected_buffer[] = {0x00, 0x01, 0x02};
	uint8_t buffer[ARRAY_SIZE(expected_buffer)] = {0};
	struct i3c_msg msg = {
		.buf = buffer,
		.len = ARRAY_SIZE(buffer),
		.flags = I3C_MSG_READ | I3C_MSG_STOP,
	};

	blocking_emul_i3c_xfers_fake.custom_fake =
		[&msg](const struct emul *, struct i3c_msg *msgs, uint8_t num_msgs) {
			zassert_equal(1, num_msgs);
			zassert_equal(msg.len, msgs[0].len);
			zassert_equal(msg.flags, msgs[0].flags);
			for (uint8_t i = 0; i < msg.len; ++i) {
				msgs[0].buf[i] = i;
			}
			return 0;
		};

	struct rtio_sqe *sqe = i3c_rtio_copy(&test_rtio_ctx, &blocking_emul_iodev, &msg, 1);

	zassert_not_null(sqe);
	zassert_ok(rtio_submit(&test_rtio_ctx, 1));
	zassert_equal(1, blocking_emul_i3c_xfers_fake.call_count);

	struct rtio_cqe *cqe = rtio_cqe_consume_block(&test_rtio_ctx);

	zassert_ok(cqe->result);
	zassert_mem_equal(buffer, expected_buffer, ARRAY_SIZE(expected_buffer));

	rtio_cqe_release(&test_rtio_ctx, cqe);
}

ZTEST(rtio_i3c, test_fallback_transaction_error)
{
	uint8_t buffer[3];
	struct rtio_sqe *phase1 = rtio_sqe_acquire(&test_rtio_ctx);
	struct rtio_sqe *phase2 = rtio_sqe_acquire(&test_rtio_ctx);

	blocking_emul_i3c_xfers_fake.return_val = -EIO;

	zassert_not_null(phase1);
	zassert_not_null(phase2);

	rtio_sqe_prep_read(phase1, &blocking_emul_iodev, RTIO_PRIO_NORM, buffer, ARRAY_SIZE(buffer),
			   NULL);
	rtio_sqe_prep_read(phase2, &blocking_emul_iodev, RTIO_PRIO_NORM, buffer, ARRAY_SIZE(buffer),
			   NULL);

	phase1->flags |= RTIO_SQE_TRANSACTION;

	zassert_ok(rtio_submit(&test_rtio_ctx, 2));
	zassert_equal(1, blocking_emul_i3c_xfers_fake.call_count);

	struct rtio_cqe *cqe = rtio_cqe_consume_block(&test_rtio_ctx);

	zassert_equal(-EIO, cqe->result);

	rtio_cqe_release(&test_rtio_ctx, cqe);

	/* The transaction yields a second CQE that must be drained. */
	cqe = rtio_cqe_consume_block(&test_rtio_ctx);
	rtio_cqe_release(&test_rtio_ctx, cqe);
}

ZTEST(rtio_i3c, test_fallback_transaction)
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
	zassert_equal(1, blocking_emul_i3c_xfers_fake.call_count);

	struct rtio_cqe *cqe;

	cqe = rtio_cqe_consume_block(&test_rtio_ctx);
	zassert_ok(cqe->result);
	rtio_cqe_release(&test_rtio_ctx, cqe);

	cqe = rtio_cqe_consume_block(&test_rtio_ctx);
	zassert_ok(cqe->result);
	rtio_cqe_release(&test_rtio_ctx, cqe);
}

ZTEST(rtio_i3c, test_fallback_submit_hdr_ddr)
{
	uint8_t data[] = {0xAA, 0x55, 0xCC, 0x33};
	struct rtio_sqe *sqe = rtio_sqe_acquire(&test_rtio_ctx);

	blocking_emul_i3c_xfers_fake.custom_fake =
		[&data](const struct emul *, struct i3c_msg *msgs, uint8_t num_msgs) {
			zassert_equal(1, num_msgs);
			zassert_equal(ARRAY_SIZE(data), msgs[0].len);
			zassert_mem_equal(data, msgs[0].buf, msgs[0].len);
			zassert_true((msgs[0].flags & I3C_MSG_HDR) != 0,
				     "I3C_MSG_HDR not delivered: flags=0x%x", msgs[0].flags);
			/* I3C_MSG_WRITE is the absence of I3C_MSG_READ. */
			zassert_true((msgs[0].flags & I3C_MSG_READ) == 0,
				     "Unexpected I3C_MSG_READ on write: flags=0x%x", msgs[0].flags);
			return 0;
		};

	zassert_not_null(sqe);
	rtio_sqe_prep_write(sqe, &blocking_emul_iodev, RTIO_PRIO_NORM, data, ARRAY_SIZE(data),
			    NULL);
	sqe->iodev_flags = RTIO_IODEV_I3C_STOP | RTIO_IODEV_I3C_HDR |
			   RTIO_IODEV_I3C_HDR_MODE_SET(I3C_MSG_HDR_DDR);

	zassert_ok(rtio_submit(&test_rtio_ctx, 1));
	zassert_equal(1, blocking_emul_i3c_xfers_fake.call_count);

	struct rtio_cqe *cqe = rtio_cqe_consume_block(&test_rtio_ctx);

	zassert_ok(cqe->result);
	rtio_cqe_release(&test_rtio_ctx, cqe);
}

ZTEST(rtio_i3c, test_work_queue_overflow)
{
	BUILD_ASSERT(CONFIG_RTIO_WORKQ_POOL_ITEMS == 2);

	uint8_t data[][2] = {
		{0x01, 0x02},
		{0x03, 0x04},
		{0x05, 0x06},
	};
	struct i3c_msg msg[] = {
		{
			.buf = data[0],
			.len = 2,
			.flags = I3C_MSG_WRITE | I3C_MSG_STOP,
		},
		{
			.buf = data[1],
			.len = 2,
			.flags = I3C_MSG_READ | I3C_MSG_STOP,
		},
		{
			.buf = data[2],
			.len = 2,
			.flags = I3C_MSG_READ | I3C_MSG_STOP,
		},
	};

	BUILD_ASSERT(ARRAY_SIZE(data) == ARRAY_SIZE(msg));

	blocking_emul_i3c_xfers_fake.custom_fake =
		[&msg](const struct emul *, struct i3c_msg *msgs, uint8_t num_msgs) {
			zassert_equal(1, num_msgs);

			/* Both reads share the same len and flags, so READ vs WRITE
			 * can't tell msg[1] from msg[2]. Which submission survives the
			 * pool overflow is nondeterministic, so identify the message by
			 * its buffer pointer, which i3c_rtio_copy preserves.
			 */
			int msg_idx = -1;

			for (unsigned int i = 0; i < ARRAY_SIZE(msg); ++i) {
				if (msgs[0].buf == msg[i].buf) {
					msg_idx = i;
					break;
				}
			}

			zassert_true(msg_idx >= 0, "xfer buffer matched no message");

			zassert_equal(msg[msg_idx].len, msgs[0].len);
			zassert_mem_equal(msg[msg_idx].buf, msgs[0].buf, msg[msg_idx].len,
					  "Expected [0x%02x, 0x%02x] but got [0x%02x, 0x%02x]",
					  msg[msg_idx].buf[0], msg[msg_idx].buf[1], msgs[0].buf[0],
					  msgs[0].buf[1]);
			zassert_equal(msg[msg_idx].flags, msgs[0].flags);
			return 0;
		};

	struct rtio_sqe *sqe_write =
		i3c_rtio_copy(&test_rtio_ctx, &blocking_emul_iodev, &msg[0], 1);
	struct rtio_sqe *sqe_read = i3c_rtio_copy(&test_rtio_ctx, &blocking_emul_iodev, &msg[1], 1);
	struct rtio_sqe *sqe_dropped =
		i3c_rtio_copy(&test_rtio_ctx, &blocking_emul_iodev, &msg[2], 1);

	zassert_not_null(sqe_write);
	zassert_not_null(sqe_read);
	zassert_not_null(sqe_dropped);

	sqe_write->userdata = &msg[0];
	sqe_read->userdata = &msg[1];
	sqe_dropped->userdata = &msg[2];

	zassert_ok(rtio_submit(&test_rtio_ctx, 3));

	struct rtio_cqe *cqe[] = {
		rtio_cqe_consume_block(&test_rtio_ctx),
		rtio_cqe_consume_block(&test_rtio_ctx),
		rtio_cqe_consume_block(&test_rtio_ctx),
	};

	uint8_t msg_seen_mask = 0;
	uint8_t enomem_count = 0;
	uint8_t ok_count = 0;

	for (unsigned int i = 0; i < ARRAY_SIZE(cqe); ++i) {
		int msg_idx = (struct i3c_msg *)cqe[i]->userdata - msg;

		zassert_true(msg_idx >= 0 && msg_idx < 3);
		msg_seen_mask |= BIT(msg_idx);
		if (cqe[i]->result == 0) {
			ok_count++;
		} else if (cqe[i]->result == -ENOMEM) {
			enomem_count++;
		} else {
			zassert_unreachable("unexpected cqe result %d for msg %d", cqe[i]->result,
					    msg_idx);
		}
	}

	zassert_equal(0x7, msg_seen_mask, "expected to see all 3 messages, mask=0x%x",
		      msg_seen_mask);
	/* At least one OK and at least one ENOMEM — the pool overflowed but the
	 * surviving allocations all completed via the xfers fake.
	 */
	zassert_true(ok_count >= 1, "no successful CQEs");
	zassert_true(enomem_count >= 1, "no -ENOMEM CQE — pool didn't overflow");
	zassert_equal(ok_count, blocking_emul_i3c_xfers_fake.call_count,
		      "fake called %u times but %u CQEs succeeded",
		      blocking_emul_i3c_xfers_fake.call_count, ok_count);

	rtio_cqe_release(&test_rtio_ctx, cqe[0]);
	rtio_cqe_release(&test_rtio_ctx, cqe[1]);
	rtio_cqe_release(&test_rtio_ctx, cqe[2]);
}
