/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/shell/shell.h>
#include <tracing_core.h>

static int cmd_tracing_enable(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	tracing_set_enabled(true);
	shell_print(sh, "tracing enabled");

	return 0;
}

static int cmd_tracing_disable(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	tracing_set_enabled(false);
	shell_print(sh, "tracing disabled");

	return 0;
}

static int cmd_tracing_stats(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(sh, "state:   %s", is_tracing_enabled() ? "enabled" : "disabled");
	shell_print(sh, "dropped: %u packets", tracing_packet_drop_count_get());

	return 0;
}

static int cmd_tracing_flush(const struct shell *sh, size_t argc, char **argv)
{
	int ret;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	ret = tracing_backends_flush();
	if (ret != 0) {
		shell_error(sh, "flush failed: %d", ret);
	} else {
		shell_print(sh, "flushed");
	}

	return ret;
}

SHELL_STATIC_SUBCMD_SET_CREATE(tracing_cmds,
	SHELL_CMD_ARG(enable, NULL, SHELL_HELP("Enable emission of tracing data", ""),
		      cmd_tracing_enable, 1, 0),
	SHELL_CMD_ARG(disable, NULL, SHELL_HELP("Disable emission of tracing data", ""),
		      cmd_tracing_disable, 1, 0),
	SHELL_CMD_ARG(stats, NULL,
		      SHELL_HELP("Show tracing state and dropped-packet count", ""),
		      cmd_tracing_stats, 1, 0),
	SHELL_CMD_ARG(flush, NULL, SHELL_HELP("Flush all registered tracing backends", ""),
		      cmd_tracing_flush, 1, 0),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(tracing, &tracing_cmds, "Tracing subsystem commands", NULL);
