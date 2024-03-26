/** @file
 * @brief Compact Framebuffer shell module
 *
 * Provide some Compact Framebuffer shell commands that can be useful for
 * testing.
 */

/*
 * Copyright (c) 2018 Diego Sueiro
 * Copyright (c) 2024 TOKITA Hiroshi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <zephyr/shell/shell.h>
#include <zephyr/display/cfb.h>
#include <zephyr/sys/math_extras.h>

#define HELP_NONE "[none]"
#define HELP_INIT "[[<devname>] <pixfmt> [<xferbuf_size> [<cmdbuf_size>]]]"
#define HELP_PRINT "<x> <y> \"<text>\""
#define HELP_DRAW_POINT "<x> <y>"
#define HELP_DRAW_LINE "<x0> <y0> <x1> <y1>"
#define HELP_DRAW_RECT "<x0> <y0> <x1> <y1>"
#define HELP_INVERT "[<x> <y> <width> <height>]"
#define HELP_FOREGROUND "<red> <green> <blue> <alpha>"
#define HELP_BACKGROUND "<red> <green> <blue> <alpha>"

static struct cfb_display *disp;
static const char * const param_name[] = {
	"height", "width", "ppt", "rows", "cols"};

static const char *const pixfmt_name[] = {"RGB_888",   "MONO01",  "MONO10",
					  "ARGB_8888", "RGB_565", "BGR_565"};

static int cmd_clear(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (!disp) {
		shell_error(sh, HELP_INIT);
		return -ENODEV;
	}

	err = cfb_clear(&disp->fb, true);
	if (err) {
		shell_error(sh, "Framebuffer clear error=%d", err);
		return err;
	}

	err = cfb_finalize(&disp->fb);
	if (err) {
		shell_error(sh, "Framebuffer finalize error=%d", err);
		return err;
	}

	shell_print(sh, "Display Cleared");

	return err;
}

static int cmd_cfb_print(const struct shell *sh, int x, int y, char *str)
{
	int err;

	if (!disp) {
		shell_error(sh, HELP_INIT);
		return -ENODEV;
	}

	err = cfb_clear(&disp->fb, false);
	if (err) {
		shell_error(sh, "Framebuffer clear failed error=%d", err);
		return err;
	}

	err = cfb_print(&disp->fb, str, x, y);
	if (err) {
		shell_error(sh, "Failed to print the string %s error=%d",
		      str, err);
		return err;
	}

	err = cfb_finalize(&disp->fb);
	if (err) {
		shell_error(sh,
			    "Failed to finalize the Framebuffer error=%d", err);
		return err;
	}

	return err;
}

static int cmd_print(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	int x, y;

	if (!disp) {
		shell_error(sh, HELP_INIT);
		return -ENODEV;
	}

	x = strtol(argv[1], NULL, 10);
	y = strtol(argv[2], NULL, 10);

	err = cmd_cfb_print(sh, x, y, argv[3]);
	if (err) {
		shell_error(sh, "Failed printing to Framebuffer error=%d",
			    err);
	}

	return err;
}

static int cmd_draw_text(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	int x, y;

	if (!disp) {
		shell_error(sh, HELP_INIT);
		return -ENODEV;
	}

	x = strtol(argv[1], NULL, 10);
	y = strtol(argv[2], NULL, 10);
	err = cfb_draw_text(&disp->fb, argv[3], x, y);
	if (err) {
		shell_error(sh, "Failed text drawing to Framebuffer error=%d", err);
		return err;
	}

	err = cfb_finalize(&disp->fb);

	return err;
}

static int cmd_draw_point(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	struct cfb_position pos;

	if (!disp) {
		shell_error(sh, HELP_INIT);
		return -ENODEV;
	}

	pos.x = strtol(argv[1], NULL, 10);
	pos.y = strtol(argv[2], NULL, 10);

	err = cfb_draw_point(&disp->fb, &pos);
	if (err) {
		shell_error(sh, "Failed point drawing to Framebuffer error=%d", err);
		return err;
	}

	err = cfb_finalize(&disp->fb);

	return err;
}

static int cmd_draw_line(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	struct cfb_position start, end;

	if (!disp) {
		shell_error(sh, HELP_INIT);
		return -ENODEV;
	}

	start.x = strtol(argv[1], NULL, 10);
	start.y = strtol(argv[2], NULL, 10);
	end.x = strtol(argv[3], NULL, 10);
	end.y = strtol(argv[4], NULL, 10);

	err = cfb_draw_line(&disp->fb, &start, &end);
	if (err) {
		shell_error(sh, "Failed text drawing to Framebuffer error=%d", err);
		return err;
	}

	err = cfb_finalize(&disp->fb);

	return err;
}

static int cmd_draw_rect(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	struct cfb_position start, end;

	if (!disp) {
		shell_error(sh, HELP_INIT);
		return -ENODEV;
	}

	start.x = strtol(argv[1], NULL, 10);
	start.y = strtol(argv[2], NULL, 10);
	end.x = strtol(argv[3], NULL, 10);
	end.y = strtol(argv[4], NULL, 10);

	err = cfb_draw_rect(&disp->fb, &start, &end);
	if (err) {
		shell_error(sh, "Failed rectanble drawing to Framebuffer error=%d", err);
		return err;
	}

	err = cfb_finalize(&disp->fb);

	return err;
}

static int cmd_scroll_vert(const struct shell *sh, size_t argc, char *argv[])
{
	int err = 0;
	int x, y;

	if (!disp) {
		shell_error(sh, HELP_INIT);
		return -ENODEV;
	}

	x = strtol(argv[1], NULL, 10);
	y = strtol(argv[2], NULL, 10);

	for (; y < cfb_get_display_parameter(disp, CFB_DISPLAY_HEIGH); y++) {
		err = cmd_cfb_print(sh, x, y, argv[3]);
		k_sleep(K_USEC(10000000 / cfb_get_display_parameter(disp, CFB_DISPLAY_HEIGH)));
		if (err) {
			shell_error(sh,
				    "Failed printing to Framebuffer error=%d",
				    err);
			break;
		}
	}

	return err;
}

static int cmd_scroll_horz(const struct shell *sh, size_t argc, char *argv[])
{
	int err = 0;
	int x, y;

	if (!disp) {
		shell_error(sh, HELP_INIT);
		return -ENODEV;
	}

	x = strtol(argv[1], NULL, 10);
	y = strtol(argv[2], NULL, 10);

	for (; x < cfb_get_display_parameter(disp, CFB_DISPLAY_WIDTH); x++) {
		err = cmd_cfb_print(sh, x, y, argv[3]);
		k_sleep(K_USEC(10000000 / cfb_get_display_parameter(disp, CFB_DISPLAY_WIDTH)));
		if (err) {
			shell_error(sh,
				    "Failed printing to Framebuffer error=%d",
				    err);
			break;
		}
	}

	return err;
}

static int cmd_set_font(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	int idx;
	uint8_t height;
	uint8_t width;

	if (!disp) {
		shell_error(sh, HELP_INIT);
		return -ENODEV;
	}

	idx = strtol(argv[1], NULL, 10);

	err = cfb_get_font_size(idx, &width, &height);
	if (err) {
		shell_error(sh, "Invalid font idx=%d err=%d\n", idx, err);
		return err;
	}

	err = cfb_set_font(&disp->fb, idx);
	if (err) {
		shell_error(sh, "Failed setting font idx=%d err=%d", idx,
			    err);
		return err;
	}

	shell_print(sh, "Font idx=%d height=%d widht=%d set", idx, height,
		    width);

	return err;
}

static int cmd_set_kerning(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	char *ep = NULL;
	long kerning;

	if (!disp) {
		shell_error(sh, HELP_INIT);
		return -ENODEV;
	}

	errno = 0;
	kerning = strtol(argv[1], &ep, 10);
	if (errno || ep == argv[1]) {
		shell_error(sh, HELP_INIT);
		return -EINVAL;
	}

	err = cfb_set_kerning(&disp->fb, kerning);
	if (err) {
		shell_error(sh, "Failed to set kerning err=%d", err);
		return err;
	}

	return err;
}

static void parse_color_args(size_t argc, char *argv[], uint8_t *r, uint8_t *g, uint8_t *b,
			     uint8_t *a)
{
	struct display_capabilities cfg;
	bool is_16bit = false;

	display_get_capabilities(disp->dev, &cfg);

	*r = 0;
	*g = 0;
	*b = 0;
	*a = 0;

	if (cfg.current_pixel_format == PIXEL_FORMAT_RGB_565 ||
	    cfg.current_pixel_format == PIXEL_FORMAT_BGR_565) {
		is_16bit = true;
	}

	switch (argc) {
	case 5:
		*a = strtol(argv[4], NULL, 0);
	case 4:
		*b = strtol(argv[3], NULL, 0);
	case 3:
		*g = strtol(argv[2], NULL, 0);
	case 2:
		*r = strtol(argv[1], NULL, 0);
	default:
	}
}

static int cmd_foreground(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t r, g, b, a;

	if (!disp) {
		shell_error(sh, HELP_INIT);
		return -ENODEV;
	}

	parse_color_args(argc, argv, &r, &g, &b, &a);
	cfb_set_fg_color(&disp->fb, r, g, b, a);

	return 0;
}

static int cmd_background(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t r, g, b, a;

	if (!disp) {
		shell_error(sh, HELP_INIT);
		return -ENODEV;
	}

	parse_color_args(argc, argv, &r, &g, &b, &a);
	cfb_set_bg_color(&disp->fb, r, g, b, a);

	return 0;
}

static int cmd_invert(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (!disp) {
		shell_error(sh, HELP_INIT);
		return -ENODEV;
	}

	if (argc == 1) {
		err = cfb_invert(&disp->fb);
		if (err) {
			shell_error(sh, "Error inverting Framebuffer");
			return err;
		}
	} else if (argc == 5) {
		int x, y, w, h;

		x = strtol(argv[1], NULL, 10);
		y = strtol(argv[2], NULL, 10);
		w = strtol(argv[3], NULL, 10);
		h = strtol(argv[4], NULL, 10);

		err = cfb_invert_area(&disp->fb, x, y, w, h);
		if (err) {
			shell_error(sh, "Error invert area");
			return err;
		}
	} else {
		shell_help(sh);
		return 0;
	}

	cfb_finalize(&disp->fb);

	shell_print(sh, "Framebuffer Inverted");

	return err;
}

static int cmd_get_fonts(const struct shell *sh, size_t argc, char *argv[])
{
	int err = 0;
	uint8_t font_height;
	uint8_t font_width;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (!disp) {
		shell_error(sh, HELP_INIT);
		return -ENODEV;
	}

	for (int idx = 0; idx < cfb_get_numof_fonts(); idx++) {
		if (cfb_get_font_size(idx, &font_width, &font_height)) {
			break;
		}
		shell_print(sh, "idx=%d height=%d width=%d", idx,
			    font_height, font_width);
	}

	return err;
}

static int cmd_get_device(const struct shell *sh, size_t argc, char *argv[])
{
	int err = 0;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (!disp) {
		shell_error(sh, HELP_INIT);
		return -ENODEV;
	}

	shell_print(sh, "Framebuffer Device: %s", disp->dev->name);

	return err;
}

static int cmd_get_param_all(const struct shell *sh, size_t argc,
			     char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (!disp) {
		shell_error(sh, HELP_INIT);
		return -ENODEV;
	}

	for (unsigned int i = 0; i <= CFB_DISPLAY_COLS; i++) {
		shell_print(sh, "param: %s=%d", param_name[i],
				cfb_get_display_parameter(disp, i));

	}

	return 0;
}

static int cmd_get_param_height(const struct shell *sh, size_t argc,
			     char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (!disp) {
		shell_error(sh, HELP_INIT);
		return -ENODEV;
	}

	shell_print(sh, "param: %s=%d", param_name[CFB_DISPLAY_HEIGH],
		    cfb_get_display_parameter(disp, CFB_DISPLAY_HEIGH));

	return 0;
}

static int cmd_get_param_width(const struct shell *sh, size_t argc,
			     char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (!disp) {
		shell_error(sh, HELP_INIT);
		return -ENODEV;
	}

	shell_print(sh, "param: %s=%d", param_name[CFB_DISPLAY_WIDTH],
		    cfb_get_display_parameter(disp, CFB_DISPLAY_WIDTH));

	return 0;
}

static int cmd_get_param_ppt(const struct shell *sh, size_t argc,
			     char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (!disp) {
		shell_error(sh, HELP_INIT);
		return -ENODEV;
	}

	shell_print(sh, "param: %s=%d", param_name[CFB_DISPLAY_PPT],
		    cfb_get_display_parameter(disp, CFB_DISPLAY_PPT));

	return 0;
}

static int cmd_get_param_rows(const struct shell *sh, size_t argc,
			     char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (!disp) {
		shell_error(sh, HELP_INIT);
		return -ENODEV;
	}

	shell_print(sh, "param: %s=%d", param_name[CFB_DISPLAY_ROWS],
		    cfb_get_display_parameter(disp, CFB_DISPLAY_ROWS));

	return 0;
}

static int cmd_get_param_cols(const struct shell *sh, size_t argc,
			     char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (!disp) {
		shell_error(sh, HELP_INIT);
		return -ENODEV;
	}

	shell_print(sh, "param: %s=%d", param_name[CFB_DISPLAY_COLS],
		    cfb_get_display_parameter(disp, CFB_DISPLAY_COLS));

	return 0;
}

static int cmd_init(const struct shell *sh, size_t argc, char *argv[])
{
	enum display_pixel_format pix_fmt;
	struct display_capabilities cfg;
	const struct device *dev;
	uint8_t xferbuf_size_idx = 0;
	uint8_t cmdbuf_size_idx = 0;
	uint8_t dev_name_idx = 0;
	uint8_t pix_fmt_idx = 0;
	size_t xferbuf_size = 8192;
	size_t cmdbuf_size = 256;
	int err;

	if (argc == 1) {
	} else if (argc == 2) {
		pix_fmt_idx = 1;
	} else if (argc == 3) {
		pix_fmt_idx = 1;
		xferbuf_size_idx = 2;
	} else if (argc == 4) {
		pix_fmt_idx = 1;
		xferbuf_size_idx = 2;
		cmdbuf_size_idx = 3;
	} else if (argc == 5) {
		dev_name_idx = 1;
		pix_fmt_idx = 2;
		xferbuf_size_idx = 3;
		cmdbuf_size_idx = 4;
	}

	if (xferbuf_size_idx > 0) {
		xferbuf_size = strtol(argv[xferbuf_size_idx], NULL, 10);
	}

	if (cmdbuf_size_idx > 0) {
		cmdbuf_size = strtol(argv[cmdbuf_size_idx], NULL, 10);
	}

	if (dev_name_idx > 0) {
		dev = device_get_binding(argv[dev_name_idx]);
		if (!dev) {
			shell_error(sh, "Not exists device.");
			return -ENODEV;
		}
	} else {
		dev = device_get_binding(DT_NODE_FULL_NAME(DT_CHOSEN(zephyr_display)));
	}

	if (!device_is_ready(dev)) {
		shell_error(sh, "Display device not ready");
		return -ENODEV;
	}

	shell_print(sh, "Display: %s", dev->name);

	if (pix_fmt_idx > 0) {
		if (strcmp(argv[pix_fmt_idx], "RGB_888") == 0) {
			pix_fmt = PIXEL_FORMAT_RGB_888;
		} else if (strcmp(argv[pix_fmt_idx], "MONO01") == 0) {
			pix_fmt = PIXEL_FORMAT_MONO01;
		} else if (strcmp(argv[pix_fmt_idx], "MONO10") == 0) {
			pix_fmt = PIXEL_FORMAT_MONO10;
		} else if (strcmp(argv[pix_fmt_idx], "ARGB_8888") == 0) {
			pix_fmt = PIXEL_FORMAT_ARGB_8888;
		} else if (strcmp(argv[pix_fmt_idx], "RGB_565") == 0) {
			pix_fmt = PIXEL_FORMAT_RGB_565;
		} else if (strcmp(argv[pix_fmt_idx], "BGR_565") == 0) {
			pix_fmt = PIXEL_FORMAT_BGR_565;
		} else {
			shell_error(sh, "Invalid pixel format. use "
					"RGB_888/MONO01/MONO10/ARGB_8888/RGB_565/BGR_565");
			return -EINVAL;
		}

		err = display_set_pixel_format(dev, pix_fmt);
		if (err) {
			shell_error(sh, "Failed to set required pixel format: %d", err);
			return err;
		}
	}

	display_get_capabilities(dev, &cfg);
	shell_print(sh, "Display pixel format: %s",
		    pixfmt_name[u32_count_trailing_zeros(cfg.current_pixel_format)]);

	err = display_blanking_off(dev);
	if (err) {
		shell_error(sh, "Failed to turn off display blanking: %d", err);
		return err;
	}

	if (disp) {
		cfb_display_free(disp);
	}

	disp = cfb_display_alloc(dev, xferbuf_size, cmdbuf_size);
	if (!disp) {
		shell_error(sh, "Framebuffer initialization failed!");
		return err;
	}

	shell_print(sh, "Framebuffer initialized: xfer_buf: %u cmd_buf: %u", xferbuf_size,
		    cmdbuf_size);
	cmd_clear(sh, argc, argv);

	return err;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_cmd_get_param,

	SHELL_CMD_ARG(all, NULL, NULL, cmd_get_param_all, 1, 0),
	SHELL_CMD_ARG(height, NULL, NULL, cmd_get_param_height, 1, 0),
	SHELL_CMD_ARG(width, NULL, NULL, cmd_get_param_width, 1, 0),
	SHELL_CMD_ARG(ppt, NULL, NULL, cmd_get_param_ppt, 1, 0),
	SHELL_CMD_ARG(rows, NULL, NULL, cmd_get_param_rows, 1, 0),
	SHELL_CMD_ARG(cols, NULL, NULL, cmd_get_param_cols, 1, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(sub_cmd_scroll,

	SHELL_CMD_ARG(vertical, NULL, HELP_PRINT, cmd_scroll_vert, 4, 0),
	SHELL_CMD_ARG(horizontal, NULL, HELP_PRINT, cmd_scroll_horz, 4, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(sub_cmd_draw,
	SHELL_CMD_ARG(text, NULL, HELP_PRINT, cmd_draw_text, 4, 0),
	SHELL_CMD_ARG(point, NULL, HELP_DRAW_POINT, cmd_draw_point, 3, 0),
	SHELL_CMD_ARG(line, NULL, HELP_DRAW_LINE, cmd_draw_line, 5, 0),
	SHELL_CMD_ARG(rect, NULL, HELP_DRAW_RECT, cmd_draw_rect, 5, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(cfb_cmds,
	SHELL_CMD_ARG(init, NULL, HELP_INIT, cmd_init, 0, 5),
	SHELL_CMD_ARG(get_device, NULL, HELP_NONE, cmd_get_device, 1, 0),
	SHELL_CMD(get_param, &sub_cmd_get_param,
		  "<all, height, width, ppt, rows, cols>", NULL),
	SHELL_CMD_ARG(get_fonts, NULL, HELP_NONE, cmd_get_fonts, 1, 0),
	SHELL_CMD_ARG(set_font, NULL, "<idx>", cmd_set_font, 2, 0),
	SHELL_CMD_ARG(set_kerning, NULL, "<kerning>", cmd_set_kerning, 2, 0),
	SHELL_CMD_ARG(invert, NULL, HELP_INVERT, cmd_invert, 1, 5),
	SHELL_CMD_ARG(print, NULL, HELP_PRINT, cmd_print, 4, 0),
	SHELL_CMD(scroll, &sub_cmd_scroll, "scroll a text in vertical or "
		  "horizontal direction", NULL),
	SHELL_CMD(draw, &sub_cmd_draw, "draw operations", NULL),
	SHELL_CMD_ARG(clear, NULL, HELP_NONE, cmd_clear, 1, 0),
	SHELL_CMD_ARG(foreground, NULL, HELP_FOREGROUND, cmd_foreground, 0, 5),
	SHELL_CMD_ARG(background, NULL, HELP_BACKGROUND, cmd_background, 0, 5),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(cfb, &cfb_cmds, "Compact Framebuffer shell commands", NULL);
