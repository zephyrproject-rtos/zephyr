/*
 * Copyright (c) 2023 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "posix/strsignal_table.h"
#include "posix_internal.h"

#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>

#include <zephyr/kernel.h>

#ifndef SIGRTMIN
#define SIGRTMIN 32
#endif

#ifndef SIGRTMAX
#define SIGRTMAX (SIGRTMIN + CONFIG_POSIX_RTSIG_MAX)
#endif

// BUILD_ASSERT(CONFIG_POSIX_RTSIG_MAX >= 0, "");

/* some libcs define these as very non-functional macros */
#undef sigemptyset
#undef sigfillset
#undef sigaddset
#undef sigdelset
#undef sigismember

static inline bool signo_valid(int signo)
{
	return ((signo > 0) && (signo <= SIGRTMAX));
}

static inline bool signo_is_rt(int signo)
{
	return ((signo >= SIGRTMIN) && (signo <= SIGRTMAX));
}

int sigemptyset(sigset_t *set)
{
	if (set == NULL) {
		errno = EINVAL;
		return -1;
	}

	z_sigemptyset((struct z_sigset *)set);
	return 0;
}

int sigfillset(sigset_t *set)
{
	if (set == NULL) {
		errno = EINVAL;
		return -1;
	}

	z_sigfillset((struct z_sigset *)set);
	return 0;
}

int sigaddset(sigset_t *set, int signo)
{
	if (set == NULL) {
		errno = EINVAL;
		return -1;
	}

	if (!signo_valid(signo)) {
		errno = EINVAL;
		return -1;
	}

	z_sigaddset((struct z_sigset *)set, signo);

	return 0;
}

int sigdelset(sigset_t *set, int signo)
{
	if (set == NULL) {
		errno = EINVAL;
		return -1;
	}

	if (!signo_valid(signo)) {
		errno = EINVAL;
		return -1;
	}

	z_sigdelset((struct z_sigset *)set, signo);

	return 0;
}

int sigismember(const sigset_t *set, int signo)
{
	if (set == NULL) {
		errno = EINVAL;
		return -1;
	}

	if (!signo_valid(signo)) {
		errno = EINVAL;
		return -1;
	}

	return z_sigismember((const struct z_sigset *)set, signo);
}

char *strsignal(int signum)
{
	/* Using -INT_MAX here because compiler resolves INT_MIN to (-2147483647 - 1) */
	static char buf[sizeof("RT signal -" STRINGIFY(INT_MAX))];

	if (!signo_valid(signum)) {
		errno = EINVAL;
		return "Invalid signal";
	}

	if (signo_is_rt(signum)) {
		snprintf(buf, sizeof(buf), "RT signal %d", signum - SIGRTMIN);
		return buf;
	}

	if (IS_ENABLED(CONFIG_POSIX_SIGNAL_STRING_DESC)) {
		if (strsignal_list[signum] != NULL) {
			return (char *)strsignal_list[signum];
		}
	}

	snprintf(buf, sizeof(buf), "Signal %d", signum);

	return buf;
}

int sigprocmask(int how, const sigset_t *ZRESTRICT set, sigset_t *ZRESTRICT oset)
{
	if (!IS_ENABLED(CONFIG_MULTITHREADING)) {
		return pthread_sigmask(how, set, oset);
	}

	/*
	 * Until Zephyr supports processes and specifically querying the number of active threads in
	 * a process For more information, see
	 * https://pubs.opengroup.org/onlinepubs/9699919799/functions/pthread_sigmask.html
	 */
	__ASSERT(false, "In multi-threaded environments, please use pthread_sigmask() instead of "
			"%s()", __func__);

	errno = ENOSYS;
	return -1;
}
