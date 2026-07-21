/*
 * Copyright (c) 2023, Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "posix_shell.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/shell/shell.h>

#define HELP_ENV_GET   "[NAME]"
#define HELP_ENV_SET   "NAME VALUE | NAME=VALUE"
#define HELP_ENV_UNSET "NAME.."

static int cmd_env_get(const struct shell *sh, size_t argc, char **argv)
{
	const char *name;
	const char *value;

	switch (argc) {
	case 1: {
		extern char **environ;
		/* list all environment variables */
		if (environ != NULL) {
			for (char **envp = environ; *envp != NULL; ++envp) {
				shell_print(sh, "%s", *envp);
			}
		}
	} break;
	case 2:
		/* list a specific environment variable */
		name = argv[1];
		value = getenv(name);
		if (value != NULL) {
			shell_print(sh, "%s", value);
		}
		break;
	default:
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

static bool is_shell_env_name(const char *name)
{
	char c;

	if (name == NULL || name[0] == '\0') {
		return false;
	}

	for (size_t i = 0, N = strlen(name); i < N; ++i) {
		c = name[i];

		if (c == '_') {
			continue;
		}

		if (isalpha(c)) {
			continue;
		}

		if (i > 0 && isdigit(c)) {
			continue;
		}

		return false;
	}

	return true;
}

static int cmd_env_set(const struct shell *sh, size_t argc, char **argv)
{
	int ret;
	char *value;
	const char *name;

	switch (argc) {
	case 2:
		name = argv[1];
		value = strchr(argv[1], '=');
		if (value != NULL) {
			*value = '\0';
			++value;
		}
		break;
	case 3:
		name = argv[1];
		value = argv[2];
		break;
	default:
		return EXIT_FAILURE;
	}

	/* silently drop "poorly conditioned" environment variables */
	if (!is_shell_env_name(name)) {
		shell_print(sh, "bad name");
		return EXIT_SUCCESS;
	}

	ret = setenv(name, value, 1);
	if (ret == -1) {
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

static int cmd_env_unset(const struct shell *sh, size_t argc, char **argv)
{
	for (--argc, ++argv; argc > 0; --argc, ++argv) {
		(void)unsetenv(argv[0]);
	}

	return EXIT_SUCCESS;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_env, SHELL_CMD(set, NULL, HELP_ENV_SET, cmd_env_set),
			       SHELL_CMD(get, NULL, HELP_ENV_GET, cmd_env_get),
			       SHELL_CMD(unset, NULL, HELP_ENV_UNSET, cmd_env_unset),
			       SHELL_SUBCMD_SET_END /* Array terminated. */
);

POSIX_CMD_ADD(env, &sub_env, "Print system information", NULL, 1, 255);
