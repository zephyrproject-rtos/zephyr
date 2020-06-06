/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/__assert.h>

/**
 *
 * @brief Handler of C standard library assert.
 */
void __assert_func(const char *file,
		int line,
		const char *func,
		const char *failedexpr)
{
	__ASSERT_LOC(...);
	__ASSERT_MSG_INFO("Assertion fail: (%s) at %s:%d",
			failedexpr, file, line);
	__ASSERT_POST_ACTION();
}
