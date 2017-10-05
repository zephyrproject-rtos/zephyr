/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _EPOCH_TIME_H_
#define _EPOCH_TIME_H_

#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Epoch time representation */
struct epoch_time {
	/* Seconds since 1 January 1970 */
	u64_t secs;
	/* Microseconds */
	u32_t usecs;
};

/**
 * @brief Get current epoch time
 *
 * @details By passing the address of an epoch time structure, the structure
 * will be filled based on the current epoch time.

 * @param time Address of an epoch time structure.
 *
 * @return 0 if ok, <0 if error.
 */
int epoch_time_get(struct epoch_time *time);

/**
 * @brief Set current epoch time
 *
 * @details By passing the address of an epoch time structure, the current
 * epoch time will be updated based on the given structure.

 * @param time Address of an epoch time structure.
 *
 * @return 0 if ok, <0 if error.
 */
int epoch_time_set(struct epoch_time *time);

#ifdef __cplusplus
}
#endif

#endif
