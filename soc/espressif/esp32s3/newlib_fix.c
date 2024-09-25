/*
 * Copyright (c) 2019, Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>
#include <stdio.h>
#include <stddef.h>
#include <unistd.h>
#include <sys/stat.h>

#include <reent.h>

int __weak _gettimeofday_r(struct _reent *r, struct timeval *__tp, void *__tzp)
{
	ARG_UNUSED(r);
	ARG_UNUSED(__tp);
	ARG_UNUSED(__tzp);

	return -1;
}
