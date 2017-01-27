/** @file
 *  @brief Interactive Bluetooth LE shell application
 *
 *  Application allows implement Bluetooth LE functional commands performing
 *  simple diagnostic interaction between LE host stack and LE controller
 */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <misc/printk.h>
#include <misc/byteorder.h>
#include <zephyr.h>

#include <console/uart_console.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/gatt.h>
#include <bluetooth/l2cap.h>
#include <bluetooth/rfcomm.h>
#include <bluetooth/storage.h>
#include <bluetooth/sdp.h>

#include <shell/shell.h>

#include <gatt/gap.h>
#include <gatt/hrs.h>

#define DEVICE_NAME		CONFIG_BLUETOOTH_DEVICE_NAME
#define DEVICE_NAME_LEN		(sizeof(DEVICE_NAME) - 1)
#define CREDITS			10
#define DATA_MTU		(23 * CREDITS)
#define DATA_BREDR_MTU		48

#define MY_SHELL_MODULE "btshell"
static struct bt_conn *default_conn;
static bt_addr_le_t id_addr;

/* Connection context for BR/EDR legacy pairing in sec mode 3 */
static struct bt_conn *pairing_conn;

#if defined(CONFIG_BLUETOOTH_L2CAP_DYNAMIC_CHANNEL)
NET_BUF_POOL_DEFINE(data_pool, 1, DATA_MTU, BT_BUF_USER_DATA_MIN, NULL);
#endif

#if defined(CONFIG_BLUETOOTH_BREDR)
NET_BUF_POOL_DEFINE(data_bredr_pool, 1, DATA_BREDR_MTU, BT_BUF_USER_DATA_MIN,
		    NULL);

#endif /* CONFIG_BLUETOOTH_BREDR */

#if defined(CONFIG_BLUETOOTH_RFCOMM)

static struct bt_sdp_attribute spp_attrs[] = {
	BT_SDP_NEW_SERVICE,
	BT_SDP_LIST(
		BT_SDP_ATTR_SVCLASS_ID_LIST,
		BT_SDP_TYPE_SIZE(BT_SDP_SEQ8, 3),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
			BT_SDP_ARRAY_16(BT_SDP_SERIAL_PORT_SVCLASS)
		},
		)
	),
	BT_SDP_LIST(
		BT_SDP_ATTR_PROTO_DESC_LIST,
		BT_SDP_TYPE_SIZE(BT_SDP_SEQ8, 12),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE(BT_SDP_SEQ8, 3),
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				BT_SDP_ARRAY_16(BT_UUID_L2CAP_VAL)
			},
			)
		},
		{
			BT_SDP_TYPE_SIZE(BT_SDP_SEQ8, 5),
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
		BT_SDP_TYPE_SIZE(BT_SDP_SEQ8, 8),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE(BT_SDP_SEQ8, 6),
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

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t evtype,
			 struct net_buf_simple *buf)
{
	char le_addr[BT_ADDR_LE_STR_LEN];
	char name[30];

	memset(name, 0, sizeof(name));

	while (buf->len > 1) {
		uint8_t len, type;

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

static void connected(struct bt_conn *conn, uint8_t err)
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

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	conn_addr_str(conn, addr, sizeof(addr));
	printk("Disconnected: %s (reason %u)\n", addr, reason);

	if (default_conn == conn) {
		bt_conn_unref(default_conn);
		default_conn = NULL;
	}
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
#if defined(CONFIG_BLUETOOTH_SMP)
	.identity_resolved = identity_resolved,
#endif
#if defined(CONFIG_BLUETOOTH_SMP) || defined(CONFIG_BLUETOOTH_BREDR)
	.security_changed = security_changed,
#endif
};

static uint16_t appearance_value = 0x0001;

static int char2hex(const char *c, uint8_t *x)
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

static int str2bt_addr_le(const char *str, const char *type, bt_addr_le_t *addr)
{
	int i, j;
	uint8_t tmp;

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

		addr->a.val[i] = addr->a.val[i] << 4;

		if (char2hex(str, &tmp) < 0) {
			return -EINVAL;
		}

		addr->a.val[i] |= tmp;
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

static ssize_t storage_read(const bt_addr_le_t *addr, uint16_t key, void *data,
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

static ssize_t storage_write(const bt_addr_le_t *addr, uint16_t key,
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

	gap_init(DEVICE_NAME, appearance_value);
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

static void cmd_active_scan_on(void)
{
	int err;

	err = bt_le_scan_start(BT_LE_SCAN_ACTIVE, device_found);
	if (err) {
		printk("Bluetooth set active scan failed (err %d)\n", err);
	} else {
		printk("Bluetooth active scan enabled\n");
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

	if (argc < 2) {
		return -EINVAL;
	}

	action = argv[1];
	if (!strcmp(action, "on")) {
		cmd_active_scan_on();
	} else if (!strcmp(action, "off")) {
		cmd_scan_off();
	} else {
		return -EINVAL;
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

static void exchange_func(struct bt_conn *conn, uint8_t err,
			  struct bt_gatt_exchange_params *params)
{
	printk("Exchange %s\n", err == 0 ? "successful" : "failed");
}

static struct bt_gatt_exchange_params exchange_params;

static int cmd_gatt_exchange_mtu(int argc, char *argv[])
{
	int err;

	if (!default_conn) {
		printk("Not connected\n");
		return 0;
	}

	exchange_params.func = exchange_func;

	err = bt_gatt_exchange_mtu(default_conn, &exchange_params);
	if (err) {
		printk("Exchange failed (err %d)\n", err);
	} else {
		printk("Exchange pending\n");
	}

	return 0;
}

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

	if (bt_le_adv_start(&param, ad, ad_len, scan_rsp, scan_rsp_len) < 0) {
		printk("Failed to start advertising\n");
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

static struct bt_gatt_discover_params discover_params;
static struct bt_uuid_16 uuid = BT_UUID_INIT_16(0);

static void print_chrc_props(uint8_t properties)
{
	printk("Properties: ");

	if (properties & BT_GATT_CHRC_BROADCAST) {
		printk("[bcast]");
	}

	if (properties & BT_GATT_CHRC_READ) {
		printk("[read]");
	}

	if (properties & BT_GATT_CHRC_WRITE) {
		printk("[write]");
	}

	if (properties & BT_GATT_CHRC_WRITE_WITHOUT_RESP) {
		printk("[write w/w rsp]");
	}

	if (properties & BT_GATT_CHRC_NOTIFY) {
		printk("[notify]");
	}

	if (properties & BT_GATT_CHRC_INDICATE) {
		printk("[indicate]");
	}

	if (properties & BT_GATT_CHRC_AUTH) {
		printk("[auth]");
	}

	if (properties & BT_GATT_CHRC_EXT_PROP) {
		printk("[ext prop]");
	}

	printk("\n");
}

static uint8_t discover_func(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr,
			     struct bt_gatt_discover_params *params)
{
	struct bt_gatt_service *gatt_service;
	struct bt_gatt_chrc *gatt_chrc;
	struct bt_gatt_include *gatt_include;
	char uuid[37];

	if (!attr) {
		printk("Discover complete\n");
		memset(params, 0, sizeof(*params));
		return BT_GATT_ITER_STOP;
	}

	switch (params->type) {
	case BT_GATT_DISCOVER_SECONDARY:
	case BT_GATT_DISCOVER_PRIMARY:
		gatt_service = attr->user_data;
		bt_uuid_to_str(gatt_service->uuid, uuid, sizeof(uuid));
		printk("Service %s found: start handle %x, end_handle %x\n",
		       uuid, attr->handle, gatt_service->end_handle);
		break;
	case BT_GATT_DISCOVER_CHARACTERISTIC:
		gatt_chrc = attr->user_data;
		bt_uuid_to_str(gatt_chrc->uuid, uuid, sizeof(uuid));
		printk("Characteristic %s found: handle %x\n", uuid,
		       attr->handle);
		print_chrc_props(gatt_chrc->properties);
		break;
	case BT_GATT_DISCOVER_INCLUDE:
		gatt_include = attr->user_data;
		bt_uuid_to_str(gatt_include->uuid, uuid, sizeof(uuid));
		printk("Include %s found: handle %x, start %x, end %x\n",
		       uuid, attr->handle, gatt_include->start_handle,
		       gatt_include->end_handle);
		break;
	default:
		bt_uuid_to_str(attr->uuid, uuid, sizeof(uuid));
		printk("Descriptor %s found: handle %x\n", uuid, attr->handle);
		break;
	}

	return BT_GATT_ITER_CONTINUE;
}

static int cmd_gatt_discover(int argc, char *argv[])
{
	int err;

	if (!default_conn) {
		printk("Not connected\n");
		return 0;
	}

	discover_params.func = discover_func;
	discover_params.start_handle = 0x0001;
	discover_params.end_handle = 0xffff;

	if (argc < 2) {
		if (!strcmp(argv[0], "gatt-discover-primary") ||
		    !strcmp(argv[0], "gatt-discover-secondary")) {
			return -EINVAL;
		}
		goto done;
	}

	/* Only set the UUID if the value is valid (non zero) */
	uuid.val = strtoul(argv[1], NULL, 16);
	if (uuid.val) {
		discover_params.uuid = &uuid.uuid;
	}

	if (argc > 2) {
		discover_params.start_handle = strtoul(argv[2], NULL, 16);
		if (argc > 3) {
			discover_params.end_handle = strtoul(argv[3], NULL, 16);
		}
	}

done:
	if (!strcmp(argv[0], "gatt-discover-secondary")) {
		discover_params.type = BT_GATT_DISCOVER_SECONDARY;
	} else if (!strcmp(argv[0], "gatt-discover-include")) {
		discover_params.type = BT_GATT_DISCOVER_INCLUDE;
	} else if (!strcmp(argv[0], "gatt-discover-characteristic")) {
		discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;
	} else if (!strcmp(argv[0], "gatt-discover-descriptor")) {
		discover_params.type = BT_GATT_DISCOVER_DESCRIPTOR;
	} else {
		discover_params.type = BT_GATT_DISCOVER_PRIMARY;
	}

	err = bt_gatt_discover(default_conn, &discover_params);
	if (err) {
		printk("Discover failed (err %d)\n", err);
	} else {
		printk("Discover pending\n");
	}

	return 0;
}

static struct bt_gatt_read_params read_params;

static uint8_t read_func(struct bt_conn *conn, uint8_t err,
			 struct bt_gatt_read_params *params,
			 const void *data, uint16_t length)
{
	printk("Read complete: err %u length %u\n", err, length);

	if (!data) {
		memset(params, 0, sizeof(*params));
		return BT_GATT_ITER_STOP;
	}

	return BT_GATT_ITER_CONTINUE;
}

static int cmd_gatt_read(int argc, char *argv[])
{
	int err;

	if (!default_conn) {
		printk("Not connected\n");
		return 0;
	}

	read_params.func = read_func;

	if (argc < 2) {
		return -EINVAL;
	}

	read_params.handle_count = 1;
	read_params.single.handle = strtoul(argv[1], NULL, 16);

	if (argc > 2) {
		read_params.single.offset = strtoul(argv[2], NULL, 16);
	}

	err = bt_gatt_read(default_conn, &read_params);
	if (err) {
		printk("Read failed (err %d)\n", err);
	} else {
		printk("Read pending\n");
	}

	return 0;
}

static int cmd_gatt_mread(int argc, char *argv[])
{
	uint16_t h[8];
	int i, err;

	if (!default_conn) {
		printk("Not connected\n");
		return 0;
	}

	if (argc < 3) {
		return -EINVAL;
	}

	if (argc - 1 >  ARRAY_SIZE(h)) {
		printk("Enter max %lu handle items to read\n", ARRAY_SIZE(h));
		return 0;
	}

	for (i = 0; i < argc - 1; i++) {
		h[i] = strtoul(argv[i + 1], NULL, 16);
	}

	read_params.func = read_func;
	read_params.handle_count = i;
	read_params.handles = h; /* not used in read func */

	err = bt_gatt_read(default_conn, &read_params);
	if (err) {
		printk("GATT multiple read request failed (err %d)\n", err);
	}

	return 0;
}

static void write_func(struct bt_conn *conn, uint8_t err,
		       struct bt_gatt_write_params *params)
{
	printk("Write complete: err %u\n", err);
}

static struct bt_gatt_write_params write_params;

static int cmd_gatt_write(int argc, char *argv[])
{
	int err;
	uint16_t handle, offset;
	uint8_t buf[100];
	uint8_t data;

	if (!default_conn) {
		printk("Not connected\n");
		return 0;
	}

	if (argc < 4) {
		return -EINVAL;
	}

	handle = strtoul(argv[1], NULL, 16);
	/* TODO: Add support for longer data */
	offset = strtoul(argv[2], NULL, 16);
	data = strtoul(argv[3], NULL, 16);

	if (argc == 5) {
		size_t len = min(strtoul(argv[4], NULL, 16), sizeof(buf));
		int i;

		for (i = 0; i < len; i++) {
			buf[i] = data;
		}

		write_params.data = buf;
		write_params.length = len;
	} else {
		write_params.data = &data;
		write_params.length = sizeof(data);
	}

	write_params.handle = handle;
	write_params.offset = offset;
	write_params.func = write_func;

	err = bt_gatt_write(default_conn, &write_params);
	if (err) {
		printk("Write failed (err %d)\n", err);
	} else {
		printk("Write pending\n");
	}

	return 0;
}

static int cmd_gatt_write_without_rsp(int argc, char *argv[])
{
	int err;
	uint16_t handle;
	uint8_t data;

	if (!default_conn) {
		printk("Not connected\n");
		return 0;
	}

	if (argc < 3) {
		return -EINVAL;
	}

	handle = strtoul(argv[1], NULL, 16);
	data = strtoul(argv[2], NULL, 16);

	err = bt_gatt_write_without_response(default_conn, handle, &data,
					     sizeof(data), false);
	printk("Write Complete (err %d)\n", err);

	return 0;
}

static int cmd_gatt_write_signed(int argc, char *argv[])
{
	int err;
	uint16_t handle;
	uint8_t data;

	if (!default_conn) {
		printk("Not connected\n");
		return 0;
	}

	if (argc < 3) {
		return -EINVAL;
	}

	handle = strtoul(argv[1], NULL, 16);
	data = strtoul(argv[2], NULL, 16);

	err = bt_gatt_write_without_response(default_conn, handle, &data,
					     sizeof(data), true);
	printk("Write Complete (err %d)\n", err);

	return 0;
}

static struct bt_gatt_subscribe_params subscribe_params;

static uint8_t notify_func(struct bt_conn *conn,
			   struct bt_gatt_subscribe_params *params,
			   const void *data, uint16_t length)
{
	if (!data) {
		printk("Ubsubscribed\n");
		params->value_handle = 0;
		return BT_GATT_ITER_STOP;
	}

	printk("Notification: data %p length %u\n", data, length);

	return BT_GATT_ITER_CONTINUE;
}

static int cmd_gatt_subscribe(int argc, char *argv[])
{
	int err;

	if (subscribe_params.value_handle) {
		printk("Cannot subscribe: subscription to %x already exists\n",
		       subscribe_params.value_handle);
		return 0;
	}

	if (!default_conn) {
		printk("Not connected\n");
		return 0;
	}

	if (argc < 3) {
		return -EINVAL;
	}

	subscribe_params.ccc_handle = strtoul(argv[1], NULL, 16);
	subscribe_params.value_handle = strtoul(argv[2], NULL, 16);
	subscribe_params.value = BT_GATT_CCC_NOTIFY;
	subscribe_params.notify = notify_func;

	if (argc > 3) {
		subscribe_params.value = strtoul(argv[3], NULL, 16);
	}

	err = bt_gatt_subscribe(default_conn, &subscribe_params);
	if (err) {
		printk("Subscribe failed (err %d)\n", err);
	} else {
		printk("Subscribed\n");
	}

	return 0;
}

static int cmd_gatt_unsubscribe(int argc, char *argv[])
{
	int err;

	if (!default_conn) {
		printk("Not connected\n");
		return 0;
	}

	if (!subscribe_params.value_handle) {
		printk("No subscription found\n");
		return 0;
	}

	err = bt_gatt_unsubscribe(default_conn, &subscribe_params);
	if (err) {
		printk("Unsubscribe failed (err %d)\n", err);
	} else {
		printk("Unsubscribe success\n");
	}

	/* Clear subscribe_params to reuse it */
	memset(&subscribe_params, 0, sizeof(subscribe_params));

	return 0;
}

/* Custom Service Variables */
static struct bt_uuid_128 vnd_uuid = BT_UUID_INIT_128(
	0xf0, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12,
	0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12);
static struct bt_uuid_128 vnd_auth_uuid = BT_UUID_INIT_128(
	0xf2, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12,
	0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12);
static const struct bt_uuid_128 vnd_long_uuid1 = BT_UUID_INIT_128(
	0xf3, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12,
	0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12);
static const struct bt_uuid_128 vnd_long_uuid2 = BT_UUID_INIT_128(
	0xde, 0xad, 0xfa, 0xce, 0x78, 0x56, 0x34, 0x12,
	0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12);

static uint8_t vnd_value[] = { 'V', 'e', 'n', 'd', 'o', 'r' };

static ssize_t read_vnd(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			void *buf, uint16_t len, uint16_t offset)
{
	const char *value = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 strlen(value));
}

static ssize_t write_vnd(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 const void *buf, uint16_t len, uint16_t offset,
			 uint8_t flags)
{
	uint8_t *value = attr->user_data;

	if (offset + len > sizeof(vnd_value)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	memcpy(value + offset, buf, len);

	return len;
}

#define MAX_DATA 30
static uint8_t vnd_long_value1[MAX_DATA] = { 'V', 'e', 'n', 'd', 'o', 'r' };
static uint8_t vnd_long_value2[MAX_DATA] = { 'S', 't', 'r', 'i', 'n', 'g' };

static ssize_t read_long_vnd(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr, void *buf,
			     uint16_t len, uint16_t offset)
{
	uint8_t *value = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 sizeof(vnd_long_value1));
}

static ssize_t write_long_vnd(struct bt_conn *conn,
			      const struct bt_gatt_attr *attr, const void *buf,
			      uint16_t len, uint16_t offset, uint8_t flags)
{
	uint8_t *value = attr->user_data;

	if (flags & BT_GATT_WRITE_FLAG_PREPARE) {
		return 0;
	}

	if (offset + len > sizeof(vnd_long_value1)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	/* Copy to buffer */
	memcpy(value + offset, buf, len);

	return len;
}

static struct bt_gatt_attr vnd_attrs[] = {
	/* Vendor Primary Service Declaration */
	BT_GATT_PRIMARY_SERVICE(&vnd_uuid),

	BT_GATT_CHARACTERISTIC(&vnd_auth_uuid.uuid,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE),
	BT_GATT_DESCRIPTOR(&vnd_auth_uuid.uuid,
			   BT_GATT_PERM_READ_AUTHEN |
			   BT_GATT_PERM_WRITE_AUTHEN,
			   read_vnd, write_vnd, vnd_value),

	BT_GATT_CHARACTERISTIC(&vnd_long_uuid1.uuid, BT_GATT_CHRC_READ |
			       BT_GATT_CHRC_WRITE | BT_GATT_CHRC_EXT_PROP),
	BT_GATT_DESCRIPTOR(&vnd_long_uuid1.uuid,
				BT_GATT_PERM_READ | BT_GATT_PERM_WRITE |
				BT_GATT_PERM_PREPARE_WRITE,
				read_long_vnd, write_long_vnd,
				&vnd_long_value1),

	BT_GATT_CHARACTERISTIC(&vnd_long_uuid2.uuid, BT_GATT_CHRC_READ |
			       BT_GATT_CHRC_WRITE | BT_GATT_CHRC_EXT_PROP),
	BT_GATT_DESCRIPTOR(&vnd_long_uuid2.uuid,
				BT_GATT_PERM_READ | BT_GATT_PERM_WRITE |
				BT_GATT_PERM_PREPARE_WRITE,
				read_long_vnd, write_long_vnd,
				&vnd_long_value2),
};

static int cmd_gatt_register_test_svc(int argc, char *argv[])
{
	bt_gatt_register(vnd_attrs, ARRAY_SIZE(vnd_attrs));

	printk("Registering test vendor service\n");

	return 0;
}

static bool hrs_simulate;

static int cmd_hrs_simulate(int argc, char *argv[])
{
	if (!strcmp(argv[1], "on")) {
		static bool hrs_registered;

		if (!hrs_registered) {
			printk("Register HRS Serice\n");
			hrs_init(0x01);
			hrs_registered = true;
		}

		printk("Start HRS simulation\n");
		hrs_simulate = true;
	} else if (!strcmp(argv[1], "off")) {
		printk("Stop HRS simulation\n");
		hrs_simulate = false;
	} else {
		printk("Incorrect value: %s\n", argv[1]);
		return -EINVAL;
	}

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
	uint8_t max = 16;

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

static int str2bt_addr(const char *str, bt_addr_t *addr)
{
	int i, j;
	uint8_t tmp;

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

static void br_device_found(const bt_addr_t *addr, int8_t rssi,
				  const uint8_t cod[3], const uint8_t eir[240])
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
static void l2cap_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	int ret;

	printk("Incoming data channel %p len %u\n", chan, buf->len);

	/* loopback the data */
	ret = bt_l2cap_chan_send(chan, buf);
	if (ret < 0) {
		printk("Unable to send: %d\n", -ret);
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
	printk("Channel %p requires buffer\n", chan);

	return net_buf_alloc(&data_pool, K_FOREVER);
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
	uint16_t psm;
	int err;

	if (!default_conn) {
		printk("Not connected\n");
		return 0;
	}

	if (argc < 2) {
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
	static uint8_t buf_data[DATA_MTU] = { [0 ... (DATA_MTU - 1)] = 0xff };
	int ret, len, count = 1;
	struct net_buf *buf;

	if (argc > 1) {
		count = strtoul(argv[1], NULL, 10);
	}

	len = min(l2cap_chan.tx.mtu, DATA_MTU - BT_L2CAP_CHAN_SEND_RESERVE);

	while (count--) {
		buf = net_buf_alloc(&data_pool, K_FOREVER);
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
	uint8_t channel;
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
	uint8_t buf_data[DATA_BREDR_MTU] = { [0 ... (DATA_BREDR_MTU - 1)] =
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
#endif

#define HELP_NONE "[none]"
#define HELP_ADDR_LE "<address: XX:XX:XX:XX:XX:XX> <type: (public|random)>"

static const struct shell_cmd commands[] = {
	{ "init", cmd_init, HELP_ADDR_LE },
	{ "connect", cmd_connect_le, HELP_ADDR_LE },
	{ "disconnect", cmd_disconnect, HELP_NONE },
	{ "auto-conn", cmd_auto_conn, HELP_ADDR_LE },
	{ "select", cmd_select, HELP_ADDR_LE },
	{ "scan", cmd_scan, "<value: on, off>" },
	{ "advertise", cmd_advertise,
	"<type: off, on, scan, nconn> <mode: discov, non_discov>"  },
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
	  "<handle> <offset> <data>" },
	{ "gatt-write-signed", cmd_gatt_write_signed,
	  "<handle> <offset> <data>" },
	{ "gatt-subscribe", cmd_gatt_subscribe, "<CCC handle> <value handle>" },
	{ "gatt-unsubscribe", cmd_gatt_unsubscribe, HELP_NONE },
	{ "gatt-register-service", cmd_gatt_register_test_svc,
	  "register pre-predefined test service" },
	{ "hrs-simulate", cmd_hrs_simulate,
	  "register and simulate Heart Rate Service <value: on, off>" },
#if defined(CONFIG_BLUETOOTH_L2CAP_DYNAMIC_CHANNEL)
	{ "l2cap-register", cmd_l2cap_register, "<psm> [sec_level]" },
	{ "l2cap-connect", cmd_l2cap_connect, "<psm>" },
	{ "l2cap-disconnect", cmd_l2cap_disconnect, HELP_NONE },
	{ "l2cap-send", cmd_l2cap_send, "<number of packets>" },
#endif
#if defined(CONFIG_BLUETOOTH_BREDR)
	{ "br-iscan", cmd_bredr_discoverable, "<value: on, off>" },
	{ "br-pscan", cmd_bredr_connectable, "value: on, off" },
	{ "br-connect", cmd_connect_bredr, "<address>" },
	{ "br-discovery", cmd_bredr_discovery,
	  "<value: on, off> [length: 1-48] [mode: limited]"  },
	{ "br-l2cap-register", cmd_bredr_l2cap_register, "<psm>" },
	{ "br-oob", cmd_bredr_oob },
#if defined(CONFIG_BLUETOOTH_RFCOMM)
	{ "br-rfcomm-register", cmd_bredr_rfcomm_register },
	{ "br-rfcomm-connect", cmd_rfcomm_connect, "<channel>" },
	{ "br-rfcomm-send", cmd_rfcomm_send, "<number of packets>"},
	{ "br-rfcomm-disconnect", cmd_rfcomm_disconnect, HELP_NONE },
#endif /* CONFIG_BLUETOOTH_RFCOMM */
#endif
	{ NULL, NULL }
};

void main(void)
{
	bt_conn_cb_register(&conn_callbacks);

	printk("Type \"help\" for supported commands.\n");
	printk("Before any Bluetooth commands you must run \"init\".\n");

	SHELL_REGISTER(MY_SHELL_MODULE, commands);
	shell_register_prompt_handler(current_prompt);
	shell_register_default_module(MY_SHELL_MODULE);

	while (1) {
		k_sleep(MSEC_PER_SEC);

		/* Heartrate measurements simulation */
		if (hrs_simulate) {
			hrs_notify();
		}
	}
}
