/*
 * Copyright (c) 2026 Måns Ansgariusson <mansgariusson@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/zassert.h>
#include <zephyr/sys/printk.h>
#include <stdarg.h>
#include <string.h>

#include "harness.h"

static char captured[256];
static bool fired;
static int arg_evals;

jmp_buf harness_escape;

void harness_reset(void *fixture)
{
	ARG_UNUSED(fixture);
	captured[0] = '\0';
	fired = false;
	arg_evals = 0;
}

int harness_counting_arg(void)
{
	return ++arg_evals;
}

bool harness_fired(void)
{
	return fired;
}

bool harness_printed(void)
{
	return captured[0] != '\0';
}

bool harness_contains(const char *needle)
{
	return strstr(captured, needle) != NULL;
}

int harness_arg_evals(void)
{
	return arg_evals;
}

FUNC_NORETURN void zassert_fail(const char *cond, const char *file,
				unsigned int line, const char *fmt, ...)
{
	size_t len = strlen(captured);

	len += snprintk(captured + len, sizeof(captured) - len,
			"ASSERTION FAIL [%s] @ %s:%d\n", cond, file, line);

	if (fmt != NULL) {
		va_list ap;

		va_start(ap, fmt);
		vsnprintk(captured + len, sizeof(captured) - len, fmt, ap);
		va_end(ap);
	}

	fired = true;
	longjmp(harness_escape, 1);
}
