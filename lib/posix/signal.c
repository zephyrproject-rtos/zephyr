/*
 * Copyright (c) 2023 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "posix/strsignal_table.h"

#include <errno.h>
#include <stdio.h>

#include <zephyr/posix/signal.h>

#define SIGNO_WORD_IDX(_signo) (signo / BITS_PER_LONG)
#define SIGNO_WORD_BIT(_signo) (signo & BIT_MASK(LOG2(BITS_PER_LONG)))

BUILD_ASSERT(CONFIG_POSIX_LIMITS_RTSIG_MAX >= 0);
BUILD_ASSERT(CONFIG_POSIX_RTSIG_MAX >= CONFIG_POSIX_LIMITS_RTSIG_MAX);

static inline bool signo_valid(int signo)
{
	return ((signo > 0) && (signo < _NSIG));
}

static inline bool signo_is_rt(int signo)
{
	return ((signo >= SIGRTMIN) && (signo <= SIGRTMAX));
}

int sigemptyset(sigset_t *set)
{
	*set = (sigset_t){0};
	return 0;
}

int sigfillset(sigset_t *set)
{
	for (int i = 0; i < ARRAY_SIZE(set->sig); i++) {
		set->sig[i] = -1;
	}

	return 0;
}

int sigaddset(sigset_t *set, int signo)
{
	if (!signo_valid(signo)) {
		errno = EINVAL;
		return -1;
	}

	WRITE_BIT(set->sig[SIGNO_WORD_IDX(signo)], SIGNO_WORD_BIT(signo), 1);

	return 0;
}

int sigdelset(sigset_t *set, int signo)
{
	if (!signo_valid(signo)) {
		errno = EINVAL;
		return -1;
	}

	WRITE_BIT(set->sig[SIGNO_WORD_IDX(signo)], SIGNO_WORD_BIT(signo), 0);

	return 0;
}

int sigismember(const sigset_t *set, int signo)
{
	if (!signo_valid(signo)) {
		errno = EINVAL;
		return -1;
	}

	return 1 & (set->sig[SIGNO_WORD_IDX(signo)] >> SIGNO_WORD_BIT(signo));
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
