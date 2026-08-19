/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup shell_remote
 * @brief Common APIs for the remote shell
 */

#ifndef ZEPHYR_INCLUDE_SHELL_SHELL_REMOTE_COMMON_H_
#define ZEPHYR_INCLUDE_SHELL_SHELL_REMOTE_COMMON_H_

#include <zephyr/shell/shell.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @anchor SHELL_REMOTE_MSG_IDS
 * @name Remote shell message IDs
 * @{
 */

/** @brief Print message. */
#define SHELL_REMOTE_MSG_PRINT      1

/** @brief Get command. */
#define SHELL_REMOTE_MSG_CMD_GET    2

/** @brief Response to get command. */
#define SHELL_REMOTE_MSG_CMD        3

/** @brief Execute command. */
#define SHELL_REMOTE_MSG_EXEC       4

/** @brief Result. */
#define SHELL_REMOTE_MSG_RESULT     5

/**@} */

/** @brief Generic message. */
struct shell_remote_msg_generic {
	/** Message ID. */
	uint8_t id;
};

/** @brief Print message. */
struct shell_remote_msg_print {
	/** Message ID. */
	uint8_t id;
	/** Color. */
	uint8_t color;
	/** Cbprintf package (pre-padded to be aligned to 8 bytes). */
	uint8_t data[0];
};

/** @brief Execute command. */
struct shell_remote_msg_exec {
	/** Message ID. */
	uint8_t id;
	/** Number of arguments. */
	uint8_t argc;
	/** Command level. */
	uint8_t cmd_lvl;
	/** Command handler. */
	shell_cmd_handler handler;
	/** Command arguments. */
	uint8_t data[0];
};

/** @brief Result. */
struct shell_remote_msg_result {
	/** Message ID. */
	uint8_t id;
	/** Result. */
	int result;
};

/** @brief Get command. */
struct shell_remote_msg_cmd_get {
	/** Message ID. */
	uint8_t id;
	/** Parent command. */
	const struct shell_static_entry *parent;
	/** Index. */
	size_t idx;
};

/** @brief Response to get command. */
struct shell_remote_msg_cmd {
	/** Message ID. */
	uint8_t id;
	/** Subcommand. */
	const union shell_cmd_entry *subcmd;
	/** Entry. */
	const struct shell_static_entry *entry;
	/** Command handler. */
	shell_cmd_handler handler;
	/** Command parameters. */
	struct shell_static_args args;
	/** Data (null-terminated syntax followed by optional help). */
	char data[0];
};

/** @brief Union of remote shell messages. */
union shell_remote_msg {
	/** Generic message. */
	struct shell_remote_msg_generic *generic;
	/** Print message. */
	struct shell_remote_msg_print *print;
	/** Command. */
	struct shell_remote_msg_cmd *cmd;
	/** Get command. */
	struct shell_remote_msg_cmd_get *cmd_get;
	/** Execute command. */
	struct shell_remote_msg_exec *exec;
	/** Result. */
	struct shell_remote_msg_result *result;
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SHELL_SHELL_REMOTE_COMMON_H_ */
