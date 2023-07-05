/*
 * Copyright (c) 2023 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <signal.h>
#include <stdint.h>

int sigfillset(sigset_t *set)
{
	for (int ndx = 0; ndx < _SIGSET_NELEM; ndx++) {
		set->_elem[ndx] = UINT32_MAX;
	}

	return 0;
}
