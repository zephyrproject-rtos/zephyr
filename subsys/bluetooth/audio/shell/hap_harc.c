/**
 * @file
 * @brief Bluetooth HAP Hearing Aid Remote Controller (HAP HARC) shell.
 *
 * Copyright (c) 2022-2023 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/audio/hap.h>
#include <zephyr/bluetooth/audio/has.h>
#include <zephyr/shell/shell.h>

#include "shell/bt.h"

#define PRESET_RECORD_NAME(_i, _j) CONCAT(CONCAT(_preset_record_name_, _j), _i)
#define __PRESET_RECORD_NAME_DEFINE(_i, _j)                                                        \
	static char PRESET_RECORD_NAME(_i, _j)[BT_HAS_PRESET_NAME_MAX]
#define _PRESET_RECORD_NAME_DEFINE(_i, _max_count)                                                 \
	LISTIFY(_max_count, __PRESET_RECORD_NAME_DEFINE, (;), _i);
#define _GET_FIRST(_i, _) _i
#define _PRESET_RECORD_INIT(_i, _j)                                                                \
{                                                                                                  \
	.name = PRESET_RECORD_NAME(_i, _j)                                                        \
}
#define _PRESET_RECORD_ARRAY_INIT(_i, _max_count)                                                  \
{                                                                                                  \
	LISTIFY(_max_count, _PRESET_RECORD_INIT, (,), _i)                                          \
}
#define _LISTIFY_COUNT(_count) LISTIFY(_count, _GET_FIRST, (,))
#define PRESET_RECORD_CACHE_DEFINE(_name, _max_harc, _max_count)                                   \
	FOR_EACH_FIXED_ARG(_PRESET_RECORD_NAME_DEFINE, (), _max_count, _LISTIFY_COUNT(_max_harc)); \
	static struct bt_has_preset_record _name[_max_harc][_max_count] = {                        \
	FOR_EACH_FIXED_ARG(_PRESET_RECORD_ARRAY_INIT, (,), _max_count, _LISTIFY_COUNT(_max_harc))  \
	};

static struct bt_hap_harc *harc_insts[CONFIG_BT_MAX_CONN];
PRESET_RECORD_CACHE_DEFINE(preset_cache, CONFIG_BT_MAX_CONN, 10);

static void harc_preset_proc_complete_cb(int err, void *params);
static void harc_preset_proc_status_cb(struct bt_hap_harc *harc, int err, void *params);
static void show_preset_cache(const struct shell *sh, struct bt_hap_harc *harc);

static void harc_connected_cb(struct bt_hap_harc *harc, int err)
{
	struct bt_hap_harc_info info;

	if (err) {
		shell_error(ctx_shell, "HARC %p connect failed (err %d)", (void *)harc, err);
		return;
	}

	shell_print(ctx_shell, "HARC %p connected", (void *)harc);

	err = bt_hap_harc_info_get(harc, &info);
	if (err != 0) {
		shell_error(ctx_shell, "bt_hap_harc_info_get (err %d)", err);
		return;
	}

	shell_print(ctx_shell, "Type: 0x%02x Features: 0x%02x Capabilities: 0x%02x",
		    info.type, info.features, info.caps);
}

static void harc_disconnected_cb(struct bt_hap_harc *harc)
{
	shell_print(ctx_shell, "HARC %p disconnected", (void *)harc);
}

static void harc_unbound_cb(struct bt_hap_harc *harc)
{
	struct bt_conn *conn;
	int err;

	shell_print(ctx_shell, "HARC %p unbound", (void *)harc);

	err = bt_hap_harc_conn_get(harc, &conn);
	if (err != 0) {
		shell_error(ctx_shell, "Failed to get conn (err %d)", err);
		return;
	}

	harc_insts[bt_conn_index(conn)] = NULL;
}

static struct bt_hap_harc_cb harc_cb = {
	.connected = harc_connected_cb,
	.disconnected = harc_disconnected_cb,
	.unbound = harc_unbound_cb,
};

static void harc_preset_active_cb(struct bt_hap_harc *harc, uint8_t index)
{
	shell_print(ctx_shell, "HARC %p preset active index 0x%02x", (void *)harc, index);

	show_preset_cache(ctx_shell, harc);
}

static struct bt_has_preset_record *record_lookup_index(struct bt_hap_harc *harc, uint8_t index)
{
	struct bt_has_preset_record *row;
	struct bt_conn *conn;
	int err;

	err = bt_hap_harc_conn_get(harc, &conn);
	if (err != 0) {
		shell_error(ctx_shell, "Failed to get conn (err %d)", err);
		return NULL;
	}

	row = preset_cache[bt_conn_index(conn)];

	for (size_t i = 0; i < ARRAY_SIZE(preset_cache[bt_conn_index(conn)]); i++) {
		if (row[i].index == index) {
			return &row[i];
		}
	}

	return NULL;
}

static void harc_preset_store_cb(struct bt_hap_harc *harc,
				 const struct bt_has_preset_record *record)
{
	struct bt_has_preset_record *slot;

	slot = record_lookup_index(harc, record->index);
	if (slot == NULL) {
		slot = record_lookup_index(harc, BT_HAS_PRESET_INDEX_NONE);
		if (slot == NULL) {
			shell_warn(ctx_shell,
				   "HARC %p No slot left for Index: 0x%02x\tProp: 0x%02x\tName: %s",
				   (void *)harc, record->index, record->properties, record->name);
			return;
		}
	}

	slot->index = record->index;
	slot->properties = record->properties;
	strcpy((char *)slot->name, record->name);
}

static void harc_preset_remove_cb(struct bt_hap_harc *harc, uint8_t start_index, uint8_t end_index)
{
	struct bt_conn *conn;
	int err;

	err = bt_hap_harc_conn_get(harc, &conn);
	if (err != 0) {
		shell_error(ctx_shell, "Failed to get conn (err %d)", err);
		return;
	}

	for (size_t i = 0; i < ARRAY_SIZE(preset_cache[bt_conn_index(conn)]); i++) {
		struct bt_has_preset_record *entry = &preset_cache[bt_conn_index(conn)][i];

		if (entry->index >= start_index && entry->index <= end_index) {
			memset(entry, 0, sizeof(*entry));
		}
	}
}

static void harc_preset_available_cb(struct bt_hap_harc *harc, uint8_t index, bool available)
{
	struct bt_has_preset_record *record;
	struct bt_conn *conn;
	int err;

	err = bt_hap_harc_conn_get(harc, &conn);
	if (err != 0) {
		shell_error(ctx_shell, "Failed to get conn (err %d)", err);
		return;
	}

	record = record_lookup_index(harc, index);
	if (record != NULL) {
		if (available) {
			record->properties |= BT_HAS_PROP_AVAILABLE;
		} else {
			record->properties &= ~BT_HAS_PROP_AVAILABLE;
		}
	}
}

static void show_preset_cache(const struct shell *sh, struct bt_hap_harc *harc)
{
	uint8_t active_preset_index;
	struct bt_conn *conn;
	int err;

	shell_print(sh, "HARC %p presets:", (void *)harc);

	err = bt_hap_harc_conn_get(harc, &conn);
	if (err != 0) {
		shell_error(sh, "Failed to get conn (err %d)", err);
		return;
	}

	err = bt_hap_harc_preset_get_active(harc, &active_preset_index);
	if (err != 0) {
		shell_error(ctx_shell, "bt_hap_harc_preset_get_active (err %d)", err);
		return;
	}

	shell_print(sh, "Active\tIndex\tProperties\tName");

	for (size_t i = 0; i < ARRAY_SIZE(preset_cache[bt_conn_index(conn)]); i++) {
		struct bt_has_preset_record *record = &preset_cache[bt_conn_index(conn)][i];
		bool is_active;

		if (record->index == BT_HAS_PRESET_INDEX_NONE) {
			break;
		}

		is_active = active_preset_index == record->index;

		shell_print(sh, "%s\t0x%02x\t0x%02x\t\t%s", is_active ? "[*]" : "[ ]",
			    record->index, record->properties, record->name);
	}
}

static void harc_preset_commit_cb(struct bt_hap_harc *harc)
{
	/* TODO: sort presets by index */
	show_preset_cache(ctx_shell, harc);
}

static int harc_preset_get_cb(struct bt_hap_harc *harc, uint8_t index,
			      struct bt_has_preset_record *record)
{
	struct bt_has_preset_record *entry;

	shell_print(ctx_shell, "HARC %p Index: 0x%02x", (void *)harc, index);

	entry = record_lookup_index(harc, index);
	if (entry == NULL) {
		return -ENOENT;
	}

	record->index = entry->index;
	record->properties = entry->properties;
	record->name = entry->name;

	return 0;
}

static const struct bt_hap_harc_preset_cb harc_preset_cb = {
	.active = harc_preset_active_cb,
	.store = harc_preset_store_cb,
	.remove = harc_preset_remove_cb,
	.available = harc_preset_available_cb,
	.commit = harc_preset_commit_cb,
	.get = harc_preset_get_cb,
};

static int cmd_harc_init(const struct shell *sh, size_t argc, char **argv)
{
	int err;

	err = bt_hap_harc_cb_register(&harc_cb);
	if (err != 0) {
		shell_error(sh, "bt_hap_harc_cb_register (err %d)", err);
	}

	err = bt_hap_harc_preset_cb_register(&harc_preset_cb);
	if (err != 0) {
		shell_error(sh, "bt_hap_harc_preset_cb_register (err %d)", err);
	}

	return err;
}

static int cmd_harc_bind(const struct shell *sh, size_t argc, char **argv)
{
	struct bt_hap_harc **harc;
	int err;

	if (default_conn == NULL) {
		shell_error(sh, "%s: no connection", sh->ctx->active_cmd.syntax);
		return -ENOEXEC;
	}

	harc = &harc_insts[bt_conn_index(default_conn)];

	err = bt_hap_harc_bind(default_conn, harc);
	if (err != 0) {
		shell_error(sh, "bt_hap_harc_bind (err %d)", err);
	}

	return err;
}

static int cmd_harc_unbind(const struct shell *sh, size_t argc, char **argv)
{
	struct bt_hap_harc *harc;
	int err;

	if (default_conn == NULL) {
		shell_error(sh, "%s: no connection", sh->ctx->active_cmd.syntax);
		return -ENOEXEC;
	}

	harc = harc_insts[bt_conn_index(default_conn)];
	if (harc == NULL) {
		shell_error(sh, "%s: %p not bound",
			    sh->ctx->active_cmd.syntax, (void *)default_conn);
		return -ENOEXEC;
	}

	err = bt_hap_harc_unbind(harc);
	if (err != 0) {
		shell_error(sh, "bt_hap_harc_unbind (err %d)", err);
	}

	return err;
}

static struct bt_hap_harc_preset_read_params preset_read_params = {
	.complete = harc_preset_proc_complete_cb,
};

static int cmd_harc_preset_read(const struct shell *sh, size_t argc, char **argv)
{
	struct bt_hap_harc *harc;
	int err;

	if (default_conn == NULL) {
		shell_error(sh, "%s: no connection", sh->ctx->active_cmd.syntax);
		return -ENOEXEC;
	}

	harc = harc_insts[bt_conn_index(default_conn)];
	if (harc == NULL) {
		shell_error(sh, "%s: %p not bound",
			    sh->ctx->active_cmd.syntax, (void *)default_conn);
		return -ENOEXEC;
	}

	preset_read_params.start_index = shell_strtoul(argv[1], 16, &err);
	if (err < 0) {
		shell_error(sh, "Invalid start_index parameter (err %d)", err);
		return err;
	}

	preset_read_params.max_count = shell_strtoul(argv[2], 16, &err);
	if (err < 0) {
		shell_error(sh, "Invalid max_count parameter (err %d)", err);
		return err;
	}

	err = bt_hap_harc_preset_read(harc, &preset_read_params);
	if (err != 0) {
		shell_error(sh, "bt_has_client_discover (err %d)", err);
	}

	return err;
}

static struct bt_hap_harc_preset_set_params preset_set_params = {
	.complete = harc_preset_proc_complete_cb,
	.status = harc_preset_proc_status_cb,
};

static int cmd_harc_preset_set(const struct shell *sh, size_t argc, char **argv)
{
	struct bt_hap_harc *harc[BT_HAP_HARC_SET_SIZE_MAX];
	struct bt_hap_harc_info info;
	uint8_t count = 1;
	uint8_t index;
	int err = 0;

	if (default_conn == NULL) {
		shell_error(sh, "%s: no connection", sh->ctx->active_cmd.syntax);
		return -ENOEXEC;
	}

	harc[0] = harc_insts[bt_conn_index(default_conn)];
	if (harc[0] == NULL) {
		shell_error(sh, "%s: %p not bound",
			    sh->ctx->active_cmd.syntax, (void *)default_conn);
		return -ENOEXEC;
	}

	index = shell_strtoul(argv[1], 16, &err);
	if (err < 0) {
		shell_error(sh, "Invalid index parameter (err %d)", err);
		return -ENOEXEC;
	}

	err = bt_hap_harc_info_get(harc[0], &info);
	if (err != 0) {
		shell_error(sh, "bt_hap_harc_info_get (err %d)", err);
		return -ENOEXEC;
	}

	if (info.type == BT_HAS_HEARING_AID_TYPE_BINAURAL && info.binaural.pair != NULL) {
		harc[1] = info.binaural.pair;
		count++;
	}

	err = bt_hap_harc_preset_set(harc, 1, index, &preset_set_params);
	if (err != 0) {
		shell_error(sh, "bt_hap_harc_preset_set (err %d)", err);
		return -ENOEXEC;
	}

	return 0;
}

static int cmd_harc_preset_set_next(const struct shell *sh, size_t argc, char **argv)
{
	struct bt_hap_harc *harc[BT_HAP_HARC_SET_SIZE_MAX];
	struct bt_hap_harc_info info;
	uint8_t count = 1;
	int err;

	if (default_conn == NULL) {
		shell_error(sh, "%s: no connection", sh->ctx->active_cmd.syntax);
		return -ENOEXEC;
	}

	harc[0] = harc_insts[bt_conn_index(default_conn)];
	if (harc[0] == NULL) {
		shell_error(sh, "%s: %p not bound",
			    sh->ctx->active_cmd.syntax, (void *)default_conn);
		return -ENOEXEC;
	}

	err = bt_hap_harc_info_get(harc[0], &info);
	if (err != 0) {
		shell_error(sh, "bt_hap_harc_info_get (err %d)", err);
		return -ENOEXEC;
	}

	if (info.type == BT_HAS_HEARING_AID_TYPE_BINAURAL && info.binaural.pair != NULL) {
		harc[1] = info.binaural.pair;
		count++;
	}

	err = bt_hap_harc_preset_set_next(harc, 1, &preset_set_params);
	if (err != 0) {
		shell_error(sh, "bt_hap_harc_preset_set_next (err %d)", err);
		return -ENOEXEC;
	}

	return 0;
}

static int cmd_harc_preset_set_prev(const struct shell *sh, size_t argc, char **argv)
{
	struct bt_hap_harc *harc[BT_HAP_HARC_SET_SIZE_MAX];
	struct bt_hap_harc_info info;
	uint8_t count = 1;
	int err;

	if (default_conn == NULL) {
		shell_error(sh, "%s: no connection", sh->ctx->active_cmd.syntax);
		return -ENOEXEC;
	}

	harc[0] = harc_insts[bt_conn_index(default_conn)];
	if (harc[0] == NULL) {
		shell_error(sh, "%s: %p not bound",
			    sh->ctx->active_cmd.syntax, (void *)default_conn);
		return -ENOEXEC;
	}

	err = bt_hap_harc_info_get(harc[0], &info);
	if (err != 0) {
		shell_error(sh, "bt_hap_harc_info_get (err %d)", err);
		return -ENOEXEC;
	}

	if (info.type == BT_HAS_HEARING_AID_TYPE_BINAURAL && info.binaural.pair != NULL) {
		harc[1] = info.binaural.pair;
		count++;
	}

	err = bt_hap_harc_preset_set_prev(harc, count, &preset_set_params);
	if (err != 0) {
		shell_error(sh, "bt_hap_harc_preset_set_prev (err %d)", err);
		return -ENOEXEC;
	}

	return 0;
}

static struct bt_hap_harc_preset_write_params preset_write_params = {
	.complete = harc_preset_proc_complete_cb,
	.status = harc_preset_proc_status_cb,
};

static int cmd_harc_preset_write(const struct shell *sh, size_t argc, char **argv)
{
	struct bt_hap_harc *harc[BT_HAP_HARC_SET_SIZE_MAX];
	struct bt_hap_harc_info info;
	uint8_t count = 1;
	int err = 0;

	if (default_conn == NULL) {
		shell_error(sh, "%s: no connection", sh->ctx->active_cmd.syntax);
		return -ENOEXEC;
	}

	harc[0] = harc_insts[bt_conn_index(default_conn)];
	if (harc[0] == NULL) {
		shell_error(sh, "%s: %p not bound",
			    sh->ctx->active_cmd.syntax, (void *)default_conn);
		return -ENOEXEC;
	}

	preset_write_params.complete = NULL;
	preset_write_params.status = NULL;
	preset_write_params.index = shell_strtoul(argv[1], 16, &err);
	if (err < 0) {
		shell_error(sh, "Invalid index parameter (err %d)", err);
		return err;
	}

	preset_write_params.name = argv[2];

	err = bt_hap_harc_info_get(harc[0], &info);
	if (err != 0) {
		shell_error(sh, "bt_hap_harc_info_get (err %d)", err);
		return -ENOEXEC;
	}

	if (info.type == BT_HAS_HEARING_AID_TYPE_BINAURAL && info.binaural.pair != NULL) {
		harc[1] = info.binaural.pair;
		count++;
	}

	err = bt_hap_harc_preset_write(harc, count, &preset_write_params);
	if (err != 0) {
		shell_error(sh, "bt_hap_harc_preset_write (err %d)", err);
		return -ENOEXEC;
	}

	return err;
}

static int cmd_harc(const struct shell *sh, size_t argc, char **argv)
{
	if (argc > 1) {
		shell_error(sh, "%s unknown parameter: %s", argv[0], argv[1]);
	} else {
		shell_error(sh, "%s missing subcomand", argv[0]);
	}

	return -ENOEXEC;
}

static void harc_preset_proc_complete_cb(int err, void *params)
{
	if (params == &preset_read_params) {
		shell_print(ctx_shell, "Preset read complete (err %d)", err);
	} else if (params == &preset_write_params) {
		shell_print(ctx_shell, "Preset write complete (err %d)", err);
	} else if (params == &preset_set_params) {
		shell_print(ctx_shell, "Preset set complete (err %d)", err);
	} else {
		shell_error(ctx_shell, "Unexpected params %p", params);
	}
}

static void harc_preset_proc_status_cb(struct bt_hap_harc *harc, int err, void *params)
{
	if (params == &preset_read_params) {
		shell_print(ctx_shell, "HARC %p Preset read status (err %d)", harc, err);
	} else if (params == &preset_write_params) {
		shell_print(ctx_shell, "HARC %p Preset write status (err %d)", harc, err);
	} else if (params == &preset_set_params) {
		shell_print(ctx_shell, "HARC %p Preset set status (err %d)", harc, err);
	} else {
		shell_error(ctx_shell, "HARC %p Unexpected params %p", harc, params);
	}
}

#define HELP_NONE "[none]"

SHELL_STATIC_SUBCMD_SET_CREATE(harc_cmds,
	SHELL_CMD_ARG(init, NULL, HELP_NONE, cmd_harc_init, 1, 0),
	SHELL_CMD_ARG(bind, NULL, HELP_NONE, cmd_harc_bind, 1, 0),
	SHELL_CMD_ARG(unbind, NULL, HELP_NONE, cmd_harc_unbind, 1, 0),
	SHELL_CMD_ARG(preset-read, NULL, "<start_index> <max_count>", cmd_harc_preset_read, 3, 0),
	SHELL_CMD_ARG(preset-set, NULL, "<index>", cmd_harc_preset_set, 2, 0),
	SHELL_CMD_ARG(preset-next, NULL, HELP_NONE, cmd_harc_preset_set_next, 1, 0),
	SHELL_CMD_ARG(preset-prev, NULL, HELP_NONE, cmd_harc_preset_set_prev, 1, 0),
	SHELL_CMD_ARG(preset-write, NULL, "<index> <name>", cmd_harc_preset_write, 3, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_ARG_REGISTER(harc, &harc_cmds, "Bluetooth HAS HARC commands", cmd_harc, 1, 1);
