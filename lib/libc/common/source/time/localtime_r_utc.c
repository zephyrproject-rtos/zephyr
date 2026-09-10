/*
 * Copyright (c) 2024 Meta Platforms
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#undef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#include <time.h>

struct tm *localtime_r(const time_t *timer, struct tm *result)
{
	return gmtime_r(timer, result);
}

struct tm *localtime(const time_t *timer)
{
	return gmtime(timer);
}
