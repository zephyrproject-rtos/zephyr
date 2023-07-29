/*
 * Copyright (c) 2023 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <signal.h>

#include <zephyr/ztest.h>

ZTEST(test_c_lib, test_signal_decl)
{
	/*
	 * Even though the use of the ANSI-C function "signal()" is not yet
	 * supported in Zephyr, some users depend on it being declared (i.e.
	 * without CONFIG_POSIX_API=y and with with CONFIG_POSIX_SIGNAL=n).
	 */

	zassert_not_null(signal);
}
