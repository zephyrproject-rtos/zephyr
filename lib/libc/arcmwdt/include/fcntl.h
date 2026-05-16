/*
 * Copyright (c) 2021 Synopsys
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LIB_LIBC_ARCMWDT_INCLUDE_FCNTL_H_
#define ZEPHYR_LIB_LIBC_ARCMWDT_INCLUDE_FCNTL_H_

#include_next <fcntl.h>
#include <zephyr/sys/fdtable.h>

#define F_DUPFD 0
#if defined(F_GETFL) && F_GETFL != ZVFS_F_GETFL
#undef F_GETFL
#endif
#ifndef F_GETFL
#define F_GETFL ZVFS_F_GETFL
#endif

#if defined(F_SETFL) && F_SETFL != ZVFS_F_SETFL
#undef F_SETFL
#endif
#ifndef F_SETFL
#define F_SETFL ZVFS_F_SETFL
#endif

/*
 * MWDT fcntl.h doesn't provide O_NONBLOCK, however it provides other file IO
 * definitions. Let's define O_NONBLOCK and check that it doesn't overlap with
 * any other file IO defines.
 */
#if defined(O_NONBLOCK) && O_NONBLOCK != ZVFS_O_NONBLOCK
#undef O_NONBLOCK
#endif
#ifndef O_NONBLOCK
  #define O_NONBLOCK ZVFS_O_NONBLOCK
#endif
#if O_NONBLOCK & (O_RDONLY | O_WRONLY | O_RDWR | O_NDELAY | O_CREAT | O_APPEND | O_TRUNC | O_EXCL)
  #error "O_NONBLOCK conflicts with other O_*** file IO defines!"
#endif

#endif /* ZEPHYR_LIB_LIBC_ARCMWDT_INCLUDE_FCNTL_H_ */
