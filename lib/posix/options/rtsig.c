/*
 * Copyright (c) 2024, Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "posix_internal.h"

#include <errno.h>
#include <time.h>

#include <zephyr/kernel.h>
#include <zephyr/posix/signal.h>

int sigqueue(pid_t pid, int signo, union sigval value)
{
	struct posix_thread *pth = to_posix_thread(pid);
	union k_sig_val val = {
		.sival_ptr = value.sival_ptr,
	};

	if (pth == NULL) {
		errno = ESRCH;
		return -1;
	}

	return k_sig_queue(&pth->thread, signo, val);
}

int sigtimedwait(const sigset_t *ZRESTRICT set, siginfo_t *ZRESTRICT info,
		 const struct timespec *ZRESTRICT timeout)
{
	int ret;
	k_timeout_t to;

	if (timeout == NULL) {
		to = K_FOREVER;
	} else {
		to = K_MSEC(timeout->tv_sec * MSEC_PER_SEC + timeout->tv_nsec / NSEC_PER_MSEC);
	}

	ret = k_sig_timedwait(set, info, to);
	if (ret < 0) {
		errno = -ret;
		return -1;
	}

	return ret;
}

int sigwaitinfo(const sigset_t *ZRESTRICT set, siginfo_t *ZRESTRICT info)
{
	int ret;

	ret = k_sig_timedwait(set, info, K_FOREVER);
	if (ret < 0) {
		errno = -ret;
		return -1;
	}

	return ret;
}
