/*
 * Copyright (c) 2019 Peter Bigot Consulting, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <time.h>
#include <zephyr/sys/libc-hooks.h>

#ifdef CONFIG_MINIMAL_LIBC_NON_REENTRANT_FUNCTIONS
static Z_LIBC_DATA struct tm gmtime_result;

struct tm *gmtime(const time_t *timep)
{
	return gmtime_r(timep, &gmtime_result);
}
#endif /* CONFIG_MINIMAL_LIBC_NON_REENTRANT_FUNCTIONS */
