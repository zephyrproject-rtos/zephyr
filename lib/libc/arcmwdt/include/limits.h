/*
 * Copyright (c) 2023 Synopsys
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIB_LIBC_ARCMWDT_INCLUDE_LIMITS_H_
#define LIB_LIBC_ARCMWDT_INCLUDE_LIMITS_H_

#include_next <limits.h>
#include <posix_limits.h>

#ifndef PATH_MAX
#define PATH_MAX 256
#endif

#endif  /* LIB_LIBC_ARCMWDT_INCLUDE_LIMITS_H_ */
