/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NSI_COMMON_SRC_NSI_SAFE_CALLL_H
#define NSI_COMMON_SRC_NSI_SAFE_CALLL_H

#include <stdbool.h>
#include "nsi_tracing.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef nsi_unlikely
#define nsi_unlikely(x) (__builtin_expect((bool)!!(x), false) != 0L)
#endif

#define NSI_SAFE_CALL(a) nsi_safe_call(a, #a)

static inline void nsi_safe_call(int test, const char *test_str)
{
	/* LCOV_EXCL_START */ /* See Note1 */
	if (nsi_unlikely(test)) {
		nsi_print_error_and_exit("Error on: %s\n",
					   test_str);
	}
	/* LCOV_EXCL_STOP */
}

#ifdef __cplusplus
}
#endif

#endif /* NSI_COMMON_SRC_NSI_SAFE_CALLL_H */
