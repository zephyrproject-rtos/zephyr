/*
 * Copyright (c) 2023 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <signal.h>
#include <errno.h>

int sigaddset(sigset_t *set, int signo)
{
	if ((set == NULL) || (signo <= 0) || (signo >= NSIG)) {
		errno = EINVAL;
		return -1;
	}

	set->_elem[_SIGSET_NDX(signo)] |= _SIGNO2SET(signo);

	return 0;
}
