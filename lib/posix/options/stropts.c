/*
 * Copyright (c) 2024 Abhinav Srivastava
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/posix/stropts.h>
#include <errno.h>
#include <zephyr/kernel.h>

int putmsg(int fildes, const struct strbuf *ctlptr, const struct strbuf *dataptr, int flags)
{
	ARG_UNUSED(fildes);
	ARG_UNUSED(ctlptr);
	ARG_UNUSED(dataptr);
	ARG_UNUSED(flags);

	errno = ENOSYS;
	return -1;
}

int fdetach(const char *path)
{
	ARG_UNUSED(path);

	errno = ENOSYS;
	return -1;
}

int fattach(int fildes, const char *path)
{
	ARG_UNUSED(fildes);
	ARG_UNUSED(path);
	errno = ENOSYS;

	return -1;
}

int getmsg(int fildes, struct strbuf *ctlptr, struct strbuf *dataptr, int *flagsp)
{
	ARG_UNUSED(fildes);
	ARG_UNUSED(ctlptr);
	ARG_UNUSED(dataptr);
	ARG_UNUSED(flagsp);

	errno = ENOSYS;
	return -1;
}

int getpmsg(int fildes, struct strbuf *ctlptr, struct strbuf *dataptr, int *bandp, int *flagsp)
{
	ARG_UNUSED(fildes);
	ARG_UNUSED(ctlptr);
	ARG_UNUSED(dataptr);
	ARG_UNUSED(bandp);
	ARG_UNUSED(flagsp);

	errno = ENOSYS;
	return -1;
}

int isastream(int fildes)
{
	ARG_UNUSED(fildes);

	errno = ENOSYS;
	return -1;
}
