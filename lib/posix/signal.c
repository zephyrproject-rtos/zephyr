/*
 * Copyright (c) 2023 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/posix/signal.h>

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
