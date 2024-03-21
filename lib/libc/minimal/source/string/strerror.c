/*
 * Copyright (c) 2022 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/sys/util.h>

/*
 * See scripts/build/gen_strerror_table.py
 *
 * #define sys_nerr N
 * const char *const sys_errlist[sys_nerr];
 * const uint8_t sys_errlen[sys_nerr];
 */
#include "libc/minimal/strerror_table.h"

/*
 * See https://pubs.opengroup.org/onlinepubs/9699919799/functions/strerror.html
 */
char *strerror(int errnum)
{
	if (IS_ENABLED(CONFIG_MINIMAL_LIBC_STRING_ERROR_TABLE) &&
	    errnum >= 0 && errnum < sys_nerr) {
		return (char *)sys_errlist[errnum];
	}

	return (char *) "";
}

/*
 * See
 * https://pubs.opengroup.org/onlinepubs/9699919799/functions/strerror_r.html
 */
int strerror_r(int errnum, char *strerrbuf, size_t buflen)
{
	const char *msg;
	size_t msg_len;

	if (errnum >= 0 && errnum < sys_nerr) {
		if (IS_ENABLED(CONFIG_MINIMAL_LIBC_STRING_ERROR_TABLE)) {
			msg = sys_errlist[errnum];
			msg_len = sys_errlen[errnum];
		} else {
			msg = "";
			msg_len = 1;
		}

		if (buflen < msg_len) {
			return ERANGE;
		}

		strncpy(strerrbuf, msg, msg_len);
	}

	if (errnum < 0 || errnum >= sys_nerr) {
		return EINVAL;
	}

	return 0;
}
