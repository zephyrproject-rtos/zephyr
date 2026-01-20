/*
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_POSIX_SYS_SELECT_H_
#define ZEPHYR_INCLUDE_POSIX_SYS_SELECT_H_

#include <zephyr/sys/fdtable.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FD_SETSIZE ZVFS_FD_SETSIZE

#if !defined(_SIGSET_T_DECLARED) && !defined(__sigset_t_defined)

#ifndef SIGRTMIN
#define SIGRTMIN 32
#endif
#if defined(_POSIX_REALTIME_SIGNALS) || defined(__DOXYGEN__)
BUILD_ASSERT(CONFIG_POSIX_RTSIG_MAX >= 0);
#define SIGRTMAX (SIGRTMIN + CONFIG_POSIX_RTSIG_MAX)
#else
#define SIGRTMAX SIGRTMIN
#endif

typedef struct {
	unsigned long sig[DIV_ROUND_UP(SIGRTMAX + 1, BITS_PER_LONG)];
} sigset_t;
#define _SIGSET_T_DECLARED
#define __sigset_t_defined
#endif

#if !defined(_SUSECONDS_T_DECLARED) && !defined(__suseconds_t_defined)
typedef long suseconds_t;
#define _SUSECONDS_T_DECLARED
#define __suseconds_t_defined
#endif

/* time_t must be defined by the libc time.h */
#include <time.h>

#if __STDC_VERSION__ >= 201112L
/* struct timespec must be defined in the libc time.h */
#else
#if !defined(_TIMESPEC_DECLARED) && !defined(__timespec_defined)
struct timespec {
	time_t tv_sec;
	long tv_nsec;
};
#define _TIMESPEC_DECLARED
#define __timespec_defined
#endif
#endif

#if !defined(_TIMEVAL_DECLARED) && !defined(__timeval_defined)
struct timeval {
	time_t tv_sec;
	suseconds_t tv_usec;
};
#define _TIMEVAL_DECLARED
#define __timeval_defined
#endif

typedef struct zvfs_fd_set fd_set;

struct timeval;

int pselect(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
	    const struct timespec *timeout, const sigset_t *sigmask);
int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *errorfds, struct timeval *timeout);
void FD_CLR(int fd, fd_set *fdset);
int FD_ISSET(int fd, fd_set *fdset);
void FD_SET(int fd, fd_set *fdset);
void FD_ZERO(fd_set *fdset);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_POSIX_SYS_SELECT_H_ */
