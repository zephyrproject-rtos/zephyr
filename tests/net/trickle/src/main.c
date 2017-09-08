/* main.c - Application main entry point */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <ztest.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <misc/printk.h>
#include <linker/sections.h>

#include <tc_util.h>

#include <net/ethernet.h>
#include <net/buf.h>
#include <net/net_ip.h>
#include <net/net_if.h>

#include <net/trickle.h>

#include "net_private.h"

#if defined(CONFIG_NET_DEBUG_TRICKLE)
#define DBG(fmt, ...) printk(fmt, ##__VA_ARGS__)
#else
#define DBG(fmt, ...)
#endif

static int token1 = 1, token2 = 2;

static struct k_sem wait;
static bool cb_1_called;
static bool cb_2_called;

#define WAIT_TIME K_SECONDS(3)

/* Set CHECK_LONG_TIMEOUT to 1 if you want to check longer timeout.
 * Do not do this for automated tests as those need to finish asap.
 */
#define CHECK_LONG_TIMEOUT 0
#if CHECK_LONG_TIMEOUT > 0
#define WAIT_TIME_LONG (10 * MSEC_PER_SEC)
#endif

#define T1_IMIN 30
#define T1_IMAX 5
#define T1_K 20

#define T2_IMIN 80
#define T2_IMAX 3
#define T2_K 40

static struct net_trickle t1;
static struct net_trickle t2;

static void cb_1(struct net_trickle *trickle, bool do_suppress,
		 void *user_data)
{
	TC_PRINT("Trickle 1 %p callback called\n", trickle);

	k_sem_give(&wait);

	cb_1_called = true;
}

static void cb_2(struct net_trickle *trickle, bool do_suppress,
		 void *user_data)
{
	TC_PRINT("Trickle 2 %p callback called\n", trickle);

	k_sem_give(&wait);

	cb_2_called = true;
}

static void test_trickle_create(void)
{
	int ret;

	ret = net_trickle_create(&t1, T1_IMIN, T1_IMAX, T1_K);
	zassert_false(ret, "Trickle 1 create failed");

	ret = net_trickle_create(&t2, T2_IMIN, T2_IMAX, T2_K);
	zassert_false(ret, "Trickle 2 create failed");
}

static void test_trickle_start(void)
{
	int ret;

	ret = net_trickle_start(&t1, cb_1, &t1);
	zassert_false(ret, "Trickle 1 start failed");

	ret = net_trickle_start(&t2, cb_2, &t2);
	zassert_false(ret, "Trickle 2 start failed");
}

static void test_trickle_stop(void)
{
	zassert_false(net_trickle_stop(&t1),
			"Trickle 1 stop failed");

	zassert_false(net_trickle_stop(&t2),
			"Trickle 2 stop failed");
}

static void test_trickle_1_status(void)
{
	zassert_true(net_trickle_is_running(&t1), "Trickle 1 not running");

	if (token1 != token2) {
		net_trickle_inconsistency(&t1);
	} else {
		net_trickle_consistency(&t1);
	}
}

static void test_trickle_2_status(void)
{
	zassert_true(net_trickle_is_running(&t2), "Trickle 2 not running");

	if (token1 == token2) {
		net_trickle_consistency(&t2);
	} else {
		net_trickle_inconsistency(&t2);
	}
}

static void test_trickle_1_wait(void)
{
	cb_1_called = false;
	k_sem_take(&wait, WAIT_TIME);

	zassert_true(cb_1_called,
			"Trickle 1 no timeout");

	zassert_true(net_trickle_is_running(&t1), "Trickle 1 not running");
}

#if CHECK_LONG_TIMEOUT > 0
static bool test_trickle_1_wait_long(void)
{
	cb_1_called = false;
	k_sem_take(&wait, WAIT_TIME_LONG);

	if (!cb_1_called) {
		TC_ERROR("Trickle 1 no timeout\n");
		return false;
	}

	if (!net_trickle_is_running(&t1)) {
		TC_ERROR("Trickle 1 not running\n");
		return false;
	}

	return true;
}
#endif

static void test_trickle_2_wait(void)
{
	cb_2_called = false;
	k_sem_take(&wait, WAIT_TIME);

	zassert_true(cb_2_called,
			"Trickle 2 no timeout");

	zassert_true(net_trickle_is_running(&t2), "Trickle 2 not running");
}

static void test_trickle_1_stopped(void)
{
	zassert_false(net_trickle_is_running(&t1), "Trickle 1 running");
}

static void test_trickle_2_inc(void)
{
	zassert_true(net_trickle_is_running(&t2), "Trickle 2 is not running");
	token2++;
}

static void test_trickle_1_update(void)
{
	zassert_true(net_trickle_is_running(&t1), "trickle 1 is not running");

	token1 = token2;
}

static void test_init(void)
{
	k_sem_init(&wait, 0, UINT_MAX);
}

/*test case main entry*/
void test_main(void)
{
	ztest_test_suite(test_tickle,
			ztest_unit_test(test_init),
			ztest_unit_test(test_trickle_create),
			ztest_unit_test(test_trickle_start),
			ztest_unit_test(test_trickle_1_status),
			ztest_unit_test(test_trickle_2_status),
			ztest_unit_test(test_trickle_1_wait),
			ztest_unit_test(test_trickle_2_wait),
			ztest_unit_test(test_trickle_1_update),
			ztest_unit_test(test_trickle_1_status),
			ztest_unit_test(test_trickle_2_inc),
			ztest_unit_test(test_trickle_2_status),
			ztest_unit_test(test_trickle_1_status),
#if CHECK_LONG_TIMEOUT > 0
			ztest_unit_test(test_trickle_1_wait_long),
#endif
			ztest_unit_test(test_trickle_stop),
			ztest_unit_test(test_trickle_1_stopped));
	ztest_run_test_suite(test_tickle);
}
