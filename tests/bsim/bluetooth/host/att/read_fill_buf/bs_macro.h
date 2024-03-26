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
