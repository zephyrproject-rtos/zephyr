/*
 * Copyright (c) 2024 Grant Ramsay <grant.ramsay@hotmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/net/ssh/keygen.h>
#include <zephyr/shell/shell.h>
#include <zephyr/settings/settings.h>

#include <stdio.h>
#include <stdlib.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_shell);

#define CTRL_C 0x03

struct key_load_param {
	void *data;
	size_t len; /* Max len in, actual len out */
	bool found;
};

#define MAX_KEY_SIZE 2048
static uint8_t key_buf[MAX_KEY_SIZE];
static int pubkey_import_key_id;
static size_t pubkey_import_len;

static int settings_load_key_cb(const char *key,
				size_t len,
				settings_read_cb read_cb,
				void *cb_arg,
				void *param)
{
	struct key_load_param *load_param = param;
	ssize_t read = read_cb(cb_arg, load_param->data, load_param->len);

	if (read < 0) {
		return (int)read;
	}

	load_param->len = read;
	load_param->found = true;
	return 0;
}

static int cmd_key_gen(const struct shell *sh, size_t argc, char **argv)
{
	int ret;
	int key_id = strtol(argv[1], NULL, 10);
	const char *key_type_str = argv[2];
	int key_bits = strtol(argv[3], NULL, 10);
	enum ssh_host_key_type key_type;

	ARG_UNUSED(argc);

	if (strcmp(key_type_str, "rsa") == 0) {
		key_type = SSH_HOST_KEY_TYPE_RSA;
	} else {
		shell_error(sh, "Unsupported key type: \"%s\"", key_type_str);
		return -EINVAL;
	}

	ret = ssh_keygen(key_id, key_type, key_bits);
	if (ret != 0) {
		shell_error(sh, "Failed to generate ssh key");
	}

	return ret;
}

static int cmd_key_free(const struct shell *sh, size_t argc, char **argv)
{
	int key_id = strtol(argv[1], NULL, 10);
	int ret;

	ARG_UNUSED(argc);

	ret = ssh_keygen_free(key_id);
	if (ret != 0) {
		shell_error(sh, "Failed to free ssh key");
	}

	return ret;
}

static int cmd_key_save(const struct shell *sh, size_t argc, char **argv)
{
	int ret;
	int key_id = strtol(argv[1], NULL, 10);
	const char *key_type = argv[2];
	const char *key_name = argv[3];
	char setting_name[64];
	bool private_key;

	ARG_UNUSED(argc);

	if (snprintf(setting_name, sizeof(setting_name), "ssh/keys/%s",
		     key_name) >= sizeof(setting_name)) {
		return -EINVAL;
	}

	if (strcmp(key_type, "pub") == 0) {
		private_key = false;
	} else if (strcmp(key_type, "priv") == 0) {
		private_key = true;
	} else {
		shell_error(sh, "Invalid key type");
		return -EINVAL;
	}

	ret = ssh_keygen_export(key_id, private_key, SSH_HOST_KEY_FORMAT_DER,
				key_buf, sizeof(key_buf));
	if (ret < 0) {
		shell_error(sh, "Key export failed: %d", ret);
		return ret;
	}

	ret = settings_save_one(setting_name, key_buf, ret);
	if (ret != 0) {
		LOG_ERR("Save setting failed: %d", ret);
		return ret;
	}

	LOG_INF("Saved %s key: %s (#%d)", key_type, key_name, key_id);

	return 0;
}

static int cmd_key_load(const struct shell *sh, size_t argc, char **argv)
{
	int ret;
	int key_id = strtol(argv[1], NULL, 10);
	const char *key_type = argv[2];
	const char *key_name = argv[3];
	char setting_name[64];
	bool private_key;
	struct key_load_param load_param = {
		.data = key_buf,
		.len = sizeof(key_buf),
		.found = false,
	};

	ARG_UNUSED(argc);

	if (snprintf(setting_name, sizeof(setting_name), "ssh/keys/%s",
		     key_name) >= sizeof(setting_name)) {
		return -EINVAL;
	}

	ret = settings_load_subtree_direct(setting_name, settings_load_key_cb,
					   &load_param);
	if (ret != 0) {
		LOG_ERR("Load setting failed: %d", ret);
		return ret;
	}

	if (!load_param.found) {
		shell_info(sh, "Key \"%s\" not found in settings", key_name);
		return -ENOENT;
	}

	if (strcmp(key_type, "pub") == 0) {
		private_key = false;
	} else if (strcmp(key_type, "priv") == 0) {
		private_key = true;
	} else {
		shell_error(sh, "Invalid key type");
		return -EINVAL;
	}

	ret = ssh_keygen_import(key_id, private_key, SSH_HOST_KEY_FORMAT_DER,
				key_buf, load_param.len);
	if (ret < 0) {
		shell_error(sh, "Key import failed: %d", ret);
		return ret;
	}

	LOG_INF("Loaded %s key: %s (#%d)", key_type, key_name, key_id);

	return 0;
}

static int cmd_pubkey_export(const struct shell *sh, size_t argc, char **argv)
{
	int ret;
	int key_id = strtol(argv[1], NULL, 10);

	ARG_UNUSED(argc);

	ret = ssh_keygen_export(key_id, false, SSH_HOST_KEY_FORMAT_PEM,
				key_buf, sizeof(key_buf));
	if (ret != 0) {
		shell_error(sh, "Key export failed: %d", ret);
		return ret;
	}

	shell_print(sh, "%s", key_buf);

	return 0;
}

static void shell_bypass_pubkey_import(const struct shell *sh, uint8_t *data, size_t len,
				       void *user_data)
{
	ARG_UNUSED(user_data);

	shell_fprintf(sh, SHELL_NORMAL, "%.*s", len, data);

	if (pubkey_import_len + len >= sizeof(key_buf)) {
		shell_error(sh, "Key too big");
		shell_set_bypass(sh, NULL, NULL);
		return;
	}

	for (size_t i = 0; i < len; i++) {
		/* Terminal sends \r for line endings, PEM parser expects \n */
		key_buf[pubkey_import_len++] = (data[i] == '\r') ? '\n' : data[i];
	}

	if (key_buf[pubkey_import_len - 1] == CTRL_C) {
		int ret;

		key_buf[pubkey_import_len - 1] = '\0';

		ret = ssh_keygen_import(pubkey_import_key_id, false,
					SSH_HOST_KEY_FORMAT_PEM, key_buf,
					pubkey_import_len);
		if (ret == 0) {
			shell_info(sh, "\nKey import success");
		} else {
			shell_error(sh, "\n");
			shell_error(sh, "Key import failed: %d", ret);
		}

		shell_set_bypass(sh, NULL, NULL);
	}
}

static int cmd_pubkey_import(const struct shell *sh, size_t argc, char **argv)
{
	int key_id = strtol(argv[1], NULL, 10);

	ARG_UNUSED(argc);

	pubkey_import_key_id = key_id;
	pubkey_import_len = 0;

	shell_set_bypass(sh, shell_bypass_pubkey_import, NULL);
	shell_info(sh, "Enter public key in PEM format followed by Ctrl+C:");

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_ssh_key_pub,
	SHELL_CMD_ARG(export, NULL,
		      SHELL_HELP("Export a public key", "<key-id>"),
		      cmd_pubkey_export, 2, 0),
	SHELL_CMD_ARG(import, NULL,
		      SHELL_HELP("Import a public key", "<key-id>"),
		      cmd_pubkey_import, 2, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(net_cmd_ssh_key,
	SHELL_CMD_ARG(gen, NULL,
		      SHELL_HELP("Generate key", "<key-id> <type> <bits>"),
		      cmd_key_gen, 4, 0),
	SHELL_CMD_ARG(free, NULL,
		      SHELL_HELP("Remove a key", "<key-id>"),
		      cmd_key_free, 2, 0),
	SHELL_CMD_ARG(save, NULL,
		      SHELL_HELP("Save a key", "<key-id> <pub|priv> <key-name>"),
		      cmd_key_save, 4, 0),
	SHELL_CMD_ARG(load, NULL,
		      SHELL_HELP("Load a key", "<key-id> <pub|priv> <key-name>"),
		      cmd_key_load, 4, 0),
	SHELL_CMD(pub, &sub_ssh_key_pub, "SSH public key commands", NULL),
	SHELL_SUBCMD_SET_END
);

SHELL_SUBCMD_ADD((net), ssh_key, &net_cmd_ssh_key, "SSH keygen commands", NULL, 1, 1);
