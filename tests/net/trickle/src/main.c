/* main.c - Application main entry point */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
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

#if defined(CONFIG_NET_DEBUG_CONTEXT)
#define DBG(fmt, ...) printk(fmt, ##__VA_ARGS__)
#else
#define DBG(fmt, ...)
#endif

static int token1 = 1, token2 = 2;

static struct k_sem wait;
static bool cb_1_called;
static bool cb_2_called;
static bool test_failed;

#define WAIT_TIME (2 * MSEC_PER_SEC)

/* Set CHECK_LONG_TIMEOUT to 1 if you want to check longer timeout.
 * Do not do this for automated tests as those need to finish asap.
 */
#define CHECK_LONG_TIMEOUT 0
#if CHECK_LONG_TIMEOUT > 0
#define WAIT_TIME_LONG (10 * MSEC_PER_SEC)
#endif

#define T1_IMIN 30
#define T1_IMAX 100
#define T1_K 20

#define T2_IMIN 80
#define T2_IMAX 200
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

static bool test_trickle_create(void)
{
	int ret;

	ret = net_trickle_create(&t1, T1_IMIN, T1_IMAX, T1_K);
	if (ret) {
		TC_ERROR("Trickle 1 create failed\n");
		return false;
	}

	ret = net_trickle_create(&t2, T2_IMIN, T2_IMAX, T2_K);
	if (ret) {
		TC_ERROR("Trickle 2 create failed\n");
		return false;
	}

	return true;
}

static bool test_trickle_start(void)
{
	int ret;

	ret = net_trickle_start(&t1, cb_1, &t1);
	if (ret) {
		TC_ERROR("Trickle 1 start failed\n");
		return false;
	}

	ret = net_trickle_start(&t2, cb_2, &t2);
	if (ret) {
		TC_ERROR("Trickle 2 start failed\n");
		return false;
	}

	return true;
}

static bool test_trickle_stop(void)
{
	int ret;

	ret = net_trickle_stop(&t1);
	if (ret) {
		TC_ERROR("Trickle 1 stop failed\n");
		return false;
	}

	ret = net_trickle_stop(&t2);
	if (ret) {
		TC_ERROR("Trickle 2 stop failed\n");
		return false;
	}

	return true;
}

static bool test_trickle_1_status(void)
{
	if (!net_trickle_is_running(&t1)) {
		TC_ERROR("Trickle 1 not running\n");
		return false;
	}

	if (token1 != token2) {
		net_trickle_inconsistency(&t1);
	} else {
		net_trickle_consistency(&t1);
	}

	return true;
}

static bool test_trickle_2_status(void)
{
	if (!net_trickle_is_running(&t2)) {
		TC_ERROR("Trickle 2 not running\n");
		return false;
	}

	if (token1 == token2) {
		net_trickle_consistency(&t2);
	} else {
		net_trickle_inconsistency(&t2);
	}

	return true;
}

static bool test_trickle_1_wait(void)
{
	cb_1_called = false;
	k_sem_take(&wait, WAIT_TIME);

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

static bool test_trickle_2_wait(void)
{
	cb_2_called = false;
	k_sem_take(&wait, WAIT_TIME);

	if (!cb_2_called) {
		TC_ERROR("Trickle 2 no timeout\n");
		return false;
	}

	if (!net_trickle_is_running(&t2)) {
		TC_ERROR("Trickle 2 not running\n");
		return false;
	}

	return true;
}

static bool test_trickle_1_stopped(void)
{
	if (net_trickle_is_running(&t1)) {
		TC_ERROR("Trickle 1 running\n");
		return false;
	}

	return true;
}

static bool test_trickle_2_inc(void)
{
	if (!net_trickle_is_running(&t2)) {
		TC_ERROR("Trickle 2 is not running\n");
		return false;
	}

	token2++;

	return true;
}

static bool test_trickle_1_update(void)
{
	if (!net_trickle_is_running(&t1)) {
		TC_ERROR("Trickle 1 is not running\n");
		return false;
	}

	token1 = token2;

	return true;
}

static bool test_init(void)
{
	k_sem_init(&wait, 0, UINT_MAX);

	return true;
}

static const struct {
	const char *name;
	bool (*func)(void);
} tests[] = {
	{ "trickle test init", test_init },
	{ "trickle create", test_trickle_create },
	{ "trickle start", test_trickle_start },
	{ "trickle 1 check status", test_trickle_1_status },
	{ "trickle 2 check status", test_trickle_2_status },
	{ "trickle 1 wait timeout", test_trickle_1_wait },
	{ "trickle 2 wait timeout", test_trickle_2_wait },
	{ "trickle 1 update", test_trickle_1_update },
	{ "trickle 1 check status", test_trickle_1_status },
	{ "trickle 2 update", test_trickle_2_inc },
	{ "trickle 2 check status", test_trickle_2_status },
	{ "trickle 1 check status", test_trickle_1_status },

#if CHECK_LONG_TIMEOUT > 0
	{ "trickle 1 wait long timeout", test_trickle_1_wait_long },
#endif
	{ "trickle stop", test_trickle_stop },
	{ "trickle 1 check stopped", test_trickle_1_stopped },
};

void main(void)
{
	int count, pass;

	for (count = 0, pass = 0; count < ARRAY_SIZE(tests); count++) {
		TC_START(tests[count].name);
		test_failed = false;
		if (!tests[count].func() || test_failed) {
			TC_END(FAIL, "failed\n");
		} else {
			TC_END(PASS, "passed\n");
			pass++;
		}
	}

	TC_END_REPORT(((pass != ARRAY_SIZE(tests)) ? TC_FAIL : TC_PASS));
}
