/*
 * Copyright (c) 2024 Meta Platforms
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <time.h>

/**
 * `ctime()` is equivalent to `asctime(localtime(clock))`
 * See: https://pubs.opengroup.org/onlinepubs/009695399/functions/ctime.html
 */

char *ctime(const time_t *clock)
{
	return asctime(localtime(clock));
}

#if defined(CONFIG_COMMON_LIBC_CTIME_R)
char *ctime_r(const time_t *clock, char *buf)
{
	struct tm tmp;

	return asctime_r(localtime_r(clock, &tmp), buf);
}
#endif /* CONFIG_COMMON_LIBC_CTIME_R */
