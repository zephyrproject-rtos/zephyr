/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/init.h>

#define DEFAULT_PASSWORD "zephyr"

static int login_init(void)
{
	printk("Shell Login Demo\nHint: password = %s\n", DEFAULT_PASSWORD);
	if (!CONFIG_SHELL_CMD_ROOT[0]) {
		shell_set_root_cmd("login");
	}
	return 0;
}

static int check_passwd(char *passwd)
{
	/* example only -- not recommended for production use */
	return strcmp(passwd, DEFAULT_PASSWORD);
}

static int cmd_login(const struct shell *sh, size_t argc, char **argv)
{
	static uint32_t attempts;

	if (check_passwd(argv[1]) != 0) {
		shell_error(sh, "Incorrect password!");
		attempts++;
		if (attempts > 3) {
			k_sleep(K_SECONDS(attempts));
		}
		return -EINVAL;
	}

	/* clear history so password not visible there */
	z_shell_history_purge(sh->history);
	shell_obscure_set(sh, false);
	shell_set_root_cmd(NULL);
	shell_prompt_change(sh, "uart:~$ ");
	shell_print(sh, "Shell Login Demo\n");
	shell_print(sh, "Hit tab for help.\n");
	attempts = 0;
	return 0;
}

static int cmd_logout(const struct shell *sh, size_t argc, char **argv)
{
	shell_set_root_cmd("login");
	shell_obscure_set(sh, true);
	shell_prompt_change(sh, "login: ");
	shell_print(sh, "\n");
	return 0;
}

SHELL_CMD_ARG_REGISTER(login, NULL, "<password>", cmd_login, 2, 0);

SHELL_CMD_REGISTER(logout, NULL, "Log out.", cmd_logout);

SYS_INIT(login_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
