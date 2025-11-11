/*
 * Copyright (c) 2024 Trackunit Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/modem/at/user_pipe.h>
#include <zephyr/modem/chat.h>
#include <zephyr/modem/pipelink.h>
#include <zephyr/sys/atomic.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(modem_at_shell, CONFIG_MODEM_LOG_LEVEL);

static struct modem_chat at_shell_chat;
static uint8_t at_shell_chat_receive_buf[CONFIG_MODEM_AT_SHELL_CHAT_RECEIVE_BUF_SIZE];
static uint8_t *at_shell_chat_argv_buf[2];
static uint8_t at_shell_request_buf[CONFIG_MODEM_AT_SHELL_COMMAND_MAX_SIZE];
static struct modem_chat_script_chat at_shell_script_chat[1];
static struct modem_chat_match at_shell_script_chat_matches[2];
static uint8_t at_shell_match_buf[CONFIG_MODEM_AT_SHELL_RESPONSE_MAX_SIZE];
static const struct shell *at_shell_active_shell;

static void at_shell_print_any_match(struct modem_chat *chat, char **argv, uint16_t argc,
				     void *user_data)
{
	if (at_shell_active_shell == NULL) {
		return;
	}

	if (argc != 2) {
		return;
	}

	shell_print(at_shell_active_shell, "%s", argv[1]);
}

static void at_shell_print_match(struct modem_chat *chat, char **argv, uint16_t argc,
				 void *user_data)
{
	if (at_shell_active_shell == NULL) {
		return;
	}

	if (argc != 1) {
		return;
	}

	shell_print(at_shell_active_shell, "%s", argv[0]);
}

MODEM_CHAT_MATCHES_DEFINE(
	at_shell_abort_matches,
	MODEM_CHAT_MATCH("ERROR", "", at_shell_print_match),
);

static void at_shell_script_callback(struct modem_chat *chat,
				     enum modem_chat_script_result result,
				     void *user_data)
{
	modem_at_user_pipe_release();
}

MODEM_CHAT_SCRIPT_DEFINE(
	at_shell_script,
	at_shell_script_chat,
	at_shell_abort_matches,
	at_shell_script_callback,
	CONFIG_MODEM_AT_SHELL_RESPONSE_TIMEOUT_S
);

static void at_shell_init_chat(void)
{
	const struct modem_chat_config at_shell_chat_config = {
		.receive_buf = at_shell_chat_receive_buf,
		.receive_buf_size = sizeof(at_shell_chat_receive_buf),
		.delimiter = "\r",
		.delimiter_size = sizeof("\r") - 1,
		.filter = "\n",
		.filter_size = sizeof("\n") - 1,
		.argv = at_shell_chat_argv_buf,
		.argv_size = ARRAY_SIZE(at_shell_chat_argv_buf),
	};

	modem_chat_init(&at_shell_chat, &at_shell_chat_config);
}

static void at_shell_init_script_chat(void)
{
	/* Match anything except the expected response without progressing script */
	modem_chat_match_init(&at_shell_script_chat_matches[0]);
	modem_chat_match_set_match(&at_shell_script_chat_matches[0], "");
	modem_chat_match_set_separators(&at_shell_script_chat_matches[0], "");
	modem_chat_match_set_callback(&at_shell_script_chat_matches[0], at_shell_print_any_match);
	modem_chat_match_set_partial(&at_shell_script_chat_matches[0], true);
	modem_chat_match_enable_wildcards(&at_shell_script_chat_matches[0], false);

	/* Match the expected response and terminate script */
	modem_chat_match_init(&at_shell_script_chat_matches[1]);
	modem_chat_match_set_match(&at_shell_script_chat_matches[1], "");
	modem_chat_match_set_separators(&at_shell_script_chat_matches[1], "");
	modem_chat_match_set_callback(&at_shell_script_chat_matches[1], at_shell_print_match);
	modem_chat_match_set_partial(&at_shell_script_chat_matches[1], false);
	modem_chat_match_enable_wildcards(&at_shell_script_chat_matches[1], false);

	modem_chat_script_chat_init(at_shell_script_chat);
	modem_chat_script_chat_set_response_matches(at_shell_script_chat,
						    at_shell_script_chat_matches,
						    ARRAY_SIZE(at_shell_script_chat_matches));
	modem_chat_script_chat_set_timeout(at_shell_script_chat,
					   CONFIG_MODEM_AT_SHELL_RESPONSE_TIMEOUT_S);
}

static int at_shell_init(void)
{
	at_shell_init_chat();
	at_shell_init_script_chat();
	modem_at_user_pipe_init(&at_shell_chat);
	return 0;
}

SYS_INIT(at_shell_init, POST_KERNEL, 99);

static int at_shell_cmd_handler(const struct shell *sh, size_t argc, char **argv)
{
	int ret;

	if (argc < 2) {
		return -EINVAL;
	}

	ret = modem_at_user_pipe_claim();
	if (ret < 0) {
		switch (ret) {
		case -EPERM:
			shell_error(sh, "modem is not ready");
			break;
		case -EBUSY:
			shell_error(sh, "script is already running");
			break;
		default:
			shell_error(sh, "unknown");
		}
		return ret;
	}

	strncpy(at_shell_request_buf, argv[1], sizeof(at_shell_request_buf) - 1);
	ret = modem_chat_script_chat_set_request(at_shell_script_chat, at_shell_request_buf);
	if (ret < 0) {
		return -EINVAL;
	}

	if (argc == 3) {
		strncpy(at_shell_match_buf, argv[2], sizeof(at_shell_match_buf) - 1);
	} else {
		strncpy(at_shell_match_buf, "OK", sizeof(at_shell_match_buf) - 1);
	}

	ret = modem_chat_match_set_match(&at_shell_script_chat_matches[1], at_shell_match_buf);
	if (ret < 0) {
		return -EINVAL;
	}

	at_shell_active_shell = sh;

	ret = modem_chat_run_script_async(&at_shell_chat, &at_shell_script);
	if (ret < 0) {
		shell_error(sh, "failed to start script");
		modem_at_user_pipe_release();
	}

	return ret;
}

SHELL_STATIC_SUBCMD_SET_CREATE(modem_sub_cmds,
	SHELL_CMD_ARG(at, NULL, "at <command> <response>", at_shell_cmd_handler, 1, 2),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(modem, &modem_sub_cmds, "Modem commands", NULL);
