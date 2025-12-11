/*
 * Copyright 2024 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <signal.h>

#include <zephyr/posix/aio.h>

int aio_cancel(int fildes, struct aiocb *aiocbp)
{
	ARG_UNUSED(fildes);
	ARG_UNUSED(aiocbp);

	errno = ENOSYS;
	return -1;
}

int aio_error(const struct aiocb *aiocbp)
{
	ARG_UNUSED(aiocbp);

	errno = ENOSYS;
	return -1;
}

int aio_fsync(int fildes, struct aiocb *aiocbp)
{
	ARG_UNUSED(fildes);
	ARG_UNUSED(aiocbp);

	errno = ENOSYS;
	return -1;
}

int aio_read(struct aiocb *aiocbp)
{
	ARG_UNUSED(aiocbp);

	errno = ENOSYS;
	return -1;
}

ssize_t aio_return(struct aiocb *aiocbp)
{
	ARG_UNUSED(aiocbp);

	errno = ENOSYS;
	return -1;
}

int aio_suspend(const struct aiocb *const list[], int nent, const struct timespec *timeout)
{
	ARG_UNUSED(list);
	ARG_UNUSED(nent);
	ARG_UNUSED(timeout);

	errno = ENOSYS;
	return -1;
}

int aio_write(struct aiocb *aiocbp)
{
	ARG_UNUSED(aiocbp);

	errno = ENOSYS;
	return -1;
}

int lio_listio(int mode, struct aiocb *const ZRESTRICT list[], int nent,
	       struct sigevent *ZRESTRICT sig)
{
	ARG_UNUSED(mode);
	ARG_UNUSED(list);
	ARG_UNUSED(nent);
	ARG_UNUSED(sig);

	errno = ENOSYS;
	return -1;
}
