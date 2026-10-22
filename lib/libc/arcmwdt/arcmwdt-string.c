/*
 * Copyright (c) 2021 Synopsys.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define __STDC_WANT_LIB_EXT1__ 1

#include <string.h>
#include <stdint.h>
#include <sys/types.h>

size_t strnlen(const char *s, size_t maxlen)
{
	return strnlen_s(s, maxlen);
}
