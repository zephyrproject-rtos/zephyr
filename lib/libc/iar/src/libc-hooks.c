/*
 * Copyright (c) 2025 IAR Systems AB
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <stdio.h>
#include <LowLevelIOInterface.h>

static int _stdout_hook_default(int c)
{
	(void)(c); /* Prevent warning about unused argument */

	return EOF;
}

static int (*_stdout_hook)(int) = _stdout_hook_default;

void __stdout_hook_install(int (*hook)(int c))
{
	_stdout_hook = hook;
}

int fputc(int c, FILE *f)
{
	return (_stdout_hook)(c);
}

#pragma weak __write
size_t __write(int handle, const unsigned char *buf, size_t bufSize)
{
	size_t nChars = 0;
	/* Check for the command to flush all handles */
	if (handle == -1) {
		return 0;
	}
	/* Check for stdout and stderr
	 * (only necessary if FILE descriptors are enabled.)
	 */
	if (handle != 1 && handle != 2) {
		return -1;
	}
	for (/* Empty */; bufSize > 0; --bufSize) {
		int ret = (_stdout_hook)(*buf);

		if (ret == EOF) {
			break;
		}
		++buf;
		++nChars;
	}
	return nChars;
}

/*
 * The remaining low-level I/O functions of the DLIB runtime are referenced by
 * the file I/O parts of the library, which are pulled in by e.g. the C++
 * stream classes. There is no file system support, so these fail (or report
 * end-of-file on stdin).
 */

#pragma weak __read
size_t __read(int handle, unsigned char *buf, size_t buf_size)
{
	ARG_UNUSED(buf);
	ARG_UNUSED(buf_size);

	if (handle == _LLIO_STDIN) {
		return 0;
	}

	return _LLIO_ERROR;
}

#pragma weak __lseek
long __lseek(int handle, long offset, int whence)
{
	ARG_UNUSED(handle);
	ARG_UNUSED(offset);
	ARG_UNUSED(whence);

	return -1;
}

#pragma weak __close
int __close(int handle)
{
	ARG_UNUSED(handle);

	return 0;
}

#pragma weak remove
int remove(const char *filename)
{
	ARG_UNUSED(filename);

	return -1;
}
