/*
 * Copyright (c) 2022 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LIB_LIBC_MINIMAL_INCLUDE_SYS_STAT_H_
#define ZEPHYR_LIB_LIBC_MINIMAL_INCLUDE_SYS_STAT_H_

#ifndef CONFIG_POSIX_API
#pragma message("#include <sys/stat.h> without CONFIG_POSIX_API is deprecated. "                   \
		"Please use CONFIG_POSIX_API or #include <zephyr/posix/sys/stat.h>")
#endif
#include <zephyr/posix/sys/stat.h>

#endif /* ZEPHYR_LIB_LIBC_MINIMAL_INCLUDE_SYS_STAT_H_ */
