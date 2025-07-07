/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/drivers/bluetooth.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/byteorder.h>

#include "common/assert.h"

struct net_buf *bt_hci_evt_create(uint8_t evt, uint8_t len)
{
	struct bt_hci_evt_hdr *hdr;
	struct net_buf *buf;

	buf = bt_buf_get_evt(evt, false, K_FOREVER);

	BT_ASSERT(buf);

	hdr = net_buf_add(buf, sizeof(*hdr));
	hdr->evt = evt;
	hdr->len = len;

	return buf;
}

struct net_buf *bt_hci_cmd_complete_create(uint16_t op, uint8_t plen)
{
	struct net_buf *buf;
	struct bt_hci_evt_cmd_complete *cc;

	buf = bt_hci_evt_create(BT_HCI_EVT_CMD_COMPLETE, sizeof(*cc) + plen);

	cc = net_buf_add(buf, sizeof(*cc));

	/* The Num_HCI_Command_Packets parameter allows the Controller to
	 * indicate the number of HCI command packets the Host can send to the
	 * Controller. If the Controller requires the Host to stop sending
	 * commands, Num_HCI_Command_Packets will be set to zero.
	 *
	 * NOTE: Zephyr Controller (and may be other Controllers) do not support
	 *       higher Number of HCI Command packets than 1.
	 */
	cc->ncmd = 1U;

	cc->opcode = sys_cpu_to_le16(op);

	return buf;
}

struct net_buf *bt_hci_cmd_status_create(uint16_t op, uint8_t status)
{
	struct net_buf *buf;
	struct bt_hci_evt_cmd_status *cs;

	buf = bt_hci_evt_create(BT_HCI_EVT_CMD_STATUS, sizeof(*cs));

	cs = net_buf_add(buf, sizeof(*cs));
	cs->status = status;

	/* The Num_HCI_Command_Packets parameter allows the Controller to
	 * indicate the number of HCI command packets the Host can send to the
	 * Controller. If the Controller requires the Host to stop sending
	 * commands, Num_HCI_Command_Packets will be set to zero.
	 *
	 * NOTE: Zephyr Controller (and may be other Controllers) do not support
	 *       higher Number of HCI Command packets than 1.
	 */
	cs->ncmd = 1U;

	cs->opcode = sys_cpu_to_le16(op);

	return buf;
}
