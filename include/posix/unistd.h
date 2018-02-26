/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __POSIX_UNISTD_H__
#define __POSIX_UNISTD_H__

#ifdef __cplusplus
extern "C" {
#endif

#include_next <unistd.h>

#ifdef CONFIG_PTHREAD_IPC
#include "sys/types.h"

/**
 * @brief Sleep for a specified number of seconds.
 *
 * See IEEE 1003.1
 */
static inline int sleep(unsigned int seconds)
{
	k_sleep(K_SECONDS(seconds));
	return 0;
}
/**
 * @brief Suspend execution for microsecond intervals.
 *
 * See IEEE 1003.1
 */
static inline int usleep(useconds_t useconds)
{
	if (useconds < USEC_PER_MSEC) {
		k_busy_wait(useconds);
	} else {
		k_sleep(useconds / USEC_PER_MSEC);
	}

	return 0;
}

#endif
#ifdef __cplusplus
}
#endif
#endif	/* __POSIX_UNISTD_H__ */
