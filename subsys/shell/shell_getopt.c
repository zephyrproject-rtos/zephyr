/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <shell/shell.h>
#include <shell/shell_getopt.h>

void z_shell_getopt_init(struct getopt_state *state)
{
	getopt_init(state);
}

int shell_getopt(const struct shell *shell, int argc, char *const argv[],
		 const char *ostr)
{
	if (!IS_ENABLED(CONFIG_SHELL_GETOPT)) {
		return 0;
	}

	__ASSERT_NO_MSG(shell);

	return getopt(&shell->ctx->getopt_state, argc, argv, ostr);
}

struct getopt_state *shell_getopt_state_get(const struct shell *shell)
{
	if (!IS_ENABLED(CONFIG_SHELL_GETOPT)) {
		return NULL;
	}

	__ASSERT_NO_MSG(shell);

	return &shell->ctx->getopt_state;
}
