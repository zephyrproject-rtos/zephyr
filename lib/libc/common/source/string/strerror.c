/*
 * Copyright (c) 2022 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/sys/util.h>
#include <zephyr/toolchain.h>

/*
 * See scripts/build/gen_strerror_table.py
 *
 * #define sys_nerr N
 * const char *const sys_errlist[sys_nerr];
 * const uint8_t sys_errlen[sys_nerr];
 */
#define __sys_errlist_declare
#include "libc/common/strerror_table.h"

char *strerror(int errnum)
{
	if (IS_ENABLED(CONFIG_COMMON_LIBC_STRING_ERROR_TABLE) && errnum >= 0 && errnum < sys_nerr) {
		return (char *)sys_errlist[errnum];
	}

	return (char *) "";
}
