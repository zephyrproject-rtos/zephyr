/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/shell/shell_remote_cli.h>
#include <string.h>

/* Sorted alphabetically for dynamic command completion (see shell_dynamic_get). */
static const char *const dyn_subcmd_names[] = {
	"sub_cmd_0",
	"sub_cmd_1",
	"sub_cmd_2",
};

static void dyn_subcmd_get(size_t idx, struct shell_static_entry *entry)
{
	if (idx < ARRAY_SIZE(dyn_subcmd_names)) {
		entry->syntax = dyn_subcmd_names[idx];
		entry->handler = NULL;
		entry->subcmd = NULL;
		entry->help = "Print this subcommand name";
	} else {
		entry->syntax = NULL;
	}
}

static int cmd_dyn_parent(const struct shell *sh, size_t argc, char **argv)
{
	if (argc < 2) {
		shell_error(sh, "Missing subcommand");
		return -EINVAL;
	}

	shell_print(sh, "%s", argv[1]);
	return 0;
}

SHELL_DYNAMIC_CMD_CREATE(m_sub_dyn_cmd, dyn_subcmd_get);
SHELL_CMD_REGISTER(dyn_cmd, &m_sub_dyn_cmd, "Command with dynamic subcommands", cmd_dyn_parent);

static int cmd_ping(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(sh, "pong");
	return 0;
}

SHELL_CMD_REGISTER(ping, NULL, "Basic remote shell command", cmd_ping);

static int cmd1_handler(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	uint64_t ll = 12345678901234LL;
	uint32_t l = 1234567890;

	shell_print(sh, "cmd1 executed %llu %u", ll, l);

	return 0;
}

static int cmd2_handler(const struct shell *sh, size_t argc, char **argv)
{
	if (argc > 1) {
		shell_print(sh, "cmd2 executed with arg: %s", argv[1]);
	} else {
		shell_print(sh, "cmd2 executed without arg");
	}

	return 0;
}

static int cmd3_handler(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_error(sh, "remote_static_cmd3_err");
	shell_warn(sh, "remote_static_cmd3_warn");
	return 0;
}

static int too_long_string_cmd_handler(const struct shell *sh, size_t argc, char **argv)
{
	/* Long string on stack that will be added to the package included
	 * in the message. It should not fit in the buffer and error message
	 * shall be sent instead.
	 */
	char long_stack_str[CONFIG_SHELL_REMOTE_CLI_BUF_SIZE];

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	memset(long_stack_str, 'x', sizeof(long_stack_str) - 1);
	long_stack_str[sizeof(long_stack_str) - 1] = '\0';

	shell_print(sh, "too_long_string_cmd: %s", long_stack_str);
	return 0;
}

SHELL_SUBCMD_ADD((static_cmd), cmd1, NULL, "help for cmd1", cmd1_handler, 1, 0);
SHELL_SUBCMD_ADD((static_cmd), cmd2, NULL, "help for cmd2", cmd2_handler, 1, 1);
SHELL_SUBCMD_ADD((static_cmd), cmd3, NULL, "help for cmd3", cmd3_handler, 1, 0);
SHELL_SUBCMD_ADD((static_cmd), too_long_string_cmd, NULL, "help for too_long_string_cmd",
		 too_long_string_cmd_handler, 1, 0);

SHELL_SUBCMD_SET_CREATE(multi_static_cmd, (static_cmd));
SHELL_CMD_REGISTER(static_cmd, &multi_static_cmd, "Static command with subcommands", NULL);

int main(void)
{
#ifndef CONFIG_MULTITHREADING
	while (1) {
		shell_remote_cmd_process();
		if (LOG_PROCESS() == false) {
			k_cpu_idle();
		}
	}
#endif
	return 0;
}
