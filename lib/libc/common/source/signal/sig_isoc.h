/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LIB_LIBC_COMMON_SOURCE_SIGNAL_SIG_ISOC_H_
#define ZEPHYR_LIB_LIBC_COMMON_SOURCE_SIGNAL_SIG_ISOC_H_

#include <signal.h>

#include <zephyr/kernel.h>
#include <zephyr/kernel/signal.h>
#include <zephyr/toolchain.h>

/*
 * The ISO C dispositions must be representable as kernel dispositions, since signal() hands
 * them straight to k_sig_action() and hands k_sig_action()'s output straight back to the caller.
 */
BUILD_ASSERT((void *)SIG_DFL == K_SIG_DFL, "SIG_DFL does not match K_SIG_DFL");
BUILD_ASSERT((void *)SIG_IGN == K_SIG_IGN, "SIG_IGN does not match K_SIG_IGN");
BUILD_ASSERT((void *)SIG_ERR == K_SIG_ERR, "SIG_ERR does not match K_SIG_ERR");

/**
 * @brief Convert a C library signal number to a kernel signal number
 *
 * Only the six signals required by ISO C are translated. A C library is free to number signals
 * however it likes, and the ones Zephyr ships with disagree: picolibc and newlib inherit Cygwin's
 * numbering, where SIGUSR1 is 30, while the kernel and the POSIX option group follow Linux, where
 * it is 10. Translating the rest needs the full table that the POSIX signals option group owns,
 * so signal() defers to it by being weak, and raise() picks the full table up through a weak
 * reference to z_sig_from_posix() when the POSIX layer is linked.
 *
 * @param sig C library signal number
 * @return the corresponding kernel signal number, or -1 if @p sig is not an ISO C signal number
 */
static inline int z_isoc_sig_to_k(int sig)
{
	switch (sig) {
	case SIGABRT:
		return K_SIG_ABRT;
	case SIGFPE:
		return K_SIG_FPE;
	case SIGILL:
		return K_SIG_ILL;
	case SIGINT:
		return K_SIG_INT;
	case SIGSEGV:
		return K_SIG_SEGV;
	case SIGTERM:
		return K_SIG_TERM;
	default:
		return -1;
	}
}

/*
 * Provided by the POSIX signals option group when it is linked; resolve to NULL otherwise.
 * z_sig_from_posix() maps a C library signal number to a kernel signal number using the full
 * table the POSIX layer owns. z_sig_shim() is the handler the POSIX layer registers with the
 * kernel: it recovers the application handler from the action's user_data and invokes it with
 * the C library's signal number.
 */
int z_sig_from_posix(int sig) __attribute__((__weak__));
void z_sig_shim(int sig, struct k_sig_info *info, void *context) __attribute__((__weak__));

#endif /* ZEPHYR_LIB_LIBC_COMMON_SOURCE_SIGNAL_SIG_ISOC_H_ */
