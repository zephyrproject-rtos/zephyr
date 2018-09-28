/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_POSIX_SIGNAL_H_
#define ZEPHYR_INCLUDE_POSIX_SIGNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "sys/types.h"

#ifndef SIGEV_NONE
#define SIGEV_NONE 1
#endif

#ifndef SIGEV_SIGNAL
#define SIGEV_SIGNAL 2
#endif

#ifndef SIGEV_THREAD
#define SIGEV_THREAD 3
#endif

typedef union sigval {
	int sival_int;
	void *sival_ptr;
} sigval;

typedef struct sigevent {
	int sigev_notify;
	int sigev_signo;
	sigval sigev_value;
	void (*sigev_notify_function)(sigval val);
	pthread_attr_t *sigev_notify_attributes;
} sigevent;

#ifdef __cplusplus
}
#endif

#endif /* POSIX__SIGNAL_H */
