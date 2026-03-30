/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>

#include <zephyr/logging/log.h>
#include <zephyr/net/ftp_client.h>

LOG_MODULE_DECLARE(net_shell);

#include "net_shell_private.h"

#define DEFAULT_FTP_PORT 21

#if defined(CONFIG_FTP_CLIENT)

static const struct shell *ftp_shell;
static struct ftp_client client;
static bool is_initialized;
static bool is_connected;

static void ftp_ctrl_cb(const uint8_t *msg, uint16_t len)
{
	const struct shell *sh = ftp_shell; /* Needed to use PR_INFO() macro */

	PR_INFO("%.*s", len, msg);
}

static void ftp_data_cb(const uint8_t *msg, uint16_t len)
{
	const struct shell *sh = ftp_shell; /* Needed to use PR() macro */

	PR("%.*s", len, msg);
}

#endif /* CONFIG_FTP_CLIENT */

static int cmd_net_ftp_connect(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_FTP_CLIENT)

	const char *username = argv[1];
	const char *password = argv[2];
	const char *hostname = argv[3];
	uint16_t port = DEFAULT_FTP_PORT;
	int sectag = -1;
	int ret = 0;

	if (is_connected) {
		PR_WARNING("Already connected to FTP server\n");
		return 0;
	}

	ftp_shell = sh;

	if (argc > 4) {
		port = shell_strtoul(argv[4], 10, &ret);
		if (ret < 0) {
			PR_WARNING("Invalid port number\n");
			return ret;
		}
	}

	if (argc > 5) {
		sectag = shell_strtol(argv[5], 10, &ret);
		if (ret < 0) {
			PR_WARNING("Invalid sectag\n");
			return ret;
		}
	}

	if (!is_initialized) {
		ret = ftp_init(&client, ftp_ctrl_cb, ftp_data_cb);
		if (ret < 0) {
			return ret;
		}

		is_initialized = true;
	}

	ret = ftp_open(&client, hostname, port, sectag);
	if (ret != 0) {
		PR_ERROR("FTP connect failed (%d)\n", ret);
		return ret;
	}

	ret = ftp_login(&client, username, password);
	if (ret != 0) {
		PR_ERROR("FTP login failed (%d)\n", ret);
		(void)ftp_close(&client);
		return ret < 0 ? ret : -EACCES;
	}

	is_connected = true;

	return 0;

#else /* CONFIG_FTP_CLIENT */

	PR_INFO("Set %s to enable %s support.\n", "CONFIG_FTP_CLIENT", "FTP client");
	return 0;

#endif /* CONFIG_FTP_CLIENT */
}

static int cmd_net_ftp_disconnect(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_FTP_CLIENT)

	int ret;

	if (!is_connected) {
		PR_WARNING("Not connected to FTP server\n");
		return 0;
	}

	ret = ftp_close(&client);
	if (ret != 0) {
		PR_ERROR("FTP disconnect error (%d)\n", ret);
		return ret;
	}

	is_connected = false;

	return 0;

#else /* CONFIG_FTP_CLIENT */

	PR_INFO("Set %s to enable %s support.\n", "CONFIG_FTP_CLIENT", "FTP client");
	return 0;

#endif /* CONFIG_FTP_CLIENT */
}

static int cmd_net_ftp_status(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_FTP_CLIENT)

	int ret;

	ret = ftp_status(&client);
	if (ret != 0) {
		PR_ERROR("FTP status error (%d)\n", ret);
	}

	return ret;

#else /* CONFIG_FTP_CLIENT */

	PR_INFO("Set %s to enable %s support.\n", "CONFIG_FTP_CLIENT", "FTP client");
	return 0;

#endif /* CONFIG_FTP_CLIENT */
}

static int cmd_net_ftp_pwd(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_FTP_CLIENT)

	int ret;

	ret = ftp_pwd(&client);
	if (ret != 0) {
		PR_ERROR("FTP pwd error (%d)\n", ret);
	}

	return ret;

#else /* CONFIG_FTP_CLIENT */

	PR_INFO("Set %s to enable %s support.\n", "CONFIG_FTP_CLIENT", "FTP client");
	return 0;

#endif /* CONFIG_FTP_CLIENT */
}

static int cmd_net_ftp_ls(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_FTP_CLIENT)

	const char *options = "";
	const char *path = ".";
	int ret;

	if (argc == 2) {
		path = argv[1];
	}

	if (argc > 2) {
		options = argv[1];
		path = argv[2];
	}

	ret = ftp_list(&client, options, path);
	if (ret != 0) {
		PR_ERROR("FTP ls error (%d)\n", ret);
	}

	return ret;

#else /* CONFIG_FTP_CLIENT */

	PR_INFO("Set %s to enable %s support.\n", "CONFIG_FTP_CLIENT", "FTP client");
	return 0;

#endif /* CONFIG_FTP_CLIENT */
}

static int cmd_net_ftp_cd(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_FTP_CLIENT)

	const char *path = argv[1];
	int ret;

	ret = ftp_cwd(&client, path);
	if (ret != 0) {
		PR_ERROR("FTP cd error (%d)\n", ret);
	}

	return ret;

#else /* CONFIG_FTP_CLIENT */

	PR_INFO("Set %s to enable %s support.\n", "CONFIG_FTP_CLIENT", "FTP client");
	return 0;

#endif /* CONFIG_FTP_CLIENT */
}

static int cmd_net_ftp_mkdir(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_FTP_CLIENT)

	const char *dir_name = argv[1];
	int ret;

	ret = ftp_mkd(&client, dir_name);
	if (ret != 0) {
		PR_ERROR("FTP mkdir error (%d)\n", ret);
	}

	return ret;

#else /* CONFIG_FTP_CLIENT */

	PR_INFO("Set %s to enable %s support.\n", "CONFIG_FTP_CLIENT", "FTP client");
	return 0;

#endif /* CONFIG_FTP_CLIENT */
}

static int cmd_net_ftp_rmdir(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_FTP_CLIENT)

	const char *dir_name = argv[1];
	int ret;

	ret = ftp_rmd(&client, dir_name);
	if (ret != 0) {
		PR_ERROR("FTP rmdir error (%d)\n", ret);
	}

	return ret;

#else /* CONFIG_FTP_CLIENT */

	PR_INFO("Set %s to enable %s support.\n", "CONFIG_FTP_CLIENT", "FTP client");
	return 0;

#endif /* CONFIG_FTP_CLIENT */
}

static int cmd_net_ftp_rename(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_FTP_CLIENT)

	const char *old_name = argv[1];
	const char *new_name = argv[2];
	int ret;

	ret = ftp_rename(&client, old_name, new_name);
	if (ret != 0) {
		PR_ERROR("FTP rename error (%d)\n", ret);
	}

	return ret;

#else /* CONFIG_FTP_CLIENT */

	PR_INFO("Set %s to enable %s support.\n", "CONFIG_FTP_CLIENT", "FTP client");
	return 0;

#endif /* CONFIG_FTP_CLIENT */
}

static int cmd_net_ftp_rm(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_FTP_CLIENT)

	const char *file_name = argv[1];
	int ret;

	ret = ftp_delete(&client, file_name);
	if (ret != 0) {
		PR_ERROR("FTP delete error (%d)\n", ret);
	}

	return ret;

#else /* CONFIG_FTP_CLIENT */

	PR_INFO("Set %s to enable %s support.\n", "CONFIG_FTP_CLIENT", "FTP client");
	return 0;

#endif /* CONFIG_FTP_CLIENT */
}

static int cmd_net_ftp_get(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_FTP_CLIENT)

	const char *file_name = argv[1];
	int ret;

	ret = ftp_get(&client, file_name);
	if (ret != 0) {
		PR_ERROR("FTP get error (%d)\n", ret);
	}

	return ret;

#else /* CONFIG_FTP_CLIENT */

	PR_INFO("Set %s to enable %s support.\n", "CONFIG_FTP_CLIENT", "FTP client");
	return 0;

#endif /* CONFIG_FTP_CLIENT */
}

#if defined(CONFIG_FTP_CLIENT)

static uint8_t file_buf[2048 + 1]; /* +1 for NULL terminator */
static size_t file_len;

static int file_data_input(const struct shell *sh)
{
	bool nextline = true;
	int ret;

	file_len = 0;

	PR_INFO("Input mode (max %d characters). End the line with \\ to start a new one:\n",
		sizeof(file_buf) - 1);

	while (nextline) {
		ret = shell_readline(sh, file_buf + file_len,
				     sizeof(file_buf) - file_len, K_FOREVER);
		if (ret < 0) {
			PR_ERROR("File input error (%d)\n", ret);
			return ret;
		}

		file_len += ret;
		nextline = false;

		if (file_len > 0 && file_buf[file_len - 1] == '\\') {
			if (file_len > 1 && file_buf[file_len - 2] == '\\') {
				/* \ escaped */
				file_len--;
			} else {
				file_buf[file_len - 1] = '\n';
				nextline = true;
			}
		}
	}

	return 0;
}
#endif /* CONFIG_FTP_CLIENT */

static int cmd_net_ftp_put(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_FTP_CLIENT)

	char file_name[128];
	int ret;

	if (strlen(argv[1]) > sizeof(file_name) - 1) {
		PR_ERROR("File name too long\n");
		return -ENOEXEC;
	}

	/* Need to copy as shell_readline() overwrites the command buffer */
	strcpy(file_name, argv[1]);

	ret = file_data_input(sh);
	if (ret != 0) {
		return ret;
	}

	ret = ftp_put(&client, file_name, file_buf, file_len, FTP_PUT_NORMAL);
	if (ret != 0) {
		PR_ERROR("FTP put error (%d)\n", ret);
	}

	return ret;

#else /* CONFIG_FTP_CLIENT */

	PR_INFO("Set %s to enable %s support.\n", "CONFIG_FTP_CLIENT", "FTP client");
	return 0;

#endif /* CONFIG_FTP_CLIENT */
}

static int cmd_net_ftp_append(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_FTP_CLIENT)

	char file_name[128];
	int ret;

	if (strlen(argv[1]) > sizeof(file_name) - 1) {
		PR_ERROR("File name too long\n");
		return -ENOEXEC;
	}

	/* Need to copy as shell_readline() overwrites the command buffer */
	strcpy(file_name, argv[1]);

	ret = file_data_input(sh);
	if (ret != 0) {
		return ret;
	}

	ret = ftp_put(&client, file_name, file_buf, file_len, FTP_PUT_APPEND);
	if (ret != 0) {
		PR_ERROR("FTP put error (%d)\n", ret);
	}

	return ret;

#else /* CONFIG_FTP_CLIENT */

	PR_INFO("Set %s to enable %s support.\n", "CONFIG_FTP_CLIENT", "FTP client");
	return 0;

#endif /* CONFIG_FTP_CLIENT */
}

static int cmd_net_ftp_mode(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_FTP_CLIENT)

	enum ftp_transfer_type transfer_type;
	const char *type = argv[1];
	int ret;

	if (strcmp(type, "ascii") == 0 || strcmp(type, "a") == 0) {
		transfer_type = FTP_TYPE_ASCII;
	} else if (strcmp(type, "binary") == 0 || strcmp(type, "b") == 0) {
		transfer_type = FTP_TYPE_BINARY;
	} else {
		PR_WARNING("Invalid transfer type\n");
		return -ENOEXEC;
	}

	ret = ftp_type(&client, transfer_type);
	if (ret != 0) {
		PR_ERROR("FTP type set error (%d)\n", ret);
	}

	return ret;

#else /* CONFIG_FTP_CLIENT */

	PR_INFO("Set %s to enable %s support.\n", "CONFIG_FTP_CLIENT", "FTP client");
	return 0;

#endif /* CONFIG_FTP_CLIENT */
}

SHELL_STATIC_SUBCMD_SET_CREATE(net_cmd_ftp,
	SHELL_CMD_ARG(connect, NULL,
		      SHELL_HELP("Connect to FTP server.",
				 "<username> <password> <hostname> [<port> <sec_tag>]"),
		      cmd_net_ftp_connect, 4, 2),
	SHELL_CMD_ARG(disconnect, NULL, SHELL_HELP("Disconnect from FTP server.", ""),
		      cmd_net_ftp_disconnect, 1, 0),
	SHELL_CMD_ARG(status, NULL, SHELL_HELP("Print connection status.", ""),
		      cmd_net_ftp_status, 1, 0),
	SHELL_CMD_ARG(pwd, NULL, SHELL_HELP("Print working directory.", ""),
		      cmd_net_ftp_pwd, 1, 0),
	SHELL_CMD_ARG(ls, NULL,
		      SHELL_HELP("List information about folder or file.",
				 "[<options> <path>]"),
		      cmd_net_ftp_ls, 1, 2),
	SHELL_CMD_ARG(cd, NULL, SHELL_HELP("Change working directory.", "<path>"),
		      cmd_net_ftp_cd, 2, 0),
	SHELL_CMD_ARG(mkdir, NULL, SHELL_HELP("Create directory.", "<dir_name>"),
		      cmd_net_ftp_mkdir, 2, 0),
	SHELL_CMD_ARG(rmdir, NULL, SHELL_HELP("Delete directory.", "<dir_name>"),
		      cmd_net_ftp_rmdir, 2, 0),
	SHELL_CMD_ARG(rename, NULL, SHELL_HELP("Rename a file.", "<old_name> <new_name>"),
		      cmd_net_ftp_rename, 3, 0),
	SHELL_CMD_ARG(rm, NULL, SHELL_HELP("Delete a file.", "<file_name>"),
		      cmd_net_ftp_rm, 2, 0),
	SHELL_CMD_ARG(get, NULL, SHELL_HELP("Read file content.", "<file_name>"),
		      cmd_net_ftp_get, 2, 0),
	SHELL_CMD_ARG(put, NULL, SHELL_HELP("Write to a file.", "<file_name>"),
		      cmd_net_ftp_put, 2, 0),
	SHELL_CMD_ARG(append, NULL, SHELL_HELP("Append a file.", "<file_name>"),
		      cmd_net_ftp_append, 2, 0),
	SHELL_CMD_ARG(mode, NULL, SHELL_HELP("Select transfer type.", "<ascii/binary>"),
		      cmd_net_ftp_mode, 2, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_SUBCMD_ADD((net), ftp, &net_cmd_ftp, "FTP client commands", NULL, 1, 0);
