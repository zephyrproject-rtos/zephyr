/*
 * Copyright (c) 2018 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <sys/types.h>

#include <net/zstream.h>

ssize_t zstream_writeall(struct zstream *stream, const void *buf, size_t size,
			 size_t *written)
{
	const char *p = buf;
	size_t cur_sz = 0;

	do {
		ssize_t out_sz = zstream_write(stream, p, size);

		if (out_sz == -1) {
			if (written) {
				*written = cur_sz;
			}

			return -1;
		}

		cur_sz += out_sz;
		size -= out_sz;
		p += out_sz;
	} while (size);

	if (written) {
		*written = cur_sz;
	}

	return cur_sz;
}
