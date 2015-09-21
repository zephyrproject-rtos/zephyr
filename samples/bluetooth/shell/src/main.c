/** @file
 *  @brief Interactive Bluetooth LE shell application
 *
 *  Application allows implement Bluetooth LE functional commands performing
 *  simple diagnostic interaction between LE host stack and LE controller
 */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Intel Corporation nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <errno.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <misc/printk.h>
#include <misc/byteorder.h>

#include <console/uart_console.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/gatt.h>

#include "btshell.h"

#define DEVICE_NAME "test shell"

static struct bt_conn *default_conn = NULL;

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t evtype,
			 const uint8_t *ad, uint8_t len)
{
	char le_addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(addr, le_addr, sizeof(le_addr));
	printk("[DEVICE]: %s, AD evt type %u, AD data len %u, RSSI %i\n",
		le_addr, evtype, len, rssi);
}

static void connected(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Connected: %s\n", addr);

	if (!default_conn) {
		default_conn = bt_conn_get(conn);
	}
}

static void disconnected(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Disconnected: %s\n", addr);

	if (default_conn == conn) {
		bt_conn_put(default_conn);
		default_conn = NULL;
	}
}

static void identity_resolved(struct bt_conn *conn, const bt_addr_le_t *rpa,
			      const bt_addr_le_t *identity)
{
	char addr_identity[BT_ADDR_LE_STR_LEN];
	char addr_rpa[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(identity, addr_identity, sizeof(addr_identity));
	bt_addr_le_to_str(rpa, addr_rpa, sizeof(addr_rpa));

	printk("Identity resolved %s -> %s\n", addr_rpa, addr_identity);
}

static void security_changed(struct bt_conn *conn, bt_security_t level)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Security changed: %s level %u\n", addr, level);
}

static struct bt_conn_cb conn_callbacks = {
		.connected = connected,
		.disconnected = disconnected,
		.identity_resolved = identity_resolved,
		.security_changed = security_changed,
};

static void cmd_init(int argc, char *argv[])
{
	int err;

	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
	} else {
		printk("Bluetooth initialized\n");
	}
}

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

static int xtoi(const char *str)
{
	int val = 0;
	uint8_t tmp;

	for (; *str != '\0'; str++) {
		val = val << 4;
		if (char2hex(str, &tmp) < 0) {
			return -EINVAL;
		}

		val |= tmp;
	}

	return val;
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

		addr->val[i] = addr->val[i] << 4;

		if (char2hex(str, &tmp) < 0) {
			return -EINVAL;
		}

		addr->val[i] |= tmp;
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

static void cmd_connect_le(int argc, char *argv[])
{
	int err;
	bt_addr_le_t addr;
	struct bt_conn *conn;

	if (argc < 2) {
		printk("Peer address required\n");
		return;
	}

	if (argc < 3) {
		printk("Peer address type required\n");
		return;
	}

	err = str2bt_addr_le(argv[1], argv[2], &addr);
	if (err) {
		printk("Invalid peer address (err %d)\n", err);
		return;
	}

	conn = bt_conn_create_le(&addr);

	if (!conn) {
		printk("Connection failed\n");
	} else {

		printk("Connection pending\n");

		/* unref connection obj in advance as app user */
		bt_conn_put(conn);
	}
}

static void cmd_disconnect(int argc, char *argv[])
{
	struct bt_conn *conn;
	int err;

	if (default_conn) {
		conn = default_conn;
	} else {
		bt_addr_le_t addr;

		if (argc < 2) {
			printk("Peer address required\n");
			return;
		}

		if (argc < 3) {
			printk("Peer address type required\n");
			return;
		}

		err = str2bt_addr_le(argv[1], argv[2], &addr);
		if (err) {
			printk("Invalid peer address (err %d)\n", err);
			return;
		}

		conn = bt_conn_lookup_addr_le(&addr);
	}

	if (!conn) {
		printk("Not connected\n");
		return;
	}

	err = bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	if (err) {
		printk("Disconnection failed (err %d)\n", err);
	}
}

static void cmd_active_scan_on(void)
{
	int err;

	err = bt_start_scanning(BT_SCAN_FILTER_DUP_ENABLE, device_found);
	if (err) {
		printk("Bluetooth set active scan failed (err %d)\n", err);
	} else {
		printk("Bluetooth active scan enabled\n");
	}
}

static void cmd_scan_off(void)
{
	int err;

	err = bt_stop_scanning();
	if (err) {
		printk("Stopping scanning failed (err %d)\n", err);
	} else {
		printk("Scan successfully stopped\n");
	}
}

static void cmd_scan(int argc, char *argv[])
{
	const char *action;

	if (argc < 2) {
		printk("Scan [on/off] parameter required\n");
		return;
	}

	action = (char*)argv[1];
	if (!strcmp(action, "on")) {
		cmd_active_scan_on();
	} else if (!strcmp(action, "off")) {
		cmd_scan_off();
	} else {
		printk("Scan [on/off] parameter required\n");
	}
}

static void cmd_security(int argc, char *argv[])
{
	int err, sec;

	if (!default_conn) {
		printk("Not connected\n");
		return;
	}

	if (argc < 2) {
		printk("Security level required\n");
		return;
	}

	sec = *argv[1] - '0';

	err = bt_conn_security(default_conn, sec);
	if (err) {
		printk("Setting security failed (err %d)\n", err);
	}
}

static void exchange_rsp(struct bt_conn *conn, uint8_t err)
{
	printk("Exchange %s\n", err == 0 ? "successful" : "failed");
}

static void cmd_gatt_exchange_mtu(int argc, char *argv[])
{
	int err;

	if (!default_conn) {
		printk("Not connected\n");
		return;
	}

	err = bt_gatt_exchange_mtu(default_conn, exchange_rsp);
	if (err) {
		printk("Exchange failed (err %d)\n", err);
	} else {
		printk("Exchange pending\n");
	}
}

static const struct bt_eir ad_discov[] = {
	{
		.len = 2,
		.type = BT_EIR_FLAGS,
		.data = { BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR },
	},
	{ }
};

static const struct bt_eir ad_non_discov[] = { { 0 } };

static const struct bt_eir sd[] = {
	{
		.len = sizeof(DEVICE_NAME),
		.type = BT_EIR_NAME_COMPLETE,
		.data = DEVICE_NAME,
	},
	{ }
};

static void cmd_advertise(int argc, char *argv[])
{
	uint8_t adv_type;
	int i;
	const struct bt_eir *ad;

	static struct {
		const char *str;
		uint8_t type;
	} adv_t[] = {
		{ "on", BT_LE_ADV_IND },
		{ "direct", BT_LE_ADV_DIRECT_IND },
		{ "scan", BT_LE_ADV_SCAN_IND },
		{ "nconn", BT_LE_ADV_NONCONN_IND },
	};

	adv_type = adv_t[0].type;
	if (argc >= 2) {
		const char *adv_type_str = (char*)argv[1];

		if (!strcmp(adv_type_str, "off")) {
			if (bt_stop_advertising() < 0) {
				printk("Failed to stop advertising\n");
			} else {
				printk("Advertising stopped\n");
			}

			return;
		}

		for (i = 0; i < ARRAY_SIZE(adv_t); i++) {
			if (!strcmp(adv_type_str, adv_t[i].str)) {
				adv_type = adv_t[i].type;
				break;
			}
		}

		if (i == ARRAY_SIZE(adv_t)) {
			goto fail;
		}
	} else {
		goto fail;
	}

	/* Parse advertisement data */
	if (argc >= 3) {
		const char *mode = (char*)argv[2];

		if (!strcmp(mode, "discov")) {
			ad = ad_discov;
		} else if (!strcmp(mode, "non_discov")) {
			ad = ad_non_discov;
		} else {
			goto fail;
		}
	} else {
		ad = ad_discov;
	}

	if (bt_start_advertising(adv_type, ad, sd) < 0) {
		printk("Failed to start advertising\n");
	} else {
		printk("Advertising started\n");
	}

	return;

fail:
	printk("Usage: advertise <type> <ad mode>\n");
	printk("type: off, ");
	for (i = 0; i < sizeof(adv_t) / sizeof(adv_t[0]); i++) {
		printk("%s, ", adv_t[i].str);
	}
	printk("\n");
	printk("ad mode: discov, non_discov\n");
}

static struct bt_gatt_discover_params discover_params;
static struct bt_uuid uuid;

static uint8_t discover_func(const struct bt_gatt_attr *attr, void *user_data)
{
	printk("Discover found handle %u\n", attr->handle);

	return BT_GATT_ITER_CONTINUE;
}

static void discover_destroy(void *user_data)
{
	struct bt_gatt_discover_params *params = user_data;

	printk("Discover destroy\n");

	memset(params, 0, sizeof(*params));
}

static void cmd_gatt_discover(int argc, char *argv[])
{
	int err;

	if (!default_conn) {
		printk("Not connected\n");
		return;
	}

	discover_params.func = discover_func;
	discover_params.destroy = discover_destroy;
	discover_params.start_handle = 0x0001;
	discover_params.end_handle = 0xffff;

	if (argc < 2) {
		if (!strcmp(argv[0], "gatt-discover")) {
			printk("UUID type required\n");
			return;
		}
		goto done;
	}

	/* Only set the UUID if the value is valid (non zero) */
	uuid.u16 = xtoi(argv[1]);
	if (uuid.u16) {
		uuid.type = BT_UUID_16;
		discover_params.uuid = &uuid;
	}

	if (argc > 2) {
		discover_params.start_handle = xtoi(argv[2]);
		if (argc > 3) {
			discover_params.end_handle = xtoi(argv[3]);
		}
	}

done:
	if (!strcmp(argv[0], "gatt-discover-characteristic")) {
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
}

static void read_func(struct bt_conn *conn, int err, const void *data,
		      uint16_t length)
{
	printk("Read complete: err %u length %u\n", err, length);
}

static void cmd_gatt_read(int argc, char *argv[])
{
	int err;
	uint16_t handle, offset = 0;

	if (!default_conn) {
		printk("Not connected\n");
		return;
	}

	if (argc < 2) {
		printk("handle required\n");
		return;
	}

	handle = xtoi(argv[1]);

	if (argc > 2) {
		offset = xtoi(argv[2]);
	}

	err = bt_gatt_read(default_conn, handle, offset, read_func);
	if (err) {
		printk("Read failed (err %d)\n", err);
	} else {
		printk("Read pending\n");
	}
}

void cmd_gatt_mread(int argc, char *argv[])
{
	uint16_t h[8];
	int i, err;

	if (!default_conn) {
		printk("Not connected\n");
		return;
	}

	if (argc < 2) {
		printk("Attribute handles in hex format to read required\n");
		return;
	}

	if (argc - 1 >  ARRAY_SIZE(h)) {
		printk("Enter max %u handle items to read\n", ARRAY_SIZE(h));
		return;
	}

	for (i = 0; i < argc - 1; i++) {
		h[i] = xtoi(argv[i + 1]);
	}

	err = bt_gatt_read_multiple(default_conn, h, i, read_func);
	if (err) {
		printk("GATT multiple read request failed (err %d)\n", err);
	}
}

static void write_func(struct bt_conn *conn, uint8_t err)
{
	printk("Write complete: err %u\n", err);
}

static void cmd_gatt_write(int argc, char *argv[])
{
	int err;
	uint16_t handle, offset;
	uint8_t data;

	if (!default_conn) {
		printk("Not connected\n");
		return;
	}

	if (argc < 2) {
		printk("handle required\n");
		return;
	}

	handle = xtoi(argv[1]);

	if (argc < 3) {
		printk("offset required\n");
		return;
	}

	/* TODO: Add support for longer data */
	offset = xtoi(argv[2]);

	if (argc < 4) {
		printk("data required\n");
		return;
	}

	/* TODO: Add support for longer data */
	data = xtoi(argv[3]);

	err = bt_gatt_write(default_conn, handle, offset, &data, sizeof(data),
			    write_func);
	if (err) {
		printk("Write failed (err %d)\n", err);
	} else {
		printk("Write pending\n");
	}
}

static void cmd_gatt_write_without_rsp(int argc, char *argv[])
{
	int err;
	uint16_t handle;
	uint8_t data;

	if (!default_conn) {
		printk("Not connected\n");
		return;
	}

	if (argc < 2) {
		printk("handle required\n");
		return;
	}

	handle = xtoi(argv[1]);

	if (argc < 3) {
		printk("data required\n");
		return;
	}

	data = xtoi(argv[2]);

	err = bt_gatt_write_without_response(default_conn, handle, &data,
					     sizeof(data), false);
	printk("Write Complete (err %d)\n", err);
}

static void cmd_gatt_write_signed(int argc, char *argv[])
{
	int err;
	uint16_t handle;
	uint8_t data;

	if (!default_conn) {
		printk("Not connected\n");
		return;
	}

	if (argc < 2) {
		printk("handle required\n");
		return;
	}

	handle = xtoi(argv[1]);

	if (argc < 3) {
		printk("data required\n");
		return;
	}

	data = xtoi(argv[2]);

	err = bt_gatt_write_without_response(default_conn, handle, &data,
					     sizeof(data), true);
	printk("Write Complete (err %d)\n", err);
}

static struct bt_gatt_subscribe_params subscribe_params;

static void subscribe_destroy(void *user_data)
{
	struct bt_gatt_subscribe_params *params = user_data;

	printk("Subscribe destroy\n");
	params->value_handle = 0;
}

static void subscribe_func(struct bt_conn *conn, int err,
			   const void *data, uint16_t length)
{
	if (!length) {
		printk("Subscribe complete: err %u\n", err);
	} else {
		printk("Notification: data %p length %u\n", data, length);
	}
}

static void cmd_gatt_subscribe(int argc, char *argv[])
{
	int err;
	uint16_t handle;

	if (subscribe_params.value_handle) {
		printk("Cannot subscribe: subscription to %u already exists\n",
		       subscribe_params.value_handle);
		return;
	}

	if (!default_conn) {
		printk("Not connected\n");
		return;
	}

	if (argc < 2) {
		printk("handle required\n");
		return;
	}

	handle = xtoi(argv[1]);

	if (argc < 3) {
		printk("value handle required\n");
		return;
	}

	subscribe_params.value_handle = xtoi(argv[2]);
	subscribe_params.value = BT_GATT_CCC_NOTIFY;
	subscribe_params.func = subscribe_func;
	subscribe_params.destroy = subscribe_destroy;

	if (argc > 3) {
		subscribe_params.value = xtoi(argv[3]);
	}

	err = bt_gatt_subscribe(default_conn, handle, &subscribe_params);
	if (err) {
		printk("Subscribe failed (err %d)\n", err);
	} else {
		printk("Subscribe pending\n");
	}
}

static void cmd_gatt_unsubscribe(int argc, char *argv[])
{
	int err;
	uint16_t handle;

	if (!default_conn) {
		printk("Not connected\n");
		return;
	}

	if (argc < 2) {
		printk("handle required\n");
		return;
	}

	handle = xtoi(argv[1]);

	if (!subscribe_params.value_handle) {
		printk("No subscription found\n");
		return;
	}

	err = bt_gatt_unsubscribe(default_conn, handle, &subscribe_params);
	if (err) {
		printk("Unsubscribe failed (err %d)\n", err);
	} else {
		printk("Unsubscribe success\n");
	}
}

static int read_string(struct bt_conn *conn, const struct bt_gatt_attr *attr,
		       void *buf, uint16_t len, uint16_t offset)
{
	const char *str = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, str,
				 strlen(str));
}

static uint16_t appearance_value = 0x0001;

static int read_appearance(struct bt_conn *conn,
			   const struct bt_gatt_attr *attr, void *buf,
			   uint16_t len, uint16_t offset)
{
	uint16_t appearance = sys_cpu_to_le16(appearance_value);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &appearance,
				 sizeof(appearance));
}

/* GAP SERVICE (0x1800) */
static struct bt_uuid gap_uuid = {
	.type = BT_UUID_16,
	.u16 = BT_UUID_GAP,
};

static struct bt_uuid device_name_uuid = {
	.type = BT_UUID_16,
	.u16 = BT_UUID_GAP_DEVICE_NAME,
};

static struct bt_gatt_chrc name_chrc = {
	.properties = BT_GATT_CHRC_READ,
	.value_handle = 0x0003,
	.uuid = &device_name_uuid,
};

static struct bt_uuid appeareance_uuid = {
	.type = BT_UUID_16,
	.u16 = BT_UUID_GAP_APPEARANCE,
};

static struct bt_gatt_chrc appearance_chrc = {
	.properties = BT_GATT_CHRC_READ,
	.value_handle = 0x0005,
	.uuid = &appeareance_uuid,
};

static struct bt_gatt_attr attrs[] = {
	BT_GATT_PRIMARY_SERVICE(0x0001, &gap_uuid),
	BT_GATT_CHARACTERISTIC(0x0002, &name_chrc),
	BT_GATT_DESCRIPTOR(0x0003, &device_name_uuid, BT_GATT_PERM_READ,
			   read_string, NULL, DEVICE_NAME),
	BT_GATT_CHARACTERISTIC(0x0004, &appearance_chrc),
	BT_GATT_DESCRIPTOR(0x0005, &appeareance_uuid, BT_GATT_PERM_READ,
			   read_appearance, NULL, NULL),
};

static void auth_passkey_display(struct bt_conn *conn, unsigned int passkey)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Passkey for %s: %u\n", addr, passkey);
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

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Pairing cancelled: %s\n", addr);
}

static struct bt_auth_cb auth_cb_display = {
	.passkey_display = auth_passkey_display,
	.passkey_entry = NULL,
	.cancel = auth_cancel,
};

static struct bt_auth_cb auth_cb_input = {
	.passkey_display = NULL,
	.passkey_entry = auth_passkey_entry,
	.cancel = auth_cancel,
};

static struct bt_auth_cb auth_cb_all = {
	.passkey_display = auth_passkey_display,
	.passkey_entry = auth_passkey_entry,
	.cancel = auth_cancel,
};

static void cmd_auth(int argc, char *argv[])
{
	if (argc < 2) {
		printk("auth [display, input, all, none] parameter required\n");
		return;
	}

	if (!strcmp(argv[1], "all")) {
		bt_auth_cb_register(&auth_cb_all);
	} else if (!strcmp(argv[1], "input")) {
		bt_auth_cb_register(&auth_cb_input);
	} else if (!strcmp(argv[1], "display")) {
		bt_auth_cb_register(&auth_cb_display);
	} else if (!strcmp(argv[1], "none")) {
		bt_auth_cb_register(NULL);
	} else {
		printk("auth [display, input, all, none] parameter required\n");
	}
}

static void cmd_auth_cancel(int argc, char *argv[])
{
	if (!default_conn) {
		printk("Not connected\n");
		return;
	}

	bt_auth_cancel(default_conn);
}

static void cmd_auth_passkey(int argc, char *argv[])
{
	unsigned int passkey;

	if (!default_conn) {
		printk("Not connected\n");
		return;
	}

	if (argc < 2) {
		printk("passkey required\n");
		return;
	}

	passkey = atoi(argv[1]);
	if (passkey > 999999) {
		printk("Passkey should be between 0-999999\n");
		return;
	}

	bt_auth_passkey_entry(default_conn, passkey);
}

struct shell_cmd commands[] = {
	{ "init", cmd_init },
	{ "connect", cmd_connect_le },
	{ "disconnect", cmd_disconnect },
	{ "scan", cmd_scan },
	{ "advertise", cmd_advertise },
	{ "security", cmd_security },
	{ "auth", cmd_auth },
	{ "auth-cancel", cmd_auth_cancel },
	{ "auth-passkey", cmd_auth_passkey },
	{ "gatt-exchange-mtu", cmd_gatt_exchange_mtu },
	{ "gatt-discover", cmd_gatt_discover },
	{ "gatt-discover-characteristic", cmd_gatt_discover },
	{ "gatt-discover-descriptor", cmd_gatt_discover },
	{ "gatt-read", cmd_gatt_read },
	{ "gatt-read-multiple", cmd_gatt_mread },
	{ "gatt-write", cmd_gatt_write },
	{ "gatt-write-without-response", cmd_gatt_write_without_rsp },
	{ "gatt-write-signed", cmd_gatt_write_signed },
	{ "gatt-subscribe", cmd_gatt_subscribe },
	{ "gatt-unsubscribe", cmd_gatt_unsubscribe },
	{ NULL, NULL }
};

#ifdef CONFIG_MICROKERNEL
void mainloop(void)
#else
void main(void)
#endif
{
	bt_conn_cb_register(&conn_callbacks);

	bt_gatt_register(attrs, ARRAY_SIZE(attrs));

	shell_init("btshell> ", commands);
}
