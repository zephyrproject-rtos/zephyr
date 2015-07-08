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
#include <string.h>
#include <misc/printk.h>

#include <console/uart_console.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/gatt.h>

#include "btshell.h"

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
}

static void disconnected(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Disconnected: %s\n", addr);
}

static struct bt_conn_cb conn_callbacks = {
		.connected = connected,
		.disconnected = disconnected,
};

static void cmd_init(int argc, char *argv[])
{
	int err;

	err = bt_init();
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
	} else {
		printk("Bluetooth initialized\n");
	}
}

static int xtoi(const char *str)
{
	int val = 0;

	for (; *str != '\0'; str++) {
		val = val << 4;
		if (*str >= '0' && *str <= '9') {
			val |= *str - '0';
		} else if (*str >= 'a' && *str <= 'f') {
			val |= *str - 'a' + 10;
		} else if (*str >= 'A' && *str <= 'F') {
			val |= *str - 'A' + 10;
		} else {
			return -EINVAL;
		}
	}

	return val;
}

static int str2bt_addr_le(const char *str, const char *type, bt_addr_le_t *addr)
{
	int i, j;

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

		if (*str >= '0' && *str <= '9') {
			addr->val[i] |= *str - '0';
		} else if (*str >= 'a' && *str <= 'f') {
			addr->val[i] |= *str - 'a' + 10;
		} else if (*str >= 'A' && *str <= 'F') {
			addr->val[i] |= *str - 'A' + 10;
		} else {
			return -EINVAL;
		}
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

	conn = bt_connect_le(&addr);

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

	conn = bt_conn_lookup_addr_le(&addr);
	if (!conn) {
		printk("Peer not connected\n");
		return;
	}

	err = bt_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	if (err) {
		printk("Disconnection failed (err %d)\n", err);
	}

	bt_conn_put(conn);
}

static void cmd_active_scan_on(void)
{
	int err;

	err = bt_start_scanning(BT_LE_SCAN_FILTER_DUP_ENABLE, device_found);
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
	if (!strncmp(action, "on", strlen("on"))) {
		cmd_active_scan_on();
	} else if (!strncmp(action, "off", strlen("off"))) {
		cmd_scan_off();
	} else {
		printk("Scan [on/off] parameter required\n");
	}
}

static void cmd_security(int argc, char *argv[])
{
	int err, sec;
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

	if (argc < 4) {
		printk("Security level required\n");
		return;
	}

	err = str2bt_addr_le(argv[1], argv[2], &addr);
	if (err) {
		printk("Invalid peer address (err %d)\n", err);
		return;
	}

	conn = bt_conn_lookup_addr_le(&addr);
	if (!conn) {
		printk("Peer not connected\n");
		return;
	}

	sec = *argv[3] - '0';

	err = bt_security(conn, sec);
	if (err) {
		printk("Setting security failed (err %d)\n", err);
	}

	bt_conn_put(conn);
}

static void exchange_rsp(struct bt_conn *conn, uint8_t err)
{
	printk("Exchange %s\n", err == 0 ? "successful" : "failed");
}

static void cmd_gatt_exchange_mtu(int argc, char *argv[])
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

	conn = bt_conn_lookup_addr_le(&addr);
	if (!conn) {
		printk("Peer not connected\n");
		return;
	}

	err = bt_gatt_exchange_mtu(conn, exchange_rsp);
	if (err) {
		printk("Exchange failed (err %d)\n", err);
	} else {
		printk("Exchange pending\n");
	}

	bt_conn_put(conn);
}

static const struct bt_eir ad[] = {
	{
		.len = 2,
		.type = BT_EIR_FLAGS,
		.data = { BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR },
	},
	{ }
};

static const struct bt_eir sd[] = {
	{
		.len = sizeof("test shell"),
		.type = BT_EIR_NAME_COMPLETE,
		.data = "test shell",
	},
	{ }
};

static void cmd_advertise(int argc, char *argv[])
{
	const char *action;

	if (argc < 2) {
		printk("Advertise [on/off] parameter required\n");
		return;
	}

	action = (char*)argv[1];
	if (!strncmp(action, "on", strlen("on"))) {
		if (bt_start_advertising(BT_LE_ADV_IND, ad, sd) < 0) {
			printk("Failed to start advertising\n");
		} else {
			printk("Advertising started\n");
		}
	} else if (!strncmp(action, "off", strlen("off"))) {
		if (bt_stop_advertising() < 0) {
			printk("Failed to stop advertising\n");
		} else {
			printk("Advertising stopped\n");
		}
	} else {
		printk("Advertise [on/off] parameter required\n");
	}
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

	conn = bt_conn_lookup_addr_le(&addr);
	if (!conn) {
		printk("Peer not connected\n");
		return;
	}

	discover_params.func = discover_func;
	discover_params.destroy = discover_destroy;
	discover_params.start_handle = 0x0001;
	discover_params.end_handle = 0xffff;

	if (argc < 4) {
		if (!strcmp(argv[0], "gatt-discover")) {
			printk("UUID type required\n");
			return;
		}
		goto done;
	}

	/* Only set the UUID if the value is valid (non zero) */
	uuid.u16 = xtoi(argv[3]);
	if (uuid.u16) {
		uuid.type = BT_UUID_16;
		discover_params.uuid = &uuid;
	}

	if (argc > 4) {
		discover_params.start_handle = xtoi(argv[4]);
		if (argc > 5) {
			discover_params.end_handle = xtoi(argv[5]);
		}
	}

done:
	if (!strcmp(argv[0], "gatt-discover-characteristic")) {
		err = bt_gatt_discover_characteristic(conn, &discover_params);
	} else if (!strcmp(argv[0], "gatt-discover-descriptor")) {
		err = bt_gatt_discover_descriptor(conn, &discover_params);
	} else {
		err = bt_gatt_discover(conn, &discover_params);
	}

	if (err) {
		printk("Discover failed (err %d)\n", err);
	} else {
		printk("Discover pending\n");
	}

	bt_conn_put(conn);
}

#ifdef CONFIG_MICROKERNEL
void mainloop(void)
#else
void main(void)
#endif
{
	shell_init("btshell> ");
	bt_conn_cb_register(&conn_callbacks);

	shell_cmd_register("init", cmd_init);
	shell_cmd_register("connect", cmd_connect_le);
	shell_cmd_register("disconnect", cmd_disconnect);
	shell_cmd_register("scan", cmd_scan);
	shell_cmd_register("advertise", cmd_advertise);
	shell_cmd_register("security", cmd_security);
	shell_cmd_register("gatt-exchange-mtu", cmd_gatt_exchange_mtu);
	shell_cmd_register("gatt-discover", cmd_gatt_discover);
	shell_cmd_register("gatt-discover-characteristic", cmd_gatt_discover);
	shell_cmd_register("gatt-discover-descriptor", cmd_gatt_discover);
}
