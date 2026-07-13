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
#endif
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(graphic, &sub_graphic_cmds, "Graphic Subsystem commands", NULL);
