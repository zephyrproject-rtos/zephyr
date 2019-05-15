/*
 * Copyright (C) 2019 Thomas Burdick <thomas.burdick@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zio/zio_fifo_buf.h>

static int zio_fifo_buf_pull(struct zio_buf *buf, void *datum)
{
	struct z_zio_fifo_buf *fifo_buf = buf->api_data;
	struct zio_fifo *fifo = fifo_buf->fifo;

	if (z_zio_fifo_used(fifo) <= 0) {
		return 0;
	}

	return z_zio_fifo_pull(fifo, datum);
}

static int zio_fifo_buf_set_watermark(struct zio_buf *buf, u32_t watermark)
{
	struct z_zio_fifo_buf *fifo_buf = buf->api_data;
	struct zio_fifo *fifo = fifo_buf->fifo;

	if (watermark > z_zio_fifo_size(fifo)) {
		return -EINVAL;
	}
	fifo_buf->watermark = watermark;
	return 0;
}

static u32_t zio_fifo_buf_get_watermark(struct zio_buf *buf)
{
	struct z_zio_fifo_buf *fifo_buf = buf->api_data;

	return fifo_buf->watermark;
}

static u32_t zio_fifo_buf_get_length(struct zio_buf *buf)
{
	struct z_zio_fifo_buf *fifo_buf = buf->api_data;
	struct zio_fifo *fifo = fifo_buf->fifo;

	return z_zio_fifo_used(fifo);
}

static u32_t zio_fifo_buf_get_capacity(struct zio_buf *buf)
{
	struct z_zio_fifo_buf *fifo_buf = buf->api_data;
	struct zio_fifo *fifo = fifo_buf->fifo;

	return z_zio_fifo_size(fifo);
}


struct zio_buf_api zio_fifo_buf_api = {
	.pull = zio_fifo_buf_pull,
	.set_watermark = zio_fifo_buf_set_watermark,
	.get_watermark = zio_fifo_buf_get_watermark,
	.get_length = zio_fifo_buf_get_length,
	.get_capacity = zio_fifo_buf_get_capacity
};
