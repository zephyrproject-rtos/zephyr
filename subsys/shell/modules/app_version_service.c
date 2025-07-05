/*
 * Copyright (c) 2025 Alex Fabre
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "app_version.h"
#include <zephyr/shell/shell.h>

static int cmd_app_version_string(const struct shell *sh)
{
	shell_print(sh, APP_VERSION_STRING);
	return 0;
}

static int cmd_app_version_extended_string(const struct shell *sh)
{
	shell_print(sh, APP_VERSION_EXTENDED_STRING);
	return 0;
}

static int cmd_app_build_version(const struct shell *sh)
{
	shell_print(sh, STRINGIFY(APP_BUILD_VERSION));
	return 0;
}

/* clang-format off */

SHELL_STATIC_SUBCMD_SET_CREATE(sub_app,
			       SHELL_CMD(version, NULL, "[none]", cmd_app_version_string),
			       SHELL_CMD(version-extended, NULL, "[none]",
					 cmd_app_version_extended_string),
			       SHELL_CMD(build-version, NULL, "[none]", cmd_app_build_version),
			       SHELL_SUBCMD_SET_END /* Array terminated. */
);

/* clang-format on */

SHELL_CMD_REGISTER(app, &sub_app, "Application version information commands", NULL);
