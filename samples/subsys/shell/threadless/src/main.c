/*
 * Copyright 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_uart.h>

static int sample_sh_echo(const struct shell *sh, size_t argc, char **argv)
{
	shell_print(sh, "%s", argv[1]);
	return 0;
}

SHELL_CMD_ARG_REGISTER(sample, NULL, "Echo", sample_sh_echo, 2, 2);

int main(void)
{
	/*
	 * Start the shell. This prints the shell prompt and enables
	 * input reception.
	 */
	shell_start(shell_backend_uart_get_ptr());

	while (1) {
		/* Check for new input and process it if present */
		shell_process(shell_backend_uart_get_ptr());
	}

	return 0;
}
