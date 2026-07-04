/*
 * Copyright (c) 2026 Måns Ansgariusson <mansgariusson@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdarg.h>
#include <zephyr/sys/zassert.h>
#include <zephyr/sys/printk.h>
#include <zephyr/kernel.h>

__weak
#ifndef CONFIG_ASSERT_TEST
FUNC_NORETURN
#endif
void zassert_fail(const char *cond, const char *file,
		  unsigned int line, const char *fmt, ...)
{
	if (cond != NULL) {
		zassert_print("ASSERTION FAIL [%s] @ %s:%d\n", cond, file, line);
	} else {
		zassert_print("ASSERTION FAIL @ %s:%d\n", file, line);
	}

	if (fmt != NULL) {
		va_list ap;

		va_start(ap, fmt);
		zassert_vprint(fmt, ap);
		va_end(ap);
	}

	zassert_post_action(file, line);

#ifndef CONFIG_ASSERT_TEST
	CODE_UNREACHABLE;
#endif
}

__weak
#ifndef CONFIG_ASSERT_TEST
FUNC_NORETURN
#endif
void zassert_post_action(const char *file, unsigned int line)
{
	ARG_UNUSED(file);
	ARG_UNUSED(line);

#ifdef CONFIG_USERSPACE
	if (k_is_user_context()) {
		k_oops();
	}
#endif

	k_panic();

#ifndef CONFIG_ASSERT_TEST
	CODE_UNREACHABLE;
#endif
}

__weak void zassert_vprint(const char *fmt, va_list ap)
{
	vprintk(fmt, ap);
}
