/** @file
 * @brief Monochrome Character Framebuffer shell module
 *
 * Provide some Character Framebuffer shell commands that can be useful for
 * testing.
 */

/*
 * Copyright (c) 2018 Diego Sueiro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <zephyr/shell/shell.h>
#include <zephyr/display/cfb.h>

#define HELP_NONE "[none]"
#define HELP_INIT "call \"cfb init\" first"
#define HELP_PRINT "<col: pos> <row: pos> \"<text>\""

static const struct device *dev;
static const char * const param_name[] = {
	"height", "width", "ppt", "rows", "cols"};

static int cmd_clear(const struct shell *shell, size_t argc, char *argv[])
{
	int err;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (!dev) {
		shell_error(shell, HELP_INIT);
		return -ENODEV;
	}

	err = cfb_framebuffer_clear(dev, true);
	if (err) {
		shell_error(shell, "Framebuffer clear error=%d", err);
		return err;
	}

	err = cfb_framebuffer_finalize(dev);
	if (err) {
		shell_error(shell, "Framebuffer finalize error=%d", err);
		return err;
	}

	shell_print(shell, "Display Cleared");

	return err;
}

static int cmd_cfb_print(const struct shell *shell, int col, int row, char *str)
{
	int err;
	uint8_t ppt;

	if (!dev) {
		shell_error(shell, HELP_INIT);
		return -ENODEV;
	}

	ppt = cfb_get_display_parameter(dev, CFB_DISPLAY_PPT);

	err = cfb_framebuffer_clear(dev, false);
	if (err) {
		shell_error(shell, "Framebuffer clear failed error=%d", err);
		return err;
	}

	err = cfb_print(dev, str, col, row * ppt);
	if (err) {
		shell_error(shell, "Failed to print the string %s error=%d",
		      str, err);
		return err;
	}

	err = cfb_framebuffer_finalize(dev);
	if (err) {
		shell_error(shell,
			    "Failed to finalize the Framebuffer error=%d", err);
		return err;
	}

	return err;
}

static int cmd_print(const struct shell *shell, size_t argc, char *argv[])
{
	int err;
	int col, row;

	if (!dev) {
		shell_error(shell, HELP_INIT);
		return -ENODEV;
	}

	col = strtol(argv[1], NULL, 10);
	if (col > cfb_get_display_parameter(dev, CFB_DISPLAY_COLS)) {
		shell_error(shell, "Invalid col=%d position", col);
		return -EINVAL;
	}

	row = strtol(argv[2], NULL, 10);
	if (row > cfb_get_display_parameter(dev, CFB_DISPLAY_ROWS)) {
		shell_error(shell, "Invalid row=%d position", row);
		return -EINVAL;
	}

	err = cmd_cfb_print(shell, col, row, argv[3]);
	if (err) {
		shell_error(shell, "Failed printing to Framebuffer error=%d",
			    err);
	}

	return err;
}

static int cmd_scroll_vert(const struct shell *shell, size_t argc, char *argv[])
{
	int err = 0;
	int col, row;
	int boundary;

	if (!dev) {
		shell_error(shell, HELP_INIT);
		return -ENODEV;
	}

	col = strtol(argv[1], NULL, 10);
	if (col > cfb_get_display_parameter(dev, CFB_DISPLAY_COLS)) {
		shell_error(shell, "Invalid col=%d position", col);
		return -EINVAL;
	}

	row = strtol(argv[2], NULL, 10);
	if (row > cfb_get_display_parameter(dev, CFB_DISPLAY_ROWS)) {
		shell_error(shell, "Invalid row=%d position", row);
		return -EINVAL;
	}

	boundary = cfb_get_display_parameter(dev, CFB_DISPLAY_ROWS) - row;

	for (int i = 0; i < boundary; i++) {
		err = cmd_cfb_print(shell, col, row, argv[3]);
		if (err) {
			shell_error(shell,
				    "Failed printing to Framebuffer error=%d",
				    err);
			break;
		}
		row++;
	}

	cmd_cfb_print(shell, 0, 0, "");

	return err;
}

static int cmd_scroll_horz(const struct shell *shell, size_t argc, char *argv[])
{
	int err = 0;
	int col, row;
	int boundary;

	if (!dev) {
		shell_error(shell, HELP_INIT);
		return -ENODEV;
	}

	col = strtol(argv[1], NULL, 10);
	if (col > cfb_get_display_parameter(dev, CFB_DISPLAY_COLS)) {
		shell_error(shell, "Invalid col=%d position", col);
		return -EINVAL;
	}

	row = strtol(argv[2], NULL, 10);
	if (row > cfb_get_display_parameter(dev, CFB_DISPLAY_ROWS)) {
		shell_error(shell, "Invalid row=%d position", row);
		return -EINVAL;
	}

	col++;
	boundary = cfb_get_display_parameter(dev, CFB_DISPLAY_COLS) - col;

	for (int i = 0; i < boundary; i++) {
		err = cmd_cfb_print(shell, col, row, argv[3]);
		if (err) {
			shell_error(shell,
				    "Failed printing to Framebuffer error=%d",
				    err);
			break;
		}
		col++;
	}

	cmd_cfb_print(shell, 0, 0, "");

	return err;
}

static int cmd_set_font(const struct shell *shell, size_t argc, char *argv[])
{
	int err;
	int idx;
	uint8_t height;
	uint8_t width;

	if (!dev) {
		shell_error(shell, HELP_INIT);
		return -ENODEV;
	}

	idx = strtol(argv[1], NULL, 10);

	err = cfb_get_font_size(dev, idx, &width, &height);
	if (err) {
		shell_error(shell, "Invalid font idx=%d err=%d\n", idx, err);
		return err;
	}

	err = cfb_framebuffer_set_font(dev, idx);
	if (err) {
		shell_error(shell, "Failed setting font idx=%d err=%d", idx,
			    err);
		return err;
	}

	shell_print(shell, "Font idx=%d height=%d widht=%d set", idx, height,
		    width);

	return err;
}

static int cmd_invert(const struct shell *shell, size_t argc, char *argv[])
{
	int err;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (!dev) {
		shell_error(shell, HELP_INIT);
		return -ENODEV;
	}

	err = cfb_framebuffer_invert(dev);
	if (err) {
		shell_error(shell, "Error inverting Framebuffer");
		return err;
	}

	cmd_cfb_print(shell, 0, 0, "");

	shell_print(shell, "Framebuffer Inverted");

	return err;
}

static int cmd_get_fonts(const struct shell *shell, size_t argc, char *argv[])
{
	int err = 0;
	uint8_t font_height;
	uint8_t font_width;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (!dev) {
		shell_error(shell, HELP_INIT);
		return -ENODEV;
	}

	for (int idx = 0; idx < cfb_get_numof_fonts(dev); idx++) {
		if (cfb_get_font_size(dev, idx, &font_width, &font_height)) {
			break;
		}
		shell_print(shell, "idx=%d height=%d width=%d", idx,
			    font_height, font_width);
	}

	return err;
}

static int cmd_get_device(const struct shell *shell, size_t argc, char *argv[])
{
	int err = 0;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (!dev) {
		shell_error(shell, HELP_INIT);
		return -ENODEV;
	}

	shell_print(shell, "Framebuffer Device: %s", dev->name);

	return err;
}

static int cmd_get_param_all(const struct shell *shell, size_t argc,
			     char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (!dev) {
		shell_error(shell, HELP_INIT);
		return -ENODEV;
	}

	for (unsigned int i = 0; i <= CFB_DISPLAY_COLS; i++) {
		shell_print(shell, "param: %s=%d", param_name[i],
				cfb_get_display_parameter(dev, i));

	}

	return 0;
}

static int cmd_get_param_height(const struct shell *shell, size_t argc,
			     char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (!dev) {
		shell_error(shell, HELP_INIT);
		return -ENODEV;
	}

	shell_print(shell, "param: %s=%d", param_name[CFB_DISPLAY_HEIGH],
		    cfb_get_display_parameter(dev, CFB_DISPLAY_HEIGH));

	return 0;
}

static int cmd_get_param_width(const struct shell *shell, size_t argc,
			     char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (!dev) {
		shell_error(shell, HELP_INIT);
		return -ENODEV;
	}

	shell_print(shell, "param: %s=%d", param_name[CFB_DISPLAY_WIDTH],
		    cfb_get_display_parameter(dev, CFB_DISPLAY_WIDTH));

	return 0;
}

static int cmd_get_param_ppt(const struct shell *shell, size_t argc,
			     char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (!dev) {
		shell_error(shell, HELP_INIT);
		return -ENODEV;
	}

	shell_print(shell, "param: %s=%d", param_name[CFB_DISPLAY_PPT],
		    cfb_get_display_parameter(dev, CFB_DISPLAY_PPT));

	return 0;
}

static int cmd_get_param_rows(const struct shell *shell, size_t argc,
			     char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (!dev) {
		shell_error(shell, HELP_INIT);
		return -ENODEV;
	}

	shell_print(shell, "param: %s=%d", param_name[CFB_DISPLAY_ROWS],
		    cfb_get_display_parameter(dev, CFB_DISPLAY_ROWS));

	return 0;
}

static int cmd_get_param_cols(const struct shell *shell, size_t argc,
			     char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (!dev) {
		shell_error(shell, HELP_INIT);
		return -ENODEV;
	}

	shell_print(shell, "param: %s=%d", param_name[CFB_DISPLAY_COLS],
		    cfb_get_display_parameter(dev, CFB_DISPLAY_COLS));

	return 0;
}

static int cmd_init(const struct shell *shell, size_t argc, char *argv[])
{
	int err;

	dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	if (!device_is_ready(dev)) {
		shell_error(shell, "Display device not ready");
		return -ENODEV;
	}

	err = cfb_framebuffer_init(dev);
	if (err) {
		shell_error(shell, "Framebuffer initialization failed!");
		return err;
	}

	shell_print(shell, "Framebuffer initialized: %s", dev->name);
	cmd_clear(shell, argc, argv);

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

SHELL_STATIC_SUBCMD_SET_CREATE(cfb_cmds,
	SHELL_CMD_ARG(init, NULL, HELP_NONE, cmd_init, 1, 0),
	SHELL_CMD_ARG(get_device, NULL, HELP_NONE, cmd_get_device, 1, 0),
	SHELL_CMD(get_param, &sub_cmd_get_param,
		  "<all, height, width, ppt, rows, cols>", NULL),
	SHELL_CMD_ARG(get_fonts, NULL, HELP_NONE, cmd_get_fonts, 1, 0),
	SHELL_CMD_ARG(set_font, NULL, "<idx>", cmd_set_font, 2, 0),
	SHELL_CMD_ARG(invert, NULL, HELP_NONE, cmd_invert, 1, 0),
	SHELL_CMD_ARG(print, NULL, HELP_PRINT, cmd_print, 4, 0),
	SHELL_CMD(scroll, &sub_cmd_scroll, "scroll a text in vertical or "
		  "horizontal direction", NULL),
	SHELL_CMD_ARG(clear, NULL, HELP_NONE, cmd_clear, 1, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(cfb, &cfb_cmds, "Character Framebuffer shell commands",
		   NULL);
