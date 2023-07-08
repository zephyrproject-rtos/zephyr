/*
 * Copyright (c) 2023 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <errno.h>

#include <zephyr/posix/signal.h>

#define SIGNO_WORD_IDX(_signo) (signo / BITS_PER_LONG)
#define SIGNO_WORD_BIT(_signo) (signo & BIT_MASK(LOG2(BITS_PER_LONG)))

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
