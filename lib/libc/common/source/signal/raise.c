/*
 * SPDX-License-Identifier: Apache-2.0
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 */

#include "sig_isoc.h"

#include <errno.h>
#include <signal.h>

#include <zephyr/kernel.h>
#include <zephyr/kernel/signal.h>

int raise(int sig)
{
	int ret;
	int ksig;

	ksig = (z_sig_from_posix != NULL) ? z_sig_from_posix(sig) : z_isoc_sig_to_k(sig);
	if (ksig <= 0) {
		errno = EINVAL;
		return -1;
	}

	/*
	 * ISO C requires the handler to have finished before raise() returns; the generated
	 * k_sig_queue() wrapper drains pending signals before returning here.
	 */
	ret = k_sig_queue(k_current_get(), ksig, (union k_sig_val){0});
	if (ret < 0) {
		errno = -ret;
		return -1;
	}

	return 0;
}
