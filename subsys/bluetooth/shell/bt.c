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

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/l2cap.h>
#include <bluetooth/rfcomm.h>
#include <bluetooth/storage.h>
#include <bluetooth/sdp.h>

#include <shell/shell.h>

#include "bt.h"
#include "gatt.h"
#include "ll.h"

#define DEVICE_NAME		CONFIG_BLUETOOTH_DEVICE_NAME
#define DEVICE_NAME_LEN		(sizeof(DEVICE_NAME) - 1)
#define CREDITS			10
#define DATA_MTU		(23 * CREDITS)
#define DATA_BREDR_MTU		48

#define BT_SHELL_MODULE "bt"
struct bt_conn *default_conn;
static bt_addr_le_t id_addr;

/* Connection context for BR/EDR legacy pairing in sec mode 3 */
static struct bt_conn *pairing_conn;

#if defined(CONFIG_BLUETOOTH_L2CAP_DYNAMIC_CHANNEL)
NET_BUF_POOL_DEFINE(data_tx_pool, 1, DATA_MTU, BT_BUF_USER_DATA_MIN, NULL);
NET_BUF_POOL_DEFINE(data_rx_pool, 1, DATA_MTU, BT_BUF_USER_DATA_MIN, NULL);
#endif

#if defined(CONFIG_BLUETOOTH_BREDR)
NET_BUF_POOL_DEFINE(data_bredr_pool, 1, DATA_BREDR_MTU, BT_BUF_USER_DATA_MIN,
		    NULL);

#define SDP_CLIENT_USER_BUF_LEN		512
NET_BUF_POOL_DEFINE(sdp_client_pool, CONFIG_BLUETOOTH_MAX_CONN,
		    SDP_CLIENT_USER_BUF_LEN, BT_BUF_USER_DATA_MIN, NULL);
#endif /* CONFIG_BLUETOOTH_BREDR */

#if defined(CONFIG_BLUETOOTH_RFCOMM)

static struct bt_sdp_attribute spp_attrs[] = {
	BT_SDP_NEW_SERVICE,
	BT_SDP_LIST(
		BT_SDP_ATTR_SVCLASS_ID_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
			BT_SDP_ARRAY_16(BT_SDP_SERIAL_PORT_SVCLASS)
		},
		)
	),
	BT_SDP_LIST(
		BT_SDP_ATTR_PROTO_DESC_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 12),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				BT_SDP_ARRAY_16(BT_UUID_L2CAP_VAL)
			},
			)
		},
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 5),
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				BT_SDP_ARRAY_16(BT_UUID_RFCOMM_VAL)
			},
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT8),
				BT_SDP_ARRAY_8(BT_RFCOMM_CHAN_SPP)
			},
			)
		},
		)
	),
	BT_SDP_LIST(
		BT_SDP_ATTR_PROFILE_DESC_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 8),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				BT_SDP_ARRAY_16(BT_SDP_SERIAL_PORT_SVCLASS)
			},
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
				BT_SDP_ARRAY_16(0x0102)
			},
			)
		},
		)
	),
	BT_SDP_SERVICE_NAME("Serial Port"),
};

static struct bt_sdp_record spp_rec = BT_SDP_RECORD(spp_attrs);

#endif /* CONFIG_BLUETOOTH_RFCOMM */

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

static void device_found(const bt_addr_le_t *addr, s8_t rssi, u8_t evtype,
			 struct net_buf_simple *buf)
{
	char le_addr[BT_ADDR_LE_STR_LEN];
	char name[30];

	memset(name, 0, sizeof(name));

	while (buf->len > 1) {
		u8_t len, type;

		len = net_buf_simple_pull_u8(buf);
		if (!len) {
			break;
		}

		/* Check if field length is correct */
		if (len > buf->len || buf->len < 1) {
			break;
		}

		type = net_buf_simple_pull_u8(buf);
		switch (type) {
		case BT_DATA_NAME_SHORTENED:
		case BT_DATA_NAME_COMPLETE:
			if (len > sizeof(name) - 1) {
				memcpy(name, buf->data, sizeof(name) - 1);
			} else {
				memcpy(name, buf->data, len - 1);
			}
			break;
		default:
			break;
		}

		net_buf_simple_pull(buf, len - 1);
	}

	bt_addr_le_to_str(addr, le_addr, sizeof(le_addr));
	printk("[DEVICE]: %s, AD evt type %u, RSSI %i %s\n", le_addr, evtype,
	       rssi, name);
}

static void conn_addr_str(struct bt_conn *conn, char *addr, size_t len)
{
	struct bt_conn_info info;

	if (bt_conn_get_info(conn, &info) < 0) {
		addr[0] = '\0';
		return;
	}

	switch (info.type) {
#if defined(CONFIG_BLUETOOTH_BREDR)
	case BT_CONN_TYPE_BR:
		bt_addr_to_str(info.br.dst, addr, len);
		break;
#endif
	case BT_CONN_TYPE_LE:
		bt_addr_le_to_str(info.le.dst, addr, len);
		break;
	}
}

#if defined(CONFIG_BLUETOOTH_BREDR)
static u8_t sdp_hfp_ag_user(struct bt_conn *conn,
			       struct bt_sdp_client_result *result)
{
	char addr[BT_ADDR_STR_LEN];
	u16_t param, version;
	u16_t features;
	int res;

	conn_addr_str(conn, addr, sizeof(addr));

	if (result) {
		printk("SDP HFPAG data@%p (len %u) hint %u from remote %s\n",
			result->resp_buf, result->resp_buf->len,
			result->next_record_hint, addr);

		/*
		 * Focus to get BT_SDP_ATTR_PROTO_DESC_LIST attribute item to
		 * get HFPAG Server Channel Number operating on RFCOMM protocol.
		 */
		res = bt_sdp_get_proto_param(result->resp_buf,
					     BT_SDP_PROTO_RFCOMM, &param);
		if (res < 0) {
			printk("Error getting Server CN, err %d\n", res);
			goto done;
		}
		printk("HFPAG Server CN param 0x%04x\n", param);

		res = bt_sdp_get_profile_version(result->resp_buf,
						 BT_SDP_HANDSFREE_SVCLASS,
						 &version);
		if (res < 0) {
			printk("Error getting profile version, err %d\n", res);
			goto done;
		}
		printk("HFP version param 0x%04x\n", version);

		/*
		 * Focus to get BT_SDP_ATTR_SUPPORTED_FEATURES attribute item to
		 * get profile Supported Features mask.
		 */
		res = bt_sdp_get_features(result->resp_buf, &features);
		if (res < 0) {
			printk("Error getting HFPAG Features, err %d\n", res);
			goto done;
		}
		printk("HFPAG Supported Features param 0x%04x\n", features);
	} else {
		printk("No SDP HFPAG data from remote %s\n", addr);
	}
done:
	return BT_SDP_DISCOVER_UUID_CONTINUE;
}

static u8_t sdp_a2src_user(struct bt_conn *conn,
			      struct bt_sdp_client_result *result)
{
	char addr[BT_ADDR_STR_LEN];
	u16_t param, version;
	u16_t features;
	int res;

	conn_addr_str(conn, addr, sizeof(addr));

	if (result) {
		printk("SDP A2SRC data@%p (len %u) hint %u from remote %s\n",
			result->resp_buf, result->resp_buf->len,
			result->next_record_hint, addr);

		/*
		 * Focus to get BT_SDP_ATTR_PROTO_DESC_LIST attribute item to
		 * get A2SRC Server PSM Number.
		 */
		res = bt_sdp_get_proto_param(result->resp_buf,
					     BT_SDP_PROTO_L2CAP, &param);
		if (res < 0) {
			printk("A2SRC PSM Number not found, err %d\n", res);
			goto done;
		}
		printk("A2SRC Server PSM Number param 0x%04x\n", param);

		/*
		 * Focus to get BT_SDP_ATTR_PROFILE_DESC_LIST attribute item to
		 * get profile version number.
		 */
		res = bt_sdp_get_profile_version(result->resp_buf,
						 BT_SDP_ADVANCED_AUDIO_SVCLASS,
						 &version);
		if (res < 0) {
			printk("A2SRC version not found, err %d\n", res);
			goto done;
		}
		printk("A2SRC version param 0x%04x\n", version);

		/*
		 * Focus to get BT_SDP_ATTR_SUPPORTED_FEATURES attribute item to
		 * get profile supported features mask.
		 */
		res = bt_sdp_get_features(result->resp_buf, &features);
		if (res < 0) {
			printk("A2SRC Features not found, err %d\n", res);
			goto done;
		}
		printk("A2SRC Supported Features param 0x%04x\n", features);
	} else {
		printk("No SDP A2SRC data from remote %s\n", addr);
	}
done:
	return BT_SDP_DISCOVER_UUID_CONTINUE;
}

static struct bt_sdp_discover_params discov_hfpag = {
	.uuid = BT_UUID_DECLARE_16(BT_SDP_HANDSFREE_AGW_SVCLASS),
	.func = sdp_hfp_ag_user,
	.pool = &sdp_client_pool,
};

static struct bt_sdp_discover_params discov_a2src = {
	.uuid = BT_UUID_DECLARE_16(BT_SDP_AUDIO_SOURCE_SVCLASS),
	.func = sdp_a2src_user,
	.pool = &sdp_client_pool,
};

static struct bt_sdp_discover_params discov;
#endif

static void connected(struct bt_conn *conn, u8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	conn_addr_str(conn, addr, sizeof(addr));

	if (err) {
		printk("Failed to connect to %s (%u)\n", addr, err);
		goto done;
	}

	printk("Connected: %s\n", addr);

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
	printk("Disconnected: %s (reason %u)\n", addr, reason);

	if (default_conn == conn) {
		bt_conn_unref(default_conn);
		default_conn = NULL;
	}
}

static bool le_param_req(struct bt_conn *conn, struct bt_le_conn_param *param)
{
	printk("LE conn  param req: int (0x%04x, 0x%04x) lat %d to %d\n",
	       param->interval_min, param->interval_max, param->latency,
	       param->timeout);

	return true;
}

static void le_param_updated(struct bt_conn *conn, u16_t interval,
			     u16_t latency, u16_t timeout)
{
	printk("LE conn param updated: int 0x%04x lat %d to %d\n", interval,
	       latency, timeout);
}

#if defined(CONFIG_BLUETOOTH_SMP)
static void identity_resolved(struct bt_conn *conn, const bt_addr_le_t *rpa,
			      const bt_addr_le_t *identity)
{
	char addr_identity[BT_ADDR_LE_STR_LEN];
	char addr_rpa[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(identity, addr_identity, sizeof(addr_identity));
	bt_addr_le_to_str(rpa, addr_rpa, sizeof(addr_rpa));

	printk("Identity resolved %s -> %s\n", addr_rpa, addr_identity);
}
#endif

#if defined(CONFIG_BLUETOOTH_SMP) || defined(CONFIG_BLUETOOTH_BREDR)
static void security_changed(struct bt_conn *conn, bt_security_t level)
{
	char addr[BT_ADDR_LE_STR_LEN];

	conn_addr_str(conn, addr, sizeof(addr));
	printk("Security changed: %s level %u\n", addr, level);
}
#endif

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
	.le_param_req = le_param_req,
	.le_param_updated = le_param_updated,
#if defined(CONFIG_BLUETOOTH_SMP)
	.identity_resolved = identity_resolved,
#endif
#if defined(CONFIG_BLUETOOTH_SMP) || defined(CONFIG_BLUETOOTH_BREDR)
	.security_changed = security_changed,
#endif
};

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

static int str2bt_addr(const char *str, bt_addr_t *addr)
{
	int i, j;
	u8_t tmp;

	if (strlen(str) != 17) {
		return -EINVAL;
	}

	for (i = 5, j = 1; *str != '\0'; str++, j++) {
		if (!(j % 3) && (*str != ':')) {
			return -EINVAL;
		} else if (*str == ':') {
			i--;
			continue;
		}

		addr->val[i] = addr->val[i] << 4;

		if (char2hex(str, &tmp) < 0) {
			return -EINVAL;
		}

		addr->val[i] |= tmp;
	}

	return 0;
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

static ssize_t storage_read(const bt_addr_le_t *addr, u16_t key, void *data,
			    size_t length)
{
	if (addr) {
		return -ENOENT;
	}

	if (key == BT_STORAGE_ID_ADDR && length == sizeof(id_addr) &&
	    bt_addr_le_cmp(&id_addr, BT_ADDR_LE_ANY)) {
		bt_addr_le_copy(data, &id_addr);
		return sizeof(id_addr);
	}

	return -EIO;
}

static ssize_t storage_write(const bt_addr_le_t *addr, u16_t key,
			     const void *data, size_t length)
{
	if (addr) {
		return -ENOENT;
	}

	if (key == BT_STORAGE_ID_ADDR && length == sizeof(id_addr)) {
		bt_addr_le_copy(&id_addr, data);
		return sizeof(id_addr);
	}

	return -EIO;
}

static int storage_clear(const bt_addr_le_t *addr)
{
	return -ENOSYS;
}

static void bt_ready(int err)
{
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	default_conn = NULL;

	bt_conn_cb_register(&conn_callbacks);
}

static int cmd_init(int argc, char *argv[])
{
	static const struct bt_storage storage = {
		.read = storage_read,
		.write = storage_write,
		.clear = storage_clear,
	};
	int err;

	if (argc > 1) {
		if (argc < 3) {
			printk("Invalid address\n");
			return -EINVAL;
		}

		err = str2bt_addr_le(argv[1], argv[2], &id_addr);
		if (err) {
			printk("Invalid address (err %d)\n", err);
			bt_addr_le_cmp(&id_addr, BT_ADDR_LE_ANY);
			return -EINVAL;
		}

		bt_storage_register(&storage);
	}

	err = bt_enable(bt_ready);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
	}

	return 0;
}

static void cmd_active_scan_on(int dups)
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
		printk("Bluetooth set active scan failed (err %d)\n", err);
	} else {
		printk("Bluetooth active scan enabled\n");
	}
}

static void cmd_passive_scan_on(int dups)
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
		printk("Bluetooth set passive scan failed (err %d)\n", err);
	} else {
		printk("Bluetooth passive scan enabled\n");
	}
}

static void cmd_scan_off(void)
{
	int err;

	err = bt_le_scan_stop();
	if (err) {
		printk("Stopping scanning failed (err %d)\n", err);
	} else {
		printk("Scan successfully stopped\n");
	}
}

static int cmd_scan(int argc, char *argv[])
{
	const char *action;
	int dups = -1;

	if (argc < 2) {
		return -EINVAL;
	}

	/* Parse duplicate filtering data */
	if (argc >= 3) {
		const char *dup_filter = argv[2];

		if (!strcmp(dup_filter, "dups")) {
			dups = BT_HCI_LE_SCAN_FILTER_DUP_DISABLE;
		} else if (!strcmp(dup_filter, "nodups")) {
			dups = BT_HCI_LE_SCAN_FILTER_DUP_ENABLE;
		} else {
			return -EINVAL;
		}
	}

	action = argv[1];
	if (!strcmp(action, "on")) {
		cmd_active_scan_on(dups);
	} else if (!strcmp(action, "off")) {
		cmd_scan_off();
	} else if (!strcmp(action, "passive")) {
		cmd_passive_scan_on(dups);
	} else {
		return -EINVAL;
	}

	return 0;
}

static int cmd_connect_le(int argc, char *argv[])
{
	int err;
	bt_addr_le_t addr;
	struct bt_conn *conn;

	if (argc < 3) {
		return -EINVAL;
	}

	err = str2bt_addr_le(argv[1], argv[2], &addr);
	if (err) {
		printk("Invalid peer address (err %d)\n", err);
		return 0;
	}

	conn = bt_conn_create_le(&addr, BT_LE_CONN_PARAM_DEFAULT);

	if (!conn) {
		printk("Connection failed\n");
	} else {

		printk("Connection pending\n");

		/* unref connection obj in advance as app user */
		bt_conn_unref(conn);
	}

	return 0;
}

static int cmd_disconnect(int argc, char *argv[])
{
	struct bt_conn *conn;
	int err;

	if (default_conn && argc < 3) {
		conn = bt_conn_ref(default_conn);
	} else {
		bt_addr_le_t addr;

		if (argc < 3) {
			return -EINVAL;
		}

		err = str2bt_addr_le(argv[1], argv[2], &addr);
		if (err) {
			printk("Invalid peer address (err %d)\n", err);
			return 0;
		}

		conn = bt_conn_lookup_addr_le(&addr);
	}

	if (!conn) {
		printk("Not connected\n");
		return 0;
	}

	err = bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	if (err) {
		printk("Disconnection failed (err %d)\n", err);
	}

	bt_conn_unref(conn);

	return 0;
}

static int cmd_auto_conn(int argc, char *argv[])
{
	bt_addr_le_t addr;
	int err;

	if (argc < 3) {
		return -EINVAL;
	}

	err = str2bt_addr_le(argv[1], argv[2], &addr);
	if (err) {
		printk("Invalid peer address (err %d)\n", err);
		return 0;
	}

	if (argc < 4) {
		bt_le_set_auto_conn(&addr, BT_LE_CONN_PARAM_DEFAULT);
	} else if (!strcmp(argv[3], "on")) {
		bt_le_set_auto_conn(&addr, BT_LE_CONN_PARAM_DEFAULT);
	} else if (!strcmp(argv[3], "off")) {
		bt_le_set_auto_conn(&addr, NULL);
	} else {
		return -EINVAL;
	}

	return 0;
}

static int cmd_select(int argc, char *argv[])
{
	struct bt_conn *conn;
	bt_addr_le_t addr;
	int err;

	if (argc < 3) {
		return -EINVAL;
	}

	err = str2bt_addr_le(argv[1], argv[2], &addr);
	if (err) {
		printk("Invalid peer address (err %d)\n", err);
		return 0;
	}

	conn = bt_conn_lookup_addr_le(&addr);
	if (!conn) {
		printk("No matching connection found\n");
		return 0;
	}

	if (default_conn) {
		bt_conn_unref(default_conn);
	}

	default_conn = conn;

	return 0;
}

static int cmd_conn_update(int argc, char *argv[])
{
	struct bt_le_conn_param param;
	int err;

	if (argc < 5) {
		return -EINVAL;
	}

	param.interval_min = strtoul(argv[1], NULL, 16);
	param.interval_max = strtoul(argv[2], NULL, 16);
	param.latency = strtoul(argv[3], NULL, 16);
	param.timeout = strtoul(argv[4], NULL, 16);

	err = bt_conn_le_param_update(default_conn, &param);
	if (err) {
		printk("conn update failed (err %d).\n", err);
	} else {
		printk("conn update initiated.\n");
	}

	return 0;
}

#if defined(CONFIG_BLUETOOTH_SMP) || defined(CONFIG_BLUETOOTH_BREDR)
static int cmd_security(int argc, char *argv[])
{
	int err, sec;

	if (!default_conn) {
		printk("Not connected\n");
		return 0;
	}

	if (argc < 2) {
		return -EINVAL;
	}

	sec = *argv[1] - '0';

	err = bt_conn_security(default_conn, sec);
	if (err) {
		printk("Setting security failed (err %d)\n", err);
	}

	return 0;
}
#endif

static const struct bt_data ad_discov[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
};

static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static int cmd_advertise(int argc, char *argv[])
{
	struct bt_le_adv_param param;
	const struct bt_data *ad, *scan_rsp;
	size_t ad_len, scan_rsp_len;
	int err;

	if (argc < 2) {
		goto fail;
	}

	if (!strcmp(argv[1], "off")) {
		if (bt_le_adv_stop() < 0) {
			printk("Failed to stop advertising\n");
		} else {
			printk("Advertising stopped\n");
		}

		return 0;
	}

	param.own_addr = NULL;
	param.interval_min = BT_GAP_ADV_FAST_INT_MIN_2;
	param.interval_max = BT_GAP_ADV_FAST_INT_MAX_2;

	if (!strcmp(argv[1], "on")) {
		param.options = BT_LE_ADV_OPT_CONNECTABLE;
		scan_rsp = sd;
		scan_rsp_len = ARRAY_SIZE(sd);
	} else if (!strcmp(argv[1], "scan")) {
		param.options = 0;
		scan_rsp = sd;
		scan_rsp_len = ARRAY_SIZE(sd);
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
		printk("Failed to start advertising (err %d)\n", err);
	} else {
		printk("Advertising started\n");
	}

	return 0;

fail:
	return -EINVAL;
}

static int cmd_oob(int argc, char *argv[])
{
	char addr[BT_ADDR_LE_STR_LEN];
	struct bt_le_oob oob;
	int err;

	err = bt_le_oob_get_local(&oob);
	if (err) {
		printk("OOB data failed\n");
		return 0;
	}

	bt_addr_le_to_str(&oob.addr, addr, sizeof(addr));

	printk("OOB data:\n");
	printk("  addr %s\n", addr);

	return 0;
}

#if defined(CONFIG_BLUETOOTH_SMP) || defined(CONFIG_BLUETOOTH_BREDR)
static void auth_passkey_display(struct bt_conn *conn, unsigned int passkey)
{
	char addr[BT_ADDR_LE_STR_LEN];
	char passkey_str[7];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	snprintk(passkey_str, 7, "%06u", passkey);

	printk("Passkey for %s: %s\n", addr, passkey_str);
}

static void auth_passkey_confirm(struct bt_conn *conn, unsigned int passkey)
{
	char addr[BT_ADDR_LE_STR_LEN];
	char passkey_str[7];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	snprintk(passkey_str, 7, "%06u", passkey);

	printk("Confirm passkey for %s: %s\n", addr, passkey_str);
}

static void auth_passkey_entry(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Enter passkey for %s\n", addr);
}

static void auth_cancel(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	conn_addr_str(conn, addr, sizeof(addr));

	printk("Pairing cancelled: %s\n", addr);

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

	printk("Confirm pairing for %s\n", addr);
}

#if defined(CONFIG_BLUETOOTH_BREDR)
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
		printk("Enter 16 digits wide PIN code for %s\n", addr);
	} else {
		printk("Enter PIN code for %s\n", addr);
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
#if defined(CONFIG_BLUETOOTH_BREDR)
	.pincode_entry = auth_pincode_entry,
#endif
	.cancel = auth_cancel,
	.pairing_confirm = auth_pairing_confirm,
};

static struct bt_conn_auth_cb auth_cb_display_yes_no = {
	.passkey_display = auth_passkey_display,
	.passkey_entry = NULL,
	.passkey_confirm = auth_passkey_confirm,
#if defined(CONFIG_BLUETOOTH_BREDR)
	.pincode_entry = auth_pincode_entry,
#endif
	.cancel = auth_cancel,
	.pairing_confirm = auth_pairing_confirm,
};

static struct bt_conn_auth_cb auth_cb_input = {
	.passkey_display = NULL,
	.passkey_entry = auth_passkey_entry,
	.passkey_confirm = NULL,
#if defined(CONFIG_BLUETOOTH_BREDR)
	.pincode_entry = auth_pincode_entry,
#endif
	.cancel = auth_cancel,
	.pairing_confirm = auth_pairing_confirm,
};

static struct bt_conn_auth_cb auth_cb_all = {
	.passkey_display = auth_passkey_display,
	.passkey_entry = auth_passkey_entry,
	.passkey_confirm = auth_passkey_confirm,
#if defined(CONFIG_BLUETOOTH_BREDR)
	.pincode_entry = auth_pincode_entry,
#endif
	.cancel = auth_cancel,
	.pairing_confirm = auth_pairing_confirm,
};

static int cmd_auth(int argc, char *argv[])
{
	if (argc < 2) {
		return -EINVAL;
	}

	if (!strcmp(argv[1], "all")) {
		bt_conn_auth_cb_register(&auth_cb_all);
	} else if (!strcmp(argv[1], "input")) {
		bt_conn_auth_cb_register(&auth_cb_input);
	} else if (!strcmp(argv[1], "display")) {
		bt_conn_auth_cb_register(&auth_cb_display);
	} else if (!strcmp(argv[1], "yesno")) {
		bt_conn_auth_cb_register(&auth_cb_display_yes_no);
	} else if (!strcmp(argv[1], "none")) {
		bt_conn_auth_cb_register(NULL);
	} else {
		return -EINVAL;
	}

	return 0;
}

static int cmd_auth_cancel(int argc, char *argv[])
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
		printk("Not connected\n");
		return 0;
	}

	bt_conn_auth_cancel(conn);

	return 0;
}

static int cmd_auth_passkey_confirm(int argc, char *argv[])
{
	if (!default_conn) {
		printk("Not connected\n");
		return 0;
	}

	bt_conn_auth_passkey_confirm(default_conn);

	return 0;
}

static int cmd_auth_pairing_confirm(int argc, char *argv[])
{
	if (!default_conn) {
		printk("Not connected\n");
		return 0;
	}

	bt_conn_auth_pairing_confirm(default_conn);

	return 0;
}

static int cmd_auth_passkey(int argc, char *argv[])
{
	unsigned int passkey;

	if (!default_conn) {
		printk("Not connected\n");
		return 0;
	}

	if (argc < 2) {
		return -EINVAL;
	}

	passkey = atoi(argv[1]);
	if (passkey > 999999) {
		printk("Passkey should be between 0-999999\n");
		return 0;
	}

	bt_conn_auth_passkey_entry(default_conn, passkey);

	return 0;
}
#endif /* CONFIG_BLUETOOTH_SMP) || CONFIG_BLUETOOTH_BREDR */

#if defined(CONFIG_BLUETOOTH_BREDR)
static int cmd_auth_pincode(int argc, char *argv[])
{
	struct bt_conn *conn;
	u8_t max = 16;

	if (default_conn) {
		conn = default_conn;
	} else if (pairing_conn) {
		conn = pairing_conn;
	} else {
		conn = NULL;
	}

	if (!conn) {
		printk("Not connected\n");
		return 0;
	}

	if (argc < 2) {
		return -EINVAL;
	}

	if (strlen(argv[1]) > max) {
		printk("PIN code value invalid - enter max %u digits\n", max);
		return 0;
	}

	printk("PIN code \"%s\" applied\n", argv[1]);

	bt_conn_auth_pincode_entry(conn, argv[1]);

	return 0;
}

static int cmd_connect_bredr(int argc, char *argv[])
{
	struct bt_conn *conn;
	bt_addr_t addr;
	int err;

	if (argc < 2) {
		return -EINVAL;
	}

	err = str2bt_addr(argv[1], &addr);
	if (err) {
		printk("Invalid peer address (err %d)\n", err);
		return 0;
	}

	conn = bt_conn_create_br(&addr, BT_BR_CONN_PARAM_DEFAULT);
	if (!conn) {
		printk("Connection failed\n");
	} else {

		printk("Connection pending\n");

		/* unref connection obj in advance as app user */
		bt_conn_unref(conn);
	}

	return 0;
}

static void br_device_found(const bt_addr_t *addr, s8_t rssi,
				  const u8_t cod[3], const u8_t eir[240])
{
	char br_addr[BT_ADDR_STR_LEN];
	char name[239];
	int len = 240;

	memset(name, 0, sizeof(name));

	while (len) {
		if (len < 2) {
			break;
		};

		/* Look for early termination */
		if (!eir[0]) {
			break;
		}

		/* Check if field length is correct */
		if (eir[0] > len - 1) {
			break;
		}

		switch (eir[1]) {
		case BT_DATA_NAME_SHORTENED:
		case BT_DATA_NAME_COMPLETE:
			if (eir[0] > sizeof(name) - 1) {
				memcpy(name, &eir[2], sizeof(name) - 1);
			} else {
				memcpy(name, &eir[2], eir[0] - 1);
			}
			break;
		default:
			break;
		}

		/* Parse next AD Structure */
		len -= eir[0] + 1;
		eir += eir[0] + 1;
	}

	bt_addr_to_str(addr, br_addr, sizeof(br_addr));

	printk("[DEVICE]: %s, RSSI %i %s\n", br_addr, rssi, name);
}

static struct bt_br_discovery_result br_discovery_results[5];

static void br_discovery_complete(struct bt_br_discovery_result *results,
				  size_t count)
{
	size_t i;

	printk("BR/EDR discovery complete\n");

	for (i = 0; i < count; i++) {
		br_device_found(&results[i].addr, results[i].rssi,
				results[i].cod, results[i].eir);
	}
}

static int cmd_bredr_discovery(int argc, char *argv[])
{
	const char *action;

	if (argc < 2) {
		return -EINVAL;
	}

	action = argv[1];
	if (!strcmp(action, "on")) {
		struct bt_br_discovery_param param;

		param.limited = false;
		param.length = 8;

		if (argc > 2) {
			param.length = atoi(argv[2]);
		}

		if (argc > 3 && !strcmp(argv[3], "limited")) {
			param.limited = true;
		}

		if (bt_br_discovery_start(&param, br_discovery_results,
					  ARRAY_SIZE(br_discovery_results),
					  br_discovery_complete) < 0) {
			printk("Failed to start discovery\n");
			return 0;
		}

		printk("Discovery started\n");
	} else if (!strcmp(action, "off")) {
		if (bt_br_discovery_stop()) {
			printk("Failed to stop discovery\n");
			return 0;
		}

		printk("Discovery stopped\n");
	} else {
		return -EINVAL;
	}

	return 0;
}

#endif /* CONFIG_BLUETOOTH_BREDR */

static int cmd_clear(int argc, char *argv[])
{
	bt_addr_le_t addr;
	int err;

	if (argc < 2) {
		printk("Specify remote address or \"all\"\n");
		return 0;
	}

	if (strcmp(argv[1], "all") == 0) {
		err = bt_storage_clear(NULL);
		if (err) {
			printk("Failed to clear storage (err %d)\n", err);
		} else {
			printk("Storage successfully cleared\n");
		}

		return 0;
	}

	if (argc < 3) {
#if defined(CONFIG_BLUETOOTH_BREDR)
		addr.type = BT_ADDR_LE_PUBLIC;
		err = str2bt_addr(argv[1], &addr.a);
#else
		printk("Both address and address type needed\n");
		return 0;
#endif
	} else {
		err = str2bt_addr_le(argv[1], argv[2], &addr);
	}

	if (err) {
		printk("Invalid address\n");
		return 0;
	}

	err = bt_storage_clear(&addr);
	if (err) {
		printk("Failed to clear storage (err %d)\n", err);
	} else {
		printk("Storage successfully cleared\n");
	}

	return 0;
}

#if defined(CONFIG_BLUETOOTH_L2CAP_DYNAMIC_CHANNEL)
static void hexdump(const u8_t *data, size_t len)
{
	int n = 0;

	while (len--) {
		if (n % 16 == 0) {
			printk("%08X ", n);
		}

		printk("%02X ", *data++);

		n++;
		if (n % 8 == 0) {
			if (n % 16 == 0) {
				printk("\n");
			} else {
				printk(" ");
			}
		}
	}

	if (n % 16) {
		printk("\n");
	}
}

static u32_t l2cap_rate;

static void l2cap_recv_metrics(struct bt_l2cap_chan *chan, struct net_buf *buf)
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
}

static void l2cap_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	printk("Incoming data channel %p len %u\n", chan, buf->len);

	if (buf->len) {
		hexdump(buf->data, buf->len);
	}
}

static void l2cap_connected(struct bt_l2cap_chan *chan)
{
	printk("Channel %p connected\n", chan);
}

static void l2cap_disconnected(struct bt_l2cap_chan *chan)
{
	printk("Channel %p disconnected\n", chan);
}

static struct net_buf *l2cap_alloc_buf(struct bt_l2cap_chan *chan)
{
	/* print if metrics is disabled */
	if (chan->ops->recv != l2cap_recv_metrics) {
		printk("Channel %p requires buffer\n", chan);
	}

	return net_buf_alloc(&data_rx_pool, K_FOREVER);
}

static struct bt_l2cap_chan_ops l2cap_ops = {
	.alloc_buf	= l2cap_alloc_buf,
	.recv		= l2cap_recv,
	.connected	= l2cap_connected,
	.disconnected	= l2cap_disconnected,
};

static struct bt_l2cap_le_chan l2cap_chan = {
	.chan.ops	= &l2cap_ops,
	.rx.mtu		= DATA_MTU,
};

static int l2cap_accept(struct bt_conn *conn, struct bt_l2cap_chan **chan)
{
	printk("Incoming conn %p\n", conn);

	if (l2cap_chan.chan.conn) {
		printk("No channels available\n");
		return -ENOMEM;
	}

	*chan = &l2cap_chan.chan;

	return 0;
}

static struct bt_l2cap_server server = {
	.accept		= l2cap_accept,
};

static int cmd_l2cap_register(int argc, char *argv[])
{
	if (argc < 2) {
		return -EINVAL;
	}

	if (server.psm) {
		printk("Already registered\n");
		return 0;
	}

	server.psm = strtoul(argv[1], NULL, 16);

	if (argc > 2) {
		server.sec_level = strtoul(argv[2], NULL, 10);
	}

	if (bt_l2cap_server_register(&server) < 0) {
		printk("Unable to register psm\n");
		server.psm = 0;
	} else {
		printk("L2CAP psm %u sec_level %u registered\n", server.psm,
		       server.sec_level);
	}

	return 0;
}

static int cmd_l2cap_connect(int argc, char *argv[])
{
	u16_t psm;
	int err;

	if (!default_conn) {
		printk("Not connected\n");
		return 0;
	}

	if (argc < 2) {
		return -EINVAL;
	}

	if (l2cap_chan.chan.conn) {
		printk("Channel already in use\n");
		return -EINVAL;
	}

	psm = strtoul(argv[1], NULL, 16);

	err = bt_l2cap_chan_connect(default_conn, &l2cap_chan.chan, psm);
	if (err < 0) {
		printk("Unable to connect to psm %u (err %u)\n", psm, err);
	} else {
		printk("L2CAP connection pending\n");
	}

	return 0;
}

static int cmd_l2cap_disconnect(int argc, char *argv[])
{
	int err;

	err = bt_l2cap_chan_disconnect(&l2cap_chan.chan);
	if (err) {
		printk("Unable to disconnect: %u\n", -err);
	}

	return 0;
}

static int cmd_l2cap_send(int argc, char *argv[])
{
	static u8_t buf_data[DATA_MTU] = { [0 ... (DATA_MTU - 1)] = 0xff };
	int ret, len, count = 1;
	struct net_buf *buf;

	if (argc > 1) {
		count = strtoul(argv[1], NULL, 10);
	}

	len = min(l2cap_chan.tx.mtu, DATA_MTU - BT_L2CAP_CHAN_SEND_RESERVE);

	while (count--) {
		buf = net_buf_alloc(&data_tx_pool, K_FOREVER);
		net_buf_reserve(buf, BT_L2CAP_CHAN_SEND_RESERVE);

		net_buf_add_mem(buf, buf_data, len);
		ret = bt_l2cap_chan_send(&l2cap_chan.chan, buf);
		if (ret < 0) {
			printk("Unable to send: %d\n", -ret);
			net_buf_unref(buf);
			break;
		}
	}

	return 0;
}

static int cmd_l2cap_metrics(int argc, char *argv[])
{
	const char *action;

	if (argc < 2) {
		printk("l2cap rate: %u bps.\n", l2cap_rate);

		return 0;
	}

	action = argv[1];

	if (!strcmp(action, "on")) {
		l2cap_ops.recv = l2cap_recv_metrics;
	} else if (!strcmp(action, "off")) {
		l2cap_ops.recv = l2cap_recv;
	} else {
		return -EINVAL;
	}

	printk("l2cap metrics %s.\n", action);

	return 0;
}

#endif

#if defined(CONFIG_BLUETOOTH_BREDR)
static void l2cap_bredr_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	printk("Incoming data channel %p len %u\n", chan, buf->len);
}

static void l2cap_bredr_connected(struct bt_l2cap_chan *chan)
{
	printk("Channel %p connected\n", chan);
}

static void l2cap_bredr_disconnected(struct bt_l2cap_chan *chan)
{
	printk("Channel %p disconnected\n", chan);
}

static struct net_buf *l2cap_bredr_alloc_buf(struct bt_l2cap_chan *chan)
{
	printk("Channel %p requires buffer\n", chan);

	return net_buf_alloc(&data_bredr_pool, K_FOREVER);
}

static struct bt_l2cap_chan_ops l2cap_bredr_ops = {
	.alloc_buf	= l2cap_bredr_alloc_buf,
	.recv		= l2cap_bredr_recv,
	.connected	= l2cap_bredr_connected,
	.disconnected	= l2cap_bredr_disconnected,
};

static struct bt_l2cap_br_chan l2cap_bredr_chan = {
	.chan.ops	= &l2cap_bredr_ops,
	 /* Set for now min. MTU */
	.rx.mtu		= 48,
};

static int l2cap_bredr_accept(struct bt_conn *conn, struct bt_l2cap_chan **chan)
{
	printk("Incoming BR/EDR conn %p\n", conn);

	if (l2cap_bredr_chan.chan.conn) {
		printk("No channels available");
		return -ENOMEM;
	}

	*chan = &l2cap_bredr_chan.chan;

	return 0;
}

static struct bt_l2cap_server br_server = {
	.accept = l2cap_bredr_accept,
};

static int cmd_bredr_l2cap_register(int argc, char *argv[])
{
	if (argc < 2) {
		return -EINVAL;
	}

	if (br_server.psm) {
		printk("Already registered\n");
		return 0;
	}

	br_server.psm = strtoul(argv[1], NULL, 16);

	if (bt_l2cap_br_server_register(&br_server) < 0) {
		printk("Unable to register psm\n");
		br_server.psm = 0;
	} else {
		printk("L2CAP psm %u registered\n", br_server.psm);
	}

	return 0;
}

#if defined(CONFIG_BLUETOOTH_RFCOMM)
static void rfcomm_bredr_recv(struct bt_rfcomm_dlc *dlci, struct net_buf *buf)
{
	printk("Incoming data dlc %p len %u\n", dlci, buf->len);
}

static void rfcomm_bredr_connected(struct bt_rfcomm_dlc *dlci)
{
	printk("Dlc %p connected\n", dlci);
}

static void rfcomm_bredr_disconnected(struct bt_rfcomm_dlc *dlci)
{
	printk("Dlc %p disconnected\n", dlci);
}

static struct bt_rfcomm_dlc_ops rfcomm_bredr_ops = {
	.recv		= rfcomm_bredr_recv,
	.connected	= rfcomm_bredr_connected,
	.disconnected	= rfcomm_bredr_disconnected,
};

static struct bt_rfcomm_dlc rfcomm_dlc = {
	.ops = &rfcomm_bredr_ops,
	.mtu = 30,
};

static int rfcomm_bredr_accept(struct bt_conn *conn, struct bt_rfcomm_dlc **dlc)
{
	printk("Incoming RFCOMM conn %p\n", conn);

	if (rfcomm_dlc.session) {
		printk("No channels available");
		return -ENOMEM;
	}

	*dlc = &rfcomm_dlc;

	return 0;
}

struct bt_rfcomm_server rfcomm_server = {
	.accept = &rfcomm_bredr_accept,
};

static int cmd_bredr_rfcomm_register(int argc, char *argv[])
{
	int ret;

	if (rfcomm_server.channel) {
		printk("Already registered\n");
		return 0;
	}

	rfcomm_server.channel = BT_RFCOMM_CHAN_SPP;

	ret = bt_rfcomm_server_register(&rfcomm_server);
	if (ret < 0) {
		printk("Unable to register channel %x\n", ret);
		rfcomm_server.channel = 0;
	} else {
		printk("RFCOMM channel %u registered\n", rfcomm_server.channel);
		bt_sdp_register_service(&spp_rec);
	}

	return 0;
}

static int cmd_rfcomm_connect(int argc, char *argv[])
{
	u8_t channel;
	int err;

	if (!default_conn) {
		printk("Not connected\n");
		return 0;
	}

	if (argc < 2) {
		return -EINVAL;
	}

	channel = strtoul(argv[1], NULL, 16);

	err = bt_rfcomm_dlc_connect(default_conn, &rfcomm_dlc, channel);
	if (err < 0) {
		printk("Unable to connect to channel %d (err %u)\n",
		       channel, err);
	} else {
		printk("RFCOMM connection pending\n");
	}

	return 0;
}

static int cmd_rfcomm_send(int argc, char *argv[])
{
	u8_t buf_data[DATA_BREDR_MTU] = { [0 ... (DATA_BREDR_MTU - 1)] =
					    0xff };
	int ret, len, count = 1;
	struct net_buf *buf;

	if (argc > 1) {
		count = strtoul(argv[1], NULL, 10);
	}

	while (count--) {
		buf = bt_rfcomm_create_pdu(&data_bredr_pool);
		/* Should reserve one byte in tail for FCS */
		len = min(rfcomm_dlc.mtu, net_buf_tailroom(buf) - 1);

		net_buf_add_mem(buf, buf_data, len);
		ret = bt_rfcomm_dlc_send(&rfcomm_dlc, buf);
		if (ret < 0) {
			printk("Unable to send: %d\n", -ret);
			net_buf_unref(buf);
			break;
		}
	}

	return 0;
}

static int cmd_rfcomm_disconnect(int argc, char *argv[])
{
	int err;

	err = bt_rfcomm_dlc_disconnect(&rfcomm_dlc);
	if (err) {
		printk("Unable to disconnect: %u\n", -err);
	}

	return 0;
}

#endif /* CONFIG_BLUETOOTH_RFCOMM) */

static int cmd_bredr_discoverable(int argc, char *argv[])
{
	int err;
	const char *action;

	if (argc < 2) {
		return -EINVAL;
	}

	action = argv[1];

	if (!strcmp(action, "on")) {
		err = bt_br_set_discoverable(true);
	} else if (!strcmp(action, "off")) {
		err = bt_br_set_discoverable(false);
	} else {
		return -EINVAL;
	}

	if (err) {
		printk("BR/EDR set/reset discoverable failed (err %d)\n", err);
	} else {
		printk("BR/EDR set/reset discoverable done\n");
	}

	return 0;
}

static int cmd_bredr_connectable(int argc, char *argv[])
{
	int err;
	const char *action;

	if (argc < 2) {
		return -EINVAL;
	}

	action = argv[1];

	if (!strcmp(action, "on")) {
		err = bt_br_set_connectable(true);
	} else if (!strcmp(action, "off")) {
		err = bt_br_set_connectable(false);
	} else {
		return -EINVAL;
	}

	if (err) {
		printk("BR/EDR set/rest connectable failed (err %d)\n", err);
	} else {
		printk("BR/EDR set/reset connectable done\n");
	}

	return 0;
}

static int cmd_bredr_oob(int argc, char *argv[])
{
	char addr[BT_ADDR_STR_LEN];
	struct bt_br_oob oob;
	int err;

	err = bt_br_oob_get_local(&oob);
	if (err) {
		printk("BR/EDR OOB data failed\n");
		return 0;
	}

	bt_addr_to_str(&oob.addr, addr, sizeof(addr));

	printk("BR/EDR OOB data:\n");
	printk("  addr %s\n", addr);

	return 0;
}

static int cmd_bredr_sdp_find_record(int argc, char *argv[])
{
	int err = 0, res;
	const char *action;

	if (!default_conn) {
		printk("Not connected\n");
		return 0;
	}

	if (argc < 2) {
		return -EINVAL;
	}

	action = argv[1];

	if (!strcmp(action, "HFPAG")) {
		discov = discov_hfpag;
	} else if (!strcmp(action, "A2SRC")) {
		discov = discov_a2src;
	} else {
		err = -EINVAL;
	}

	if (err) {
		printk("SDP UUID to resolve not valid (err %d)\n", err);
		printk("Supported UUID are \'HFPAG\' \'A2SRC\' only\n");
		return err;
	}

	printk("SDP UUID \'%s\' gets applied\n", action);

	res = bt_sdp_discover(default_conn, &discov);
	if (res) {
		printk("SDP discovery failed: result %d\n", res);
	} else {
		printk("SDP discovery started\n");
	}

	return 0;
}
#endif

#define HELP_NONE "[none]"
#define HELP_ADDR_LE "<address: XX:XX:XX:XX:XX:XX> <type: (public|random)>"

static const struct shell_cmd bt_commands[] = {
	{ "init", cmd_init, HELP_ADDR_LE },
	{ "scan", cmd_scan,
	  "<value: on, passive, off> <dup filter: dups, nodups>" },
	{ "advertise", cmd_advertise,
	  "<type: off, on, scan, nconn> <mode: discov, non_discov>" },
	{ "connect", cmd_connect_le, HELP_ADDR_LE },
	{ "disconnect", cmd_disconnect, HELP_NONE },
	{ "auto-conn", cmd_auto_conn, HELP_ADDR_LE },
	{ "select", cmd_select, HELP_ADDR_LE },
	{ "conn-update", cmd_conn_update, "<min> <max> <latency> <timeout>" },
	{ "oob", cmd_oob },
	{ "clear", cmd_clear },
#if defined(CONFIG_BLUETOOTH_SMP) || defined(CONFIG_BLUETOOTH_BREDR)
	{ "security", cmd_security, "<security level: 0, 1, 2, 3>" },
	{ "auth", cmd_auth,
	  "<authentication method: all, input, display, yesno, none>" },
	{ "auth-cancel", cmd_auth_cancel, HELP_NONE },
	{ "auth-passkey", cmd_auth_passkey, "<passkey>" },
	{ "auth-passkey-confirm", cmd_auth_passkey_confirm, HELP_NONE },
	{ "auth-pairing-confirm", cmd_auth_pairing_confirm, HELP_NONE },
#if defined(CONFIG_BLUETOOTH_BREDR)
	{ "auth-pincode", cmd_auth_pincode, "<pincode>" },
#endif /* CONFIG_BLUETOOTH_BREDR */
#endif /* CONFIG_BLUETOOTH_SMP || CONFIG_BLUETOOTH_BREDR) */
#if defined(CONFIG_BLUETOOTH_GATT_CLIENT)
	{ "gatt-exchange-mtu", cmd_gatt_exchange_mtu, HELP_NONE },
	{ "gatt-discover-primary", cmd_gatt_discover,
	  "<UUID> [start handle] [end handle]" },
	{ "gatt-discover-secondary", cmd_gatt_discover,
	  "<UUID> [start handle] [end handle]" },
	{ "gatt-discover-include", cmd_gatt_discover,
	  "[UUID] [start handle] [end handle]" },
	{ "gatt-discover-characteristic", cmd_gatt_discover,
	  "[UUID] [start handle] [end handle]" },
	{ "gatt-discover-descriptor", cmd_gatt_discover,
	  "[UUID] [start handle] [end handle]" },
	{ "gatt-read", cmd_gatt_read, "<handle> [offset]" },
	{ "gatt-read-multiple", cmd_gatt_mread, "<handle 1> <handle 2> ..." },
	{ "gatt-write", cmd_gatt_write, "<handle> <offset> <data> [length]" },
	{ "gatt-write-without-response", cmd_gatt_write_without_rsp,
	  "<handle> <data> [length] [repeat]" },
	{ "gatt-write-signed", cmd_gatt_write_without_rsp,
	  "<handle> <data> [length] [repeat]" },
	{ "gatt-subscribe", cmd_gatt_subscribe,
	  "<CCC handle> <value handle> [ind]" },
	{ "gatt-unsubscribe", cmd_gatt_unsubscribe, HELP_NONE },
#endif /* CONFIG_BLUETOOTH_GATT_CLIENT */
	{ "gatt-register-service", cmd_gatt_register_test_svc,
	  "register pre-predefined test service" },
	{ "gatt-metrics", cmd_gatt_write_cmd_metrics,
	  "register vendr char and measure rx [value on, off]" },
#if defined(CONFIG_BLUETOOTH_L2CAP_DYNAMIC_CHANNEL)
	{ "l2cap-register", cmd_l2cap_register, "<psm> [sec_level]" },
	{ "l2cap-connect", cmd_l2cap_connect, "<psm>" },
	{ "l2cap-disconnect", cmd_l2cap_disconnect, HELP_NONE },
	{ "l2cap-send", cmd_l2cap_send, "<number of packets>" },
	{ "l2cap-metrics", cmd_l2cap_metrics, "<value on, off>" },
#endif
#if defined(CONFIG_BLUETOOTH_BREDR)
	{ "br-iscan", cmd_bredr_discoverable, "<value: on, off>" },
	{ "br-pscan", cmd_bredr_connectable, "value: on, off" },
	{ "br-connect", cmd_connect_bredr, "<address>" },
	{ "br-discovery", cmd_bredr_discovery,
	  "<value: on, off> [length: 1-48] [mode: limited]"  },
	{ "br-l2cap-register", cmd_bredr_l2cap_register, "<psm>" },
	{ "br-oob", cmd_bredr_oob },
	{ "br-sdp-find", cmd_bredr_sdp_find_record, "<HFPAG>" },
#if defined(CONFIG_BLUETOOTH_RFCOMM)
	{ "br-rfcomm-register", cmd_bredr_rfcomm_register },
	{ "br-rfcomm-connect", cmd_rfcomm_connect, "<channel>" },
	{ "br-rfcomm-send", cmd_rfcomm_send, "<number of packets>"},
	{ "br-rfcomm-disconnect", cmd_rfcomm_disconnect, HELP_NONE },
#endif /* CONFIG_BLUETOOTH_RFCOMM */
#endif
#if defined(CONFIG_BLUETOOTH_CONTROLLER_ADV_EXT)
	{ "advx", cmd_advx, "<on off> [coded] [anon] [txp]" },
	{ "scanx", cmd_scanx, "<on passive off> [coded]" },
#endif /* CONFIG_BLUETOOTH_CONTROLLER_ADV_EXT */
	{ NULL, NULL, NULL }
};

SHELL_REGISTER_WITH_PROMPT(BT_SHELL_MODULE, bt_commands, current_prompt);
