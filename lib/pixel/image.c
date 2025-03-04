/*
 * Copyright (c) 2025 tinyVision.ai Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/pixel/image.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(pixel_image, CONFIG_PIXEL_LOG_LEVEL);

K_HEAP_DEFINE(pixel_heap, CONFIG_PIXEL_HEAP_SIZE);

static void pixel_image_free(struct pixel_image *img)
{
	struct pixel_operation *op, *tmp;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&img->operations, op, tmp, node) {
		if (op->is_heap) {
			k_heap_free(&pixel_heap, op->ring.buffer);
		}
		memset(op, 0x00, sizeof(*op));
		k_heap_free(&pixel_heap, op);
	}
	memset(&img->operations, 0x00, sizeof(img->operations));
}

int pixel_image_error(struct pixel_image *img, int err)
{
	if (err != 0 && img->err == 0) {
		pixel_image_free(img);
		img->err = err;
	}
	return err;
}

static void *pixel_image_alloc(struct pixel_image *img, size_t size)
{
	void *mem;

	mem = k_heap_alloc(&pixel_heap, size, K_NO_WAIT);
	if (mem == NULL) {
		LOG_ERR("Out of memory whle allocating %zu bytes", size);
	}
	return mem;
}

int pixel_image_add_operation(struct pixel_image *img, const struct pixel_operation *template,
			      size_t buffer_size, size_t threshold)
{
	struct pixel_operation *op;

	if (img->err) {
		return -ECANCELED;
	}

	if (template->fourcc_in != img->fourcc) {
		LOG_ERR("Wrong format for this operation: image has %s, operation uses %s",
			VIDEO_FOURCC_TO_STR(template->fourcc_in), VIDEO_FOURCC_TO_STR(img->fourcc));
		return pixel_image_error(img, -EINVAL);
	}

	op = pixel_image_alloc(img, sizeof(*op));
	if (op == NULL) {
		return pixel_image_error(img, -ENOMEM);
	}

	memcpy(op, template, sizeof(*op));
	op->threshold = threshold;
	op->width = img->width;
	op->height = img->height;
	op->ring.buffer = NULL; /* allocated later */
	op->ring.size = buffer_size;

	img->fourcc = op->fourcc_out;

	sys_slist_append(&img->operations, &op->node);

	return 0;
}

int pixel_image_add_uncompressed(struct pixel_image *img, const struct pixel_operation *template)
{
	size_t bpp = video_bits_per_pixel(img->fourcc);
	size_t size = template->window_size * img->width * bpp / BITS_PER_BYTE;

	__ASSERT(bpp > 0, "Unknown image pitch for format %s.", VIDEO_FOURCC_TO_STR(img->fourcc));

	return pixel_image_add_operation(img, template, size, size);
}

int pixel_image_process(struct pixel_image *img)
{
	struct pixel_operation *op;
	uint8_t *p;

	if (img->err) {
		return -ECANCELED;
	}

	if (img->buffer == NULL) {
		LOG_ERR("No input buffer configured");
		return pixel_image_error(img, -ENOBUFS);
	}

	op = SYS_SLIST_PEEK_HEAD_CONTAINER(&img->operations, op, node);
	if (op == NULL) {
		LOG_ERR("No operation to perform on image");
		return pixel_image_error(img, -ENOSYS);
	}

	if (ring_buf_capacity_get(&op->ring) < op->ring.size) {
		LOG_ERR("Not enough space (%u) in input buffer to run the first operation (%u)",
			ring_buf_capacity_get(&op->ring), op->ring.size);
		return pixel_image_error(img, -ENOSPC);
	}

	ring_buf_init(&op->ring, img->size, img->buffer);
	ring_buf_put_claim(&op->ring, &p, img->size);
	ring_buf_put_finish(&op->ring, img->size);

	while ((op = SYS_SLIST_PEEK_NEXT_CONTAINER(op, node)) != NULL) {
		if (op->ring.buffer == NULL) {
			op->ring.buffer = pixel_image_alloc(img, op->ring.size);
			if (op->ring.buffer == NULL) {
				return pixel_image_error(img, -ENOMEM);
			}
			op->is_heap = true;
		}
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&img->operations, op, node) {
		LOG_DBG("- %s %ux%u to %s, %s, threshold %u",
			VIDEO_FOURCC_TO_STR(op->fourcc_in), op->width, op->height,
			VIDEO_FOURCC_TO_STR(op->fourcc_out), op->name, op->threshold);
	}

	op = SYS_SLIST_PEEK_HEAD_CONTAINER(&img->operations, op, node);

	pixel_operation_run(op);
	pixel_image_free(img);

	return 0;
}

void pixel_image_from_buffer(struct pixel_image *img, uint8_t *buffer, size_t size,
			     uint16_t width, uint16_t height, uint32_t fourcc)
{
	memset(img, 0x00, sizeof(*img));
	img->buffer = buffer;
	img->size = size;
	img->width = width;
	img->height = height;
	img->fourcc = fourcc;
}

void pixel_image_from_vbuf(struct pixel_image *img, struct video_buffer *vbuf,
			   struct video_format *fmt)
{
	pixel_image_from_buffer(img, vbuf->buffer, vbuf->size, fmt->width, fmt->height,
				fmt->pixelformat);
}

int pixel_image_to_buffer(struct pixel_image *img, uint8_t *buffer, size_t size)
{
	struct pixel_operation *op;
	int ret;

	if (img->err) {
		return -ECANCELED;
	}

	op = pixel_image_alloc(img, sizeof(struct pixel_operation));
	if (op == NULL) {
		return pixel_image_error(img, -ENOMEM);
	}

	memset(op, 0x00, sizeof(*op));
	op->name = __func__;
	op->threshold = size;
	op->fourcc_in = img->fourcc;
	op->fourcc_out = img->fourcc;
	op->width = img->width;
	op->height = img->height;
	op->ring.buffer = buffer;
	op->ring.size = size;

	sys_slist_append(&img->operations, &op->node);

	ret = pixel_image_process(img);

	img->buffer = buffer;
	img->size = size;

	return ret;
}

int pixel_image_to_vbuf(struct pixel_image *img, struct video_buffer *vbuf)
{
	int ret;

	ret = pixel_image_to_buffer(img, vbuf->buffer, vbuf->size);
	vbuf->bytesused = img->size;

	return ret;
}
