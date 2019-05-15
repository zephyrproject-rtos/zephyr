/*
 * Copyright (c) 2019 Thomas Burdick <thomas.burdick@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Test zio fifo buf
 */


#include <tc_util.h>
#include <stdbool.h>
#include <zephyr.h>
#include <ztest.h>
#include <zio/zio_fifo_buf.h>
#include <zio/zio_buf.h>

struct random {
	u32_t something;
	u32_t something2;
	u8_t something3;
};

/* test static define, would be a compile error if it didn't work correctly */
static ZIO_FIFO_BUF_DEFINE(mybufforever, struct random, 8);

static void test_zio_fifo_buf_define(void)
{
	ZIO_FIFO_BUF_DEFINE(mybuf, u16_t, 8);

	/* test second define, would be a compile time error if it failed */
	ZIO_FIFO_BUF_DEFINE(mybuf2, u16_t, 8);
}

static void test_zio_fifo_buf_push(void)
{
	ZIO_FIFO_BUF_DEFINE(mybuf, u16_t, 8);
	zio_fifo_buf_push(&mybuf, 5);
	zassert_equal(zio_fifo_size(&mybuf.fifo), 256, "Unexpected size");
	zassert_equal(zio_fifo_used(&mybuf.fifo), 1, "Unexpected used");
}


static void test_zio_fifo_buf_attach(void)
{
	ZIO_FIFO_BUF_DEFINE(myfifo, u16_t, 8);
	ZIO_BUF_DEFINE(mybuf);
	zio_fifo_buf_attach(&myfifo, &mybuf);
	zassert_equal(myfifo.buf.zbuf, &mybuf, "Unexpected zbuf address");
	zassert_equal(mybuf.api, &zio_fifo_buf_api, "Unexpected api address");
	zassert_equal(mybuf.api_data, &myfifo, "Unexpected api data address");
}


static void test_zio_fifo_buf_detach(void)
{
	ZIO_FIFO_BUF_DEFINE(myfifo, u16_t, 8);
	ZIO_BUF_DEFINE(mybuf);
	zassert_equal(myfifo.buf.zbuf, NULL, "Unexpected zbuf address");
	zassert_equal(mybuf.api, NULL, "Unexpected api address");
	zassert_equal(mybuf.api_data, NULL, "Unexpected api data address");
	zio_fifo_buf_attach(&myfifo, &mybuf);
	zio_fifo_buf_detach(&myfifo);
	zassert_equal(myfifo.buf.zbuf, NULL, "Unexpected zbuf address");
	zassert_equal(mybuf.api, NULL, "Unexpected api address");
	zassert_equal(mybuf.api_data, NULL, "Unexpected api data address");
}

static void test_zio_fifo_buf_watermark(void)
{
	ZIO_FIFO_BUF_DEFINE(myfifo, u16_t, 8);
	ZIO_BUF_DEFINE(mybuf);
	zio_fifo_buf_attach(&myfifo, &mybuf);
	u32_t watermark = 1;
	int res = zio_buf_set_watermark(&mybuf, watermark);

	zassert_equal(res, 0, "Unexpected set watermark result");
	watermark = 0;
	res = zio_buf_get_watermark(&mybuf, &watermark);
	zassert_equal(res, 0, "Unexpected get watermark result");
	zassert_equal(watermark, 1, "Unexpected watermark");
}

static void test_zio_fifo_buf_poll_ready(void)
{
	ZIO_FIFO_BUF_DEFINE(myfifo, u16_t, 8);
	ZIO_BUF_DEFINE(mybuf);
	zio_fifo_buf_attach(&myfifo, &mybuf);
	int res = zio_buf_set_watermark(&mybuf, 1);

	zassert_equal(res, 0, "Unexpected get watermark result");
	struct k_poll_event events[1] = {
		K_POLL_EVENT_STATIC_INITIALIZER(K_POLL_TYPE_SEM_AVAILABLE,
				K_POLL_MODE_NOTIFY_ONLY, &mybuf.sem, 0),
	};
	res = k_poll(events, 1, 100);
	zassert_equal(res, -EAGAIN, "Unexpected k_poll result");
	u16_t sample = 1234;

	res = zio_fifo_buf_push(&myfifo, sample);
	zassert_equal(res, 1, "Unexpected push result");
	sample = 0;
	res = k_poll(events, 1, 100);
	zassert_equal(res, 0, "Unexpected k_poll result");
	res = zio_buf_pull(&mybuf, &sample);
	zassert_equal(res, 1, "Unexpected pull result");
	zassert_equal(sample, 1234, "Unexpected sample value");
	res = k_poll(events, 1, 100);
	zassert_equal(res, -EAGAIN, "Unexpected k_poll result");
}

/*test case main entry*/
void test_main(void)
{
	ztest_test_suite(test_zio_fifo_buf_list,
			ztest_unit_test(test_zio_fifo_buf_define),
			ztest_unit_test(test_zio_fifo_buf_push),
			ztest_unit_test(test_zio_fifo_buf_attach),
			ztest_unit_test(test_zio_fifo_buf_detach),
			ztest_unit_test(test_zio_fifo_buf_watermark),
			ztest_unit_test(test_zio_fifo_buf_poll_ready)
			);
	ztest_run_test_suite(test_zio_fifo_buf_list);
}
