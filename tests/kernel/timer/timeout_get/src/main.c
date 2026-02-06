/*
 * Copyright (c) 2026 Qualcomm Technologies, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/sys/util_macro.h>

static struct k_timer t1;
static struct k_timer t2;

uint32_t t1_count;
uint32_t t2_count;

static void t1_expiry_cb(struct k_timer *timer)
{
	t1_count++;
}

static void t2_expiry_cb(struct k_timer *timer)
{
	t2_count++;
}

ZTEST(timeout_get, test_timeout_get)
{
	bool found_t1_timeout = false;
	struct _timeout *iter = NULL;

	/* Initialize the timers */
	k_timer_init(&t1, t1_expiry_cb, NULL);
	k_timer_init(&t2, t2_expiry_cb, NULL);

	/* Start timers */
	k_timer_start(&t1, K_SECONDS(2), K_NO_WAIT);
	k_timer_start(&t2, K_SECONDS(3), K_NO_WAIT);

	k_sleep(K_SECONDS(1));

	while (1) {
		struct _timeout *t = k_get_next_timeout(iter);

		if (t == NULL) {
			break;
		}

		if (t == &t1.timeout) {
			found_t1_timeout = true;
			k_timer_stop(&t1);
			break;
		}

		iter = t;
	}

	zassert_true(found_t1_timeout, "Did not find t1's timeout via iteration");

	k_timer_stop(&t2);
}

ZTEST_SUITE(timeout_get, NULL, NULL, NULL, NULL, NULL);
