/*
 * Copyright (c) 2023 Intel
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LIB_LIBC_ARMSTDC_INCLUDE_LIMITS_H_
#define ZEPHYR_LIB_LIBC_ARMSTDC_INCLUDE_LIMITS_H_

#include_next <limits.h>
#include <zephyr/posix/posix_limits.h>

#ifndef PATH_MAX
#define PATH_MAX 256
#endif

#endif  /* ZEPHYR_LIB_LIBC_ARMSTDC_INCLUDE_LIMITS_H_ */
