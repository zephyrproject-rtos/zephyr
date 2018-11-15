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
#include <shell/shell.h>
#include <display/cfb.h>

#define print(_sh, _ft, ...) \
	shell_fprintf(_sh, SHELL_NORMAL, _ft "\r\n", \
		      ##__VA_ARGS__)
#define error(_sh, _ft, ...) \
	shell_fprintf(_sh, SHELL_ERROR, _ft "\r\n", \
		      ##__VA_ARGS__)

#define HELP_NONE "[none]"
#define HELP_INIT "call \"cfb init\" first"

#define DISPLAY_DRIVER	CONFIG_CHARACTER_FRAMEBUFFER_SHELL_DRIVER_NAME


static struct device *dev;

static int cmd_clear(const struct shell *shell, size_t argc, char *argv[])
{
	int err;

	if (!dev) {
		error(shell, HELP_INIT);
		return -ENODEV;
	}

	err = cfb_framebuffer_clear(dev, true);
	if (err) {
		error(shell, "Framebuffer clear error=%d", err);
		return err;
	}

	err = cfb_framebuffer_finalize(dev);
	if (err) {
		error(shell, "Framebuffer finalize error=%d", err);
		return err;
	}

	print(shell, "Display Cleared");

	return err;
}

static int cmd_cfb_print(const struct shell *shell, int col, int row, char *str)
{
	int err;
	u8_t ppt;

	if (!dev) {
		error(shell, HELP_INIT);
		return -ENODEV;
	}

	ppt = cfb_get_display_parameter(dev, CFB_DISPLAY_PPT);

	err = cfb_framebuffer_clear(dev, false);
	if (err) {
		error(shell, "Framebuffer clear failed error=%d", err);
		return err;
	}

	err = cfb_print(dev, str, col, row * ppt);
	if (err) {
		error(shell, "Failed to print the string %s error=%d",
		      str, err);
		return err;
	}

	err = cfb_framebuffer_finalize(dev);
	if (err) {
		error(shell, "Failed to finalize the Framebuffer error=%d",
		      err);
		return err;
	}

	return err;
}

static int cmd_print(const struct shell *shell, size_t argc, char *argv[])
{
	int err;
	int col, row;

	if (!dev) {
		error(shell, HELP_INIT);
		return -ENODEV;
	}

	err = shell_cmd_precheck(shell, (argc >= 3), NULL, 0);
	if (err) {
		return err;
	}

	col = strtol(argv[1], NULL, 10);
	if (col > cfb_get_display_parameter(dev, CFB_DISPLAY_COLS)) {
		error(shell, "Invalid col=%d position", col);
		return -EINVAL;
	}

	row = strtol(argv[2], NULL, 10);
	if (row > cfb_get_display_parameter(dev, CFB_DISPLAY_ROWS)) {
		error(shell, "Invalid row=%d position", row);
		return -EINVAL;
	}

	err = cmd_cfb_print(shell, col, row, argv[3]);
	if (err) {
		error(shell, "Failed printing to Framebuffer error=%d", err);
	}

	return err;
}

static int cmd_scroll(const struct shell *shell, size_t argc, char *argv[])
{
	int err;
	int col, row;
	int scroll, boundary;

	if (!dev) {
		error(shell, HELP_INIT);
		return -ENODEV;
	}

	err = shell_cmd_precheck(shell, (argc >= 4), NULL, 0);
	if (err) {
		return err;
	}

	if (!strcmp(argv[1], "vertical")) {
		scroll = 1;
		boundary = cfb_get_display_parameter(dev, CFB_DISPLAY_ROWS);
	} else if (!strcmp(argv[1], "horizontal")) {
		scroll = 0;
		boundary = cfb_get_display_parameter(dev, CFB_DISPLAY_COLS);
	} else {
		shell_help_print(shell, NULL, 0);
		/* shell_cmd_precheck returns 1 when help is printed */
		return 1;
	}

	col = strtol(argv[2], NULL, 10);
	if (col > cfb_get_display_parameter(dev, CFB_DISPLAY_COLS)) {
		error(shell, "Invalid col=%d position", col);
		return -EINVAL;
	}

	row = strtol(argv[3], NULL, 10);
	if (row > cfb_get_display_parameter(dev, CFB_DISPLAY_ROWS)) {
		error(shell, "Invalid row=%d position", row);
		return -EINVAL;
	}

	if (scroll) {
		boundary -= row;
	} else {
		col++;
		boundary -= col;
	}

	for (int i = 0; i < boundary; i++) {
		err = cmd_cfb_print(shell, col, row, argv[4]);
		if (err) {
			error(shell, "Failed printing to Framebuffer error=%d",
			      err);
		}
		if (scroll) {
			row++;
		} else {
			col++;
		}
	}

	cmd_cfb_print(shell, 0, 0, "");

	return err;
}

static int cmd_set_font(const struct shell *shell, size_t argc, char *argv[])
{
	int err;
	int idx;
	u8_t height;
	u8_t width;

	if (!dev) {
		error(shell, HELP_INIT);
		return -ENODEV;
	}

	err = shell_cmd_precheck(shell, (argc >= 1), NULL, 0);
	if (err) {
		return err;
	}

	idx = strtol(argv[1], NULL, 10);

	err = cfb_get_font_size(dev, idx, &width, &height);
	if (err) {
		error(shell, "Invalid font idx=%d err=%d\n", idx, err);
		return err;
	}

	err = cfb_framebuffer_set_font(dev, idx);
	if (err) {
		error(shell, "Failed setting font idx=%d err=%d", idx, err);
		return err;
	}

	print(shell, "Font idx=%d height=%d widht=%d set", idx, height, width);

	return err;
}

static int cmd_invert(const struct shell *shell, size_t argc, char *argv[])
{
	int err;

	if (!dev) {
		error(shell, HELP_INIT);
		return -ENODEV;
	}

	err = cfb_framebuffer_invert(dev);
	if (err) {
		error(shell, "Error inverting Framebuffer");
		return err;
	}

	cmd_cfb_print(shell, 0, 0, "");

	print(shell, "Framebuffer Inverted");

	return err;
}

static int cmd_get_fonts(const struct shell *shell, size_t argc, char *argv[])
{
	int err = 0;
	u8_t font_height;
	u8_t font_width;

	if (!dev) {
		error(shell, HELP_INIT);
		return -ENODEV;
	}

	for (int idx = 0; idx < cfb_get_numof_fonts(dev); idx++) {
		if (cfb_get_font_size(dev, idx, &font_width, &font_height)) {
			break;
		}
		print(shell, "idx=%d height=%d width=%d", idx, font_height,
		      font_width);
	}

	return err;
}

static int cmd_get_device(const struct shell *shell, size_t argc, char *argv[])
{
	int err = 0;

	if (!dev) {
		error(shell, HELP_INIT);
		return -ENODEV;
	}

	print(shell, "Framebuffer Device: %s", DISPLAY_DRIVER);

	return err;
}

static int cmd_get_param(const struct shell *shell, size_t argc, char *argv[])
{
	int err;
	int param = -1;
	static const char * const param_name[] = {
		"height", "width", "ppt", "rows", "cols"};

	if (!dev) {
		error(shell, HELP_INIT);
		return -ENODEV;
	}

	err = shell_cmd_precheck(shell, (argc >= 1), NULL, 0);
	if (err) {
		return err;
	}

	/* Parse duplicate filtering data */
	if (argc >= 1) {
		const char *param_filter = argv[1];

		if (!strcmp(param_filter, "all")) {
			param = -1;
		} else if (!strcmp(param_filter, param_name[0])) {
			param = CFB_DISPLAY_HEIGH;
		} else if (!strcmp(param_filter, param_name[1])) {
			param = CFB_DISPLAY_WIDTH;
		} else if (!strcmp(param_filter, param_name[2])) {
			param = CFB_DISPLAY_PPT;
		} else if (!strcmp(param_filter, param_name[3])) {
			param = CFB_DISPLAY_ROWS;
		} else if (!strcmp(param_filter, param_name[4])) {
			param = CFB_DISPLAY_COLS;
		} else {
			shell_help_print(shell, NULL, 0);
			/* shell_cmd_precheck returns 1 when help is printed */
			return 1;
		}
	}

	if (param >= CFB_DISPLAY_HEIGH) {
		print(shell, "param: %s=%d", param_name[param],
		      cfb_get_display_parameter(dev, param));
	} else {
		for (unsigned int i = 0; i <= CFB_DISPLAY_COLS; i++) {
			print(shell, "param: %s=%d", param_name[i],
			      cfb_get_display_parameter(dev, i));

		}
	}

	return err;
}

static int cmd_init(const struct shell *shell, size_t argc, char *argv[])
{
	int err;

	dev = device_get_binding(DISPLAY_DRIVER);

	if (dev == NULL) {
		error(shell, "Device not found");
		return -ENODEV;
	}

	err = cfb_framebuffer_init(dev);
	if (err) {
		error(shell, "Framebuffer initialization failed!");
		return err;
	}

	print(shell, "Framebuffer initialized: %s", DISPLAY_DRIVER);

	cmd_clear(shell, argc, argv);

	return err;
}

SHELL_CREATE_STATIC_SUBCMD_SET(cfb_cmds) {
	SHELL_CMD(init, NULL, HELP_NONE, cmd_init),
	SHELL_CMD(get_device, NULL, HELP_NONE, cmd_get_device),
	SHELL_CMD(get_param, NULL, "<all, height, width, ppt, rows, cols>",
		  cmd_get_param),
	SHELL_CMD(get_fonts, NULL, HELP_NONE, cmd_get_fonts),
	SHELL_CMD(set_font, NULL, "<idx>", cmd_set_font),
	SHELL_CMD(invert, NULL, HELP_NONE, cmd_invert),
	SHELL_CMD(print, NULL, "<col: pos> <row: pos> <text>", cmd_print),
	SHELL_CMD(scroll, NULL, "<dir: (vertical|horizontal)> "
		"<col: pos> <row: pos> <text>", cmd_scroll),
	SHELL_CMD(clear, NULL, HELP_NONE, cmd_clear),
	SHELL_SUBCMD_SET_END
};

static int cmd_cfb(const struct shell *shell, size_t argc, char **argv)
{
	int err;

	if (argc == 1) {
		shell_help_print(shell, NULL, 0);
		/* shell_cmd_precheck returns 1 when help is printed */
		return 1;
	}

	err = shell_cmd_precheck(shell, (argc == 2), NULL, 0);
	if (err) {
		return err;
	}

	error(shell, "%s unknown parameter: %s", argv[0], argv[1]);

	return -EINVAL;
}

SHELL_CMD_REGISTER(cfb, &cfb_cmds, "Character Framebuffer shell commands",
		   cmd_cfb);
