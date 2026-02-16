/*
 * SPDX-License-Identifier: Apache-2.0
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
*/

#include <errno.h>
#include <signal.h>

int raise(int sig)
{
	(void)sig;      
	errno = EINVAL;
	return -1;
}