/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_LIB_POSIX_TS_TO_MS_H_
#define ZEPHYR_LIB_POSIX_TS_TO_MS_H_

#ifdef __cplusplus
extern "C" {
#endif

static inline int32_t _ts_to_ms(const struct timespec *to)
{
	return (to->tv_sec * MSEC_PER_SEC) + (to->tv_nsec / NSEC_PER_MSEC);
}

#ifdef __cplusplus
}
#endif

#endif
