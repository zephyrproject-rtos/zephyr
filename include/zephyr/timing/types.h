/*
 * Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_TIMING_TYPES_H_
#define ZEPHYR_INCLUDE_TIMING_TYPES_H_

#ifdef CONFIG_ARCH_POSIX
#include <time.h>
typedef struct timespec timing_t;
#else
typedef uint64_t timing_t;
#endif

#endif /* ZEPHYR_INCLUDE_TIMING_TYPES_H_ */
