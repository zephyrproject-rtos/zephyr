/**
 * @file
 * @brief Network stream API definitions
 *
 * An API to abstract different transport protocols for SOCK_STREAMs, etc.
 */

/*
 * Copyright (c) 2018 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __NET_ZSTREAM_H
#define __NET_ZSTREAM_H

#include <sys/types.h>

struct zstream_api;

struct zstream {
	const struct zstream_api *api;
};

struct zstream_api {
	ssize_t (*read)(struct zstream *stream, void *buf, size_t size);
	ssize_t (*write)(struct zstream *stream, const void *buf, size_t size);
	int (*flush)(struct zstream *stream);
	int (*close)(struct zstream *stream);
};

static inline ssize_t zstream_read(struct zstream *stream, void *buf,
				   size_t size)
{
	return stream->api->read(stream, buf, size);
}

static inline ssize_t zstream_write(struct zstream *stream, const void *buf,
				    size_t size)
{
	return stream->api->write(stream, buf, size);
}

ssize_t zstream_writeall(struct zstream *stream, const void *buf, size_t size,
			 size_t *written);

static inline ssize_t zstream_flush(struct zstream *stream)
{
	return stream->api->flush(stream);
}

static inline ssize_t zstream_close(struct zstream *stream)
{
	return stream->api->close(stream);
}

/* Stream object implementation for socket. */
struct zstream_sock {
	const struct zstream_api *api;
	int fd;
};

int zstream_sock_init(struct zstream_sock *self, int fd);

#endif /* __NET_ZSTREAM_H */
