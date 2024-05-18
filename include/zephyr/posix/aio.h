/*
 * Copyright 2024 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ZEPHYR_POSIX_AIO_H_
#define ZEPHYR_INCLUDE_ZEPHYR_POSIX_AIO_H_

#include <signal.h>
#include <sys/types.h>
#include <time.h>
#include <sys/features.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_POSIX_ASYNCHRONOUS_IO) || defined(__DOXYGEN__)

struct aiocb {
	int aio_fildes;
	off_t aio_offset;
	volatile void *aio_buf;
	size_t aio_nbytes;
	int aio_reqprio;
#if defined(_POSIX_REALTIME_SIGNALS) || (_POSIX_VERSION >= 199309L)
	struct sigevent aio_sigevent;
#endif /* #if defined(_POSIX_REALTIME_SIGNALS) || (_POSIX_VERSION >= 199309L) */
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

#endif /* defined(_POSIX_ASYNCHRONOUS_IO) || defined(__DOXYGEN__) */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ZEPHYR_POSIX_AIO_H_ */
