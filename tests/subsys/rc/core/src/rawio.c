/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/rc.h>
#include <zephyr/drivers/rc/rc_loopback.h>
#include <zephyr/kernel.h>
#include <zephyr/rc/rc.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/ztest.h>

#define CHUNK   CONFIG_RC_RAW_CHUNK_WORDS
#define IDLE_MS 20

static const struct device *const dev9 = DEVICE_DT_GET(DT_NODELABEL(rc_loopback9));

RC_DT_RAW_IODEV(raw_iodev, DT_NODELABEL(rc_loopback9));
/* One reader per raw iodev: tests that leave loss state use their own */
RC_DT_RAW_IODEV(raw_iodev_oneshot, DT_NODELABEL(rc_loopback9));
RC_DT_RAW_IODEV(raw_iodev_stopped, DT_NODELABEL(rc_loopback9));
RC_DT_RAW_IODEV(raw_iodev_short, DT_NODELABEL(rc_loopback9));

RTIO_DEFINE_WITH_MEMPOOL(raw_ctx, 4, 8, 8, 64, sizeof(uint32_t));
RTIO_DEFINE_WITH_MEMPOOL(raw_ctx_one, 4, 8, 1, 64, sizeof(uint32_t));
RTIO_DEFINE_WITH_MEMPOOL(raw_ctx_small, 4, 8, 2, 4, sizeof(uint32_t));
RTIO_DEFINE(raw_ctx_plain, 4, 8);

static const uint32_t frame[] = {
	RC_RAW_PULSE_US(9000), RC_RAW_SPACE_US(4500), RC_RAW_PULSE_US(560), RC_RAW_SPACE_US(560),
	RC_RAW_PULSE_US(560),  RC_RAW_SPACE_US(1690), RC_RAW_PULSE_US(560), RC_RAW_SPACE_US(560),
	RC_RAW_PULSE_US(560),  RC_RAW_SPACE_US(1690), RC_RAW_PULSE_US(560), RC_RAW_SPACE_US(560),
};

static void inject(const uint32_t *words, size_t count)
{
	k_sched_lock();
	zassert_ok(rc_loopback_inject(dev9, words, count));
	k_sched_unlock();
}

static struct rtio_cqe *wait_cqe(struct rtio *r)
{
	struct rtio_cqe *cqe;

	for (int i = 0; i < 200; i++) {
		cqe = rtio_cqe_consume(r);
		if (cqe != NULL) {
			return cqe;
		}
		k_sleep(K_MSEC(1));
	}

	zassert_unreachable("no completion");

	return NULL;
}

static int next_words(struct rtio *r, uint32_t *words, size_t max)
{
	struct rtio_cqe *cqe = wait_cqe(r);
	int result = cqe->result;
	uint32_t len = 0U;
	uint8_t *buf = NULL;

	if ((result > 0) && (rtio_cqe_get_mempool_buffer(r, cqe, &buf, &len) == 0) &&
	    (buf != NULL)) {
		zassert_true((size_t)result <= max);
		memcpy(words, buf, (size_t)result * sizeof(uint32_t));
		rtio_release_buffer(r, buf, len);
	}
	rtio_cqe_release(r, cqe);

	return result;
}

static void expect_no_cqe(struct rtio *r)
{
	k_sleep(K_MSEC(IDLE_MS));
	zassert_is_null(rtio_cqe_consume(r), "unexpected completion");
}

static void stop(struct rtio *r, rtio_sqe_handle_t handle)
{
	uint32_t words[CHUNK];

	zassert_ok(rtio_sqe_cancel(r, handle));
	while (next_words(r, words, ARRAY_SIZE(words)) != -ECANCELED) {
	}
}

static void rc_rawio_before(void *fixture)
{
	ARG_UNUSED(fixture);

	k_sleep(K_MSEC(IDLE_MS));
	zassert_ok(rc_rx_enable(dev9, true));
}

static void rc_rawio_after(void *fixture)
{
	struct rtio *const ctxs[] = {&raw_ctx, &raw_ctx_one, &raw_ctx_small, &raw_ctx_plain};

	ARG_UNUSED(fixture);

	zassert_ok(rc_rx_enable(dev9, false));
	/* RTIO frees a submission just after its completion */
	k_msleep(5);
	ARRAY_FOR_EACH(ctxs, i) {
		zassert_is_null(rtio_cqe_consume(ctxs[i]), "completion left in ctx %zu", i);
		zassert_equal(ctxs[i]->sqe_pool->pool_free, ctxs[i]->sqe_pool->pool_size,
			      "submission leaked in ctx %zu", i);
		if (ctxs[i]->block_pool != NULL) {
			zassert_equal(ctxs[i]->block_pool->info.used_blocks, 0U,
				      "pool buffer leaked in ctx %zu", i);
		}
	}
}

ZTEST(rc_rawio, test_words_then_timeout)
{
	rtio_sqe_handle_t handle = 0;
	uint32_t words[CHUNK];

	zassert_ok(rc_stream(&raw_iodev, &raw_ctx, NULL, &handle));
	inject(frame, 4);

	zassert_equal(next_words(&raw_ctx, words, ARRAY_SIZE(words)), 4);
	zassert_mem_equal(words, frame, 4 * sizeof(uint32_t));

	zassert_equal(next_words(&raw_ctx, words, ARRAY_SIZE(words)), 1);
	zassert_true((words[0] & RC_RAW_TIMEOUT) != 0U);
	zassert_false(rc_raw_is_pulse(words[0]));

	stop(&raw_ctx, handle);
}

ZTEST(rc_rawio, test_chunks)
{
	rtio_sqe_handle_t handle = 0;
	uint32_t words[CHUNK];

	zassert_ok(rc_stream(&raw_iodev, &raw_ctx, NULL, &handle));
	inject(frame, ARRAY_SIZE(frame));

	zassert_equal(next_words(&raw_ctx, words, ARRAY_SIZE(words)), CHUNK);
	zassert_mem_equal(words, frame, CHUNK * sizeof(uint32_t));
	zassert_equal(next_words(&raw_ctx, words, ARRAY_SIZE(words)), ARRAY_SIZE(frame) - CHUNK);
	zassert_mem_equal(words, &frame[CHUNK], (ARRAY_SIZE(frame) - CHUNK) * sizeof(uint32_t));

	stop(&raw_ctx, handle);
}

ZTEST(rc_rawio, test_app_buffer_too_small)
{
	uint32_t buf[3];
	struct rtio_sqe *sqe = rtio_sqe_acquire(&raw_ctx_plain);
	struct rtio_cqe *cqe;

	zassert_not_null(sqe);
	rtio_sqe_prep_read(sqe, &raw_iodev_short, RTIO_PRIO_NORM, (uint8_t *)buf, sizeof(buf),
			   NULL);
	zassert_ok(rtio_submit(&raw_ctx_plain, 0));
	inject(frame, 5);

	cqe = wait_cqe(&raw_ctx_plain);
	zassert_equal(cqe->result, 3);
	rtio_cqe_release(&raw_ctx_plain, cqe);
	zassert_equal(buf[0], frame[0]);
	zassert_equal(buf[1], frame[1]);
	zassert_equal(buf[2], RC_RAW_OVERFLOW, "cut short without marker");
}

ZTEST(rc_rawio, test_pool_buffer_too_small)
{
	size_t cap = rtio_mempool_block_size(&raw_ctx_small) *
		     raw_ctx_small.block_pool->info.num_blocks / sizeof(uint32_t);
	rtio_sqe_handle_t handle = 0;
	uint32_t words[CHUNK];
	int result;

	zassert_true(cap < CHUNK);
	zassert_ok(rc_stream(&raw_iodev, &raw_ctx_small, NULL, &handle));
	inject(frame, CHUNK);

	result = next_words(&raw_ctx_small, words, ARRAY_SIZE(words));
	zassert_equal(result, (int)cap, "result %d", result);
	zassert_mem_equal(words, frame, (cap - 1U) * sizeof(uint32_t));
	zassert_equal(words[cap - 1U], RC_RAW_OVERFLOW, "cut short without marker");

	stop(&raw_ctx_small, handle);
}

ZTEST(rc_rawio, test_pool_empty)
{
	rtio_sqe_handle_t handle = 0;
	struct rtio_cqe *held;
	uint32_t words[CHUNK];
	uint32_t len = 0U;
	uint8_t *buf = NULL;

	zassert_ok(rc_stream(&raw_iodev_stopped, &raw_ctx_one, NULL, &handle));
	inject(frame, 2);
	held = wait_cqe(&raw_ctx_one);
	zassert_equal(held->result, 2);

	inject(frame, 2);
	zassert_equal(next_words(&raw_ctx_one, words, ARRAY_SIZE(words)), -ENOMEM);

	zassert_ok(rtio_cqe_get_mempool_buffer(&raw_ctx_one, held, &buf, &len));
	rtio_release_buffer(&raw_ctx_one, buf, len);
	rtio_cqe_release(&raw_ctx_one, held);

	inject(frame, 2);
	expect_no_cqe(&raw_ctx_one);

	zassert_ok(rtio_sqe_cancel(&raw_ctx_one, handle));
}

ZTEST(rc_rawio, test_missed_between_reads)
{
	uint32_t buf[CHUNK];
	struct rtio_sqe *sqe;
	struct rtio_cqe *cqe;

	for (int round = 0; round < 2; round++) {
		sqe = rtio_sqe_acquire(&raw_ctx_plain);
		zassert_not_null(sqe);
		rtio_sqe_prep_read(sqe, &raw_iodev_oneshot, RTIO_PRIO_NORM, (uint8_t *)buf,
				   sizeof(buf), NULL);
		zassert_ok(rtio_submit(&raw_ctx_plain, 0));
		inject(&frame[2], 1);

		cqe = wait_cqe(&raw_ctx_plain);
		if (round == 0) {
			zassert_equal(cqe->result, 1);
			zassert_equal(buf[0], frame[2]);
		} else {
			zassert_equal(cqe->result, 2);
			zassert_equal(buf[0], RC_RAW_OVERFLOW);
			zassert_equal(buf[1], frame[2]);
		}
		rtio_cqe_release(&raw_ctx_plain, cqe);

		inject(frame, 3);
		k_sleep(K_MSEC(IDLE_MS));
	}

	sqe = rtio_sqe_acquire(&raw_ctx_plain);
	zassert_not_null(sqe);
	rtio_sqe_prep_read(sqe, &raw_iodev_oneshot, RTIO_PRIO_NORM, (uint8_t *)buf, sizeof(buf),
			   NULL);
	zassert_ok(rtio_sqe_cancel(&raw_ctx_plain, rtio_sqe_handle(&raw_ctx_plain, sqe)));
	zassert_ok(rtio_submit(&raw_ctx_plain, 0));
	cqe = wait_cqe(&raw_ctx_plain);
	zassert_equal(cqe->result, -ECANCELED);
	rtio_cqe_release(&raw_ctx_plain, cqe);
	expect_no_cqe(&raw_ctx_plain);

	sqe = rtio_sqe_acquire(&raw_ctx_plain);
	zassert_not_null(sqe);
	rtio_sqe_prep_read(sqe, &raw_iodev_oneshot, RTIO_PRIO_NORM, (uint8_t *)buf, sizeof(buf),
			   NULL);
	zassert_ok(rtio_submit(&raw_ctx_plain, 0));
	inject(&frame[2], 1);
	cqe = wait_cqe(&raw_ctx_plain);
	zassert_equal(cqe->result, 1);
	zassert_equal(buf[0], frame[2]);
	rtio_cqe_release(&raw_ctx_plain, cqe);
}

ZTEST(rc_rawio, test_cancel_oneshot)
{
	uint32_t buf[CHUNK];
	struct rtio_sqe *sqe = rtio_sqe_acquire(&raw_ctx_plain);
	rtio_sqe_handle_t handle;
	struct rtio_cqe *cqe;

	zassert_not_null(sqe);
	rtio_sqe_prep_read(sqe, &raw_iodev_oneshot, RTIO_PRIO_NORM, (uint8_t *)buf, sizeof(buf),
			   NULL);
	handle = rtio_sqe_handle(&raw_ctx_plain, sqe);
	zassert_ok(rtio_submit(&raw_ctx_plain, 0));
	k_sleep(K_MSEC(IDLE_MS));

	zassert_ok(rtio_sqe_cancel(&raw_ctx_plain, handle));
	cqe = wait_cqe(&raw_ctx_plain);
	zassert_equal(cqe->result, -ECANCELED);
	rtio_cqe_release(&raw_ctx_plain, cqe);
	inject(frame, 2);
	expect_no_cqe(&raw_ctx_plain);
}

ZTEST(rc_rawio, test_cancel_without_words)
{
	rtio_sqe_handle_t handle = 0;

	zassert_ok(rc_stream(&raw_iodev, &raw_ctx, NULL, &handle));
	k_sleep(K_MSEC(IDLE_MS));
	stop(&raw_ctx, handle);
	zassert_equal(raw_ctx.sqe_pool->pool_free, raw_ctx.sqe_pool->pool_size);
}

ZTEST_SUITE(rc_rawio, NULL, NULL, rc_rawio_before, rc_rawio_after, NULL);
