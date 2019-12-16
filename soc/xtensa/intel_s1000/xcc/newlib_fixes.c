/*
 * Copyright (c) 2019, Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <toolchain.h>
#include <stdio.h>
#include <stddef.h>
#include <unistd.h>
#include <sys/stat.h>

#include <reent.h>

/* these externs are from lib/libc/newlib/libc-hooks.c */
extern int _read(int fd, char *buf, int nbytes);
extern int _write(int fd, const void *buf, int nbytes);
extern int _open(const char *name, int mode);
extern int _close(int file);
extern int _lseek(int file, int ptr, int dir);
extern int _isatty(int file);
extern int _kill(int i, int j);
extern int _getpid(void);
extern int _fstat(int file, struct stat *st);
extern void _exit(int status);
extern void *_sbrk(int count);

_ssize_t _read_r(struct _reent *r, int fd, void *buf, size_t nbytes)
{
	ARG_UNUSED(r);

	return _read(fd, (char *)buf, nbytes);
}

_ssize_t _write_r(struct _reent *r, int fd, const void *buf, size_t nbytes)
{
	ARG_UNUSED(r);

	return _write(fd, buf, nbytes);
}

int _open_r(struct _reent *r, const char *name, int flags, int mode)
{
	ARG_UNUSED(r);
	ARG_UNUSED(flags);

	return _open(name, mode);
}

int _close_r(struct _reent *r, int file)
{
	ARG_UNUSED(r);

	return _close(file);
}

_off_t _lseek_r(struct _reent *r, int file, _off_t ptr, int dir)
{
	ARG_UNUSED(r);

	return _lseek(file, ptr, dir);
}

int _isatty_r(struct _reent *r, int file)
{
	ARG_UNUSED(r);

	return _isatty(file);
}

int _kill_r(struct _reent *r, int i, int j)
{
	ARG_UNUSED(r);

	return _kill(i, j);
}

int _getpid_r(struct _reent *r)
{
	ARG_UNUSED(r);

	return _getpid();
}

int _fstat_r(struct _reent *r, int file, struct stat *st)
{
	ARG_UNUSED(r);

	return _fstat(file, st);
}

void _exit_r(struct _reent *r, int status)
{
	ARG_UNUSED(r);

	_exit(status);
}

void *_sbrk_r(struct _reent *r, int count)
{
	ARG_UNUSED(r);

	return _sbrk(count);
}
