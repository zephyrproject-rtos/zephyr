/*
 * SPDX-License-Identifier: Apache-2.0
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 */

#include <errno.h>
#include <signal.h>

void (*signal(int signum, void (*handler)(int)))(int)
{
	ARG_UNUSED(signum);
	ARG_UNUSED(handler);
	errno = EINVAL;
	return SIG_ERR;
}
