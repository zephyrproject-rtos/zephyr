/*
 * Copyright (c) 2021 Synopsys
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LIB_LIBC_ARCMWDT_INCLUDE_FCNTL_H_
#define ZEPHYR_LIB_LIBC_ARCMWDT_INCLUDE_FCNTL_H_

#include_next <fcntl.h>

#define F_DUPFD 0
#define F_GETFL 3
#define F_SETFL 4

/*
 * MWDT fcntl.h doesn't provide O_NONBLOCK, however it provides other file IO
 * definitions. Let's define O_NONBLOCK and check that it doesn't overlap with
 * any other file IO defines.
 */
#ifndef O_NONBLOCK
  #define O_NONBLOCK 0x4000
  #if O_NONBLOCK & (O_RDONLY | O_WRONLY | O_RDWR | O_NDELAY | O_CREAT | O_APPEND | O_TRUNC | O_EXCL)
    #error "O_NONBLOCK conflicts with other O_*** file IO defines!"
  #endif
#endif

#endif /* ZEPHYR_LIB_LIBC_ARCMWDT_INCLUDE_FCNTL_H_ */
