/*! @file
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

	if (strcmp(type, "public") == 0) {
		addr->type = BT_ADDR_LE_PUBLIC;
	} else if (strcmp(type, "random") == 0) {
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

	err = bt_connect_le(&addr);
	if (err) {
		printk("Connection failed (err %d)\n", err);
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
	shell_cmd_register("security", cmd_security);
}
