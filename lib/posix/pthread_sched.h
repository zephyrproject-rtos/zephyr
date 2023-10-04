/*
 * Copyright (c) 2023 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LIB_POSIX_POSIX_PTHREAD_SCHED_H_
#define ZEPHYR_LIB_POSIX_POSIX_PTHREAD_SCHED_H_

#include <stdbool.h>

#include <zephyr/posix/sched.h>

static inline bool valid_posix_policy(int policy)
{
	return policy == SCHED_FIFO || policy == SCHED_RR || policy == SCHED_OTHER;
}

#endif
