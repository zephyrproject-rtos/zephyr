/*
 * Copyright 2024 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Asynchronous input and output.
 * @ingroup posix
 *
 * Provides the aiocb control block and the asynchronous I/O functions that
 * allow read, write, and fsync operations to proceed in the background.
 *
 * @note In Zephyr, the asynchronous I/O functions are currently stubs that fail
 * with @c errno set to @c ENOSYS.
 *
 * @posix_header{aio.h}
 */

#ifndef ZEPHYR_INCLUDE_POSIX_AIO_H_
#define ZEPHYR_INCLUDE_POSIX_AIO_H_

#include <signal.h>
#include <sys/types.h>
#include <time.h>

#include <zephyr/toolchain.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Asynchronous I/O control block.
 */
struct aiocb {
	int aio_fildes;               /**< File descriptor. */
	off_t aio_offset;             /**< File offset. */
	volatile void *aio_buf;       /**< Location of buffer. */
	size_t aio_nbytes;            /**< Length of transfer. */
	int aio_reqprio;              /**< Request priority offset. */
	struct sigevent aio_sigevent; /**< Signal number and value. */
	int aio_lio_opcode;           /**< Operation to be performed. */
};

#if _POSIX_C_SOURCE >= 200112L

/**
 * @brief Cancel an asynchronous I/O request.
 *
 * @param fildes File descriptor.
 * @param aiocbp Control block to cancel, or NULL to cancel all requests for @p fildes.
 *
 * @return -1 with errno set on failure.
 * @retval AIO_CANCELED    All requested operations were canceled.
 * @retval AIO_NOTCANCELED At least one operation could not be canceled.
 * @retval AIO_ALLDONE     All operations had already completed before the call.
 *
 * @posix_func{aio_cancel}
 */
int aio_cancel(int fildes, struct aiocb *aiocbp);

/**
 * @brief Retrieve error status for an asynchronous I/O operation.
 *
 * @param aiocbp Asynchronous I/O control block.
 *
 * @return 0 on success, or a positive error number on failure.
 * @retval EINPROGRESS Operation is still running.
 *
 * @posix_func{aio_error}
 */
int aio_error(const struct aiocb *aiocbp);

/**
 * @brief Asynchronously synchronize a file's data and metadata to storage.
 *
 * @param op     Completion guarantee to request: @c O_SYNC (data and metadata) or @c O_DSYNC
 *               (data only).
 * @param aiocbp Control block specifying the file descriptor.
 *
 * @return 0 if the request was successfully queued, or -1 with errno set on failure.
 *
 * @posix_func{aio_fsync}
 */
int aio_fsync(int op, struct aiocb *aiocbp);

/**
 * @brief Enqueue an asynchronous read operation.
 *
 * @param aiocbp Control block specifying the file, offset, buffer, and size.
 *
 * @return 0 if the request was successfully queued, or -1 with errno set on failure.
 *
 * @posix_func{aio_read}
 */
int aio_read(struct aiocb *aiocbp);

/**
 * @brief Retrieve the return status of a completed asynchronous I/O operation.
 *
 * Must be called exactly once after aio_error() returns a value other than
 * @c EINPROGRESS. Calling it a second time yields undefined behavior.
 *
 * @param aiocbp Asynchronous I/O control block.
 *
 * @return Number of bytes transferred on success, or -1 with errno set on failure.
 *
 * @posix_func{aio_return}
 */
ssize_t aio_return(struct aiocb *aiocbp);

/**
 * @brief Wait for one or more asynchronous I/O requests to complete.
 *
 * @param list    Array of pointers to control blocks to wait on.
 * @param nent    Number of entries in @p list.
 * @param timeout Maximum wait time, or NULL to block indefinitely.
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @posix_func{aio_suspend}
 */
int aio_suspend(const struct aiocb *const list[], int nent, const struct timespec *timeout);

/**
 * @brief Enqueue an asynchronous write operation.
 *
 * @param aiocbp Control block specifying the file, offset, buffer, and size.
 *
 * @return 0 if the request was successfully queued, or -1 with errno set on failure.
 *
 * @posix_func{aio_write}
 */
int aio_write(struct aiocb *aiocbp);

/**
 * @brief Initiate a list of asynchronous I/O requests.
 *
 * @param mode @c LIO_WAIT (block until all complete) or @c LIO_NOWAIT (return immediately).
 * @param list Array of control block pointers (@c LIO_NOP entries are skipped).
 * @param nent Number of entries in @p list.
 * @param sig  Notification on completion (only for @c LIO_NOWAIT), or NULL.
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @posix_func{lio_listio}
 */
int lio_listio(int mode, struct aiocb *const ZRESTRICT list[], int nent,
	       struct sigevent *ZRESTRICT sig);

#endif /* _POSIX_C_SOURCE >= 200112L */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_POSIX_AIO_H_ */
