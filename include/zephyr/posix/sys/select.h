/*
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_POSIX_SYS_SELECT_H_
#define ZEPHYR_INCLUDE_POSIX_SYS_SELECT_H_

#include <zephyr/posix/posix_types.h>
#include <zephyr/sys/fdtable.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FD_SETSIZE ZVFS_FD_SETSIZE

typedef struct zvfs_fd_set fd_set;

struct timeval;

int pselect(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
	    const struct timespec *timeout, const void *sigmask);
int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *errorfds, struct timeval *timeout);
void FD_CLR(int fd, fd_set *fdset);
int FD_ISSET(int fd, fd_set *fdset);
void FD_SET(int fd, fd_set *fdset);
void FD_ZERO(fd_set *fdset);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_POSIX_SYS_SELECT_H_ */
