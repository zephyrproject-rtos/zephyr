/*
 * Copyright (c) 2025 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <errno.h>
#include <stdlib.h>

#include <zephyr/ztest.h>

ZTEST(xsi_single_process, test_putenv)
{
	char buf[64];

	{
		/* degenerate cases */
		static const char *const cases[] = {
			NULL, "", "=", "abc", "42", "=abc",
			/*
			 * Note:
			 * There are many poorly-formatted environment variable names and values
			 * that are invalid (from the perspective of a POSIX shell), but still
			 * accepted by setenv() and subsequently putenv().
			 *
			 * See also tests/posix/single_process/src/env.c
			 * See also lib/posix/shell/env.c:101
			 */
		};

		ARRAY_FOR_EACH(cases, i) {
			char *s;

			if (cases[i] == NULL) {
				s = NULL;
			} else {
				strncpy(buf, cases[i], sizeof(buf));
				buf[sizeof(buf) - 1] = '\0';
				s = buf;
			}

			errno = 0;
			zexpect_equal(-1, putenv(s), "putenv(%s) unexpectedly succeeded", s);
			zexpect_not_equal(0, errno, "putenv(%s) did not set errno", s);
		}
	}

	{
		/* valid cases */
		static const char *const cases[] = {
			"FOO=bar",
		};

		ARRAY_FOR_EACH(cases, i) {
			strncpy(buf, cases[i], sizeof(buf));
			buf[sizeof(buf) - 1] = '\0';

			zexpect_ok(putenv(buf), "putenv(%s) failed: %d", buf, errno);
		}
	}
}
