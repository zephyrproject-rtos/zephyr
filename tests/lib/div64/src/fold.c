/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

#include "fold.h"

void div64_check_folded_cpp(void);

ZTEST(div64, test_fold)
{
#if defined(CONFIG_NO_OPTIMIZATIONS)
	ztest_test_skip();
#else
	check_folded();
	div64_check_folded_cpp();
#endif
}
