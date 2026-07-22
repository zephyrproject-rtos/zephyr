/*
 * SPDX-License-Identifier: Apache-2.0
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 */

#include "sig_isoc.h"

#include <errno.h>
#include <signal.h>

#include <zephyr/kernel.h>
#include <zephyr/kernel/signal.h>

void (*signal(int sig, void (*func)(int)))(int)
{
	int ret;
	int ksig;
	struct k_sig_action oact;
	struct k_sig_action act = {0};

	ksig = (z_sig_from_posix != NULL) ? z_sig_from_posix(sig) : z_isoc_sig_to_k(sig);
	if (ksig <= 0) {
		errno = EINVAL;
		return SIG_ERR;
	}

	if ((func == SIG_DFL) || (func == SIG_IGN)) {
		act.handler = (func == SIG_IGN) ? K_SIG_IGN : K_SIG_DFL;
	} else if (z_sig_shim != NULL) {
		/*
		 * The identical registration sigaction() makes for a plain handler, so a
		 * disposition installed through either function is visible to the other.
		 */
		act.handler = z_sig_shim;
		act.user_data = (void *)func;
	} else {
		/*
		 * Without the POSIX layer only the six ISO C signal numbers are reachable, and
		 * those coincide with the kernel's, so the handler can take delivery directly; it
		 * ignores the trailing arguments the kernel passes.
		 */
		act.handler = (k_sig_handler_t)func;
	}

	/* ISO C dispositions carry no mask */
	k_sig_emptyset(&act.mask);

	ret = k_sig_action(ksig, &act, &oact);
	if (ret < 0) {
		errno = -ret;
		return SIG_ERR;
	}

	if ((z_sig_shim != NULL) && (oact.handler == z_sig_shim)) {
		return (void (*)(int))oact.user_data;
	}

	return (void (*)(int))oact.handler;
}
