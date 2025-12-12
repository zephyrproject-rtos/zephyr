/* hci_vs.c - Zephyr vendor specific HCI command wrappers */

/*
 * Copyright (c) 2025 Embeint Pty Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/hci_vs.h>

int hci_vs_write_bd_addr(bt_addr_t bdaddr)
{
	struct bt_hci_cp_vs_write_bd_addr *cp;
	struct net_buf *buf;

	/* Allocate and populate the command buffer */
	buf = bt_hci_cmd_alloc(K_FOREVER);
	if (!buf) {
		return -ENOMEM;
	}
	cp = net_buf_add(buf, sizeof(*cp));
	cp->bdaddr = bdaddr;

	/* Send the command */
	return bt_hci_cmd_send(BT_HCI_OP_VS_WRITE_BD_ADDR, buf);
}
