/*
 * Copyright (c) 2019 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_64BIT) && defined(__GNUC__) && !defined(__clang__)
const char *__asan_default_options(void)
{
	/* Running leak detection at exit could lead to a deadlock on
	 * 64-bit boards if GCC is used.
	 * https://github.com/zephyrproject-rtos/zephyr/issues/20122
	 */
	return "leak_check_at_exit=0:";
}
#endif

#ifdef CONFIG_HAS_SDL
const char *__lsan_default_suppressions(void)
{
	/* The SDL2 library does not clean-up all it resources on exit,
	 * as such suppress all memory leaks coming from libSDL2 and the
	 * underlying X11 library
	 */
	return "leak:libX11\nleak:libSDL2\n";
}
#endif /* CONFIG_HAS_SDL */

#ifdef CONFIG_ASAN_NOP_DLCLOSE
/* LSAN has a known limitation that if dlcose is called before performing the
 * leak check; "<unknown module>" is reported in the stack traces during the
 * leak check and these can not be suppressed, see
 * https://github.com/google/sanitizers/issues/89 for more info.
 *
 * A workaround for this is to implement a NOP version of dlclose.
 */
int dlclose(void *handler)
{
	return 0;
}
#endif /* CONFIG_ASAN_NOP_DLCLOSE */
