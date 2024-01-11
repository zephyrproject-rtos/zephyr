/*
 * Common functions and helpers for L2CAP tests
 *
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>

#include <zephyr/types.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/l2cap.h>
#include "bs_types.h"
#include "bs_tracing.h"
#include "bstests.h"
#include "bs_pc_backchannel.h"

extern enum bst_result_t bst_result;

#define CREATE_FLAG(flag) static atomic_t flag = (atomic_t)false
#define SET_FLAG(flag) (void)atomic_set(&flag, (atomic_t)true)
#define UNSET_FLAG(flag) (void)atomic_set(&flag, (atomic_t)false)
#define TEST_FLAG(flag) (atomic_get(&flag) == (atomic_t)true)
#define WAIT_FOR_FLAG_SET(flag)		   \
	while (!(bool)atomic_get(&flag)) { \
		(void)k_sleep(K_MSEC(1));  \
	}
#define WAIT_FOR_FLAG_UNSET(flag)	  \
	while ((bool)atomic_get(&flag)) { \
		(void)k_sleep(K_MSEC(1)); \
	}


#define WAIT_SECONDS 30 /* seconds */
#define WAIT_TIME (WAIT_SECONDS * USEC_PER_SEC) /* microseconds*/

#define FAIL(...)				       \
	do {					       \
		bst_result = Failed;		       \
		bs_trace_error_time_line(__VA_ARGS__); \
	} while (0)

#define PASS(...)				    \
	do {					    \
		bst_result = Passed;		    \
		bs_trace_info_time(1, __VA_ARGS__); \
	} while (0)

#define ASSERT(expr, ...) if (!(expr)) {FAIL(__VA_ARGS__); }

void test_init(void);
void test_tick(bs_time_t HW_device_time);
