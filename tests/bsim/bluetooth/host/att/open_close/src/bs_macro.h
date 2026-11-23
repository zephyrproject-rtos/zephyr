/* Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <bs_tracing.h>
#include <bstests.h>

#define PASS(...)                                                                                  \
	do {                                                                                       \
		extern enum bst_result_t bst_result;                                               \
		bst_result = Passed;                                                               \
		bs_trace_info_time(1, __VA_ARGS__);                                                \
	} while (0)

static inline void bt_testlib_expect_zero(int err, char *where_file, int where_line)
{
	if (err) {
		bs_trace_print(BS_TRACE_ERROR, where_file, where_line, 0, BS_TRACE_AUTOTIME, 0,
			       "err %d\n", err);
	}
}

#define FAIL(...)                                                                                  \
	bs_trace_print(BS_TRACE_ERROR, __FILE__, __LINE__, 0, BS_TRACE_AUTOTIME, 0, __VA_ARGS__);

#define EXPECT_ZERO(expr) bt_testlib_expect_zero((expr), __FILE__, __LINE__)
