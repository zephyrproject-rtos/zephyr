/*
 * Copyright (c) 2025  Ahmed Adel <a.adel2010@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdio.h>
#include <stdlib.h>
#include <zephyr/posix/fcntl.h>
#include <errno.h>

static int mode2flags(const char *mode)
{
	int flags = 0;

	switch (mode[0]) {
	case 'r':
		flags = O_RDONLY;
		if (mode[1] == '+') {
			flags = O_RDWR;
		}
		break;
	case 'w':
		flags = O_WRONLY | O_CREAT | O_TRUNC;
		if (mode[1] == '+') {
			flags = O_RDWR | O_CREAT | O_TRUNC;
		}
		break;
	case 'a':
		flags = O_WRONLY | O_CREAT | O_APPEND;
		if (mode[1] == '+') {
			flags = O_RDWR | O_CREAT | O_APPEND;
		}
		break;
	default:
		errno = EINVAL;
		return -1;
	}

	return flags;
}

FILE *fopen(const char *ZRESTRICT filename, const char *ZRESTRICT mode)
{
	int flags = 0;
	int fd = 0;
	FILE *stream = NULL;

	if (filename == NULL || mode == NULL) {
		errno = EINVAL;
		return NULL;
	}

	flags = mode2flags(mode);
	if (flags < 0) {
		errno = EINVAL;
		return NULL;
	}

	fd = open(filename, flags);
	if (fd < 0) {
		errno = EIO;
		return NULL;
	}

	stream = (FILE *)calloc(1, sizeof(FILE));
	if (stream != NULL) {
		stream->_file = fd;
	} else {
		close(fd);
		errno = ENOMEM;
	}

	return stream;
}

int fclose(FILE *stream)
{
	if (stream == NULL) {
		errno = EINVAL;
		return EOF;
	}

	int fd = stream->_file;
	int rc = close(fd);

	if (rc < 0) {
		errno = EIO;
		return EOF;
	}

	free((void *)stream);

	return 0;
}
