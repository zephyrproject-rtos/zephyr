/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */


/**
 * @file
 * @ingroup shell_remote
 * @brief Support for remote clients
 */

#ifndef ZEPHYR_INCLUDE_SHELL_SHELL_REMOTE_H_
#define ZEPHYR_INCLUDE_SHELL_SHELL_REMOTE_H_

/**
 * @brief Shell remote API
 * @defgroup shell_remote Shell remote
 * @ingroup shell_api
 * @{
 */

#include "zephyr/shell/shell_remote_common.h"
#include <zephyr/shell/shell.h>
#include <zephyr/ipc/ipc_service.h>
#include <zephyr/sys/slist.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CONFIG_SHELL_REMOTE_TMP_BUF_SIZE
/** @brief Default size of the temporary buffer for remote shell command data. */
#define CONFIG_SHELL_REMOTE_TMP_BUF_SIZE 128
#endif

/** @brief Structure to store the data for the remote shell command. */
struct shell_remote_cmd {
	/** Command entry. */
	struct shell_static_entry cmd;

	/** Pointer to the parent remote command. */
	const struct shell_static_entry *rem_cmd;

	/** Temporary buffer for the command syntax and help text. */
	char tmp_buf[CONFIG_SHELL_REMOTE_TMP_BUF_SIZE];
};

/** @brief Structure to store the data for the remote shell instance. */
struct shell_remote_data {
	/** IPC endpoint. */
	struct ipc_ept ept;

	/** Semaphore to synchronize the access to the remote shell instance. */
	struct k_sem sem;

	/** Work queue to process the print messages. */
	struct k_work work;

	/** Pointer to the shell instance. */
	const struct shell *sh;

	/** Space to store the two most recent commands (current and parent). */
	struct shell_remote_cmd cmds[2];

	/** Pointer to the current command. */
	struct shell_remote_cmd *current_cmd;

	/** Result of the last command. */
	int result;

	/** Length of the print message. */
	size_t msg_len;

	/** Pointer to the print message (heap allocated). */
	void *msg;

	/** Flag to indicate if the remote shell instance is ready (bounded). */
	bool ready;
};

/** @brief Structure to store the configuration for the remote shell instance. */
struct shell_remote {
	/** Name of the remote shell instance (command syntax). */
	const char *name;

	/** IPC device. */
	const struct device *ipc;

	/** IPC endpoint configuration. */
	struct ipc_ept_cfg ep_cfg;
};

/** @brief Macro to define a remote shell instance.
 *
 * @param _name Name of the remote shell instance (command syntax).
 * @param ipc_node Device node for the IPC device.
 */
#define SHELL_REMOTE_CONN(_name, ipc_node)                                                         \
	static struct shell_remote_data shell_remote_data_##_name;                                 \
	static const STRUCT_SECTION_ITERABLE(shell_remote, _name) = {                              \
		.name = STRINGIFY(_name), .ipc = DEVICE_DT_GET(ipc_node),                          \
				  .ep_cfg = {.name = CONFIG_SHELL_REMOTE_EP_NAME,                  \
					     .cb =                                                 \
						     {                                             \
							     .bound = shell_remote_ep_bound,       \
							     .received = shell_remote_ep_recv,     \
						     },                                            \
					     .priv = &shell_remote_data_##_name}}

/** @brief Function to get the command entry for the remote shell instance.
 *
 * @param parent Parent command entry.
 * @param idx Index of the command.
 * @param dloc Pointer to the command entry to fill.
 *
 * @return Pointer to the command entry.
 */
const struct shell_static_entry *z_shell_remote_cmd_get(const struct shell_static_entry *parent,
							size_t idx,
							struct shell_static_entry *dloc);

/** @brief Function to execute a command on the remote shell instance.
 *
 * @param shell Shell instance.
 * @param cmd Command entry.
 * @param argc Number of arguments.
 * @param argv Array of arguments.
 * @param cmd_lvl Command level.
 *
 * @return Result of the command execution.
 */
int z_shell_remote_cmd_exec(const struct shell *shell, const struct shell_static_entry *cmd,
			    uint8_t argc, const char **argv, size_t cmd_lvl);

/** @brief Callback function to handle the bound event of the remote shell instance.
 *
 * @param priv Pointer to the private data.
 */
void shell_remote_ep_bound(void *priv);

/** @brief Callback function to handle the received event of the remote shell instance.
 *
 * @param data Pointer to the data.
 * @param len Length of the data.
 * @param priv Pointer to the private data.
 */
void shell_remote_ep_recv(const void *data, size_t len, void *priv);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SHELL_SHELL_REMOTE_H_ */
