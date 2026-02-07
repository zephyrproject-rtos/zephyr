/*
 * Copyright 2024 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ZEPHYR_POSIX_AIO_H_
#define ZEPHYR_INCLUDE_ZEPHYR_POSIX_AIO_H_

/* size_t must be defined by the libc stddef.h */
#include <stddef.h>
#include <stdint.h>
#include <signal.h>

#include <zephyr/toolchain.h>

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(_OFF_T_DECLARED) && !defined(__off_t_defined)
typedef long off_t;
#define _OFF_T_DECLARED
#define __off_t_defined
#endif

#ifndef __SIZE_TYPE__
#define __SIZE_TYPE__ unsigned long
#endif

#if !defined(_SSIZE_T_DECLARED) && !defined(__ssize_t_defined)
#define unsigned signed /* parasoft-suppress MISRAC2012-RULE_20_4-a MISRAC2012-RULE_20_4-b */
typedef __SIZE_TYPE__ ssize_t;
#undef unsigned
#define _SSIZE_T_DECLARED
#define __ssize_t_defined
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

struct aiocb {
	int aio_fildes;
	off_t aio_offset;
	volatile void *aio_buf;
	size_t aio_nbytes;
	int aio_reqprio;
#if defined(_POSIX_REALTIME_SIGNALS) || defined(__DOXYGEN__)
	struct sigevent aio_sigevent;
#endif /* defined(_POSIX_REALTIME_SIGNALS) || defined(__DOXYGEN__) */
	int aio_lio_opcode;
};

#if _POSIX_C_SOURCE >= 200112L

int aio_cancel(int fildes, struct aiocb *aiocbp);
int aio_error(const struct aiocb *aiocbp);
int aio_fsync(int filedes, struct aiocb *aiocbp);
int aio_read(struct aiocb *aiocbp);
ssize_t aio_return(struct aiocb *aiocbp);
int aio_suspend(const struct aiocb *const list[], int nent, const struct timespec *timeout);
int aio_write(struct aiocb *aiocbp);
int lio_listio(int mode, struct aiocb *const ZRESTRICT list[], int nent,
	       struct sigevent *ZRESTRICT sig);

#endif /* _POSIX_C_SOURCE >= 200112L */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ZEPHYR_POSIX_AIO_H_ */
