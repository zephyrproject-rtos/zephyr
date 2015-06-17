/* main.c - Application main entry point */

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

static struct bt_conn *conns[CONFIG_BLUETOOTH_MAX_CONN];

static void connected(struct bt_conn *conn)
{
	int i;

	for (i = 0; i < CONFIG_BLUETOOTH_MAX_CONN + 1; i++) {
		if (conns[i] == NULL) {
			conns[i] = bt_conn_get(conn);;
			printk("Connected %p, id %d\n", conn, i);
			return;
		}
	}
}

static void disconnected(struct bt_conn *conn)
{
	int i;

	for (i = 0; i < CONFIG_BLUETOOTH_MAX_CONN + 1; i++) {
		if (conns[i] == conn) {
			bt_conn_put(conn);
			conns[i] = NULL;
			printk("Disconnected %p\n", conn);
			return;
		}
	}
}

static struct bt_conn_cb conn_callbacks = {
		.connected = connected,
		.disconnected = disconnected,
};

static int cmd_init(int argc, char *argv[])
{
	int err;

	err = bt_init();
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
	} else {
		printk("Bluetooth initialized\n");
	}

	return 0;
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

static int cmd_connect_le(int argc, char *argv[])
{
	int err;
	bt_addr_le_t addr;

	if (argc < 2) {
		printk("Peer address required\n");
		return -EINVAL;
	}

	if (argc < 3) {
		printk("Peer address type required\n");
		return -EINVAL;
	}

	err = str2bt_addr_le(argv[1], argv[2], &addr);
	if (err) {
		printk("Invalid peer address (err %d)\n", err);
		return err;
	}

	err = bt_connect_le(&addr);
	if (err) {
		printk("Connection failed (err %d)\n", err);
		return err;
	}

	return 0;
}

static int cmd_disconnect(int argc, char *argv[])
{
	int err;
	uint8_t conn_id;

	if (argc < 2) {
		printk("Disconnect reason code required\n");
		return -EINVAL;
	}

	conn_id = *argv[1] - '0';

	if (conn_id > CONFIG_BLUETOOTH_MAX_CONN - 1 || conns[conn_id] == NULL) {
		printk("Invalid conn id %d\n", conn_id);
	}

	err = bt_disconnect(conns[conn_id], BT_HCI_ERR_REMOTE_USER_TERM_CONN);

	if (err) {
		printk("Disconnection failed (err %d)\n", err);
		return err;
	}

	return 0;
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
}
