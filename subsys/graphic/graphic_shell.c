/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 STMicroelectronics
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/graphic.h>
#include <zephyr/drivers/video.h>
#include <zephyr/shell/shell.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(graphic_shell, CONFIG_LOG_DEFAULT_LEVEL);

static bool device_is_graphic_and_ready(const struct device *dev)
{
	return device_is_ready(dev) && DEVICE_API_IS(graphic, dev);
}

static bool device_is_display_and_ready(const struct device *dev)
{
	return device_is_ready(dev) && DEVICE_API_IS(display, dev);
}

static bool device_is_video_and_ready(const struct device *dev)
{
	return device_is_ready(dev) && DEVICE_API_IS(video, dev);
}

static int parse_u32_arg(const struct shell *sh, const char *arg_name, const char *arg_value,
			 uint32_t *value)
{
	unsigned long parsed;
	char *endptr;

	errno = 0;
	parsed = strtoul(arg_value, &endptr, 10);
	if (errno != 0 || *endptr != '\0' || parsed > UINT32_MAX) {
		shell_error(sh, "Invalid %s value: %s", arg_name, arg_value);
		return -EINVAL;
	}

	*value = (uint32_t)parsed;

	return 0;
}

static int parse_u8_arg(const struct shell *sh, const char *arg_name, const char *arg_value,
			uint8_t *value)
{
	uint32_t parsed;
	int ret;

	ret = parse_u32_arg(sh, arg_name, arg_value, &parsed);
	if (ret < 0) {
		return ret;
	}

	if (parsed > UINT8_MAX) {
		shell_error(sh, "%s must be in [0, %u]", arg_name, UINT8_MAX);
		return -EINVAL;
	}

	*value = (uint8_t)parsed;

	return 0;
}

#if defined(CONFIG_DISPLAY)
static int cmd_graphic_memcpy_fb(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *graphic_dev, *display_dev;
	struct graphic_area area = {0};
	struct graphic_output_area output = {0};
	struct display_capabilities capabilities;
	uint32_t bits_per_pixel, bytes_per_pixel;
	uint32_t src_x, src_y, dst_x, dst_y;
	enum graphic_operation_status status;
	uint8_t *fb_addr;
	int op_id, ret;

	graphic_dev = shell_device_get_binding(argv[1]);
	if (!graphic_dev) {
		shell_error(sh, "Invalid graphic device");
		return -ENODEV;
	}
	if (!device_is_graphic_and_ready(graphic_dev)) {
		shell_error(sh, "Device %s is either not ready or not graphic", graphic_dev->name);
		return -ENODEV;
	}

	display_dev = shell_device_get_binding(argv[2]);
	if (!display_dev) {
		shell_error(sh, "Invalid display device");
		return -ENODEV;
	}
	if (!device_is_display_and_ready(display_dev)) {
		shell_error(sh, "Device %s is either not ready or not display", display_dev->name);
		return -ENODEV;
	}

	ret = parse_u32_arg(sh, "src_x", argv[3], &src_x);
	if (ret < 0) {
		return ret;
	}

	ret = parse_u32_arg(sh, "src_y", argv[4], &src_y);
	if (ret < 0) {
		return ret;
	}

	ret = parse_u32_arg(sh, "width", argv[5], &area.width);
	if (ret < 0) {
		return ret;
	}

	ret = parse_u32_arg(sh, "height", argv[6], &area.height);
	if (ret < 0) {
		return ret;
	}

	ret = parse_u32_arg(sh, "dst_x", argv[7], &dst_x);
	if (ret < 0) {
		return ret;
	}

	ret = parse_u32_arg(sh, "dst_y", argv[8], &dst_y);
	if (ret < 0) {
		return ret;
	}

	if (area.width == 0 || area.height == 0) {
		shell_error(sh, "width and height must be greater than zero");
		return -EINVAL;
	}

	fb_addr = display_get_framebuffer(display_dev);
	if (fb_addr == NULL) {
		shell_error(sh, "Failed to get framebuffer address");
		return -ENODEV;
	}

	display_get_capabilities(display_dev, &capabilities);

	bits_per_pixel = DISPLAY_BITS_PER_PIXEL(capabilities.current_pixel_format);
	if (bits_per_pixel == 0 || (bits_per_pixel % 8) != 0) {
		shell_error(sh, "Unsupported display pixel format for framebuffer addressing");
		return -EINVAL;
	}

	bytes_per_pixel = bits_per_pixel / 8;

	area.stride = capabilities.x_resolution;
	area.pixelformat = capabilities.current_pixel_format;
	area.opacity = 0;

	output.pixelformat = capabilities.current_pixel_format;
	output.stride = area.stride;

	if (src_x >= capabilities.x_resolution || src_y >= capabilities.y_resolution ||
	    area.width > (capabilities.x_resolution - src_x) ||
	    area.height > (capabilities.y_resolution - src_y)) {
		shell_error(sh, "Source origin is outside framebuffer bounds");
		return -EINVAL;
	}

	if (dst_x >= capabilities.x_resolution || dst_y >= capabilities.y_resolution ||
	    area.width > (capabilities.x_resolution - dst_x) ||
	    area.height > (capabilities.y_resolution - dst_y)) {
		shell_error(sh, "Destination point is outside framebuffer bounds");
		return -EINVAL;
	}

	area.addr = fb_addr + (src_y * area.stride * bytes_per_pixel) + (src_x * bytes_per_pixel);
	output.addr =
		fb_addr + (dst_y * output.stride * bytes_per_pixel) + (dst_x * bytes_per_pixel);

	op_id = graphic_alloc_memcpy_conv_operation(graphic_dev, &output, &area);
	if (op_id < 0) {
		shell_error(sh, "Failed to allocate a memcpy operation");
		return -EIO;
	}

	ret = graphic_submit_operation(graphic_dev, op_id);
	if (ret < 0) {
		shell_error(sh, "Failed to perform graphic operation");
		return ret;
	}

	ret = graphic_sync_operation(graphic_dev, op_id, K_FOREVER);
	if (ret < 0) {
		shell_error(sh, "Failed to sync graphic operation");
		return ret;
	}

	ret = graphic_get_operation_status(graphic_dev, op_id, &status);
	if (ret < 0) {
		shell_error(sh, "Failed to get graphic operation status");
	} else {
		shell_print(sh, "Operation completed with %s",
			    status == GRAPHIC_OP_COMPLETED ? "success" : "error");
	}

	ret = graphic_free_operation(graphic_dev, op_id);
	if (ret < 0) {
		shell_error(sh, "Failed to perform free operation");
		return ret;
	}

	return 0;
}

static int cmd_graphic_fill_fb(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *graphic_dev, *display_dev;
	struct graphic_filled_area area = {0};
	struct graphic_output_area output = {0};
	struct display_capabilities capabilities;
	uint32_t bits_per_pixel, bytes_per_pixel;
	uint32_t x, y;
	uint8_t *fb_addr;
	enum graphic_operation_status status;
	int op_id, ret;

	graphic_dev = shell_device_get_binding(argv[1]);
	if (!graphic_dev) {
		shell_error(sh, "Invalid graphic device");
		return -ENODEV;
	}
	if (!device_is_graphic_and_ready(graphic_dev)) {
		shell_error(sh, "Device %s is either not ready or not graphic", graphic_dev->name);
		return -ENODEV;
	}

	display_dev = shell_device_get_binding(argv[2]);
	if (!display_dev) {
		shell_error(sh, "Invalid display device");
		return -ENODEV;
	}
	if (!device_is_display_and_ready(display_dev)) {
		shell_error(sh, "Device %s is either not ready or not display", display_dev->name);
		return -ENODEV;
	}

	ret = parse_u32_arg(sh, "x", argv[3], &x);
	if (ret < 0) {
		return ret;
	}

	ret = parse_u32_arg(sh, "y", argv[4], &y);
	if (ret < 0) {
		return ret;
	}

	ret = parse_u32_arg(sh, "width", argv[5], &area.width);
	if (ret < 0) {
		return ret;
	}

	ret = parse_u32_arg(sh, "height", argv[6], &area.height);
	if (ret < 0) {
		return ret;
	}

	ret = parse_u8_arg(sh, "red", argv[7], &area.red);
	if (ret < 0) {
		return ret;
	}

	ret = parse_u8_arg(sh, "green", argv[8], &area.green);
	if (ret < 0) {
		return ret;
	}

	ret = parse_u8_arg(sh, "blue", argv[9], &area.blue);
	if (ret < 0) {
		return ret;
	}

	ret = parse_u8_arg(sh, "opacity", argv[10], &area.opacity);
	if (ret < 0) {
		return ret;
	}

	if (area.width == 0 || area.height == 0) {
		shell_error(sh, "width and height must be greater than zero");
		return -EINVAL;
	}

	fb_addr = display_get_framebuffer(display_dev);
	if (fb_addr == NULL) {
		shell_error(sh, "Failed to get framebuffer address");
		return -ENODEV;
	}

	display_get_capabilities(display_dev, &capabilities);

	bits_per_pixel = DISPLAY_BITS_PER_PIXEL(capabilities.current_pixel_format);
	if (bits_per_pixel == 0 || (bits_per_pixel % 8) != 0) {
		shell_error(sh, "Unsupported display pixel format for framebuffer addressing");
		return -EINVAL;
	}

	bytes_per_pixel = bits_per_pixel / 8;

	output.pixelformat = capabilities.current_pixel_format;
	output.stride = capabilities.x_resolution;

	if (x >= capabilities.x_resolution || y >= capabilities.y_resolution ||
	    area.width > (capabilities.x_resolution - x) ||
	    area.height > (capabilities.y_resolution - y)) {
		shell_error(sh, "Rectangle is outside framebuffer bounds");
		return -EINVAL;
	}

	output.addr = fb_addr + (y * output.stride * bytes_per_pixel) + (x * bytes_per_pixel);

	op_id = graphic_alloc_fill_operation(graphic_dev, &output, &area);
	if (op_id < 0) {
		shell_error(sh, "Failed to allocate a fill operation");
		return -EIO;
	}

	ret = graphic_submit_operation(graphic_dev, op_id);
	if (ret < 0) {
		shell_error(sh, "Failed to perform graphic operation");
		return ret;
	}

	ret = graphic_sync_operation(graphic_dev, op_id, K_FOREVER);
	if (ret < 0) {
		shell_error(sh, "Failed to sync graphic operation");
		return ret;
	}

	ret = graphic_get_operation_status(graphic_dev, op_id, &status);
	if (ret < 0) {
		shell_error(sh, "Failed to get graphic operation status");
	} else {
		shell_print(sh, "Operation completed with %s",
			    status == GRAPHIC_OP_COMPLETED ? "success" : "error");
	}

	ret = graphic_free_operation(graphic_dev, op_id);
	if (ret < 0) {
		shell_error(sh, "Failed to perform free operation");
		return ret;
	}

	return 0;
}
#endif

#if defined(CONFIG_VIDEO) && defined(CONFIG_DISPLAY)
static int video_pixfmt_to_display_pixfmt(uint32_t video_pixfmt,
					  enum display_pixel_format *display_pixfmt)
{
	switch (video_pixfmt) {
	case VIDEO_PIX_FMT_RGB565:
		*display_pixfmt = PIXEL_FORMAT_RGB_565;
		return 0;
	case VIDEO_PIX_FMT_RGB565X:
		*display_pixfmt = PIXEL_FORMAT_RGB_565X;
		return 0;
	case VIDEO_PIX_FMT_RGB24:
		*display_pixfmt = PIXEL_FORMAT_RGB_888;
		return 0;
	case VIDEO_PIX_FMT_BGR24:
		*display_pixfmt = PIXEL_FORMAT_BGR_888;
		return 0;
	case VIDEO_PIX_FMT_ARGB32:
		*display_pixfmt = PIXEL_FORMAT_ARGB_8888;
		return 0;
	case VIDEO_PIX_FMT_ABGR32:
		*display_pixfmt = PIXEL_FORMAT_ABGR_8888;
		return 0;
	case VIDEO_PIX_FMT_RGBA32:
		*display_pixfmt = PIXEL_FORMAT_RGBA_8888;
		return 0;
	case VIDEO_PIX_FMT_BGRA32:
		*display_pixfmt = PIXEL_FORMAT_BGRA_8888;
		return 0;
	case VIDEO_PIX_FMT_XRGB32:
		*display_pixfmt = PIXEL_FORMAT_XRGB_8888;
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int release_video_buffers(const struct device *video_dev,
				 struct video_buffer *buffers[], size_t buffer_count)
{
	struct video_buffer *buffer;
	int ret = 0;
	int release_ret;

	while (video_dequeue(video_dev, &buffer, K_NO_WAIT) == 0) {
		for (size_t i = 0; i < buffer_count; i++) {
			if (buffers[i] == buffer) {
				buffers[i] = NULL;
				break;
			}
		}

		release_ret = video_buffer_release(buffer);
		if (release_ret < 0 && ret == 0) {
			ret = release_ret;
		}
	}

	for (size_t i = 0; i < buffer_count; i++) {
		if (buffers[i] != NULL) {
			release_ret = video_buffer_release(buffers[i]);
			if (release_ret < 0 && ret == 0) {
				ret = release_ret;
			}
			buffers[i] = NULL;
		}
	}

	return ret;
}

int cmd_graphic_memcpy_video_fb(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *graphic_dev, *display_dev, *video_dev;
	struct graphic_area area = {0};
	struct graphic_output_area output = {0};
	struct display_capabilities display_caps;
	struct video_format video_fmt = { .type = VIDEO_BUF_TYPE_OUTPUT };
	struct video_caps video_caps = { .type = VIDEO_BUF_TYPE_OUTPUT };
	struct video_buffer *buffers[CONFIG_VIDEO_BUFFER_POOL_NUM_MAX] = {0};
	struct video_buffer *vbuf = NULL;
	uint32_t src_x, src_y, dst_x, dst_y;
	uint32_t video_bits_per_px, video_bytes_per_px;
	uint32_t display_bits_per_px, display_bytes_per_px;
	uint8_t *fb_addr;
	size_t buffer_count = 0;
	enum graphic_operation_status status;
	bool stream_started = false;
	bool buffer_requeued = false;
	int op_id = -1;
	int ret, cleanup_ret;

	graphic_dev = shell_device_get_binding(argv[1]);
	display_dev = shell_device_get_binding(argv[2]);
	video_dev = shell_device_get_binding(argv[3]);
	if (!device_is_graphic_and_ready(graphic_dev) ||
	    !device_is_display_and_ready(display_dev) ||
	    !device_is_video_and_ready(video_dev)) {
		shell_error(sh, "Invalid or unavailable graphic, display, or video device");
		return -ENODEV;
	}

	ret = parse_u32_arg(sh, "src_x", argv[4], &src_x);
	if (ret < 0) {
		return ret;
	}
	ret = parse_u32_arg(sh, "src_y", argv[5], &src_y);
	if (ret < 0) {
		return ret;
	}
	ret = parse_u32_arg(sh, "width", argv[6], &area.width);
	if (ret < 0) {
		return ret;
	}
	ret = parse_u32_arg(sh, "height", argv[7], &area.height);
	if (ret < 0) {
		return ret;
	}
	ret = parse_u32_arg(sh, "dst_x", argv[8], &dst_x);
	if (ret < 0) {
		return ret;
	}
	ret = parse_u32_arg(sh, "dst_y", argv[9], &dst_y);
	if (ret < 0) {
		return ret;
	}

	if (area.width == 0 || area.height == 0) {
		shell_error(sh, "width and height must be greater than zero");
		return -EINVAL;
	}

	fb_addr = display_get_framebuffer(display_dev);
	if (fb_addr == NULL) {
		shell_error(sh, "Failed to get framebuffer address");
		return -ENODEV;
	}

	display_get_capabilities(display_dev, &display_caps);
	display_bits_per_px = DISPLAY_BITS_PER_PIXEL(display_caps.current_pixel_format);
	if (display_bits_per_px == 0 || (display_bits_per_px % 8) != 0) {
		shell_error(sh, "Unsupported display pixel format for framebuffer addressing");
		return -EINVAL;
	}
	display_bytes_per_px = display_bits_per_px / 8;

	if (dst_x >= display_caps.x_resolution || dst_y >= display_caps.y_resolution ||
	    area.width > (display_caps.x_resolution - dst_x) ||
	    area.height > (display_caps.y_resolution - dst_y)) {
		shell_error(sh, "Destination rectangle is outside framebuffer bounds");
		return -EINVAL;
	}

	ret = video_get_format(video_dev, &video_fmt);
	if (ret < 0) {
		shell_error(sh, "Failed to get video format from %s", video_dev->name);
		return ret;
	}

	ret = video_get_caps(video_dev, &video_caps);
	if (ret < 0) {
		shell_error(sh, "Failed to get video capabilities from %s", video_dev->name);
		return ret;
	}

	ret = video_pixfmt_to_display_pixfmt(video_fmt.pixelformat, &area.pixelformat);
	if (ret < 0) {
		shell_error(sh, "Unsupported video pixel format '%s'",
			    VIDEO_FOURCC_TO_STR(video_fmt.pixelformat));
		return ret;
	}

	video_bits_per_px = video_bits_per_pixel(video_fmt.pixelformat);
	if (video_bits_per_px == 0 || (video_bits_per_px % 8) != 0) {
		shell_error(sh, "Unsupported video pixel format '%s'",
			    VIDEO_FOURCC_TO_STR(video_fmt.pixelformat));
		return -ENOTSUP;
	}
	video_bytes_per_px = video_bits_per_px / 8;

	if (video_fmt.pitch == 0 || (video_fmt.pitch % video_bytes_per_px) != 0 ||
	    video_fmt.width == 0 || video_fmt.height == 0) {
		shell_error(sh, "Invalid active video format from %s", video_dev->name);
		return -EINVAL;
	}

	if (src_x >= video_fmt.width || src_y >= video_fmt.height ||
	    area.width > (video_fmt.width - src_x) ||
	    area.height > (video_fmt.height - src_y)) {
		shell_error(sh, "Source rectangle is outside video frame bounds");
		return -EINVAL;
	}

	for (buffer_count = 0; buffer_count < video_caps.min_vbuf_count; buffer_count++) {
		buffers[buffer_count] = video_buffer_alloc(video_fmt.size, K_NO_WAIT);
		if (buffers[buffer_count] == NULL) {
			shell_error(sh, "Failed to allocate video buffer %u", buffer_count);
			ret = -ENOMEM;
			goto cleanup;
		}

		buffers[buffer_count]->type = VIDEO_BUF_TYPE_OUTPUT;
		ret = video_enqueue(video_dev, buffers[buffer_count]);
		if (ret < 0) {
			shell_error(sh, "Failed to enqueue video buffer %u", buffer_count);
			goto cleanup;
		}
	}

	ret = video_stream_start(video_dev, VIDEO_BUF_TYPE_OUTPUT);
	if (ret < 0) {
		shell_error(sh, "Failed to start video stream on %s", video_dev->name);
		goto cleanup;
	}
	stream_started = true;

	ret = video_dequeue(video_dev, &vbuf, K_FOREVER);
	if (ret < 0) {
		shell_error(sh, "Failed to dequeue video buffer from %s", video_dev->name);
		goto cleanup;
	}

	area.stride = video_fmt.pitch / video_bytes_per_px;
	area.opacity = 0;
	area.addr = vbuf->buffer + (src_y * video_fmt.pitch) + (src_x * video_bytes_per_px);
	output.pixelformat = display_caps.current_pixel_format;
	output.stride = display_caps.x_resolution;
	output.addr = fb_addr + (dst_y * output.stride * display_bytes_per_px) +
		      (dst_x * display_bytes_per_px);

	op_id = graphic_alloc_memcpy_conv_operation(graphic_dev, &output, &area);
	if (op_id < 0) {
		shell_error(sh, "Failed to allocate a memcpy operation");
		ret = -EIO;
		goto cleanup;
	}

	ret = graphic_submit_operation(graphic_dev, op_id);
	if (ret < 0) {
		shell_error(sh, "Failed to perform graphic operation");
		goto cleanup;
	}

	ret = graphic_sync_operation(graphic_dev, op_id, K_FOREVER);
	if (ret < 0) {
		shell_error(sh, "Failed to sync graphic operation");
		goto cleanup;
	}

	ret = graphic_get_operation_status(graphic_dev, op_id, &status);
	if (ret < 0) {
		shell_error(sh, "Failed to get graphic operation status");
		goto cleanup;
	}

	if (status == GRAPHIC_OP_ERROR) {
		ret = -EIO;
		goto cleanup;
	}

	ret = video_enqueue(video_dev, vbuf);
	if (ret < 0) {
		shell_error(sh, "Failed to requeue video buffer");
		goto cleanup;
	}
	buffer_requeued = true;

	shell_print(sh, "Successful video-to-framebuffer memcpy on %s", graphic_dev->name);

cleanup:
	if (op_id >= 0) {
		cleanup_ret = graphic_free_operation(graphic_dev, op_id);
		if (cleanup_ret < 0 && ret == 0) {
			ret = cleanup_ret;
		}
	}

	if (vbuf != NULL && !buffer_requeued) {
		cleanup_ret = video_enqueue(video_dev, vbuf);
		if (cleanup_ret < 0) {
			shell_error(sh, "Failed to requeue video buffer during cleanup");
			if (ret == 0) {
				ret = cleanup_ret;
			}
		}
	}

	if (stream_started) {
		cleanup_ret = video_stream_stop(video_dev, VIDEO_BUF_TYPE_OUTPUT);
		if (cleanup_ret < 0 && ret == 0) {
			ret = cleanup_ret;
		}
	}

	cleanup_ret = release_video_buffers(video_dev, buffers, buffer_count);
	if (cleanup_ret < 0 && ret == 0) {
		ret = cleanup_ret;
	}

	return ret;
}
#endif

static void complete_graphic_dev(size_t idx, struct shell_static_entry *entry)
{
	const struct device *dev;

	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = NULL;
	entry->syntax = NULL;

	dev = shell_device_filter(idx, device_is_graphic_and_ready);
	if (dev == NULL) {
		return;
	}

	entry->syntax = dev->name;
}

SHELL_DYNAMIC_CMD_CREATE(dsub_graphic_dev, complete_graphic_dev);

/* Graphic shell commands declaration */

SHELL_STATIC_SUBCMD_SET_CREATE(sub_graphic_cmds,
#if defined(CONFIG_DISPLAY)
	SHELL_CMD_ARG(memcpy_fb, &dsub_graphic_dev,
		SHELL_HELP("Memcpy within framebuffer from source rect to destination point",
			   "<graphic_dev> <fb_dev> <src_x> <src_y> <width> <height> "
			   "<dst_x> <dst_y>"),
		cmd_graphic_memcpy_fb, 9, 0),
	SHELL_CMD_ARG(fill_fb, &dsub_graphic_dev,
		SHELL_HELP("Fill a framebuffer rectangle with RGBA color",
			   "<graphic_dev> <fb_dev> <x> <y> <width> <height> "
			   "<red> <green> <blue> <opacity>"),
		cmd_graphic_fill_fb, 11, 0),
#if defined(CONFIG_VIDEO) && defined(CONFIG_DISPLAY)
	SHELL_CMD_ARG(memcpy_video_fb, &dsub_graphic_dev,
		SHELL_HELP("Copy from dequeued video frame rectangle to framebuffer destination",
			   "<graphic_dev> <fb_dev> <video_dev> <src_x> <src_y> "
			   "<width> <height> <dst_x> <dst_y>"),
		cmd_graphic_memcpy_video_fb, 10, 0),
#endif
#endif
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(graphic, &sub_graphic_cmds, "Graphic Subsystem commands", NULL);
