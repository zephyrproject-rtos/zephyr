/*
 * Copyright (c) 2019 Thomas Burdick <thomas.burdick@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Test zio fifo
 *
 */


#include <tc_util.h>
#include <stdbool.h>
#include <zephyr.h>
#include <ztest.h>
#include <zio/zio_fifo.h>

struct random {
	u32_t something;
	u32_t something2;
	u8_t something3;
};

/* test static define, would be a compile error if it didn't work correctly */
static ZIO_FIFO_DEFINE(mybufforever, struct random, 8);

static void test_zio_fifo_define(void)
{
	ZIO_FIFO_DEFINE(mybuf, u16_t, 8);
	zassert_equal(mybuf.zfifo.in, 0, "Unexpected in index");
	zassert_equal(mybuf.zfifo.out, 0, "Unexpected out index");
	zassert_equal(zio_fifo_size(&mybuf), 256, "Unexpected size");
	zassert_equal(zio_fifo_used(&mybuf), 0, "Unexpected used");
	zassert_equal(sizeof(mybuf.buffer), 512, "Unexpected sizeof buf");

	u16_t myval = 0;

	zassert_equal(zio_fifo_pull(&mybuf, myval), 0,
		      "Unexpected pull result");
	zassert_equal(zio_fifo_peek(&mybuf, myval), 0,
		      "Unexpected peek result");

	/* test second define, would be a compile time error if it didn't work
	 */
	ZIO_FIFO_DEFINE(mybuf2, u16_t, 8);
}

static void test_zio_fifo_clear(void)
{
	ZIO_FIFO_DEFINE(mybuf, u16_t, 8);
	mybuf.zfifo.in = 10;
	mybuf.zfifo.out = 5;
	zio_fifo_clear(&mybuf);
	zassert_equal(mybuf.zfifo.in, 0, "Unexpected in idx");
	zassert_equal(mybuf.zfifo.out, 0, "Unexpected out idx");
}

static void test_zio_fifo_push(void)
{
	ZIO_FIFO_DEFINE(mybuf, u16_t, 8);
	zio_fifo_push(&mybuf, 5);
	zassert_equal(zio_fifo_size(&mybuf), 256, "Unexpected size");
	zassert_equal(zio_fifo_used(&mybuf), 1, "Unexpected used");
}

static void test_zio_fifo_pull(void)
{
	ZIO_FIFO_DEFINE(mybuf, u16_t, 8);
	zio_fifo_push(&mybuf, 5);
	zassert_equal(zio_fifo_size(&mybuf), 256, "Unexpected size");
	zassert_equal(zio_fifo_used(&mybuf), 1, "Unexpected used");

	u16_t myval = 0;

	zassert_equal(zio_fifo_pull(&mybuf, myval), 1,
		      "Unexpected pull result");
	zassert_equal(myval, 5, "Unexpected value");
	zassert_equal(zio_fifo_used(&mybuf), 0, "Unexpected used");
}

static void test_zio_fifo_peek(void)
{
	ZIO_FIFO_DEFINE(mybuf, u16_t, 8);
	zio_fifo_push(&mybuf, 5);
	zassert_equal(zio_fifo_size(&mybuf), 256, "Unexpected size");
	zassert_equal(zio_fifo_used(&mybuf), 1, "Unexpected used");

	u16_t myval = 0;

	zio_fifo_peek(&mybuf, myval);
	zassert_equal(myval, 5, "Unexpected value");
	zassert_equal(zio_fifo_used(&mybuf), 1, "Unexpected used");
}

static void test_zio_fifo_wrap(void)
{
	ZIO_FIFO_DEFINE(mybuf, u16_t, 2);
	zassert_equal(zio_fifo_avail(&mybuf), 4, "Unexpected avail");
	zassert_equal(zio_fifo_used(&mybuf), 0, "Unexpected used");

	u16_t myval = 0;

	for (int i = 0; i < 4; i++) {
		zassert_equal(zio_fifo_push(&mybuf, i), 1,
			      "Unexpected push result");
		zassert_equal(zio_fifo_used(&mybuf), i + 1,
			      "Unexpected used");
		zassert_equal(zio_fifo_avail(&mybuf), 4 - (i + 1),
			      "Unexpected avail");
	}
	zassert_equal(zio_fifo_push(&mybuf, 4), 0, "Unexpected push result");
	zassert_equal(zio_fifo_used(&mybuf), 4, "Unexpected used");
	zassert_equal(zio_fifo_avail(&mybuf), 0, "Unexpected avail");
	for (int i = 0; i < 4; i++) {
		zassert_equal(zio_fifo_peek(&mybuf, myval), 1,
			      "Unexpected peek result");
		zassert_equal(myval, i, "Unexpected peek value");
		zassert_equal(zio_fifo_pull(&mybuf, myval), 1,
			      "Unexpected pull result");
		zassert_equal(myval, i, "Unexpected pull value");
		zassert_equal(zio_fifo_used(&mybuf), 4 - (i + 1),
			      "Unexpected used");
		zassert_equal(zio_fifo_avail(&mybuf), i + 1,
			      "Unexpected avail");
	}
	zassert_equal(zio_fifo_used(&mybuf), 0, "Unexpected used");
	zassert_equal(zio_fifo_avail(&mybuf), 4, "Unexpected avail");
}

static void test_zio_fifo_idx_wrap(void)
{
	ZIO_FIFO_DEFINE(mybuf, u16_t, 2);

	u32_t max = UINT32_MAX;

	mybuf.zfifo.in = max - 2;
	mybuf.zfifo.out = max - 2;

	u16_t myval = 0;

	for (int i = 0; i < 4; i++) {
		zassert_equal(zio_fifo_push(&mybuf, i), 1,
			      "Unexpected push result");
		zassert_equal(zio_fifo_used(&mybuf), i + 1,
			      "Unexpected used");
		zassert_equal(zio_fifo_avail(&mybuf), 4 - (i + 1),
			      "Unexpected avail");
	}
	zassert_equal(zio_fifo_push(&mybuf, 4), 0, "Unexpected push result");
	zassert_equal(zio_fifo_used(&mybuf), 4, "Unexpected used");
	zassert_equal(zio_fifo_avail(&mybuf), 0, "Unexpected avail");
	for (int i = 0; i < 4; i++) {
		zassert_equal(zio_fifo_peek(&mybuf, myval), 1,
			      "Unexpected peek result");
		zassert_equal(myval, i, "Unexpected peek value");
		zassert_equal(zio_fifo_pull(&mybuf, myval), 1,
			      "Unexpected pull result");
		zassert_equal(myval, i, "Unexpected pull value");
		zassert_equal(zio_fifo_used(&mybuf), 4 - (i + 1),
			      "Unexpected used");
		zassert_equal(zio_fifo_avail(&mybuf), i + 1,
			      "Unexpected avail");
	}
	zassert_equal(zio_fifo_used(&mybuf), 0, "Unexpected used");
	zassert_equal(zio_fifo_avail(&mybuf), 4, "Unexpected avail");
}



/*test case main entry*/
void test_main(void)
{
	ztest_test_suite(test_zio_fifo_list,
			 ztest_unit_test(test_zio_fifo_define),
			 ztest_unit_test(test_zio_fifo_clear),
			 ztest_unit_test(test_zio_fifo_push),
			 ztest_unit_test(test_zio_fifo_pull),
			 ztest_unit_test(test_zio_fifo_peek),
			 ztest_unit_test(test_zio_fifo_wrap),
			 ztest_unit_test(test_zio_fifo_idx_wrap)
			 );
	ztest_run_test_suite(test_zio_fifo_list);
}
