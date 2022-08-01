/* main.c - Application main entry point */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_test, CONFIG_NET_TRICKLE_LOG_LEVEL);

#include <zephyr/types.h>
#include <zephyr/ztest.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/printk.h>
#include <zephyr/linker/sections.h>

#include <zephyr/tc_util.h>

#include <zephyr/net/buf.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_if.h>

#include <zephyr/net/trickle.h>

#include "net_private.h"

#if defined(CONFIG_NET_TRICKLE_LOG_LEVEL_DBG)
#define DBG(fmt, ...) printk(fmt, ##__VA_ARGS__)
#else
#define DBG(fmt, ...)
#endif

static int token1 = 1, token2 = 2;

static struct k_sem wait;
static struct k_sem wait2;
static bool cb_1_called;
static bool cb_2_called;

#define WAIT_TIME K_SECONDS(3)

/* Set CHECK_LONG_TIMEOUT to 1 if you want to check longer timeout.
 * Do not do this for automated tests as those need to finish asap.
 */
#define CHECK_LONG_TIMEOUT 0
#if CHECK_LONG_TIMEOUT > 0
#define WAIT_TIME_LONG K_SECONDS(10)
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

	k_sem_give(&wait2);

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

	cb_1_called = false;
	cb_2_called = false;

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
	k_sem_take(&wait, WAIT_TIME);

	zassert_true(cb_1_called, "Trickle 1 no timeout");

	zassert_true(net_trickle_is_running(&t1), "Trickle 1 not running");
}

#if CHECK_LONG_TIMEOUT > 0
static void test_trickle_1_wait_long(void)
{
	cb_1_called = false;

	k_sem_take(&wait, WAIT_TIME_LONG);

	zassert_false(cb_1_called, "Trickle 1 no timeout");

	zassert_true(net_trickle_is_running(&t1), "Trickle 1 not running");
}
#else
static void test_trickle_1_wait_long(void)
{
	ztest_test_skip();
}
#endif

static void test_trickle_2_wait(void)
{
	k_sem_take(&wait2, WAIT_TIME);

	zassert_true(cb_2_called, "Trickle 2 no timeout");

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
	k_sem_init(&wait2, 0, UINT_MAX);
}

ZTEST(net_trickle, test_trickle)
{
	test_init();
	test_trickle_create();
	test_trickle_start();
	test_trickle_1_status();
	test_trickle_2_status();
	test_trickle_1_wait();
	test_trickle_2_wait();
	test_trickle_1_update();
	test_trickle_2_inc();
	test_trickle_1_status();
	test_trickle_1_wait_long();
	test_trickle_stop();
	test_trickle_1_stopped();
}

ZTEST_SUITE(net_trickle, NULL, NULL, NULL, NULL, NULL);
