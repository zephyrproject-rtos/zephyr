/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __POSIX_SCHED_H__
#define __POSIX_SCHED_H__

/* Cooperative scheduling policy */
#ifndef SCHED_FIFO
#define SCHED_FIFO 0
#endif

/* Priority based prempetive scheduling policy */
#ifndef SCHED_RR
#define SCHED_RR 1
#endif

struct sched_param {
	int priority; /* Thread execution priority */
};
#endif
