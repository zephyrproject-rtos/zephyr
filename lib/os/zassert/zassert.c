/*
 * Copyright (c) 2026 Måns Ansgariusson <mansgariusson@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdarg.h>
#include <zephyr/sys/zassert.h>
#include <zephyr/sys/printk.h>
#include <zephyr/kernel.h>

__weak FUNC_NORETURN void zassert_fail(const char *cond, const char *file,
				       unsigned int line, const char *fmt, ...)
{
	printk("ASSERTION FAIL [%s] @ %s:%d\n", cond, file, line);

	if (fmt != NULL) {
		va_list ap;

		va_start(ap, fmt);
		vprintk(fmt, ap);
		va_end(ap);
	}

	zassert_post_action(file, line);

	CODE_UNREACHABLE;
}

__weak FUNC_NORETURN void zassert_post_action(const char *file, unsigned int line)
{
	ARG_UNUSED(file);
	ARG_UNUSED(line);

#ifdef CONFIG_USERSPACE
	if (k_is_user_context()) {
		k_oops();
	}
#endif

	k_panic();

	CODE_UNREACHABLE;
}
