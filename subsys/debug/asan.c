/*
 * Copyright (c) 2019 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

const char *__asan_default_options(void)
{
	return
#if defined(CONFIG_64BIT) && defined(__GNUC__) && !defined(__clang__)
		/* Running leak detection at exit could lead to a deadlock on
		 * 64-bit boards if GCC is used.
		 * https://github.com/zephyrproject-rtos/zephyr/issues/20122
		 */
		"leak_check_at_exit=0:"
#endif
		"";
}
