/*
 * Copyright (c) 2023 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "posix/strsignal_table.h"

#include <errno.h>
#include <stdio.h>

#include <zephyr/posix/pthread.h>
#include <zephyr/posix/signal.h>

#define SIGNO_WORD_IDX(_signo) (_signo / BITS_PER_LONG)
#define SIGNO_WORD_BIT(_signo) (_signo & BIT_MASK(LOG2(BITS_PER_LONG)))

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
	int ret;

	ret = k_sig_emptyset(set);
	if (ret < 0) {
		errno = -ret;
		return -1;
	}

	return 0;
}

int sigfillset(sigset_t *set)
{
	int ret;

	ret = k_sig_fillset(set);
	if (ret < 0) {
		errno = -ret;
		return -1;
	}

	return 0;
}

int sigaddset(sigset_t *set, int signo)
{
	int ret;

	ret = k_sig_addset(set, signo);
	if (ret < 0) {
		errno = -ret;
		return -1;
	}

	return 0;
}

int sigdelset(sigset_t *set, int signo)
{
	int ret;

	ret = k_sig_delset(set, signo);
	if (ret < 0) {
		errno = -ret;
		return -1;
	}

	return 0;
}

int sigismember(const sigset_t *set, int signo)
{
	int ret;

	ret = k_sig_ismember(set, signo);
	if (ret < 0) {
		errno = -ret;
		return -1;
	}

	return ret;
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

int pthread_sigmask(int how, const sigset_t *ZRESTRICT set, sigset_t *ZRESTRICT oset)
{
	return -k_sig_mask(how, set, oset);
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
