/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bs_tracing.h"
#include "bs_types.h"
#include "bstests.h"

#define BS_SECONDS(dur_sec)    ((bs_time_t)dur_sec * USEC_PER_SEC)
#define TEST_TIMEOUT_SIMULATED BS_SECONDS(60)

extern enum bst_result_t bst_result;

#define DECLARE_FLAG(flag) extern atomic_t flag
#define DEFINE_FLAG(flag)  atomic_t flag = (atomic_t)false
#define SET_FLAG(flag)     (void)atomic_set(&flag, (atomic_t)true)
#define UNSET_FLAG(flag)   (void)atomic_set(&flag, (atomic_t)false)

#define WAIT_FOR_VAL(var, val)                                                                    \
	while (atomic_get(&var) != val) {                                                         \
		(void)k_sleep(K_MSEC(1));                                                          \
	}
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

#define ASSERT(expr, ...)                                                                          \
	do {                                                                                       \
		if (!(expr)) {                                                                     \
			FAIL(__VA_ARGS__);                                                         \
		}                                                                                  \
	} while (0)

#define FAIL(...)                                                                                  \
	do {                                                                                       \
		bst_result = Failed;                                                               \
		bs_trace_error_time_line(__VA_ARGS__);                                             \
	} while (0)

#define PASS(...)                                                                                  \
	do {                                                                                       \
		bst_result = Passed;                                                               \
		bs_trace_info_time(1, __VA_ARGS__);                                                \
	} while (0)
