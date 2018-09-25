/** @file
 * @brief Bluetooth shell module
 *
 * Provide some Bluetooth shell commands that can be useful to applications.
 */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/types.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <misc/printk.h>
#include <misc/byteorder.h>
#include <zephyr.h>

#include <settings/settings.h>

#include <bluetooth/hci.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/l2cap.h>
#include <bluetooth/rfcomm.h>
#include <bluetooth/sdp.h>

#include <shell/shell.h>

#include "bt.h"
#include "gatt.h"
#include "ll.h"

#define DEVICE_NAME		CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN		(sizeof(DEVICE_NAME) - 1)
#define CREDITS			10
#define DATA_MTU		(23 * CREDITS)

static u8_t selected_id = BT_ID_DEFAULT;
const struct shell *ctx_shell;

#if defined(CONFIG_BT_CONN)
struct bt_conn *default_conn;

/* Connection context for BR/EDR legacy pairing in sec mode 3 */
static struct bt_conn *pairing_conn;
#endif /* CONFIG_BT_CONN */

#if defined(CONFIG_BT_L2CAP_DYNAMIC_CHANNEL)
NET_BUF_POOL_DEFINE(data_tx_pool, 1, DATA_MTU, BT_BUF_USER_DATA_MIN, NULL);
NET_BUF_POOL_DEFINE(data_rx_pool, 1, DATA_MTU, BT_BUF_USER_DATA_MIN, NULL);
#endif

#define NAME_LEN 30

static bool data_cb(struct bt_data *data, void *user_data)
{
	char *name = user_data;

	switch (data->type) {
	case BT_DATA_NAME_SHORTENED:
	case BT_DATA_NAME_COMPLETE:
		memcpy(name, data->data, min(data->data_len, NAME_LEN - 1));
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
	print(NULL, "[DEVICE]: %s, AD evt type %u, RSSI %i %s\n",
	      le_addr, evtype, rssi, name);
}

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
		bt_addr_le_to_str(bt_conn_get_dst(conn), addr, len);
		break;
	}
}

static void connected(struct bt_conn *conn, u8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	conn_addr_str(conn, addr, sizeof(addr));

	if (err) {
		error(NULL, "Failed to connect to %s (%u)\n", addr,
			     err);
		goto done;
	}

	print(NULL, "Connected: %s\n", addr);

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
	print(NULL, "Disconnected: %s (reason %u)\n", addr, reason);

	if (default_conn == conn) {
		bt_conn_unref(default_conn);
		default_conn = NULL;
	}
}

static bool le_param_req(struct bt_conn *conn, struct bt_le_conn_param *param)
{
	print(NULL, "LE conn  param req: int (0x%04x, 0x%04x) lat %d "
		     "to %d\n", param->interval_min, param->interval_max,
		     param->latency, param->timeout);

	return true;
}

static void le_param_updated(struct bt_conn *conn, u16_t interval,
			     u16_t latency, u16_t timeout)
{
	print(NULL, "LE conn param updated: int 0x%04x lat %d "
		     "to %d\n", interval, latency, timeout);
}

#if defined(CONFIG_BT_SMP)
static void identity_resolved(struct bt_conn *conn, const bt_addr_le_t *rpa,
			      const bt_addr_le_t *identity)
{
	char addr_identity[BT_ADDR_LE_STR_LEN];
	char addr_rpa[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(identity, addr_identity, sizeof(addr_identity));
	bt_addr_le_to_str(rpa, addr_rpa, sizeof(addr_rpa));

	print(NULL, "Identity resolved %s -> %s\n", addr_rpa,
	      addr_identity);
}
#endif

#if defined(CONFIG_BT_SMP) || defined(CONFIG_BT_BREDR)
static void security_changed(struct bt_conn *conn, bt_security_t level)
{
	char addr[BT_ADDR_LE_STR_LEN];

	conn_addr_str(conn, addr, sizeof(addr));
	print(NULL, "Security changed: %s level %u\n", addr, level);
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

static int char2hex(const char *c, u8_t *x)
{
	if (*c >= '0' && *c <= '9') {
		*x = *c - '0';
	} else if (*c >= 'a' && *c <= 'f') {
		*x = *c - 'a' + 10;
	} else if (*c >= 'A' && *c <= 'F') {
		*x = *c - 'A' + 10;
	} else {
		return -EINVAL;
	}

	return 0;
}

static int hexstr2array(const char *str, u8_t *array, u8_t size)
{
	int i, j;
	u8_t tmp;

	if (strlen(str) != ((size * 2) + (size - 1))) {
		return -EINVAL;
	}

	for (i = size - 1, j = 1; *str != '\0'; str++, j++) {
		if (!(j % 3) && (*str != ':')) {
			return -EINVAL;
		} else if (*str == ':') {
			i--;
			continue;
		}

		array[i] = array[i] << 4;

		if (char2hex(str, &tmp) < 0) {
			return -EINVAL;
		}

		array[i] |= tmp;
	}

	return 0;
}

int str2bt_addr(const char *str, bt_addr_t *addr)
{
	return hexstr2array(str, addr->val, 6);
}

static int str2bt_addr_le(const char *str, const char *type, bt_addr_le_t *addr)
{
	int err;

	err = str2bt_addr(str, &addr->a);
	if (err < 0) {
		return err;
	}

	if (!strcmp(type, "public") || !strcmp(type, "(public)")) {
		addr->type = BT_ADDR_LE_PUBLIC;
	} else if (!strcmp(type, "random") || !strcmp(type, "(random)")) {
		addr->type = BT_ADDR_LE_RANDOM;
	} else {
		return -EINVAL;
	}

	return 0;
}

static void bt_ready(int err)
{
	if (err) {
		error(NULL, "Bluetooth init failed (err %d)\n", err);
		return;
	}

	print(NULL, "Bluetooth initialized\n");

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		settings_load();
	}

#if defined(CONFIG_BT_CONN)
	default_conn = NULL;

	bt_conn_cb_register(&conn_callbacks);
#endif /* CONFIG_BT_CONN */
}

static void cmd_init(const struct shell *shell, size_t argc, char *argv[])
{
	int err;

	err = bt_enable(bt_ready);
	if (err) {
		error(shell, "Bluetooth init failed (err %d)\n", err);
	}

	ctx_shell = shell;
}

#if defined(CONFIG_BT_HCI) || defined(CONFIG_BT_L2CAP_DYNAMIC_CHANNEL)
static void hexdump(const struct shell *shell, const u8_t *data, size_t len)
{
	int n = 0;

	while (len--) {
		if (n % 16 == 0) {
			print(shell, "%08X ", n);
		}

		print(shell, "%02X ", *data++);

		n++;
		if (n % 8 == 0) {
			if (n % 16 == 0) {
				print(shell, "\n");
			} else {
				print(shell, " ");
			}
		}
	}

	if (n % 16) {
		print(shell, "\n");
	}
}
#endif /* CONFIG_BT_HCI || CONFIG_BT_L2CAP_DYNAMIC_CHANNEL */

#if defined(CONFIG_BT_HCI)
static void cmd_hci_cmd(const struct shell *shell, size_t argc, char *argv[])
{
	u8_t ogf;
	u16_t ocf;
	struct net_buf *buf = NULL, *rsp;
	int err;

	if (!shell_cmd_precheck(shell, (argc == 3), NULL, 0)) {
		return;
	}

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
		error(shell, "HCI command failed (err %d)\n", err);
	} else {
		hexdump(shell, rsp->data, rsp->len);
		net_buf_unref(rsp);
	}
}
#endif /* CONFIG_BT_HCI */

static void cmd_name(const struct shell *shell, size_t argc, char *argv[])
{
	int err;

	if (argc < 2) {
		print(shell, "Bluetooth Local Name: %s\n", bt_get_name());
	}

	err = bt_set_name(argv[1]);
	if (err) {
		error(shell, "Unable to set name %s (err %d)", argv[1], err);
	}
}

static void cmd_id_create(const struct shell *shell, size_t argc, char *argv[])
{
	bt_addr_le_t addr;
	int err;

	if (argc > 1) {
		err = str2bt_addr_le(argv[1], "random", &addr);
		if (err) {
			error(shell, "Invalid address\n");
		}
	} else {
		bt_addr_le_copy(&addr, BT_ADDR_LE_ANY);
	}

	err = bt_id_create(&addr, NULL);
	if (err < 0) {
		error(shell, "Creating new ID failed (err %d)\n", err);
	}

	print(shell, "New identity (%d) created: %s\n", err,
	      bt_addr_le_str(&addr));
}

static void cmd_id_reset(const struct shell *shell, size_t argc, char *argv[])
{
	bt_addr_le_t addr;
	u8_t id;
	int err;

	if (argc < 2) {
		error(shell, "Identity identifier not specified\n");
		return;
	}

	id = strtol(argv[1], NULL, 10);

	if (argc > 2) {
		err = str2bt_addr_le(argv[2], "random", &addr);
		if (err) {
			print(shell, "Invalid address\n");
			return;
		}
	} else {
		bt_addr_le_copy(&addr, BT_ADDR_LE_ANY);
	}

	err = bt_id_reset(id, &addr, NULL);
	if (err < 0) {
		print(shell, "Resetting ID %u failed (err %d)\n", id, err);
		return;
	}

	print(shell, "Identity %u reset: %s\n", id, bt_addr_le_str(&addr));
}

static void cmd_id_delete(const struct shell *shell, size_t argc, char *argv[])
{
	u8_t id;
	int err;

	if (argc < 2) {
		error(shell, "Identity identifier not specified\n");
		return;
	}

	id = strtol(argv[1], NULL, 10);

	err = bt_id_delete(id);
	if (err < 0) {
		error(shell, "Deleting ID %u failed (err %d)\n", id, err);
		return;
	}

	print(shell, "Identity %u deleted\n", id);
}

static void cmd_id_show(const struct shell *shell, size_t argc, char *argv[])
{
	bt_addr_le_t addrs[CONFIG_BT_ID_MAX];
	size_t i, count = CONFIG_BT_ID_MAX;

	bt_id_get(addrs, &count);

	for (i = 0; i < count; i++) {
		print(shell, "%s%zu: %s\n", i == selected_id ? "*" : " ", i,
		      bt_addr_le_str(&addrs[i]));
	}
}

static void cmd_id_select(const struct shell *shell, size_t argc, char *argv[])
{
	bt_addr_le_t addrs[CONFIG_BT_ID_MAX];
	size_t count = CONFIG_BT_ID_MAX;
	u8_t id;

	if (argc < 2) {
		shell_help_print(shell, NULL, 0);
		return;
	}

	id = strtol(argv[1], NULL, 10);

	bt_id_get(addrs, &count);
	if (count <= id) {
		error(shell, "Invalid identity\n");
		return;
	}

	print(shell, "Selected identity: %s\n",
		     bt_addr_le_str(&addrs[id]));
	selected_id = id;
}

static void cmd_active_scan_on(const struct shell *shell, int dups)
{
	int err;
	struct bt_le_scan_param param = {
			.type       = BT_HCI_LE_SCAN_ACTIVE,
			.filter_dup = BT_HCI_LE_SCAN_FILTER_DUP_ENABLE,
			.interval   = BT_GAP_SCAN_FAST_INTERVAL,
			.window     = BT_GAP_SCAN_FAST_WINDOW };

	if (dups >= 0) {
		param.filter_dup = dups;
	}

	err = bt_le_scan_start(&param, device_found);
	if (err) {
		error(shell, "Bluetooth set active scan failed "
		      "(err %d)\n", err);
		return;
	} else {
		print(shell, "Bluetooth active scan enabled\n");
	}
}

static void cmd_passive_scan_on(const struct shell *shell, int dups)
{
	struct bt_le_scan_param param = {
			.type       = BT_HCI_LE_SCAN_PASSIVE,
			.filter_dup = BT_HCI_LE_SCAN_FILTER_DUP_DISABLE,
			.interval   = 0x10,
			.window     = 0x10 };
	int err;

	if (dups >= 0) {
		param.filter_dup = dups;
	}

	err = bt_le_scan_start(&param, device_found);
	if (err) {
		error(shell, "Bluetooth set passive scan failed "
			    "(err %d)\n", err);
		return;
	} else {
		print(shell, "Bluetooth passive scan enabled\n");
	}
}

static void cmd_scan_off(const struct shell *shell)
{
	int err;

	err = bt_le_scan_stop();
	if (err) {
		error(shell, "Stopping scanning failed (err %d)\n", err);
	} else {
		print(shell, "Scan successfully stopped\n");
	}
}

static void cmd_scan(const struct shell *shell, size_t argc, char *argv[])
{
	const char *action;
	int dups = -1;

	if (!shell_cmd_precheck(shell, (argc >= 2), NULL, 0)) {
		return;
	}

	/* Parse duplicate filtering data */
	if (argc >= 3) {
		const char *dup_filter = argv[2];

		if (!strcmp(dup_filter, "dups")) {
			dups = BT_HCI_LE_SCAN_FILTER_DUP_DISABLE;
		} else if (!strcmp(dup_filter, "nodups")) {
			dups = BT_HCI_LE_SCAN_FILTER_DUP_ENABLE;
		} else {
			shell_help_print(shell, NULL, 0);
			return;
		}
	}

	action = argv[1];
	if (!strcmp(action, "on")) {
		cmd_active_scan_on(shell, dups);
	} else if (!strcmp(action, "off")) {
		cmd_scan_off(shell);
	} else if (!strcmp(action, "passive")) {
		cmd_passive_scan_on(shell, dups);
	} else {
		shell_help_print(shell, NULL, 0);
		return;
	}
}

static const struct bt_data ad_discov[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
};

static void cmd_advertise(const struct shell *shell, size_t argc, char *argv[])
{
	struct bt_le_adv_param param;
	const struct bt_data *ad, *scan_rsp;
	size_t ad_len, scan_rsp_len;
	int err;

	if (!shell_cmd_precheck(shell, (argc >= 2), NULL, 0)) {
		return;
	}

	if (!strcmp(argv[1], "off")) {
		if (bt_le_adv_stop() < 0) {
			error(shell, "Failed to stop advertising\n");
		} else {
			print(shell, "Advertising stopped\n");
		}

		return;
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
		param.options = 0;
		scan_rsp = NULL;
		scan_rsp_len = 0;
	} else {
		goto fail;
	}

	/* Parse advertisement data */
	if (argc >= 3) {
		const char *mode = argv[2];

		if (!strcmp(mode, "discov")) {
			ad = ad_discov;
			ad_len = ARRAY_SIZE(ad_discov);
		} else if (!strcmp(mode, "non_discov")) {
			ad = NULL;
			ad_len = 0;
		} else {
			goto fail;
		}
	} else {
		ad = ad_discov;
		ad_len = ARRAY_SIZE(ad_discov);
	}

	err = bt_le_adv_start(&param, ad, ad_len, scan_rsp, scan_rsp_len);
	if (err < 0) {
		error(shell, "Failed to start advertising (err %d)\n",
			    err);
	} else {
		print(shell, "Advertising started\n");
	}

	return;

fail:
	shell_help_print(shell, NULL, 0);
}

#if defined(CONFIG_BT_CONN)
static void cmd_connect_le(const struct shell *shell, size_t argc, char *argv[])
{
	int err;
	bt_addr_le_t addr;
	struct bt_conn *conn;

	if (argc < 3) {
		shell_help_print(shell, NULL, 0);
		return;
	}

	err = str2bt_addr_le(argv[1], argv[2], &addr);
	if (err) {
		error(shell, "Invalid peer address (err %d)\n", err);
		return;
	}

	conn = bt_conn_create_le(&addr, BT_LE_CONN_PARAM_DEFAULT);

	if (!conn) {
		error(shell, "Connection failed\n");
	} else {

		print(shell, "Connection pending\n");

		/* unref connection obj in advance as app user */
		bt_conn_unref(conn);
	}
}

static void cmd_disconnect(const struct shell *shell, size_t argc, char *argv[])
{
	struct bt_conn *conn;
	int err;

	if (default_conn && argc < 3) {
		conn = bt_conn_ref(default_conn);
	} else {
		bt_addr_le_t addr;

		if (argc < 3) {
			shell_help_print(shell, NULL, 0);
			return;
		}

		err = str2bt_addr_le(argv[1], argv[2], &addr);
		if (err) {
			error(shell, "Invalid peer address (err %d)\n",
				    err);
			return;
		}

		conn = bt_conn_lookup_addr_le(selected_id, &addr);
	}

	if (!conn) {
		error(shell, "Not connected\n");
		return;
	}

	err = bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	if (err) {
		error(shell, "Disconnection failed (err %d)\n", err);
	}

	bt_conn_unref(conn);
}

static void cmd_auto_conn(const struct shell *shell, size_t argc, char *argv[])
{
	bt_addr_le_t addr;
	int err;

	if (argc < 3) {
		shell_help_print(shell, NULL, 0);
		return;
	}

	err = str2bt_addr_le(argv[1], argv[2], &addr);
	if (err) {
		error(shell, "Invalid peer address (err %d)\n", err);
		return;
	}

	if (argc < 4) {
		bt_le_set_auto_conn(&addr, BT_LE_CONN_PARAM_DEFAULT);
	} else if (!strcmp(argv[3], "on")) {
		bt_le_set_auto_conn(&addr, BT_LE_CONN_PARAM_DEFAULT);
	} else if (!strcmp(argv[3], "off")) {
		bt_le_set_auto_conn(&addr, NULL);
	} else {
		shell_help_print(shell, NULL, 0);
	}
}

static void cmd_directed_adv(const struct shell *shell,
			     size_t argc, char *argv[])
{
	int err;
	bt_addr_le_t addr;
	struct bt_conn *conn;
	struct bt_le_adv_param *param = BT_LE_ADV_CONN_DIR;

	if (!shell_cmd_precheck(shell, (argc >= 2), NULL, 0)) {
		return;
	}

	err = str2bt_addr_le(argv[1], argv[2], &addr);
	if (err) {
		error(shell, "Invalid peer address (err %d)\n", err);
		return;
	}

	if (argc > 3) {
		if (!strcmp(argv[3], "low")) {
			param = BT_LE_ADV_CONN_DIR_LOW_DUTY;
		} else {
			shell_help_print(shell, NULL, 0);
			return;
		}
	}

	conn = bt_conn_create_slave_le(&addr, param);
	if (!conn) {
		error(shell, "Failed to start directed advertising\n");
	} else {
		print(shell, "Started directed advertising\n");

		/* unref connection obj in advance as app user */
		bt_conn_unref(conn);
	}
}

static void cmd_select(const struct shell *shell, size_t argc, char *argv[])
{
	struct bt_conn *conn;
	bt_addr_le_t addr;
	int err;

	if (!shell_cmd_precheck(shell, argc == 3, NULL, 0)) {
		return;
	}

	err = str2bt_addr_le(argv[1], argv[2], &addr);
	if (err) {
		error(shell, "Invalid peer address (err %d)\n", err);
		return;
	}

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &addr);
	if (!conn) {
		error(shell, "No matching connection found\n");
		return;
	}

	if (default_conn) {
		bt_conn_unref(default_conn);
	}

	default_conn = conn;
}

static void cmd_conn_update(const struct shell *shell,
			    size_t argc, char *argv[])
{
	struct bt_le_conn_param param;
	int err;

	if (!shell_cmd_precheck(shell, argc == 5, NULL, 0)) {
		return;
	}

	param.interval_min = strtoul(argv[1], NULL, 16);
	param.interval_max = strtoul(argv[2], NULL, 16);
	param.latency = strtoul(argv[3], NULL, 16);
	param.timeout = strtoul(argv[4], NULL, 16);

	err = bt_conn_le_param_update(default_conn, &param);
	if (err) {
		error(shell, "conn update failed (err %d).\n", err);
	} else {
		print(shell, "conn update initiated.\n");
	}
}

static void cmd_oob(const struct shell *shell, size_t argc, char *argv[])
{
	char addr[BT_ADDR_LE_STR_LEN];
	struct bt_le_oob oob;
	int err;

	err = bt_le_oob_get_local(selected_id, &oob);
	if (err) {
		error(shell, "OOB data failed\n");
		return;
	}

	bt_addr_le_to_str(&oob.addr, addr, sizeof(addr));

	print(shell, "OOB data:\n");
	print(shell, "  addr %s\n", addr);
}

static void cmd_clear(const struct shell *shell, size_t argc, char *argv[])
{
	bt_addr_le_t addr;
	int err;

	if (argc < 2) {
		error(shell, "Specify remote address or \"all\"\n");
		return;
	}

	if (strcmp(argv[1], "all") == 0) {
		err = bt_unpair(selected_id, NULL);
		if (err) {
			error(shell, "Failed to clear pairings (err %d)\n",
			      err);
		} else {
			print(shell, "Pairings successfully cleared\n");
		}

		return;
	}

	if (argc < 3) {
#if defined(CONFIG_BT_BREDR)
		addr.type = BT_ADDR_LE_PUBLIC;
		err = str2bt_addr(argv[1], &addr.a);
#else
		print(shell, "Both address and address type needed\n");
		return;
#endif
	} else {
		err = str2bt_addr_le(argv[1], argv[2], &addr);
	}

	if (err) {
		print(shell, "Invalid address\n");
		return;
	}

	err = bt_unpair(selected_id, &addr);
	if (err) {
		error(shell, "Failed to clear pairing (err %d)\n", err);
	} else {
		print(shell, "Pairing successfully cleared\n");
	}
}

static void cmd_chan_map(const struct shell *shell, size_t argc, char *argv[])
{
	u8_t chan_map[5];
	int err;

	if (!shell_cmd_precheck(shell, argc == 2, NULL, 0)) {
		return;
	}

	err = hexstr2array(argv[1], chan_map, 5);
	if (err) {
		error(shell, "Invalid channel map\n");
		return;
	}

	err = bt_le_set_chan_map(chan_map);
	if (err) {
		error(shell, "Failed to set channel map (err %d)\n", err);
	} else {
		print(shell, "Channel map set\n");
	}
}
#endif /* CONFIG_BT_CONN */

#if defined(CONFIG_BT_SMP) || defined(CONFIG_BT_BREDR)
static void cmd_security(const struct shell *shell, size_t argc, char *argv[])
{
	int err, sec;

	if (!default_conn) {
		error(shell, "Not connected\n");
		return;
	}

	if (!shell_cmd_precheck(shell, argc == 2, NULL, 0)) {
		return;
	}

	sec = *argv[1] - '0';

	err = bt_conn_security(default_conn, sec);
	if (err) {
		error(shell, "Setting security failed (err %d)\n", err);
	}
}

static void cmd_bondable(const struct shell *shell, size_t argc, char *argv[])
{
	const char *bondable;

	if (!shell_cmd_precheck(shell, argc == 2, NULL, 0)) {
		return;
	}

	bondable = argv[1];
	if (!strcmp(bondable, "on")) {
		bt_set_bondable(true);
	} else if (!strcmp(bondable, "off")) {
		bt_set_bondable(false);
	} else {
		shell_help_print(shell, NULL, 0);
	}
}

static void auth_passkey_display(struct bt_conn *conn, unsigned int passkey)
{
	char addr[BT_ADDR_LE_STR_LEN];
	char passkey_str[7];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	print(NULL, passkey_str, 7, "%06u", passkey);

	print(NULL, "Passkey for %s: %s\n", addr, passkey_str);
}

static void auth_passkey_confirm(struct bt_conn *conn, unsigned int passkey)
{
	char addr[BT_ADDR_LE_STR_LEN];
	char passkey_str[7];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	print(NULL, passkey_str, 7, "%06u", passkey);

	print(NULL, "Confirm passkey for %s: %s\n", addr, passkey_str);
}

static void auth_passkey_entry(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	print(NULL, "Enter passkey for %s\n", addr);
}

static void auth_cancel(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	conn_addr_str(conn, addr, sizeof(addr));

	print(NULL, "Pairing cancelled: %s\n", addr);

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

	print(NULL, "Confirm pairing for %s\n", addr);
}

static void auth_pairing_complete(struct bt_conn *conn, bool bonded)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	print(NULL, "%s with %s\n", bonded ? "Bonded" : "Paired",  addr);
}

static void auth_pairing_failed(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	print(NULL, "Pairing failed with %s\n", addr);
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
		print(NULL, "Enter 16 digits wide PIN code for %s\n",
		      addr);
	} else {
		print(NULL, "Enter PIN code for %s\n", addr);
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
	.cancel = auth_cancel,
	.pairing_confirm = auth_pairing_confirm,
	.pairing_failed = auth_pairing_failed,
	.pairing_complete = auth_pairing_complete,
};

static struct bt_conn_auth_cb auth_cb_confirm = {
#if defined(CONFIG_BT_BREDR)
	.pincode_entry = auth_pincode_entry,
#endif
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
	.cancel = auth_cancel,
	.pairing_confirm = auth_pairing_confirm,
	.pairing_failed = auth_pairing_failed,
	.pairing_complete = auth_pairing_complete,
};

static void cmd_auth(const struct shell *shell, size_t argc, char *argv[])
{
	if (!shell_cmd_precheck(shell, argc == 2, NULL, 0)) {
		return;
	}

	if (!strcmp(argv[1], "all")) {
		bt_conn_auth_cb_register(&auth_cb_all);
	} else if (!strcmp(argv[1], "input")) {
		bt_conn_auth_cb_register(&auth_cb_input);
	} else if (!strcmp(argv[1], "display")) {
		bt_conn_auth_cb_register(&auth_cb_display);
	} else if (!strcmp(argv[1], "yesno")) {
		bt_conn_auth_cb_register(&auth_cb_display_yes_no);
	} else if (!strcmp(argv[1], "confirm")) {
		bt_conn_auth_cb_register(&auth_cb_confirm);
	} else if (!strcmp(argv[1], "none")) {
		bt_conn_auth_cb_register(NULL);
	} else {
		shell_help_print(shell, NULL, 0);
	}
}

static void cmd_auth_cancel(const struct shell *shell,
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
		print(shell, "Not connected\n");
		return;
	}

	bt_conn_auth_cancel(conn);
}

static void cmd_auth_passkey_confirm(const struct shell *shell,
				    size_t argc, char *argv[])
{
	if (!default_conn) {
		print(shell, "Not connected\n");
		return;
	}

	bt_conn_auth_passkey_confirm(default_conn);
}

static void cmd_auth_pairing_confirm(const struct shell *shell,
				    size_t argc, char *argv[])
{
	if (!default_conn) {
		print(shell, "Not connected\n");
		return;
	}

	bt_conn_auth_pairing_confirm(default_conn);
}

#if defined(CONFIG_BT_FIXED_PASSKEY)
static void cmd_fixed_passkey(const struct shell *shell,
			     size_t argc, char *argv[])
{
	unsigned int passkey;
	int err;

	if (argc < 2) {
		bt_passkey_set(BT_PASSKEY_INVALID);
		print(shell, "Fixed passkey cleared\n");
		return;
	}

	passkey = atoi(argv[1]);
	if (passkey > 999999) {
		print(shell, "Passkey should be between 0-999999\n");
		return;
	}

	err = bt_passkey_set(passkey);
	if (err) {
		print(shell, "Setting fixed passkey failed (err %d)\n", err);
	}
}
#endif

static void cmd_auth_passkey(const struct shell *shell,
			    size_t argc, char *argv[])
{
	unsigned int passkey;

	if (!default_conn) {
		print(shell, "Not connected\n");
		return;
	}

	if (!shell_cmd_precheck(shell, argc == 2, NULL, 0)) {
		return;
	}

	passkey = atoi(argv[1]);
	if (passkey > 999999) {
		print(shell, "Passkey should be between 0-999999\n");
		return;
	}

	bt_conn_auth_passkey_entry(default_conn, passkey);
}
#endif /* CONFIG_BT_SMP) || CONFIG_BT_BREDR */

#if defined(CONFIG_BT_L2CAP_DYNAMIC_CHANNEL)
static u32_t l2cap_rate;
static u32_t l2cap_recv_delay;
static K_FIFO_DEFINE(l2cap_recv_fifo);
struct l2ch {
	struct k_delayed_work recv_work;
	struct bt_l2cap_le_chan ch;
};
#define L2CH_CHAN(_chan) CONTAINER_OF(_chan, struct l2ch, ch.chan)
#define L2CH_WORK(_work) CONTAINER_OF(_work, struct l2ch, recv_work)
#define L2CAP_CHAN(_chan) _chan->ch.chan

static int l2cap_recv_metrics(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	static u32_t len;
	static u32_t cycle_stamp;
	u32_t delta;

	delta = k_cycle_get_32() - cycle_stamp;
	delta = SYS_CLOCK_HW_CYCLES_TO_NS(delta);

	/* if last data rx-ed was greater than 1 second in the past,
	 * reset the metrics.
	 */
	if (delta > 1000000000) {
		len = 0;
		l2cap_rate = 0;
		cycle_stamp = k_cycle_get_32();
	} else {
		len += buf->len;
		l2cap_rate = ((u64_t)len << 3) * 1000000000 / delta;
	}

	return 0;
}

static void l2cap_recv_cb(struct k_work *work)
{
	struct l2ch *c = L2CH_WORK(work);
	struct net_buf *buf;

	while ((buf = net_buf_get(&l2cap_recv_fifo, K_NO_WAIT))) {
		print(NULL, "Confirming reception\n");
		bt_l2cap_chan_recv_complete(&c->ch.chan, buf);
	}
}

static int l2cap_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	struct l2ch *l2ch = L2CH_CHAN(chan);

	print(NULL, "Incoming data channel %p len %u\n", chan, buf->len);

	if (buf->len) {
		hexdump(ctx_shell, buf->data, buf->len);
	}

	if (l2cap_recv_delay) {
		/* Submit work only if queue is empty */
		if (k_fifo_is_empty(&l2cap_recv_fifo)) {
			print(NULL, "Delaying response in %u ms...\n",
			      l2cap_recv_delay);
			k_delayed_work_submit(&l2ch->recv_work,
					      l2cap_recv_delay);
		}
		net_buf_put(&l2cap_recv_fifo, buf);
		return -EINPROGRESS;
	}

	return 0;
}

static void l2cap_connected(struct bt_l2cap_chan *chan)
{
	struct l2ch *c = L2CH_CHAN(chan);

	k_delayed_work_init(&c->recv_work, l2cap_recv_cb);

	print(NULL, "Channel %p connected\n", chan);
}

static void l2cap_disconnected(struct bt_l2cap_chan *chan)
{
	print(NULL, "Channel %p disconnected\n", chan);
}

static struct net_buf *l2cap_alloc_buf(struct bt_l2cap_chan *chan)
{
	/* print if metrics is disabled */
	if (chan->ops->recv != l2cap_recv_metrics) {
		print(NULL, "Channel %p requires buffer\n", chan);
	}

	return net_buf_alloc(&data_rx_pool, K_FOREVER);
}

static struct bt_l2cap_chan_ops l2cap_ops = {
	.alloc_buf	= l2cap_alloc_buf,
	.recv		= l2cap_recv,
	.connected	= l2cap_connected,
	.disconnected	= l2cap_disconnected,
};


static struct l2ch l2ch_chan = {
	.ch.chan.ops	= &l2cap_ops,
	.ch.rx.mtu	= DATA_MTU,
};

static int l2cap_accept(struct bt_conn *conn, struct bt_l2cap_chan **chan)
{
	print(NULL, "Incoming conn %p\n", conn);

	if (l2ch_chan.ch.chan.conn) {
		print(NULL, "No channels available\n");
		return -ENOMEM;
	}

	*chan = &l2ch_chan.ch.chan;

	return 0;
}

static struct bt_l2cap_server server = {
	.accept		= l2cap_accept,
};

static void cmd_l2cap_register(const struct shell *shell,
			      size_t argc, char *argv[])
{

	if (!shell_cmd_precheck(shell, argc >= 2, NULL, 0)) {
		return;
	}

	if (server.psm) {
		error(shell, "Already registered\n");
		return;
	}

	server.psm = strtoul(argv[1], NULL, 16);

	if (argc > 2) {
		server.sec_level = strtoul(argv[2], NULL, 10);
	}

	if (bt_l2cap_server_register(&server) < 0) {
		error(shell, "Unable to register psm\n");
		server.psm = 0;
	} else {
		print(shell, "L2CAP psm %u sec_level %u registered\n",
		      server.psm, server.sec_level);
	}
}

static void cmd_l2cap_connect(const struct shell *shell,
			     size_t argc, char *argv[])
{
	u16_t psm;
	int err;

	if (!default_conn) {
		error(shell, "Not connected\n");
		return;
	}

	if (!shell_cmd_precheck(shell, argc == 2, NULL, 0)) {
		return;
	}

	if (l2ch_chan.ch.chan.conn) {
		error(shell, "Channel already in use\n");
		return;
	}

	psm = strtoul(argv[1], NULL, 16);

	err = bt_l2cap_chan_connect(default_conn, &l2ch_chan.ch.chan, psm);
	if (err < 0) {
		error(shell, "Unable to connect to psm %u (err %u)\n", psm,
		      err);
	} else {
		print(shell, "L2CAP connection pending\n");
	}
}

static void cmd_l2cap_disconnect(const struct shell *shell,
				 size_t argc, char *argv[])
{
	int err;

	err = bt_l2cap_chan_disconnect(&l2ch_chan.ch.chan);
	if (err) {
		print(shell, "Unable to disconnect: %u\n", -err);
	}
}

static void cmd_l2cap_send(const struct shell *shell, size_t argc, char *argv[])
{
	static u8_t buf_data[DATA_MTU] = { [0 ... (DATA_MTU - 1)] = 0xff };
	int ret, len, count = 1;
	struct net_buf *buf;

	if (argc > 1) {
		count = strtoul(argv[1], NULL, 10);
	}

	len = min(l2ch_chan.ch.tx.mtu, DATA_MTU - BT_L2CAP_CHAN_SEND_RESERVE);

	while (count--) {
		buf = net_buf_alloc(&data_tx_pool, K_FOREVER);
		net_buf_reserve(buf, BT_L2CAP_CHAN_SEND_RESERVE);

		net_buf_add_mem(buf, buf_data, len);
		ret = bt_l2cap_chan_send(&l2ch_chan.ch.chan, buf);
		if (ret < 0) {
			print(shell, "Unable to send: %d\n", -ret);
			net_buf_unref(buf);
			break;
		}
	}
}

static void cmd_l2cap_recv(const struct shell *shell, size_t argc, char *argv[])
{
	if (argc > 1) {
		l2cap_recv_delay = strtoul(argv[1], NULL, 10);
	} else {
		print(shell, "l2cap receive delay: %u ms\n",
			     l2cap_recv_delay);
	}
}

static void cmd_l2cap_metrics(const struct shell *shell,
			      size_t argc, char *argv[])
{
	const char *action;

	if (argc < 2) {
		print(shell, "l2cap rate: %u bps.\n", l2cap_rate);

		return;
	}

	action = argv[1];

	if (!strcmp(action, "on")) {
		l2cap_ops.recv = l2cap_recv_metrics;
	} else if (!strcmp(action, "off")) {
		l2cap_ops.recv = l2cap_recv;
	} else {
		shell_help_print(shell, NULL, 0);
		return;
	}

	print(shell, "l2cap metrics %s.\n", action);
}

#endif

#define HELP_NONE "[none]"
#define HELP_ADDR_LE "<address: XX:XX:XX:XX:XX:XX> <type: (public|random)>"

SHELL_CREATE_STATIC_SUBCMD_SET(bt_cmds) {
	SHELL_CMD(init, NULL, HELP_ADDR_LE, cmd_init),
#if defined(CONFIG_BT_HCI)
	SHELL_CMD(hci-cmd, NULL, "<ogf> <ocf> [data]", cmd_hci_cmd),
#endif
	SHELL_CMD(id-create, NULL, "[addr]", cmd_id_create),
	SHELL_CMD(id-reset, NULL, "<id> [addr]", cmd_id_reset),
	SHELL_CMD(id-delete, NULL, "<id>", cmd_id_delete),
	SHELL_CMD(id-show, NULL, HELP_NONE, cmd_id_show),
	SHELL_CMD(id-select, NULL, "<id>", cmd_id_select),
	SHELL_CMD(name, NULL, "[name]", cmd_name),
	SHELL_CMD(scan, NULL,
		  "<value: on, passive, off> <dup filter: dups, nodups>",
		  cmd_scan),
	SHELL_CMD(advertise, NULL,
		  "<type: off, on, scan, nconn> <mode: discov, non_discov>",
		  cmd_advertise),
#if defined(CONFIG_BT_CONN)
	SHELL_CMD(connect, NULL, HELP_ADDR_LE, cmd_connect_le),
	SHELL_CMD(disconnect, NULL, HELP_NONE, cmd_disconnect),
	SHELL_CMD(auto-conn, NULL, HELP_ADDR_LE, cmd_auto_conn),
	SHELL_CMD(directed-adv, NULL, HELP_ADDR_LE " [mode: low]",
		  cmd_directed_adv),
	SHELL_CMD(select, NULL, HELP_ADDR_LE, cmd_select),
	SHELL_CMD(conn-update, NULL, "<min> <max> <latency> <timeout>",
		  cmd_conn_update),
	SHELL_CMD(oob, NULL, NULL, cmd_oob),
	SHELL_CMD(clear, NULL, NULL, cmd_clear),
	SHELL_CMD(channel-map, NULL, "<channel-map: XX:XX:XX:XX:XX> (36-0)",
		  cmd_chan_map),
#if defined(CONFIG_BT_SMP) || defined(CONFIG_BT_BREDR)
	SHELL_CMD(security, NULL, "<security level: 0, 1, 2, 3>", cmd_security),
	SHELL_CMD(bondable, NULL, "<bondable: on, off>", cmd_bondable),
	SHELL_CMD(auth, NULL,
		  "<auth method: all, input, display, yesno, confirm, none>",
		  cmd_auth),
	SHELL_CMD(auth-cancel, NULL, HELP_NONE, cmd_auth_cancel),
	SHELL_CMD(auth-passkey, NULL, "<passkey>", cmd_auth_passkey),
	SHELL_CMD(auth-passkey-confirm, NULL, HELP_NONE,
		  cmd_auth_passkey_confirm),
	SHELL_CMD(auth-pairing-confirm, NULL, HELP_NONE,
		  cmd_auth_pairing_confirm),
#if defined(CONFIG_BT_FIXED_PASSKEY)
	SHELL_CMD(fixed-passkey, NULL, "[passkey]", cmd_fixed_passkey),
#endif
#endif /* CONFIG_BT_SMP || CONFIG_BT_BREDR) */
#if defined(CONFIG_BT_GATT_CLIENT)
	SHELL_CMD(gatt-exchange-mtu, NULL, HELP_NONE, cmd_gatt_exchange_mtu),
	SHELL_CMD(gatt-discover-primary, NULL,
		  "[UUID] [start handle] [end handle]", cmd_gatt_discover),
	SHELL_CMD(gatt-discover-secondary, NULL,
		  "[UUID] [start handle] [end handle]", cmd_gatt_discover),
	SHELL_CMD(gatt-discover-include, NULL,
		  "[UUID] [start handle] [end handle]", cmd_gatt_discover),
	SHELL_CMD(gatt-discover-characteristic, NULL,
		  "[UUID] [start handle] [end handle]", cmd_gatt_discover),
	SHELL_CMD(gatt-discover-descriptor, NULL,
		  "[UUID] [start handle] [end handle]", cmd_gatt_discover),
	SHELL_CMD(gatt-read, NULL, "<handle> [offset]", cmd_gatt_read),
	SHELL_CMD(gatt-read-multiple, NULL, "<handle 1> <handle 2> ...",
		  cmd_gatt_mread),
	SHELL_CMD(gatt-write, NULL, "<handle> <offset> <data> [length]",
		  cmd_gatt_write),
	SHELL_CMD(gatt-write-without-response, NULL,
		  "<handle> <data> [length] [repeat]",
		  cmd_gatt_write_without_rsp),
	SHELL_CMD(gatt-write-signed, NULL, "<handle> <data> [length] [repeat]",
		  cmd_gatt_write_without_rsp),
	SHELL_CMD(gatt-subscribe, NULL, "<CCC handle> <value handle> [ind]",
		  cmd_gatt_subscribe),
	SHELL_CMD(gatt-unsubscribe, NULL, HELP_NONE, cmd_gatt_unsubscribe),
#endif /* CONFIG_BT_GATT_CLIENT */
	SHELL_CMD(gatt-show-db, NULL, HELP_NONE, cmd_gatt_show_db),
	SHELL_CMD(gatt-register-service, NULL,
		  "register pre-predefined test service",
		  cmd_gatt_register_test_svc),
	SHELL_CMD(gatt-unregister-service, NULL,
		  "unregister pre-predefined test service",
		  cmd_gatt_unregister_test_svc),
	SHELL_CMD(gatt-metrics, NULL,
		  "register vendr char and measure rx [value on, off]",
		  cmd_gatt_write_cmd_metrics),
#if defined(CONFIG_BT_L2CAP_DYNAMIC_CHANNEL)
	SHELL_CMD(l2cap-register, NULL, "<psm> [sec_level]",
		  cmd_l2cap_register),
	SHELL_CMD(l2cap-connect, NULL, "<psm>", cmd_l2cap_connect),
	SHELL_CMD(l2cap-disconnect, NULL, HELP_NONE, cmd_l2cap_disconnect),
	SHELL_CMD(l2cap-send, NULL, "<number of packets>", cmd_l2cap_send),
	SHELL_CMD(l2cap-recv, NULL, "[delay (in miliseconds)", cmd_l2cap_recv),
	SHELL_CMD(l2cap-metrics, NULL, "<value on, off>", cmd_l2cap_metrics),
#endif
#endif /* CONFIG_BT_CONN */
#if defined(CONFIG_BT_CTLR_ADV_EXT)
	SHELL_CMD(advx, NULL, "<on off> [coded] [anon] [txp]", cmd_advx),
	SHELL_CMD(scanx, NULL, "<on passive off> [coded]", cmd_scanx),
#endif /* CONFIG_BT_CTLR_ADV_EXT */
#if defined(CONFIG_BT_CTLR_DTM)
	SHELL_CMD(test_tx, NULL, "<chan> <len> <type> <phy>", cmd_test_tx),
	SHELL_CMD(test_rx, NULL, "<chan> <phy> <mod_idx>", cmd_test_rx),
	SHELL_CMD(test_end, NULL, HELP_NONE, cmd_test_end),
#endif /* CONFIG_BT_CTLR_ADV_EXT */
	SHELL_SUBCMD_SET_END
};

static void cmd_bt(const struct shell *shell, size_t argc, char **argv)
{
	if (argc == 1) {
		shell_help_print(shell, NULL, 0);
		return;
	}

	if (!shell_cmd_precheck(shell, (argc == 2), NULL, 0)) {
		return;
	}

	error(shell, "%s:%s%s\r\n", argv[0], "unknown parameter: ", argv[1]);
}

SHELL_CMD_REGISTER(bt, &bt_cmds, "Bluetooth shell commands", cmd_bt);
