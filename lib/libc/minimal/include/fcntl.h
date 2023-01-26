/*
 * Copyright (c) 2022 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LIB_LIBC_MINIMAL_INCLUDE_FCNTL_H_
#define ZEPHYR_LIB_LIBC_MINIMAL_INCLUDE_FCNTL_H_

#ifndef CONFIG_POSIX_API
#pragma message("#include <fcntl.h> without CONFIG_POSIX_API is deprecated. "                      \
		"Please use CONFIG_POSIX_API or #include <zephyr/posix/fcntl.h>")
#endif
#include <zephyr/posix/fcntl.h>

#endif /* ZEPHYR_LIB_LIBC_MINIMAL_INCLUDE_SYS_FCNTL_H_ */
