/*
 * Copyright (c) 2020 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <zephyr/kernel.h>

#ifdef CONFIG_COMMON_LIBC_SIGNAL

#include <signal.h>
#include <zephyr/kernel/signal.h>

/*
 * ISO C says abort() raises SIGABRT. POSIX adds that SIGABRT must not be blocked when abort() is
 * called, and that abort() must terminate the caller even when SIGABRT is caught or ignored.
 */
static void abort_sigabrt(void)
{
	struct k_sig_set set;
	struct k_sig_action dfl = {
		.handler = K_SIG_DFL,
		.flags = 0,
	};

	k_sig_emptyset(&dfl.mask);
	k_sig_emptyset(&set);
	(void)k_sig_addset(&set, K_SIG_ABRT);
	(void)k_sig_mask(K_SIG_UNBLOCK, &set, NULL);

	(void)raise(SIGABRT);

	/*
	 * Still here, so SIGABRT was ignored or a handler caught it and returned. Restore the
	 * default disposition, which terminates the calling thread, and raise it once more.
	 */
	(void)k_sig_action(K_SIG_ABRT, &dfl, NULL);
	(void)raise(SIGABRT);
}

#else

#define abort_sigabrt() do { } while (false)

#endif /* CONFIG_COMMON_LIBC_SIGNAL */

FUNC_NORETURN void abort(void)
{
	abort_sigabrt();

	printk("abort()\n");
	k_panic();
	CODE_UNREACHABLE;
}
