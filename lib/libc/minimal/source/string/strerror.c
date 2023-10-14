/*
 * Copyright (c) 2022 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * See scripts/build/gen_strerror_table.py
 *
 * #define sys_nerr N
 * const char *const sys_errlist[sys_nerr];
 * const uint8_t sys_errlen[sys_nerr];
 */
#include "libc/minimal/strerror_table.h"

#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/sys/util.h>

#define UNKNOWN_ERROR_PREFIX "Unknown error"

#define _swap(a, b, t)                                                                             \
	do {                                                                                       \
		t = *a;                                                                            \
		*a = *b;                                                                           \
		*b = t;                                                                            \
	} while (false)

static void _rev(char *s, size_t n)
{
	char *a;
	char *b;
	char t;

	for (a = s, b = s + n - 1; b > a; ++a, --b) {
		_swap(a, b, t);
	}
}

static void _itoa(int i, char *a)
{
	if (i == 0) {
		*a++ = '0';
		*a = '\0';
		return;
	}

	uint8_t n;
	bool last_digit_is_8 = false;

	if (i < 0) {
		*a++ = '-';
		if (i == INT32_MIN) {
			last_digit_is_8 = true;
			i++;
		}
		i = -i;
	}

	for (n = 0; i != 0; ++n, i /= 10) {
		*a++ = '0' + (i % 10);
	}
	*a = '\0';

	a -= n;
	if (i == 0 && last_digit_is_8) {
		*a = '8';
	}

	_rev(a, n);
}

/*
 * See https://pubs.opengroup.org/onlinepubs/9699919799/functions/strerror.html
 */
char *strerror(int errnum)
{
	static char invalid[sizeof(UNKNOWN_ERROR_PREFIX " " STRINGIFY(INT32_MIN))];

	if (errnum >= 0 && errnum < sys_nerr) {
		return IS_ENABLED(CONFIG_MINIMAL_LIBC_STRING_ERROR_TABLE)
			       ? (char *)sys_errlist[errnum]
			       : "";
	}

	bytecpy(invalid, UNKNOWN_ERROR_PREFIX, sizeof(UNKNOWN_ERROR_PREFIX) - 1);
	invalid[sizeof(UNKNOWN_ERROR_PREFIX) - 1] = ' ';
	_itoa(errnum, &invalid[sizeof(UNKNOWN_ERROR_PREFIX)]);

	return invalid;
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
