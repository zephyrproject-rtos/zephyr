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
#include <bluetooth/conn.h>
#include <bluetooth/gatt.h>

#include "btshell.h"

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
	int err;

	if (!default_conn) {
		printk("Not connected\n");
		return;
	}

	err = bt_conn_disconnect(default_conn,
				 BT_HCI_ERR_REMOTE_USER_TERM_CONN);
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
		err = bt_gatt_discover_characteristic(default_conn,
						      &discover_params);
	} else if (!strcmp(argv[0], "gatt-discover-descriptor")) {
		err = bt_gatt_discover_descriptor(default_conn,
						  &discover_params);
	} else {
		err = bt_gatt_discover(default_conn, &discover_params);
	}

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

	/* TODO: Add support for longer data */
	data = xtoi(argv[2]);

	err = bt_gatt_write(default_conn, handle, &data, sizeof(data),
			    write_func);
	if (err) {
		printk("Write failed (err %d)\n", err);
	} else {
		printk("Write pending\n");
	}
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
	subscribe_params.func = subscribe_func;
	subscribe_params.destroy = subscribe_destroy;

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
	shell_cmd_register("gatt-read", cmd_gatt_read);
	shell_cmd_register("gatt-read-multiple", cmd_gatt_mread);
	shell_cmd_register("gatt-write", cmd_gatt_write);
	shell_cmd_register("gatt-subscribe", cmd_gatt_subscribe);
	shell_cmd_register("gatt-unsubscribe", cmd_gatt_unsubscribe);
}
