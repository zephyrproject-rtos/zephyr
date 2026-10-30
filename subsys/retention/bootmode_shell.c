/*
 * SPDX-FileCopyrightText: 2026 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>
#include <zephyr/retention/bootmode.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_string_conv.h>
#include <zephyr/sys/util.h>

struct bootmode_name {
	const char *name;
	uint8_t mode;
};

static const struct bootmode_name bootmode_names[] = {
	{"normal", BOOT_MODE_TYPE_NORMAL},
	{"bootloader", BOOT_MODE_TYPE_BOOTLOADER},
};

static int bootmode_parse(const struct shell *sh, const char *arg, uint8_t *mode)
{
	unsigned long val;
	int err = 0;

	for (size_t i = 0; i < ARRAY_SIZE(bootmode_names); i++) {
		if (strcmp(arg, bootmode_names[i].name) == 0) {
			*mode = bootmode_names[i].mode;
			return 0;
		}
	}

	val = shell_strtoul(arg, 0, &err);
	if ((err != 0) || (val > UINT8_MAX)) {
		shell_error(sh, "Invalid boot mode: %s", arg);
		return -EINVAL;
	}

	*mode = (uint8_t)val;

	return 0;
}

static int cmd_bootmode_check(const struct shell *sh, size_t argc, char **argv)
{
	uint8_t mode;
	int rc;

	ARG_UNUSED(argc);

	rc = bootmode_parse(sh, argv[1], &mode);
	if (rc != 0) {
		return rc;
	}

	rc = bootmode_check(mode);
	if (rc < 0) {
		shell_error(sh, "Failed to check boot mode: %d", rc);
		return rc;
	}

	if (rc == 0) {
		shell_print(sh, "Boot mode %s: not set", argv[1]);
		return -ENOENT;
	}

	shell_print(sh, "Boot mode %s: set", argv[1]);

	return 0;
}

static int cmd_bootmode_set(const struct shell *sh, size_t argc, char **argv)
{
	uint8_t mode;
	int rc;

	ARG_UNUSED(argc);

	rc = bootmode_parse(sh, argv[1], &mode);
	if (rc != 0) {
		return rc;
	}

	rc = bootmode_set(mode);
	if (rc != 0) {
		shell_error(sh, "Failed to set boot mode: %d", rc);
	}

	return rc;
}

static int cmd_bootmode_clear(const struct shell *sh, size_t argc, char **argv)
{
	int rc;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	rc = bootmode_clear();
	if (rc != 0) {
		shell_error(sh, "Failed to clear boot mode: %d", rc);
	}

	return rc;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_bootmode,
	SHELL_CMD_ARG(check, NULL, SHELL_HELP("Check boot mode", "<normal|bootloader|0-255>"),
		      cmd_bootmode_check, 2, 0),
	SHELL_CMD_ARG(set, NULL, SHELL_HELP("Set boot mode", "<normal|bootloader|0-255>"),
		      cmd_bootmode_set, 2, 0),
	SHELL_CMD_ARG(clear, NULL, SHELL_HELP("Clear boot mode", ""), cmd_bootmode_clear, 1, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(bootmode, &sub_bootmode, "Boot mode commands", NULL);
