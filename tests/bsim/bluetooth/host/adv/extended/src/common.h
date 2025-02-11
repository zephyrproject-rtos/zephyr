/**
 * Copyright (c) 2024 Croxel, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_TEST_BSIM_BT_EXT_ADV_TEST_
#define ZEPHYR_TEST_BSIM_BT_EXT_ADV_TEST_

#include <zephyr/kernel.h>

#include "bs_types.h"
#include "bs_tracing.h"
#include "time_machine.h"
#include "bstests.h"

#define WAIT_SECONDS 30                         /* seconds */
#define WAIT_TIME (WAIT_SECONDS * USEC_PER_SEC) /* microseconds*/

#define CREATE_FLAG(flag) static atomic_t flag = (atomic_t)false
#define SET_FLAG(flag)	   (void)atomic_set(&flag, (atomic_t) true)
#define TEST_FLAG(flag)    (atomic_get(&flag) == (atomic_t)true)
#define UNSET_FLAG(flag)   (void)atomic_set(&flag, (atomic_t) false)
#define WAIT_FOR_FLAG(flag)                                                                        \
	while (!(bool)atomic_get(&flag)) {                                                         \
		(void)k_sleep(K_MSEC(1));                                                          \
	}
#define WAIT_FOR_FLAG_UNSET(flag)                                                                  \
	while ((bool)atomic_get(&flag)) {                                                          \
		(void)k_sleep(K_MSEC(1));                                                          \
	}
#define TAKE_FLAG(flag)                                                                            \
	while (!(bool)atomic_cas(&flag, true, false)) {                                            \
		(void)k_sleep(K_MSEC(1));                                                          \
	}


#define FAIL(...) \
	do { \
		bst_result = Failed; \
		bs_trace_error_time_line(__VA_ARGS__); \
	} while (0)

#define PASS(...) \
	do { \
		bst_result = Passed; \
		bs_trace_info_time(1, "PASSED: " __VA_ARGS__); \
	} while (0)

void test_tick(bs_time_t HW_device_time);
void test_init(void);

#endif /* ZEPHYR_TEST_BSIM_BT_EXT_ADV_TEST_ */
