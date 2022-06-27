/*
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_POSIX_SYS_SELECT_H_
#define ZEPHYR_INCLUDE_POSIX_SYS_SELECT_H_

#include <zephyr/net/socket_select.h>
#include <sys/_timeval.h>

#define fd_set zsock_fd_set
#define FD_SETSIZE ZSOCK_FD_SETSIZE
#define FD_ZERO ZSOCK_FD_ZERO
#define FD_SET ZSOCK_FD_SET
#define FD_CLR ZSOCK_FD_CLR
#define FD_ISSET ZSOCK_FD_ISSET

struct timeval;

static inline int select(int nfds, fd_set *readfds,
			 fd_set *writefds, fd_set *exceptfds,
			 struct timeval *timeout)
{
	return zsock_select(nfds, readfds, writefds, exceptfds,
			    (struct zsock_timeval *)timeout);
}

#endif /* ZEPHYR_INCLUDE_POSIX_SYS_SELECT_H_ */
