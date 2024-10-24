/*
 * Copyright © 2021, Keith Packard <keithp@keithp.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "picolibc-hooks.h"

#ifdef CONFIG_PICOLIBC_ASSERT_VERBOSE

FUNC_NORETURN void __assert_func(const char *file, int line,
				 const char *function, const char *expression)
{
	__ASSERT(0, "assertion \"%s\" failed: file \"%s\", line %d%s%s\n",
		 expression, file, line,
		 function ? ", function: " : "", function ? function : "");
	CODE_UNREACHABLE;
}

#else

FUNC_NORETURN void __assert_no_args(void)
{
	__ASSERT_NO_MSG(0);
	CODE_UNREACHABLE;
}

#endif
