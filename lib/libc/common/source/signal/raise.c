/*
 * SPDX-License-Identifier: Apache-2.0
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 */

#include <signal.h>
#include <zephyr/kernel.h>

#if defined(CONFIG_POSIX_THREADS)
/* Forward declarations - pthread_kill() is declared in posix_signal.h (included via signal.h),
 * but pthread_self() is in pthread.h. We avoid including pthread.h to prevent typedef conflicts.
 * pthread_t is unsigned int per posix_signal.h.
 */
extern int pthread_kill(unsigned int thread, int sig);
extern unsigned int pthread_self(void);
#endif

#if defined(CONFIG_POSIX_MULTI_PROCESS)
#include <zephyr/posix/unistd.h>
#endif

int raise(int sig)
{
	/*
	 * According to C11 7.14.2.1, raise(sig) is equivalent to:
	 * - kill(getpid(), sig) in single-threaded programs
	 * - pthread_kill(pthread_self(), sig) in multi-threaded programs
	 *
	 * We follow the standard structure, even though in Zephyr,
	 * signal delivery is not fully implemented and both kill()
	 * and pthread_kill() are stubs that return ENOSYS.
	 *
	 * Note: For SIGABRT, abort() calls raise(SIGABRT) and then
	 * ensures termination if raise() returns (C11 7.22.4.1).
	 */
#if defined(CONFIG_POSIX_THREADS)
	/* Multi-threaded: use pthread_kill() */
	return pthread_kill(pthread_self(), sig);
#elif defined(CONFIG_POSIX_MULTI_PROCESS)
	/* Single-threaded: use kill() */
	return kill(getpid(), sig);
#else
	/* No POSIX support: signal delivery not available */
	errno = ENOSYS;
	return -1;
#endif
}
