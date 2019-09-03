/** @file
 * @brief Bluetooth shell module
 *
 * Provide some Bluetooth shell commands that can be useful to applications.
 */

/*
 * Copyright (c) 2017 Intel Corporation
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/types.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/printk.h>
#include <sys/byteorder.h>
#include <sys/util.h>
#include <zephyr.h>

#include <settings/settings.h>

#include <bluetooth/hci.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/rfcomm.h>
#include <bluetooth/sdp.h>
#include <bluetooth/hci.h>

#include <shell/shell.h>

#include "bt.h"
#include "ll.h"
#include "hci.h"

#define DEVICE_NAME		CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN		(sizeof(DEVICE_NAME) - 1)

static u8_t selected_id = BT_ID_DEFAULT;
const struct shell *ctx_shell;

#if defined(CONFIG_BT_CONN)
struct bt_conn *default_conn;

/* Connection context for BR/EDR legacy pairing in sec mode 3 */
static struct bt_conn *pairing_conn;

static struct bt_le_oob oob_local;
static struct bt_le_oob oob_remote;
#endif /* CONFIG_BT_CONN */

#define NAME_LEN 30

#define KEY_STR_LEN 33

#if defined(CONFIG_BT_OBSERVER)
static bool data_cb(struct bt_data *data, void *user_data)
{
	char *name = user_data;

	switch (data->type) {
	case BT_DATA_NAME_SHORTENED:
	case BT_DATA_NAME_COMPLETE:
		memcpy(name, data->data, MIN(data->data_len, NAME_LEN - 1));
		return false;
	default:
		return true;
	}
}

static void device_found(const bt_addr_le_t *addr, s8_t rssi, u8_t evtype,
			 struct net_buf_simple *buf)
{
	char le_addr[BT_ADDR_LE_STR_LEN];
	char name[NAME_LEN];

	(void)memset(name, 0, sizeof(name));

	bt_data_parse(buf, data_cb, name);

	bt_addr_le_to_str(addr, le_addr, sizeof(le_addr));
	shell_print(ctx_shell, "[DEVICE]: %s, AD evt type %u, RSSI %i %s",
	      le_addr, evtype, rssi, name);
}
#endif /* CONFIG_BT_OBSERVER */

#if !defined(CONFIG_BT_CONN)
#if 0 /* FIXME: Add support for changing prompt */
static const char *current_prompt(void)
{
	return NULL;
}
#endif
#endif /* !CONFIG_BT_CONN */

#if defined(CONFIG_BT_CONN)
#if 0 /* FIXME: Add support for changing prompt */
static const char *current_prompt(void)
{
	static char str[BT_ADDR_LE_STR_LEN + 2];
	static struct bt_conn_info info;

	if (!default_conn) {
		return NULL;
	}

	if (bt_conn_get_info(default_conn, &info) < 0) {
		return NULL;
	}

	if (info.type != BT_CONN_TYPE_LE) {
		return NULL;
	}

	bt_addr_le_to_str(info.le.dst, str, sizeof(str) - 2);
	strcat(str, "> ");
	return str;
}
#endif

void conn_addr_str(struct bt_conn *conn, char *addr, size_t len)
{
	struct bt_conn_info info;

	if (bt_conn_get_info(conn, &info) < 0) {
		addr[0] = '\0';
		return;
	}

	switch (info.type) {
#if defined(CONFIG_BT_BREDR)
	case BT_CONN_TYPE_BR:
		bt_addr_to_str(info.br.dst, addr, len);
		break;
#endif
	case BT_CONN_TYPE_LE:
		bt_addr_le_to_str(info.le.dst, addr, len);
		break;
	}
}

static void connected(struct bt_conn *conn, u8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	conn_addr_str(conn, addr, sizeof(addr));

	if (err) {
		shell_error(ctx_shell, "Failed to connect to %s (%u)", addr,
			     err);
		goto done;
	}

	shell_print(ctx_shell, "Connected: %s", addr);

	if (!default_conn) {
		default_conn = bt_conn_ref(conn);
	}

done:
	/* clear connection reference for sec mode 3 pairing */
	if (pairing_conn) {
		bt_conn_unref(pairing_conn);
		pairing_conn = NULL;
	}
}

static void disconnected(struct bt_conn *conn, u8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	conn_addr_str(conn, addr, sizeof(addr));
	shell_print(ctx_shell, "Disconnected: %s (reason 0x%02x)", addr, reason);

	if (default_conn == conn) {
		bt_conn_unref(default_conn);
		default_conn = NULL;
	}
}

static bool le_param_req(struct bt_conn *conn, struct bt_le_conn_param *param)
{
	shell_print(ctx_shell, "LE conn  param req: int (0x%04x, 0x%04x) lat %d"
		    " to %d", param->interval_min, param->interval_max,
		    param->latency, param->timeout);

	return true;
}

static void le_param_updated(struct bt_conn *conn, u16_t interval,
			     u16_t latency, u16_t timeout)
{
	shell_print(ctx_shell, "LE conn param updated: int 0x%04x lat %d "
		     "to %d", interval, latency, timeout);
}

#if defined(CONFIG_BT_SMP)
static void identity_resolved(struct bt_conn *conn, const bt_addr_le_t *rpa,
			      const bt_addr_le_t *identity)
{
	char addr_identity[BT_ADDR_LE_STR_LEN];
	char addr_rpa[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(identity, addr_identity, sizeof(addr_identity));
	bt_addr_le_to_str(rpa, addr_rpa, sizeof(addr_rpa));

	shell_print(ctx_shell, "Identity resolved %s -> %s", addr_rpa,
	      addr_identity);
}
#endif

#if defined(CONFIG_BT_SMP) || defined(CONFIG_BT_BREDR)
static void security_changed(struct bt_conn *conn, bt_security_t level,
			     enum bt_security_err err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	conn_addr_str(conn, addr, sizeof(addr));

	if (!err) {
		shell_print(ctx_shell, "Security changed: %s level %u", addr,
			    level);
	} else {
		shell_print(ctx_shell, "Security failed: %s level %u reason %d",
			    addr, level, err);
	}
}
#endif

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
	.le_param_req = le_param_req,
	.le_param_updated = le_param_updated,
#if defined(CONFIG_BT_SMP)
	.identity_resolved = identity_resolved,
#endif
#if defined(CONFIG_BT_SMP) || defined(CONFIG_BT_BREDR)
	.security_changed = security_changed,
#endif
};
#endif /* CONFIG_BT_CONN */

static void bt_ready(int err)
{
	if (err) {
		shell_error(ctx_shell, "Bluetooth init failed (err %d)", err);
		return;
	}

	shell_print(ctx_shell, "Bluetooth initialized");

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		settings_load();
	}

#if defined(CONFIG_BT_CONN)
	default_conn = NULL;

	bt_conn_cb_register(&conn_callbacks);
#endif /* CONFIG_BT_CONN */
}

static int cmd_init(const struct shell *shell, size_t argc, char *argv[])
{
	int err;

	ctx_shell = shell;

	err = bt_enable(bt_ready);
	if (err) {
		shell_error(shell, "Bluetooth init failed (err %d)", err);
	}

	return err;
}

#if defined(CONFIG_BT_HCI)
static int cmd_hci_cmd(const struct shell *shell, size_t argc, char *argv[])
{
	u8_t ogf;
	u16_t ocf;
	struct net_buf *buf = NULL, *rsp;
	int err;

	ogf = strtoul(argv[1], NULL, 16);
	ocf = strtoul(argv[2], NULL, 16);

	if (argc > 3) {
		int i;

		buf = bt_hci_cmd_create(BT_OP(ogf, ocf), argc - 3);

		for (i = 3; i < argc; i++) {
			net_buf_add_u8(buf, strtoul(argv[i], NULL, 16));
		}
	}

	err = bt_hci_cmd_send_sync(BT_OP(ogf, ocf), buf, &rsp);
	if (err) {
		shell_error(shell, "HCI command failed (err %d)", err);
		return err;
	} else {
		shell_hexdump(shell, rsp->data, rsp->len);
		net_buf_unref(rsp);
	}

	return 0;
}
#endif /* CONFIG_BT_HCI */

static int cmd_name(const struct shell *shell, size_t argc, char *argv[])
{
	int err;

	if (argc < 2) {
		shell_print(shell, "Bluetooth Local Name: %s", bt_get_name());
	}

	err = bt_set_name(argv[1]);
	if (err) {
		shell_error(shell, "Unable to set name %s (err %d)", argv[1],
			    err);
		return err;
	}

	return 0;
}

static int cmd_id_create(const struct shell *shell, size_t argc, char *argv[])
{
	char addr_str[BT_ADDR_LE_STR_LEN];
	bt_addr_le_t addr;
	int err;

	if (argc > 1) {
		err = bt_addr_le_from_str(argv[1], "random", &addr);
		if (err) {
			shell_error(shell, "Invalid address");
		}
	} else {
		bt_addr_le_copy(&addr, BT_ADDR_LE_ANY);
	}

	err = bt_id_create(&addr, NULL);
	if (err < 0) {
		shell_error(shell, "Creating new ID failed (err %d)", err);
	}

	bt_addr_le_to_str(&addr, addr_str, sizeof(addr_str));
	shell_print(shell, "New identity (%d) created: %s", err, addr_str);

	return 0;
}

static int cmd_id_reset(const struct shell *shell, size_t argc, char *argv[])
{
	char addr_str[BT_ADDR_LE_STR_LEN];
	bt_addr_le_t addr;
	u8_t id;
	int err;

	if (argc < 2) {
		shell_error(shell, "Identity identifier not specified");
		return -ENOEXEC;
	}

	id = strtol(argv[1], NULL, 10);

	if (argc > 2) {
		err = bt_addr_le_from_str(argv[2], "random", &addr);
		if (err) {
			shell_print(shell, "Invalid address");
			return err;
		}
	} else {
		bt_addr_le_copy(&addr, BT_ADDR_LE_ANY);
	}

	err = bt_id_reset(id, &addr, NULL);
	if (err < 0) {
		shell_print(shell, "Resetting ID %u failed (err %d)", id, err);
		return err;
	}

	bt_addr_le_to_str(&addr, addr_str, sizeof(addr_str));
	shell_print(shell, "Identity %u reset: %s", id, addr_str);

	return 0;
}

static int cmd_id_delete(const struct shell *shell, size_t argc, char *argv[])
{
	u8_t id;
	int err;

	if (argc < 2) {
		shell_error(shell, "Identity identifier not specified");
		return -ENOEXEC;
	}

	id = strtol(argv[1], NULL, 10);

	err = bt_id_delete(id);
	if (err < 0) {
		shell_error(shell, "Deleting ID %u failed (err %d)", id, err);
		return err;
	}

	shell_print(shell, "Identity %u deleted", id);

	return 0;
}

static int cmd_id_show(const struct shell *shell, size_t argc, char *argv[])
{
	bt_addr_le_t addrs[CONFIG_BT_ID_MAX];
	size_t i, count = CONFIG_BT_ID_MAX;

	bt_id_get(addrs, &count);

	for (i = 0; i < count; i++) {
		char addr_str[BT_ADDR_LE_STR_LEN];

		bt_addr_le_to_str(&addrs[i], addr_str, sizeof(addr_str));
		shell_print(shell, "%s%zu: %s", i == selected_id ? "*" : " ", i,
		      addr_str);
	}

	return 0;
}

static int cmd_id_select(const struct shell *shell, size_t argc, char *argv[])
{
	char addr_str[BT_ADDR_LE_STR_LEN];
	bt_addr_le_t addrs[CONFIG_BT_ID_MAX];
	size_t count = CONFIG_BT_ID_MAX;
	u8_t id;

	id = strtol(argv[1], NULL, 10);

	bt_id_get(addrs, &count);
	if (count <= id) {
		shell_error(shell, "Invalid identity");
		return -ENOEXEC;
	}

	bt_addr_le_to_str(&addrs[id], addr_str, sizeof(addr_str));
	shell_print(shell, "Selected identity: %s", addr_str);
	selected_id = id;

	return 0;
}

#if defined(CONFIG_BT_OBSERVER)
static int cmd_active_scan_on(const struct shell *shell, u8_t filter)
{
	int err;
	struct bt_le_scan_param param = {
			.type       = BT_HCI_LE_SCAN_ACTIVE,
			.filter_dup = BT_HCI_LE_SCAN_FILTER_DUP_ENABLE,
			.interval   = BT_GAP_SCAN_FAST_INTERVAL,
			.window     = BT_GAP_SCAN_FAST_WINDOW };

	param.filter_dup = filter;

	err = bt_le_scan_start(&param, device_found);
	if (err) {
		shell_error(shell, "Bluetooth set active scan failed "
		      "(err %d)", err);
		return err;
	} else {
		shell_print(shell, "Bluetooth active scan enabled");
	}

	return 0;
}

static int cmd_passive_scan_on(const struct shell *shell, u8_t filter)
{
	struct bt_le_scan_param param = {
			.type       = BT_HCI_LE_SCAN_PASSIVE,
			.filter_dup = BT_HCI_LE_SCAN_FILTER_DUP_DISABLE,
			.interval   = 0x10,
			.window     = 0x10 };
	int err;

	param.filter_dup = filter;

	err = bt_le_scan_start(&param, device_found);
	if (err) {
		shell_error(shell, "Bluetooth set passive scan failed "
			    "(err %d)", err);
		return err;
	} else {
		shell_print(shell, "Bluetooth passive scan enabled");
	}

	return 0;
}

static int cmd_scan_off(const struct shell *shell)
{
	int err;

	err = bt_le_scan_stop();
	if (err) {
		shell_error(shell, "Stopping scanning failed (err %d)", err);
		return err;
	} else {
		shell_print(shell, "Scan successfully stopped");
	}

	return 0;
}

static int cmd_scan(const struct shell *shell, size_t argc, char *argv[])
{
	const char *action;
	u8_t filter = 0;

	/* Parse duplicate filtering data */
	for (size_t argn = 2; argn < argc; argn++) {
		const char *arg = argv[argn];

		if (!strcmp(arg, "dups")) {
			filter |= BT_LE_SCAN_FILTER_DUPLICATE;
		} else if (!strcmp(arg, "nodups")) {
			filter &= ~BT_LE_SCAN_FILTER_DUPLICATE;
		} else if (!strcmp(arg, "wl")) {
			filter |= BT_LE_SCAN_FILTER_WHITELIST;
		} else {
			shell_help(shell);
			return SHELL_CMD_HELP_PRINTED;
		}
	}

	action = argv[1];
	if (!strcmp(action, "on")) {
		return cmd_active_scan_on(shell, filter);
	} else if (!strcmp(action, "off")) {
		return cmd_scan_off(shell);
	} else if (!strcmp(action, "passive")) {
		return cmd_passive_scan_on(shell, filter);
	} else {
		shell_help(shell);
		return SHELL_CMD_HELP_PRINTED;
	}

	return 0;
}
#endif /* CONFIG_BT_OBSERVER */

#if defined(CONFIG_BT_BROADCASTER)
static const struct bt_data ad_discov[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
};

static int cmd_advertise(const struct shell *shell, size_t argc, char *argv[])
{
	struct bt_le_adv_param param;
	const struct bt_data *ad;
	size_t ad_len;
	int err;

	if (!strcmp(argv[1], "off")) {
		if (bt_le_adv_stop() < 0) {
			shell_error(shell, "Failed to stop advertising");
			return -ENOEXEC;
		} else {
			shell_print(shell, "Advertising stopped");
		}

		return 0;
	}

	param.id = selected_id;
	param.interval_min = BT_GAP_ADV_FAST_INT_MIN_2;
	param.interval_max = BT_GAP_ADV_FAST_INT_MAX_2;

	if (!strcmp(argv[1], "on")) {
		param.options = (BT_LE_ADV_OPT_CONNECTABLE |
				 BT_LE_ADV_OPT_USE_NAME);
	} else if (!strcmp(argv[1], "scan")) {
		param.options = BT_LE_ADV_OPT_USE_NAME;
	} else if (!strcmp(argv[1], "nconn")) {
		param.options = 0U;
	} else {
		goto fail;
	}

	ad = ad_discov;
	ad_len = ARRAY_SIZE(ad_discov);

	for (size_t argn = 2; argn < argc; argn++) {
		const char *arg = argv[argn];

		if (!strcmp(arg, "discov")) {
			/* Default */
		} else if (!strcmp(arg, "non_discov")) {
			ad = NULL;
			ad_len = 0;
		} else if (!strcmp(arg, "wl")) {
			param.options |= BT_LE_ADV_OPT_FILTER_SCAN_REQ;
			param.options |= BT_LE_ADV_OPT_FILTER_CONN;
		} else if (!strcmp(arg, "wl-scan")) {
			param.options |= BT_LE_ADV_OPT_FILTER_SCAN_REQ;
		} else if (!strcmp(arg, "wl-conn")) {
			param.options |= BT_LE_ADV_OPT_FILTER_CONN;
		} else {
			goto fail;
		}
	}

	err = bt_le_adv_start(&param, ad, ad_len, NULL, 0);
	if (err < 0) {
		shell_error(shell, "Failed to start advertising (err %d)",
			    err);
		return err;
	} else {
		shell_print(shell, "Advertising started");
	}

	return 0;

fail:
	shell_help(shell);
	return -ENOEXEC;
}

#if defined(CONFIG_BT_PERIPHERAL)
static int cmd_directed_adv(const struct shell *shell,
			     size_t argc, char *argv[])
{
	int err;
	bt_addr_le_t addr;
	struct bt_conn *conn;
	struct bt_le_adv_param *param = BT_LE_ADV_CONN_DIR;

	err = bt_addr_le_from_str(argv[1], argv[2], &addr);
	if (err) {
		shell_error(shell, "Invalid peer address (err %d)", err);
		return err;
	}

	if (argc > 3) {
		if (!strcmp(argv[3], "low")) {
			param = BT_LE_ADV_CONN_DIR_LOW_DUTY;
		} else {
			shell_help(shell);
			return -ENOEXEC;
		}
	}

	conn = bt_conn_create_slave_le(&addr, param);
	if (!conn) {
		shell_error(shell, "Failed to start directed advertising");
		return -ENOEXEC;
	} else {
		shell_print(shell, "Started directed advertising");

		/* unref connection obj in advance as app user */
		bt_conn_unref(conn);
	}

	return 0;
}
#endif /* CONFIG_BT_PERIPHERAL */
#endif /* CONFIG_BT_BROADCASTER */

#if defined(CONFIG_BT_CONN)
#if defined(CONFIG_BT_CENTRAL)
static int cmd_connect_le(const struct shell *shell, size_t argc, char *argv[])
{
	int err;
	bt_addr_le_t addr;
	struct bt_conn *conn;

	err = bt_addr_le_from_str(argv[1], argv[2], &addr);
	if (err) {
		shell_error(shell, "Invalid peer address (err %d)", err);
		return err;
	}

	conn = bt_conn_create_le(&addr, BT_LE_CONN_PARAM_DEFAULT);

	if (!conn) {
		shell_error(shell, "Connection failed");
		return -ENOEXEC;
	} else {

		shell_print(shell, "Connection pending");

		/* unref connection obj in advance as app user */
		bt_conn_unref(conn);
	}

	return 0;
}

#if !defined(CONFIG_BT_WHITELIST)
static int cmd_auto_conn(const struct shell *shell, size_t argc, char *argv[])
{
	bt_addr_le_t addr;
	int err;

	err = bt_addr_le_from_str(argv[1], argv[2], &addr);
	if (err) {
		shell_error(shell, "Invalid peer address (err %d)", err);
		return err;
	}

	if (argc < 4) {
		return bt_le_set_auto_conn(&addr, BT_LE_CONN_PARAM_DEFAULT);
	} else if (!strcmp(argv[3], "on")) {
		return bt_le_set_auto_conn(&addr, BT_LE_CONN_PARAM_DEFAULT);
	} else if (!strcmp(argv[3], "off")) {
		return bt_le_set_auto_conn(&addr, NULL);
	} else {
		shell_help(shell);
		return SHELL_CMD_HELP_PRINTED;
	}

	return 0;
}
#endif /* !defined(CONFIG_BT_WHITELIST) */
#endif /* CONFIG_BT_CENTRAL */

static int cmd_disconnect(const struct shell *shell, size_t argc, char *argv[])
{
	struct bt_conn *conn;
	int err;

	if (default_conn && argc < 3) {
		conn = bt_conn_ref(default_conn);
	} else {
		bt_addr_le_t addr;

		if (argc < 3) {
			shell_help(shell);
			return SHELL_CMD_HELP_PRINTED;
		}

		err = bt_addr_le_from_str(argv[1], argv[2], &addr);
		if (err) {
			shell_error(shell, "Invalid peer address (err %d)",
				    err);
			return err;
		}

		conn = bt_conn_lookup_addr_le(selected_id, &addr);
	}

	if (!conn) {
		shell_error(shell, "Not connected");
		return -ENOEXEC;
	}

	err = bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	if (err) {
		shell_error(shell, "Disconnection failed (err %d)", err);
		return err;
	}

	bt_conn_unref(conn);

	return 0;
}

static int cmd_select(const struct shell *shell, size_t argc, char *argv[])
{
	struct bt_conn *conn;
	bt_addr_le_t addr;
	int err;

	err = bt_addr_le_from_str(argv[1], argv[2], &addr);
	if (err) {
		shell_error(shell, "Invalid peer address (err %d)", err);
		return err;
	}

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &addr);
	if (!conn) {
		shell_error(shell, "No matching connection found");
		return -ENOEXEC;
	}

	if (default_conn) {
		bt_conn_unref(default_conn);
	}

	default_conn = conn;

	return 0;
}

static const char *get_conn_type_str(u8_t type)
{
	switch (type) {
	case BT_CONN_TYPE_LE: return "LE";
	case BT_CONN_TYPE_BR: return "BR/EDR";
	case BT_CONN_TYPE_SCO: return "SCO";
	default: return "Invalid";
	}
}

static const char *get_conn_role_str(u8_t role)
{
	switch (role) {
	case BT_CONN_ROLE_MASTER: return "master";
	case BT_CONN_ROLE_SLAVE: return "slave";
	default: return "Invalid";
	}
}

static void print_le_addr(const char *desc, const bt_addr_le_t *addr)
{
	char addr_str[BT_ADDR_LE_STR_LEN];

	const char *addr_desc = bt_addr_le_is_identity(addr) ? "identity" :
				bt_addr_le_is_rpa(addr) ? "resolvable" :
				"non-resolvable";

	bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));

	shell_print(ctx_shell, "%s address: %s (%s)", desc, addr_str,
		    addr_desc);
}

static int cmd_info(const struct shell *shell, size_t argc, char *argv[])
{
	struct bt_conn *conn = NULL;
	bt_addr_le_t addr;
	struct bt_conn_info info;
	int err;

	switch (argc) {
	case 1:
		if (default_conn) {
			conn = bt_conn_ref(default_conn);
		}
		break;
	case 2:
		addr.type = BT_ADDR_LE_PUBLIC;
		err = bt_addr_from_str(argv[1], &addr.a);
		if (err) {
			shell_error(shell, "Invalid peer address (err %d)",
				    err);
			return err;
		}
		conn = bt_conn_lookup_addr_le(selected_id, &addr);
		break;
	case 3:
		err = bt_addr_le_from_str(argv[1], argv[2], &addr);

		if (err) {
			shell_error(shell, "Invalid peer address (err %d)",
				    err);
			return err;
		}
		conn = bt_conn_lookup_addr_le(selected_id, &addr);
		break;
	}

	if (!conn) {
		shell_error(shell, "Not connected");
		return -ENOEXEC;
	}

	err = bt_conn_get_info(conn, &info);
	if (err) {
		shell_print(ctx_shell, "Failed to get info");
		goto done;
	}

	shell_print(ctx_shell, "Type: %s, Role: %s, Id: %u",
		    get_conn_type_str(info.type),
		    get_conn_role_str(info.role),
		    info.id);

	if (info.type == BT_CONN_TYPE_LE) {
		print_le_addr("Remote", info.le.dst);
		print_le_addr("Local", info.le.src);
		print_le_addr("Remote on-air", info.le.remote);
		print_le_addr("Local on-air", info.le.local);

		shell_print(ctx_shell, "Interval: 0x%04x (%.2f ms)",
			    info.le.interval, info.le.interval * 1.25);
		shell_print(ctx_shell, "Latency: 0x%04x (%.2f ms)",
			    info.le.latency, info.le.latency * 1.25);
		shell_print(ctx_shell, "Supervision timeout: 0x%04x (%d ms)",
			    info.le.timeout, info.le.timeout * 10);
	}

#if defined(CONFIG_BT_BREDR)
	if (info.type == BT_CONN_TYPE_BR) {
		char addr_str[BT_ADDR_STR_LEN];

		bt_addr_to_str(info.br.dst, addr_str, sizeof(addr_str));
		shell_print(ctx_shell, "Peer address %s", addr_str);
	}
#endif /* defined(CONFIG_BT_BREDR) */

done:
	bt_conn_unref(conn);

	return err;
}

static int cmd_conn_update(const struct shell *shell,
			    size_t argc, char *argv[])
{
	struct bt_le_conn_param param;
	int err;

	param.interval_min = strtoul(argv[1], NULL, 16);
	param.interval_max = strtoul(argv[2], NULL, 16);
	param.latency = strtoul(argv[3], NULL, 16);
	param.timeout = strtoul(argv[4], NULL, 16);

	err = bt_conn_le_param_update(default_conn, &param);
	if (err) {
		shell_error(shell, "conn update failed (err %d).", err);
	} else {
		shell_print(shell, "conn update initiated.");
	}

	return err;
}

#if defined(CONFIG_BT_CENTRAL)
static int cmd_chan_map(const struct shell *shell, size_t argc, char *argv[])
{
	u8_t chan_map[5] = {};
	int err;

	if (hex2bin(argv[1], strlen(argv[1]), chan_map, 5) == 0) {
		shell_error(shell, "Invalid channel map");
		return -ENOEXEC;
	}
	sys_mem_swap(chan_map, 5);

	err = bt_le_set_chan_map(chan_map);
	if (err) {
		shell_error(shell, "Failed to set channel map (err %d)", err);
	} else {
		shell_print(shell, "Channel map set");
	}

	return err;
}
#endif /* CONFIG_BT_CENTRAL */

static int cmd_oob(const struct shell *shell, size_t argc, char *argv[])
{
	char addr[BT_ADDR_LE_STR_LEN];
	char c[KEY_STR_LEN];
	char r[KEY_STR_LEN];
	int err;

	err = bt_le_oob_get_local(selected_id, &oob_local);
	if (err) {
		shell_error(shell, "OOB data failed");
		return err;
	}

	bt_addr_le_to_str(&oob_local.addr, addr, sizeof(addr));
	bin2hex(oob_local.le_sc_data.c, sizeof(oob_local.le_sc_data.c), c,
		sizeof(c));
	bin2hex(oob_local.le_sc_data.r, sizeof(oob_local.le_sc_data.r), r,
		sizeof(r));

	shell_print(shell, "OOB data:");
	shell_print(shell, "%-26s %-32s %-32s", "addr", "random", "confirm");
	shell_print(shell, "%26s %32s %32s", addr, r, c);

	return 0;
}

static int cmd_oob_remote(const struct shell *shell, size_t argc,
			     char *argv[])
{
	int err;
	bt_addr_le_t addr;

	err = bt_addr_le_from_str(argv[1], argv[2], &addr);
	if (err) {
		shell_error(shell, "Invalid peer address (err %d)", err);
		return err;
	}

	bt_addr_le_copy(&oob_remote.addr, &addr);

	if (argc == 5) {
		hex2bin(argv[3], strlen(argv[3]), oob_remote.le_sc_data.r,
			sizeof(oob_remote.le_sc_data.r));
		hex2bin(argv[4], strlen(argv[4]), oob_remote.le_sc_data.c,
			sizeof(oob_remote.le_sc_data.c));
		bt_set_oob_data_flag(true);
	} else {
		shell_error(shell, "legacy not implemented (%d)", argc);
		return -ENOEXEC;
	}

	return 0;
}

static int cmd_oob_clear(const struct shell *shell, size_t argc, char *argv[])
{
	memset(&oob_remote, 0, sizeof(oob_remote));
	bt_set_oob_data_flag(false);

	return 0;
}

static int cmd_clear(const struct shell *shell, size_t argc, char *argv[])
{
	bt_addr_le_t addr;
	int err;

	if (strcmp(argv[1], "all") == 0) {
		err = bt_unpair(selected_id, NULL);
		if (err) {
			shell_error(shell, "Failed to clear pairings (err %d)",
			      err);
			return err;
		} else {
			shell_print(shell, "Pairings successfully cleared");
		}

		return 0;
	}

	if (argc < 3) {
#if defined(CONFIG_BT_BREDR)
		addr.type = BT_ADDR_LE_PUBLIC;
		err = bt_addr_from_str(argv[1], &addr.a);
#else
		shell_print(shell, "Both address and address type needed");
		return -ENOEXEC;
#endif
	} else {
		err = bt_addr_le_from_str(argv[1], argv[2], &addr);
	}

	if (err) {
		shell_print(shell, "Invalid address");
		return err;
	}

	err = bt_unpair(selected_id, &addr);
	if (err) {
		shell_error(shell, "Failed to clear pairing (err %d)", err);
	} else {
		shell_print(shell, "Pairing successfully cleared");
	}

	return err;
}
#endif /* CONFIG_BT_CONN */

#if defined(CONFIG_BT_SMP) || defined(CONFIG_BT_BREDR)
static int cmd_security(const struct shell *shell, size_t argc, char *argv[])
{
	int err, sec;
	struct bt_conn_info info;

	if (!default_conn || (bt_conn_get_info(default_conn, &info) < 0)) {
		shell_error(shell, "Not connected");
		return -ENOEXEC;
	}

	sec = *argv[1] - '0';

	if ((info.type == BT_CONN_TYPE_BR &&
	    (sec < BT_SECURITY_L0 || sec > BT_SECURITY_L3))) {
		shell_error(shell, "Invalid BR/EDR security level (%d)", sec);
		return -ENOEXEC;
	}

	if ((info.type == BT_CONN_TYPE_LE &&
	    (sec < BT_SECURITY_L1 || sec > BT_SECURITY_L4))) {
		shell_error(shell, "Invalid LE security level (%d)", sec);
		return -ENOEXEC;
	}

	if (argc > 2) {
		if (!strcmp(argv[2], "force-pair")) {
			sec |= BT_SECURITY_FORCE_PAIR;
		} else {
			shell_help(shell);
			return -ENOEXEC;
		}
	}

	err = bt_conn_set_security(default_conn, sec);
	if (err) {
		shell_error(shell, "Setting security failed (err %d)", err);
	}

	return err;
}

static int cmd_bondable(const struct shell *shell, size_t argc, char *argv[])
{
	const char *bondable;

	bondable = argv[1];
	if (!strcmp(bondable, "on")) {
		bt_set_bondable(true);
	} else if (!strcmp(bondable, "off")) {
		bt_set_bondable(false);
	} else {
		shell_help(shell);
		return SHELL_CMD_HELP_PRINTED;
	}

	return 0;
}

static void auth_passkey_display(struct bt_conn *conn, unsigned int passkey)
{
	char addr[BT_ADDR_LE_STR_LEN];
	char passkey_str[7];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	snprintk(passkey_str, 7, "%06u", passkey);

	shell_print(ctx_shell, "Passkey for %s: %s", addr, passkey_str);
}

static void auth_passkey_confirm(struct bt_conn *conn, unsigned int passkey)
{
	char addr[BT_ADDR_LE_STR_LEN];
	char passkey_str[7];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	snprintk(passkey_str, 7, "%06u", passkey);

	shell_print(ctx_shell, "Confirm passkey for %s: %s", addr, passkey_str);
}

static void auth_passkey_entry(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	shell_print(ctx_shell, "Enter passkey for %s", addr);
}

static void auth_cancel(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	conn_addr_str(conn, addr, sizeof(addr));

	shell_print(ctx_shell, "Pairing cancelled: %s", addr);

	/* clear connection reference for sec mode 3 pairing */
	if (pairing_conn) {
		bt_conn_unref(pairing_conn);
		pairing_conn = NULL;
	}
}

static void auth_pairing_confirm(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	shell_print(ctx_shell, "Confirm pairing for %s", addr);
}

static const char *oob_config_str(int oob_config)
{
	switch (oob_config) {
	case BT_CONN_OOB_LOCAL_ONLY:
		return "Local";
	case BT_CONN_OOB_REMOTE_ONLY:
		return "Remote";
	case BT_CONN_OOB_BOTH_PEERS:
		return "Local and Remote";
	case BT_CONN_OOB_NO_DATA:
	default:
		return "no";
	}
}

static void auth_pairing_oob_data_request(struct bt_conn *conn,
					  struct bt_conn_oob_info *oob_info)
{
	char addr[BT_ADDR_LE_STR_LEN];
	struct bt_conn_info info;
	int err;

	err = bt_conn_get_info(conn, &info);
	if (err) {
		return;
	}

	if (oob_info->type == BT_CONN_OOB_LE_SC) {
		struct bt_le_oob_sc_data *oobd_local =
			oob_info->lesc.oob_config != BT_CONN_OOB_REMOTE_ONLY
						  ? &oob_local.le_sc_data
						  : NULL;
		struct bt_le_oob_sc_data *oobd_remote =
			oob_info->lesc.oob_config != BT_CONN_OOB_LOCAL_ONLY
						  ? &oob_remote.le_sc_data
						  : NULL;

		if (oobd_remote &&
		    bt_addr_le_cmp(info.le.remote, &oob_remote.addr)) {
			bt_addr_le_to_str(info.le.remote, addr, sizeof(addr));
			shell_print(ctx_shell,
				    "No OOB data available for remote %s",
				    addr);
			bt_conn_auth_cancel(conn);
			return;
		}

		if (oobd_local &&
		    bt_addr_le_cmp(info.le.local, &oob_local.addr)) {
			bt_addr_le_to_str(info.le.local, addr, sizeof(addr));
			shell_print(ctx_shell,
				    "No OOB data available for local %s",
				    addr);
			bt_conn_auth_cancel(conn);
			return;
		}

		bt_le_oob_set_sc_data(conn, oobd_local, oobd_remote);

		bt_addr_le_to_str(info.le.dst, addr, sizeof(addr));
		shell_print(ctx_shell, "Set %s OOB SC data for %s, ",
			    oob_config_str(oob_info->lesc.oob_config), addr);
	} else {
		shell_print(ctx_shell, "Legacy OOB not supported");
		bt_conn_auth_cancel(conn);
	}
}

static void auth_pairing_complete(struct bt_conn *conn, bool bonded)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	shell_print(ctx_shell, "%s with %s", bonded ? "Bonded" : "Paired",
		    addr);
}

static void auth_pairing_failed(struct bt_conn *conn,
				enum bt_security_err reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	shell_print(ctx_shell, "Pairing failed with %s reason %d", addr,
		    reason);
}

#if defined(CONFIG_BT_BREDR)
static void auth_pincode_entry(struct bt_conn *conn, bool highsec)
{
	char addr[BT_ADDR_STR_LEN];
	struct bt_conn_info info;

	if (bt_conn_get_info(conn, &info) < 0) {
		return;
	}

	if (info.type != BT_CONN_TYPE_BR) {
		return;
	}

	bt_addr_to_str(info.br.dst, addr, sizeof(addr));

	if (highsec) {
		shell_print(ctx_shell, "Enter 16 digits wide PIN code for %s",
			    addr);
	} else {
		shell_print(ctx_shell, "Enter PIN code for %s", addr);
	}

	/*
	 * Save connection info since in security mode 3 (link level enforced
	 * security) PIN request callback is called before connected callback
	 */
	if (!default_conn && !pairing_conn) {
		pairing_conn = bt_conn_ref(conn);
	}
}
#endif

static struct bt_conn_auth_cb auth_cb_display = {
	.passkey_display = auth_passkey_display,
	.passkey_entry = NULL,
	.passkey_confirm = NULL,
#if defined(CONFIG_BT_BREDR)
	.pincode_entry = auth_pincode_entry,
#endif
	.oob_data_request = NULL,
	.cancel = auth_cancel,
	.pairing_confirm = auth_pairing_confirm,
	.pairing_failed = auth_pairing_failed,
	.pairing_complete = auth_pairing_complete,
};

static struct bt_conn_auth_cb auth_cb_display_yes_no = {
	.passkey_display = auth_passkey_display,
	.passkey_entry = NULL,
	.passkey_confirm = auth_passkey_confirm,
#if defined(CONFIG_BT_BREDR)
	.pincode_entry = auth_pincode_entry,
#endif
	.oob_data_request = NULL,
	.cancel = auth_cancel,
	.pairing_confirm = auth_pairing_confirm,
	.pairing_failed = auth_pairing_failed,
	.pairing_complete = auth_pairing_complete,
};

static struct bt_conn_auth_cb auth_cb_input = {
	.passkey_display = NULL,
	.passkey_entry = auth_passkey_entry,
	.passkey_confirm = NULL,
#if defined(CONFIG_BT_BREDR)
	.pincode_entry = auth_pincode_entry,
#endif
	.oob_data_request = NULL,
	.cancel = auth_cancel,
	.pairing_confirm = auth_pairing_confirm,
	.pairing_failed = auth_pairing_failed,
	.pairing_complete = auth_pairing_complete,
};

static struct bt_conn_auth_cb auth_cb_confirm = {
#if defined(CONFIG_BT_BREDR)
	.pincode_entry = auth_pincode_entry,
#endif
	.oob_data_request = NULL,
	.cancel = auth_cancel,
	.pairing_confirm = auth_pairing_confirm,
	.pairing_failed = auth_pairing_failed,
	.pairing_complete = auth_pairing_complete,
};

static struct bt_conn_auth_cb auth_cb_all = {
	.passkey_display = auth_passkey_display,
	.passkey_entry = auth_passkey_entry,
	.passkey_confirm = auth_passkey_confirm,
#if defined(CONFIG_BT_BREDR)
	.pincode_entry = auth_pincode_entry,
#endif
	.oob_data_request = auth_pairing_oob_data_request,
	.cancel = auth_cancel,
	.pairing_confirm = auth_pairing_confirm,
	.pairing_failed = auth_pairing_failed,
	.pairing_complete = auth_pairing_complete,
};

static struct bt_conn_auth_cb auth_cb_oob = {
	.passkey_display = NULL,
	.passkey_entry = NULL,
	.passkey_confirm = NULL,
#if defined(CONFIG_BT_BREDR)
	.pincode_entry = NULL,
#endif
	.oob_data_request = auth_pairing_oob_data_request,
	.cancel = auth_cancel,
	.pairing_confirm = NULL,
	.pairing_failed = auth_pairing_failed,
	.pairing_complete = auth_pairing_complete,
};


static int cmd_auth(const struct shell *shell, size_t argc, char *argv[])
{
	int err;

	if (!strcmp(argv[1], "all")) {
		err = bt_conn_auth_cb_register(&auth_cb_all);
	} else if (!strcmp(argv[1], "input")) {
		err = bt_conn_auth_cb_register(&auth_cb_input);
	} else if (!strcmp(argv[1], "display")) {
		err = bt_conn_auth_cb_register(&auth_cb_display);
	} else if (!strcmp(argv[1], "yesno")) {
		err = bt_conn_auth_cb_register(&auth_cb_display_yes_no);
	} else if (!strcmp(argv[1], "confirm")) {
		err = bt_conn_auth_cb_register(&auth_cb_confirm);
	} else if (!strcmp(argv[1], "oob")) {
		err = bt_conn_auth_cb_register(&auth_cb_oob);
	} else if (!strcmp(argv[1], "none")) {
		err = bt_conn_auth_cb_register(NULL);
	} else {
		shell_help(shell);
		return SHELL_CMD_HELP_PRINTED;
	}

	return err;
}

static int cmd_auth_cancel(const struct shell *shell,
			   size_t argc, char *argv[])
{
	struct bt_conn *conn;

	if (default_conn) {
		conn = default_conn;
	} else if (pairing_conn) {
		conn = pairing_conn;
	} else {
		conn = NULL;
	}

	if (!conn) {
		shell_print(shell, "Not connected");
		return -ENOEXEC;
	}

	bt_conn_auth_cancel(conn);

	return 0;
}

static int cmd_auth_passkey_confirm(const struct shell *shell,
				    size_t argc, char *argv[])
{
	if (!default_conn) {
		shell_print(shell, "Not connected");
		return -ENOEXEC;
	}

	bt_conn_auth_passkey_confirm(default_conn);
	return 0;
}

static int cmd_auth_pairing_confirm(const struct shell *shell,
				    size_t argc, char *argv[])
{
	if (!default_conn) {
		shell_print(shell, "Not connected");
		return -ENOEXEC;
	}

	bt_conn_auth_pairing_confirm(default_conn);
	return 0;
}

#if defined(CONFIG_BT_WHITELIST)
static int cmd_wl_add(const struct shell *shell, size_t argc, char *argv[])
{
	bt_addr_le_t addr;
	int err;

	err = bt_addr_le_from_str(argv[1], argv[2], &addr);
	if (err) {
		shell_error(shell, "Invalid peer address (err %d)", err);
		return err;
	}

	err = bt_le_whitelist_add(&addr);
	if (err) {
		shell_error(shell, "Add to whitelist failed (err %d)", err);
		return err;
	}

	return 0;
}

static int cmd_wl_rem(const struct shell *shell, size_t argc, char *argv[])
{
	bt_addr_le_t addr;
	int err;

	err = bt_addr_le_from_str(argv[1], argv[2], &addr);
	if (err) {
		shell_error(shell, "Invalid peer address (err %d)", err);
		return err;
	}

	err = bt_le_whitelist_rem(&addr);
	if (err) {
		shell_error(shell, "Remove from whitelist failed (err %d)",
			    err);
		return err;
	}
	return 0;
}

static int cmd_wl_clear(const struct shell *shell, size_t argc, char *argv[])
{
	int err;

	err = bt_le_whitelist_clear();
	if (err) {
		shell_error(shell, "Clearing whitelist failed (err %d)", err);
		return err;
	}

	return 0;
}

static int cmd_wl_connect(const struct shell *shell, size_t argc, char *argv[])
{
	int err;
	const char *action = argv[1];

	if (!strcmp(action, "on")) {
		err = bt_conn_create_auto_le(BT_LE_CONN_PARAM_DEFAULT);

		if (err) {
			shell_error(shell, "Auto connect failed (err %d)", err);
			return err;
		}
	} else if (!strcmp(action, "off")) {
		err = bt_conn_create_auto_stop();
		if (err) {
			shell_error(shell, "Auto connect stop failed (err %d)",
				    err);
		}
		return err;
	}

	return 0;
}
#endif /* defined(CONFIG_BT_WHITELIST) */

#if defined(CONFIG_BT_FIXED_PASSKEY)
static int cmd_fixed_passkey(const struct shell *shell,
			     size_t argc, char *argv[])
{
	unsigned int passkey;
	int err;

	if (argc < 2) {
		bt_passkey_set(BT_PASSKEY_INVALID);
		shell_print(shell, "Fixed passkey cleared");
		return 0;
	}

	passkey = atoi(argv[1]);
	if (passkey > 999999) {
		shell_print(shell, "Passkey should be between 0-999999");
		return -ENOEXEC;
	}

	err = bt_passkey_set(passkey);
	if (err) {
		shell_print(shell, "Setting fixed passkey failed (err %d)",
			    err);
	}

	return err;
}
#endif

static int cmd_auth_passkey(const struct shell *shell,
			    size_t argc, char *argv[])
{
	unsigned int passkey;

	if (!default_conn) {
		shell_print(shell, "Not connected");
		return -ENOEXEC;
	}

	passkey = atoi(argv[1]);
	if (passkey > 999999) {
		shell_print(shell, "Passkey should be between 0-999999");
		return -EINVAL;
	}

	bt_conn_auth_passkey_entry(default_conn, passkey);
	return 0;
}
#endif /* CONFIG_BT_SMP) || CONFIG_BT_BREDR */


#define HELP_NONE "[none]"
#define HELP_ADDR_LE "<address: XX:XX:XX:XX:XX:XX> <type: (public|random)>"

SHELL_STATIC_SUBCMD_SET_CREATE(bt_cmds,
	SHELL_CMD_ARG(init, NULL, HELP_ADDR_LE, cmd_init, 1, 0),
#if defined(CONFIG_BT_HCI)
	SHELL_CMD_ARG(hci-cmd, NULL, "<ogf> <ocf> [data]", cmd_hci_cmd, 3, 1),
#endif
	SHELL_CMD_ARG(id-create, NULL, "[addr]", cmd_id_create, 1, 1),
	SHELL_CMD_ARG(id-reset, NULL, "<id> [addr]", cmd_id_reset, 2, 1),
	SHELL_CMD_ARG(id-delete, NULL, "<id>", cmd_id_delete, 2, 0),
	SHELL_CMD_ARG(id-show, NULL, HELP_NONE, cmd_id_show, 1, 0),
	SHELL_CMD_ARG(id-select, NULL, "<id>", cmd_id_select, 2, 0),
	SHELL_CMD_ARG(name, NULL, "[name]", cmd_name, 1, 1),
#if defined(CONFIG_BT_OBSERVER)
	SHELL_CMD_ARG(scan, NULL,
		      "<value: on, passive, off> [filter: dups, nodups] [wl]",
		      cmd_scan, 2, 2),
#endif /* CONFIG_BT_OBSERVER */
#if defined(CONFIG_BT_BROADCASTER)
	SHELL_CMD_ARG(advertise, NULL,
		      "<type: off, on, scan, nconn> [mode: discov, non_discov] "
		      "[whitelist: wl, wl-scan, wl-conn]",
		      cmd_advertise, 2, 2),
#if defined(CONFIG_BT_PERIPHERAL)
	SHELL_CMD_ARG(directed-adv, NULL, HELP_ADDR_LE " [mode: low]",
		      cmd_directed_adv, 3, 1),
#endif /* CONFIG_BT_PERIPHERAL */
#endif /* CONFIG_BT_BROADCASTER */
#if defined(CONFIG_BT_CONN)
#if defined(CONFIG_BT_CENTRAL)
	SHELL_CMD_ARG(connect, NULL, HELP_ADDR_LE, cmd_connect_le, 3, 0),
#if !defined(CONFIG_BT_WHITELIST)
	SHELL_CMD_ARG(auto-conn, NULL, HELP_ADDR_LE, cmd_auto_conn, 3, 0),
#endif /* !defined(CONFIG_BT_WHITELIST) */
#endif /* CONFIG_BT_CENTRAL */
	SHELL_CMD_ARG(disconnect, NULL, HELP_NONE, cmd_disconnect, 1, 2),
	SHELL_CMD_ARG(select, NULL, HELP_ADDR_LE, cmd_select, 3, 0),
	SHELL_CMD_ARG(info, NULL, HELP_ADDR_LE, cmd_info, 1, 2),
	SHELL_CMD_ARG(conn-update, NULL, "<min> <max> <latency> <timeout>",
		      cmd_conn_update, 5, 0),
#if defined(CONFIG_BT_CENTRAL)
	SHELL_CMD_ARG(channel-map, NULL, "<channel-map: XXXXXXXXXX> (36-0)",
		      cmd_chan_map, 2, 1),
#endif /* CONFIG_BT_CENTRAL */
	SHELL_CMD_ARG(oob, NULL, NULL, cmd_oob, 1, 0),
	SHELL_CMD_ARG(clear, NULL, "<remote: addr, all>", cmd_clear, 2, 1),
#if defined(CONFIG_BT_SMP) || defined(CONFIG_BT_BREDR)
	SHELL_CMD_ARG(security, NULL, "<security level BR/EDR: 0 - 3, "
				      "LE: 1 - 4> [force-pair]",
		      cmd_security, 2, 1),
	SHELL_CMD_ARG(bondable, NULL, "<bondable: on, off>", cmd_bondable,
		      2, 0),
	SHELL_CMD_ARG(auth, NULL,
		      "<method: all, input, display, yesno, confirm, "
		      "oob, none>",
		      cmd_auth, 2, 0),
	SHELL_CMD_ARG(auth-cancel, NULL, HELP_NONE, cmd_auth_cancel, 1, 0),
	SHELL_CMD_ARG(auth-passkey, NULL, "<passkey>", cmd_auth_passkey, 2, 0),
	SHELL_CMD_ARG(auth-passkey-confirm, NULL, HELP_NONE,
		      cmd_auth_passkey_confirm, 1, 0),
	SHELL_CMD_ARG(auth-pairing-confirm, NULL, HELP_NONE,
		      cmd_auth_pairing_confirm, 1, 0),
	SHELL_CMD_ARG(oob-remote, NULL,
		      HELP_ADDR_LE" <oob rand> <oob confirm>",
		      cmd_oob_remote, 3, 2),
	SHELL_CMD_ARG(oob-clear, NULL, HELP_NONE, cmd_oob_clear, 1, 0),
#if defined(CONFIG_BT_WHITELIST)
	SHELL_CMD_ARG(wl-add, NULL, HELP_ADDR_LE, cmd_wl_add, 3, 0),
	SHELL_CMD_ARG(wl-rem, NULL, HELP_ADDR_LE, cmd_wl_rem, 3, 0),
	SHELL_CMD_ARG(wl-clear, NULL, HELP_NONE, cmd_wl_clear, 2, 0),
	SHELL_CMD_ARG(wl-connect, NULL, "<on, off>", cmd_wl_connect, 2, 0),
#endif /* defined(CONFIG_BT_WHITELIST) */
#if defined(CONFIG_BT_FIXED_PASSKEY)
	SHELL_CMD_ARG(fixed-passkey, NULL, "[passkey]", cmd_fixed_passkey,
		      1, 1),
#endif
#endif /* CONFIG_BT_SMP || CONFIG_BT_BREDR) */
#endif /* CONFIG_BT_CONN */
#if defined(CONFIG_BT_HCI_MESH_EXT)
	SHELL_CMD(mesh_adv, NULL, "<on, off>", cmd_mesh_adv),
#endif /* CONFIG_BT_HCI_MESH_EXT */
#if defined(CONFIG_BT_LL_SW_LEGACY) || defined(CONFIG_BT_LL_SW_SPLIT)
#if defined(CONFIG_BT_CTLR_ADV_EXT)
#if defined(CONFIG_BT_BROADCASTER)
	SHELL_CMD_ARG(advx, NULL, "<on off> [coded] [anon] [txp]", cmd_advx,
		      2, 3),
#endif /* CONFIG_BT_BROADCASTER */
#if defined(CONFIG_BT_OBSERVER)
	SHELL_CMD_ARG(scanx, NULL, "<on passive off> [coded]", cmd_scanx,
		      2, 1),
#endif /* CONFIG_BT_OBSERVER */
#endif /* CONFIG_BT_CTLR_ADV_EXT */
#if defined(CONFIG_BT_LL_SW_LEGACY)
	SHELL_CMD_ARG(ll-addr, NULL, "<random|public>", cmd_ll_addr_get, 2, 0),
#endif
#if defined(CONFIG_BT_CTLR_DTM)
	SHELL_CMD_ARG(test_tx, NULL, "<chan> <len> <type> <phy>", cmd_test_tx,
		      5, 0),
	SHELL_CMD_ARG(test_rx, NULL, "<chan> <phy> <mod_idx>", cmd_test_rx,
		      4, 0),
	SHELL_CMD_ARG(test_end, NULL, HELP_NONE, cmd_test_end, 1, 0),
#endif /* CONFIG_BT_CTLR_ADV_EXT */
#endif /* defined(CONFIG_BT_LL_SW_LEGACY) || defined(CONFIG_BT_LL_SW_SPLIT) */
#if defined(CONFIG_BT_LL_SW_SPLIT)
	SHELL_CMD(ull_reset, NULL, HELP_NONE, cmd_ull_reset),
#endif /* CONFIG_BT_LL_SW_SPLIT */
	SHELL_SUBCMD_SET_END
);

static int cmd_bt(const struct shell *shell, size_t argc, char **argv)
{
	if (argc == 1) {
		shell_help(shell);
		return SHELL_CMD_HELP_PRINTED;
	}

	shell_error(shell, "%s unknown parameter: %s", argv[0], argv[1]);

	return -EINVAL;
}

SHELL_CMD_REGISTER(bt, &bt_cmds, "Bluetooth shell commands", cmd_bt);
