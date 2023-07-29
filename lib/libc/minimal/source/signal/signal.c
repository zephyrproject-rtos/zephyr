/*
 * Copyright (c) 2023 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <signal.h>

sighandler_t signal(int signum, sighandler_t handler)
{
	return SIG_DFL;
}
