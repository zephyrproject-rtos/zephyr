/*
 * Copyright (c) 2019, Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * display_video is a shim layer allowing to expose a display API compatible
 * interface for video (display/output) devices. The video device needs to
 * register display_video* methods as display_drv_api callbacks.
 */

#include <zephyr.h>

#include <display/video.h>

#include <drivers/video.h>
#include <drivers/display.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(display_video, CONFIG_DISPLAY_LOG_LEVEL);

/* Only one double-buffered display instance supported for now */
static struct video_buffer *vbufa, *vbufb;

int display_video_write(const struct device *dev,
			const uint16_t x, const uint16_t y,
			const struct display_buffer_descriptor *desc,
			const void *buf)
{
	struct video_buffer *vbuf;
	struct video_format fmt;
	int err, bpp, loop;
	const uint8_t *src;
	uint8_t *dst;

	video_get_format((struct device *)dev, VIDEO_EP_IN, &fmt);

	/* retrieve available buffer */
	err = video_dequeue((struct device *)dev, VIDEO_EP_IN, &vbuf, K_FOREVER);
	if (err) {
		return err;
	}

	/* LVGL no double buffering aware support ? just copy previous one */
	src = (vbuf == vbufa) ? vbufb->buffer : vbufa->buffer;
	memcpy(vbuf->buffer, src, vbuf->bytesused);

	/* Write data */
	bpp = fmt.pitch / fmt.width;
	src = buf;
	dst = vbuf->buffer +  y * fmt.pitch + x * bpp;
	loop = desc->height;
	while (loop--) {
		memcpy(dst, src, bpp * desc->width);
		src += bpp * desc->pitch;
		dst += fmt.pitch;
	}

	/* requeue buffer for display */
	err = video_enqueue((struct device *)dev, VIDEO_EP_IN, vbuf);
	if (err) {
		return err;
	}

	return 0;
}

int display_video_read(const struct device *dev,
		       const uint16_t x, const uint16_t y,
		       const struct display_buffer_descriptor *desc, void *buf)
{
	return -ENOTSUP;
}

void *display_video_get_framebuffer(const struct device *dev)
{
	return NULL;
}

int display_video_blanking_off(const struct device *dev)
{
	return -ENOTSUP;
}

int display_video_blanking_on(const struct device *dev)
{
	return -ENOTSUP;
}

int display_video_set_brightness(const struct device *dev,
				 const uint8_t brightness)
{
	return -ENOTSUP;
}

int display_video_set_contrast(const struct device *dev, const uint8_t contrast)
{
	return -ENOTSUP;
}

int display_video_set_pixel_format(const struct device *dev,
				   const enum display_pixel_format pixel_format)
{
	struct video_format fmt;

	video_get_format((struct device *)dev, VIDEO_EP_IN, &fmt);

	switch (pixel_format) {
	case PIXEL_FORMAT_RGB_565:
		fmt.pixelformat = VIDEO_PIX_FMT_RGB565;
		break;
	default:
		return -ENOTSUP;
	}

	return video_set_format((struct device *)dev, VIDEO_EP_IN, &fmt);
}

int display_video_set_orientation(const struct device *dev,
				  const enum display_orientation orientation)
{
	if (orientation == DISPLAY_ORIENTATION_NORMAL) {
		return 0;
	}

	return -ENOTSUP;
}

void display_video_get_capabilities(const struct device *dev,
				    struct display_capabilities *capabilities)
{
	struct video_format fmt;

	video_get_format((struct device *)dev, VIDEO_EP_IN, &fmt);

	memset(capabilities, 0, sizeof(struct display_capabilities));

	capabilities->x_resolution = fmt.width;
	capabilities->y_resolution = fmt.height;

	switch (fmt.pixelformat) {
	case VIDEO_PIX_FMT_RGB565:
		capabilities->supported_pixel_formats = PIXEL_FORMAT_RGB_565;
		capabilities->current_pixel_format = PIXEL_FORMAT_RGB_565;
		break;
	default:
		LOG_ERR("Video pixformat not supported");
	}

	capabilities->current_orientation = DISPLAY_ORIENTATION_NORMAL;
}

void display_video_init(struct device *dev)
{
	struct video_format fmt;
	int bsize;

	if (vbufa != NULL || vbufb != NULL) {
		LOG_ERR("Only one display video instance supported\n");
		return;
	}

	/* Retrieve buffer size to alloc */
	video_get_format((struct device *)dev, VIDEO_EP_IN, &fmt);
	bsize = fmt.height * fmt.pitch;

	/* Alloc two buffers (double buffering) and enqueue */
	vbufa = video_buffer_alloc(bsize);
	vbufa->bytesused = bsize;
	video_enqueue((struct device *)dev, VIDEO_EP_IN, vbufa);
	vbufb = video_buffer_alloc(bsize);
	vbufb->bytesused = bsize;
	video_enqueue((struct device *)dev, VIDEO_EP_IN, vbufb);

	/* start display now */
	video_stream_start((struct device *)dev);
}
