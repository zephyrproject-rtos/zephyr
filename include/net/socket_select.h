/*
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_SOCKET_SELECT_H_
#define ZEPHYR_INCLUDE_NET_SOCKET_SELECT_H_

#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct zsock_timeval {
	/* Using longs, as many (?) implementations seem to use it. */
	long tv_sec;
	long tv_usec;
};

typedef struct zsock_fd_set {
	u32_t bitset[(CONFIG_POSIX_MAX_FDS + 31) / 32];
} zsock_fd_set;

/* select() API is inefficient, and implemented as inefficient wrapper on
 * top of poll(). Avoid select(), use poll directly().
 */
int zsock_select(int nfds, zsock_fd_set *readfds, zsock_fd_set *writefds,
		 zsock_fd_set *exceptfds, struct zsock_timeval *timeout);

#define ZSOCK_FD_SETSIZE (sizeof(((zsock_fd_set *)0)->bitset) * 8)

void ZSOCK_FD_ZERO(zsock_fd_set *set);
int ZSOCK_FD_ISSET(int fd, zsock_fd_set *set);
void ZSOCK_FD_CLR(int fd, zsock_fd_set *set);
void ZSOCK_FD_SET(int fd, zsock_fd_set *set);

#ifdef CONFIG_NET_SOCKETS_POSIX_NAMES

#define fd_set zsock_fd_set
#define timeval zsock_timeval
#define FD_SETSIZE ZSOCK_FD_SETSIZE

static inline int select(int nfds, zsock_fd_set *readfds,
			 zsock_fd_set *writefds, zsock_fd_set *exceptfds,
			 struct timeval *timeout)
{
	return zsock_select(nfds, readfds, writefds, exceptfds, timeout);
}

static inline void FD_ZERO(zsock_fd_set *set)
{
	ZSOCK_FD_ZERO(set);
}

static inline int FD_ISSET(int fd, zsock_fd_set *set)
{
	return ZSOCK_FD_ISSET(fd, set);
}

static inline void FD_CLR(int fd, zsock_fd_set *set)
{
	ZSOCK_FD_CLR(fd, set);
}

static inline void FD_SET(int fd, zsock_fd_set *set)
{
	ZSOCK_FD_SET(fd, set);
}

#endif /* CONFIG_NET_SOCKETS_POSIX_NAMES */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_SOCKET_SELECT_H_ */
