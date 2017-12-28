/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __POSIX_SCHED_H__
#define __POSIX_SCHED_H__

#ifdef __cplusplus
extern "C" {
#endif

#include_next <sched.h>

/* Cooperative scheduling policy */
#ifndef SCHED_FIFO
#define SCHED_FIFO 0
#endif /* SCHED_FIFO */

/* Priority based prempetive scheduling policy */
#ifndef SCHED_RR
#define SCHED_RR 1
#endif /* SCHED_RR */

struct sched_param {
	int priority; /* Thread execution priority */
};

#ifdef __cplusplus
}
#endif

#endif /* __POSIX_SCHED_H__ */
